#include "Scene/PhaseBossScene.hpp"

#include <heap/seadFrameHeap.h>
#include "Layout/SingleModeSceneLayout.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Audio/AudioDirector.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"
#include "Library/Clipping/ClippingDirectorBase.hpp"
#include "Library/File/FileUtil.hpp"
#include "Library/LiveActor/Common/LiveActorKit.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/LiveActor/Util/ActorClippingUtil.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorMovementUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/Memory/HeapUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Play/Placement/PlacementInfo.hpp"
#include "Library/Resource/ResourceFunction.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "Library/Scene/SceneUtil.hpp"
#include "Library/Stage/StageInfo.hpp"
#include "Library/Stage/StageResourceKeeper.hpp"
#include "Library/Stage/StageResourceList.hpp"
#include "Library/Yaml/ByamlIter.hpp"
#include "MapObj/LighthouseSimple.hpp"
#include "Player/Normal/PlayerRetargettingSelectorSceneObj.hpp"
#include "Project/AreaObj/AreaObj.hpp"
#include "Project/AreaObj/AreaObjDirector.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Scene/PlayerStocker.hpp"
#include "Scene/ProjectActorFactory.hpp"
#include "Scene/SceneObjID.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/GameDataHolderWriter.hpp"
#include "System/SaveDataAccessFunction.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/DemoActorGroupUtil.hpp"

namespace {
NERVE_DECL(PhaseBossScene, GameEnd)
NERVE_DECL(PhaseBossScene, PhaseEnd)
NERVES_MAKE_NOSTRUCT(PhaseBossScene, GameEnd, PhaseEnd)

/** Name of the heap holding the assets of the phase after the boss fight. */
constexpr const char* cBossResourceHeapName = "BossResourceHeap";
/** Resource category of the assets of the phase after the boss fight. */
constexpr const char* cBossSceneCategoryName = "BossScene";

/** Stationed asset list names of the phases, indexed by the unlocked phase minus 3. */
const char* const cPhaseAssetNames[] = {
    "Phase2", "Phase1", "Phase3", "Phase1", "Phase3PlessieChase", "Phase4", "Phase1",
    "Phase4PlessieChase",
};

/**
 * @brief Stage BGM names requested by the audio director.
 * @note The owner of these fields (at AudioDirector + 0x28) is not reconstructed yet.
 */
struct StageBgmNames {
    u8 _0[0x68];
    const char* mBgmStageName;     // 0x68
    u8 _70[0x88 - 0x70];
    const char* mBgmScenarioName;  // 0x88
};

/**
 * Gets the stage BGM names of the audio director.
 * @param pDirector The audio director.
 * @return The stage BGM names.
 */
StageBgmNames* getStageBgmNames(al::AudioDirector* pDirector) {
    return *reinterpret_cast<StageBgmNames**>(reinterpret_cast<u8*>(pDirector) + 0x28);
}

/**
 * Initializes an area init info from the area list of a stage, if the stage is the main stage.
 * @param pAreaInfo The area init info to initialize.
 * @param pStageInfo The stage.
 * @param rStageName The name of the main stage.
 * @param rInfo The actor init info.
 * @return Whether the area init info was initialized.
 */
bool tryInitAreaInitInfo(al::AreaInitInfo* pAreaInfo, const al::StageInfo* pStageInfo,
                         const sead::BufferedSafeString& rStageName,
                         const al::ActorInitInfo& rInfo) {
    // Scoped so that the name shares its stack slot with the placement info.
    {
        sead::SafeString stageName = rStageName.cstr();
        if (!al::isEqualString(pStageInfo->mName, stageName)) {
            return false;
        }
    }

    al::PlacementInfo placementInfo;
    if (!al::tryGetPlacementInfo(&placementInfo, pStageInfo, "AreaList")) {
        return false;
    }

    pAreaInfo->set(placementInfo, rInfo.getStageSwitchDirector());
    return true;
}
}  // namespace

/**
 * Constructs the boss scene heap allocator; it starts enabled with no heap.
 */
PhaseBossScene::MemorySceneHeapCustomAlloc::MemorySceneHeapCustomAlloc() = default;

/**
 * Creates the heap holding the assets of the phase after the boss fight.
 * @param isCreate Whether the scene resource heap is created.
 * @return Whether the heap was created by this allocator.
 */
bool PhaseBossScene::MemorySceneHeapCustomAlloc::createCustomSceneHeap(bool isCreate) {
    bool isCreated = mIsDisabled && isCreate;
    if (!mIsDisabled && isCreate) {
        mIsLoadedNextPhaseAssets = false;
        mIsForceDestroy = false;
        sead::Heap* heap = sead::FrameHeap::create(0x12c00000, cBossResourceHeapName, nullptr, 8,
                                                   sead::Heap::cHeapDirection_Forward, false);
        heap->enableWarning(false);
        mHeap = heap;
        al::addNamedHeap(heap, cBossResourceHeapName);
        al::addResourceCategory(cBossSceneCategoryName, 0x400, mHeap);
        isCreated = true;
    }

    return isCreated;
}

/**
 * Destroys the heap holding the assets of the phase after the boss fight.
 * @param isDestroy Whether the scene resource heap is destroyed.
 * @return Whether the scene resource heap has to be destroyed as well.
 */
bool PhaseBossScene::MemorySceneHeapCustomAlloc::destroyCustomSceneHeap(bool isDestroy) {
    if (mIsDisabled) {
        return isDestroy;
    }

    if (!isDestroy) {
        return false;
    }

    sead::Heap* heap = al::findNamedHeap(cBossResourceHeapName);
    al::removeResourceCategory(cBossSceneCategoryName);
    al::removeNamedHeap(cBossResourceHeapName);
    heap->freeAll();
    heap->destroy();
    mHeap = nullptr;
    al::clearFileLoaderEntry();
    if (al::isCategoryAdded("Scene")) {
        al::setCurrentCategoryName("Scene");
    }

    return mIsForceDestroy;
}

/**
 * Checks whether the scene resource heap has to be destroyed.
 * @return Whether the scene resource heap has to be destroyed.
 */
bool PhaseBossScene::MemorySceneHeapCustomAlloc::isDestroySceneResourceHeap() const {
    return mIsDisabled || mIsForceDestroy;
}

/**
 * Gets the phase that follows the current boss fight.
 * @return The next phase.
 */
s32 PhaseBossScene::MemorySceneHeapCustomAlloc::getNextPhase() {
    s32 nextPhase = 3;
    switch (SingleModeDataFunction::getUnlockedPhase(GameDataHolderAccessor(mGameDataHolder))) {
    case 6:
        nextPhase = SingleModeDataFunction::getPhase3DarkBowserHitPoint(
                        GameDataHolderAccessor(mGameDataHolder)) <= 100 ?
                        7 :
                        5;
        break;
    case 9:
        nextPhase = 10;
        break;
    case 4:
        // Phase 2 has no Plessie chase variant.
        nextPhase = SingleModeDataFunction::getPhase2DarkBowserHitPoint(
                        GameDataHolderAccessor(mGameDataHolder)) <= 100 ?
                        3 :
                        3;
        break;
    default:
        break;
    }

    return nextPhase;
}

/**
 * Loads the stationed assets of the phase after the boss fight into the boss heap.
 * @return Whether the assets are loaded.
 */
bool PhaseBossScene::MemorySceneHeapCustomAlloc::tryLoadNextPhaseAssets() {
    if (mHeap == nullptr) {
        return false;
    }

    if (mIsLoadedNextPhaseAssets) {
        return true;
    }

    al::ByamlIter fileList;
    s32 nextPhase = getNextPhase();
    al::StringTmp<256> categoryName;
    categoryName.format("Stationed[%s]", cPhaseAssetNames[nextPhase - 3]);
    if (!al::tryGetCategoryFileListIter(&fileList, categoryName)) {
        return false;
    }

    al::setCurrentCategoryName(cBossSceneCategoryName);
    for (s32 i = 0; i < fileList.getSize(); i++) {
        const char* fileName = nullptr;
        if (fileList.tryGetStringByIndex(&fileName, i)) {
            al::findOrCreateResource(fileName, nullptr);
        }
    }

    mIsLoadedNextPhaseAssets = true;
    return true;
}

/**
 * Adjusts the size of the scene resource heap to the phase after the boss fight.
 * @param size The default size.
 * @return The adjusted size.
 */
size_t PhaseBossScene::MemorySceneHeapCustomAlloc::adjustStageResourceSize(size_t size) {
    if (mIsDisabled) {
        return size;
    }

    return rc::isPlessieChase(getNextPhase()) ? 0xd200000 : 0x6400000;
}

/**
 * Forces the scene resource heap to be destroyed with the boss heap.
 */
void PhaseBossScene::MemorySceneHeapCustomAlloc::forceDestroySceneResourceHeap() {
    mIsLoadedNextPhaseAssets = false;
    mIsForceDestroy = true;
}

/**
 * Constructs the boss phase scene.
 */
PhaseBossScene::PhaseBossScene() : SingleModeScene("PhaseBossScene") {}

/**
 * Constructs the boss phase scene.
 * @param pName The scene name.
 */
PhaseBossScene::PhaseBossScene(const char* pName) : SingleModeScene(pName) {}

/**
 * Destroys the boss phase scene.
 */
PhaseBossScene::~PhaseBossScene() = default;

/**
 * Runs the placement steps that precede the object placement.
 * @param rInfo The actor init info.
 */
void PhaseBossScene::preInitPlacement(const al::ActorInitInfo& rInfo) {
    SingleModeScene::preInitPlacement(rInfo);
}

/**
 * Initializes the scene, loading the assets of the phase after the boss fight first.
 * @param rInfo The scene init info.
 */
void PhaseBossScene::init(const al::SceneInitInfo& rInfo) {
    if (al::isCategoryAdded(cBossSceneCategoryName)) {
        sCustomAlloc.tryLoadNextPhaseAssets();
        if (al::isCategoryAdded(cBossSceneCategoryName)) {
            al::setCurrentCategoryName(cBossSceneCategoryName);
        }
    }

    SingleModeScene::init(rInfo);
}

/**
 * Initializes the island data list.
 */
void PhaseBossScene::initIslandDataList() {
    SingleModeScene::initIslandDataList();
}

/**
 * Places the area objects of the main map, design and sound stages.
 * @param rInfo The actor init info.
 */
void PhaseBossScene::initAreaObj(const al::ActorInitInfo& rInfo) {
    mLiveActorKit->getClippingDirector()->setCollisionClippingDisabled(true);

    al::AreaInitInfo areaInfos[512];
    s32 areaNum = 0;
    for (s32 i = 0; i < al::getStageInfoMapNum(this); i++) {
        if (tryInitAreaInitInfo(&areaInfos[areaNum], al::getStageInfoMap(this, i), mStageName,
                                rInfo)) {
            areaNum++;
        }
    }

    for (s32 i = 0; i < al::getStageInfoDesignNum(this); i++) {
        if (tryInitAreaInitInfo(&areaInfos[areaNum], al::getStageInfoDesign(this, i), mStageName,
                                rInfo)) {
            areaNum++;
        }
    }

    for (s32 i = 0; i < al::getStageInfoSoundNum(this); i++) {
        if (tryInitAreaInitInfo(&areaInfos[areaNum], al::getStageInfoSound(this, i), mStageName,
                                rInfo)) {
            areaNum++;
        }
    }

    mLiveActorKit->getAreaObjDirector()->placement(areaInfos, areaNum, getSceneObjHolder(), this);
    rc::initAreaObjIndex(mLiveActorKit->getAreaObjDirector());
}

/**
 * Places the players, lighthouses and objects of the boss stage.
 * @param rInfo The actor init info.
 */
void PhaseBossScene::initPlacement(al::ActorInitInfo& rInfo) {
    s32 mapNum = al::getStageInfoMapNum(this);
    mLiveActorKit->getClippingDirector()->disableForceClipAreas();
    preInitPlacement(rInfo);
    initPlacementOceanWater(rInfo);
    initAreaObj(rInfo);

    ProjectActorFactory factory;
    mGoalItemHolder = al::createSceneObj(this, SceneObjID_GoalItemHolder);
    initLighthouses(rInfo);
    auto* selector = static_cast<PlayerRetargettingSelectorSceneObj*>(
        al::createSceneObj(this, SceneObjID_PlayerRetargettingSelector));
    initPlacementPlayer(al::getStageInfoMap(this, 0), rInfo, selector);
    PlayerStockerFunction::tryCreateDoubleMario(this, rInfo, selector, nullptr, false);
    al::initPlacementByStageInfo(al::getStageInfoMap(this, 0), "SkyList", factory, rInfo);

    for (s32 i = 0; i < mapNum; i++) {
        const al::StageInfo* stageInfo = al::getStageInfoMap(this, i);
        if (al::isEqualString(stageInfo->mName, mStageName.cstr())) {
            initPlacementObject(stageInfo, rInfo, "Map");
        } else {
            initPlacementBossLOD(stageInfo, rInfo, "Map");
        }
    }

    for (s32 i = 0; i < al::getStageInfoDesignNum(this); i++) {
        initPlacementObject(al::getStageInfoDesign(this, i), rInfo, "Design");
    }

    for (s32 i = 0; i < al::getStageInfoSoundNum(this); i++) {
        initPlacementObject(al::getStageInfoSound(this, i), rInfo, "Sound");
    }
}

/**
 * Places the simple lighthouses listed in the island flag lists of the map stages.
 * @param rInfo The actor init info.
 */
void PhaseBossScene::initLighthouses(const al::ActorInitInfo& rInfo) {
    s32 mapNum = al::getStageInfoMapNum(this);
    for (s32 i = 0; i < mapNum; i++) {
        const al::StageInfo* stageInfo =
            mStageResourceKeeper->getMapStageInfo()->getStageInfo(i);
        if (!isValidPlacementParent(stageInfo->getPlacementInfo())) {
            continue;
        }

        al::PlacementInfo listInfo;
        s32 count = 0;
        al::tryGetPlacementInfoAndCount(&listInfo, &count, stageInfo, "IslandFlagList");
        for (s32 j = 0; j < count; j++) {
            al::PlacementInfo placementInfo;
            al::getPlacementInfoByIndex(&placementInfo, listInfo, j);
            if (!isValidPlacement(placementInfo)) {
                continue;
            }

            auto* lighthouse = new LighthouseSimple("LighthouseSimple");
            al::ActorInitInfo actorInfo;
            actorInfo.initViewIdSelf(&placementInfo, rInfo);
            lighthouse->init(actorInfo);
        }
    }
}

/**
 * Places the lighthouses and the boss LOD actors of a sub stage.
 * @param pStageInfo The sub stage.
 * @param rInfo The actor init info.
 * @param pListName Unused.
 */
void PhaseBossScene::initPlacementBossLOD(const al::StageInfo* pStageInfo,
                                          const al::ActorInitInfo& rInfo, const char* pListName) {
    if (!isValidPlacementParent(pStageInfo->getPlacementInfo())) {
        return;
    }

    al::PlacementInfo listInfo;
    s32 count = 0;
    al::getPlacementInfoAndCount(&listInfo, &count, pStageInfo, "ObjectList");
    ProjectActorFactory factory;
    sead::Vector3f scale;
    sead::Vector3f rotate;
    sead::Vector3f trans;
    for (s32 i = 0; i < count; i++) {
        al::PlacementInfo placementInfo;
        al::getPlacementInfoByIndex(&placementInfo, listInfo, i);
        if (!isValidPlacement(placementInfo)) {
            continue;
        }

        const char* name = nullptr;
        al::getClassName(&name, placementInfo);
        if (al::isEqualString(name, "Lighthouse")) {
            al::ActorInitInfo actorInfo;
            actorInfo.initViewIdSelf(&placementInfo, rInfo);
            auto* lighthouse = new LighthouseSimple("LighthouseSimple");
            lighthouse->init(actorInfo);
            continue;
        }

        if (al::calcLinkChildNum(placementInfo, "BossLOD") == 0) {
            continue;
        }

        al::PlacementInfo linkInfo;
        trans = sead::Vector3f::zero;
        rotate = sead::Vector3f::zero;
        scale = sead::Vector3f::ones;
        al::tryGetTrans(&trans, placementInfo);
        al::tryGetRotate(&rotate, placementInfo);
        al::tryGetScale(&scale, placementInfo);
        al::getLinksInfo(&linkInfo, placementInfo, "BossLOD");
        al::getObjectName(&name, linkInfo);
        if (al::isEqualString(name, "ProfilingWarpPoint")) {
            continue;
        }

        al::ActorInitInfo actorInfo;
        actorInfo.initViewIdSelf(&placementInfo, rInfo);
        al::LiveActor* actor = al::createLinksActorFromFactory(factory, actorInfo, "BossLOD", 0);
        if (actor == nullptr) {
            continue;
        }

        al::setTrans(actor, trans);
        al::updatePoseRotate(actor, rotate);
        al::setScale(actor, scale);
        al::resetPosition(actor, false);
        al::recreateClipping(actor, actorInfo);
        al::onDrawClipping(actor);

        s32 zoneId;
        if (al::tryGetZoneID(&zoneId, placementInfo) && zoneId >= 0) {
            actor->changeScenarioID(SingleModeDataFunction::getFirstAvailableScenario(
                                        GameDataHolderAccessor(actor), zoneId - 1),
                                    true);
        }

        DemoActorGroupUtil::initDemoActorGroup(actor, rInfo, placementInfo);
    }
}

/**
 * Updates the collision terrain kits during cutscenes.
 */
void PhaseBossScene::updateDemoCutsceneAddOn() {
    al::updateKitList(this, "コリジョン地形");
}

/**
 * Checks whether the game ended (the final boss was defeated).
 * @return Whether the game ended.
 */
bool PhaseBossScene::isGameEnd() const {
    return al::isNerve(this, &NrvPhaseBossSceneGameEnd) && !mIsPhaseEnd;
}

/**
 * Checks whether the phase ended.
 * @return Whether the phase ended.
 */
bool PhaseBossScene::isPhaseEnd() const {
    return mIsPhaseEnd;
}

/**
 * Checks whether the scene changes to another phase.
 * @return Whether the scene changes to another phase.
 */
bool PhaseBossScene::isChangePhase() const {
    return mIsPhaseEnd;
}

/**
 * Records the result of the boss fight and moves on to the next phase.
 */
void PhaseBossScene::handlePhaseEnd() {
    al::stopAllSequenceBgm(this, 10);
    GameDataFunction::updatePlayerFigures(this, -1);

    s32 phase = SingleModeDataFunction::getUnlockedPhase(this);
    if (phase == 4) {
        if (SingleModeDataFunction::getPhase2DarkBowserHitPoint(this) > 0) {
            SingleModeDataFunction::clearAllButGigaBellCheckpoint(
                GameDataHolderAccessor(mGameDataHolder));
            SingleModeDataFunction::recordDisasterMode(GameDataHolderAccessor(mGameDataHolder),
                                                       SingleModeDataFunction::DisasterForceSetting_Off,
                                                       this);
            SingleModeDataFunction::setUnlockedPhase(this, 3);
            SingleModeDataFunction::setFirstPhase2BossDefeated(this);
            GameDataHolderWriter writer = GameDataHolderAccessor(mGameDataHolder);
            SingleModeDataFunction::setGigaBellLockCount(
                writer, -SingleModeDataFunction::getGigaBellLockCount(writer));
        } else {
            SingleModeDataFunction::clearAllCheckpoints(GameDataHolderAccessor(mGameDataHolder));
            SingleModeDataFunction::recordDisasterMode(GameDataHolderAccessor(mGameDataHolder),
                                                       SingleModeDataFunction::DisasterForceSetting_Off,
                                                       this);
            SingleModeDataFunction::reportPhaseClearEvent(
                this, this,
                SingleModeDataFunction::getUnlockedPhase(GameDataHolderAccessor(mGameDataHolder)));
            SingleModeDataFunction::setUnlockedPhase(this, 5);
            SingleModeDataFunction::clearHasSeenCutscene(this, 2);
        }
    } else if (phase == 6 || phase == 9) {
        if ((phase == 6 && SingleModeDataFunction::getPhase3DarkBowserHitPoint(this) > 0) ||
            (phase == 9 && SingleModeDataFunction::getPhase4DarkBowserHitPoint(this) > 0)) {
            SingleModeDataFunction::clearAllButGigaBellCheckpoint(
                GameDataHolderAccessor(mGameDataHolder));
            SingleModeDataFunction::recordDisasterMode(GameDataHolderAccessor(mGameDataHolder),
                                                       SingleModeDataFunction::DisasterForceSetting_Off,
                                                       this);
            SingleModeDataFunction::setUnlockedPhase(this, phase - 1);
            if (phase == 6) {
                SingleModeDataFunction::setFirstPhase3BossDefeated(this);
            }

            GameDataHolderWriter writer = GameDataHolderAccessor(mGameDataHolder);
            SingleModeDataFunction::setGigaBellLockCount(
                writer, -SingleModeDataFunction::getGigaBellLockCount(writer));
        } else {
            if (phase == 9) {
                SingleModeDataFunction::setPhase4BossDefeated(this);
            }

            SingleModeDataFunction::setPhase4DarkBowserHitPoint(this, -1);
            SingleModeDataFunction::clearAllButGigaBellCheckpoint(
                GameDataHolderAccessor(mGameDataHolder));
            SingleModeDataFunction::recordDisasterMode(GameDataHolderAccessor(mGameDataHolder),
                                                       SingleModeDataFunction::DisasterForceSetting_Off,
                                                       this);
            SingleModeDataFunction::setDisasterModePostBossPeaceFrames(this, 0);
            SingleModeDataFunction::setUnlockedPhase(this, phase + 1);
        }
    } else if (phase <= 7) {
        SingleModeDataFunction::clearAllButGigaBellCheckpoint(
            GameDataHolderAccessor(mGameDataHolder));
        SingleModeDataFunction::recordDisasterMode(GameDataHolderAccessor(mGameDataHolder),
                                                   SingleModeDataFunction::DisasterForceSetting_Off,
                                                   this);
        SingleModeDataFunction::reportPhaseClearEvent(
            this, this,
            SingleModeDataFunction::getUnlockedPhase(GameDataHolderAccessor(mGameDataHolder)));
        SingleModeDataFunction::setUnlockedPhase(this, phase + 1);
        SingleModeDataFunction::clearHasSeenCutscene(this, 2);
    } else {
        mIsPhaseEnd = false;
        al::setNerve(this, &NrvPhaseBossSceneGameEnd);
        SingleModeDataFunction::setUnlockedPhase(this, 8);
        goto setPeaceFrames;
    }

    SaveDataAccessFunction::startSaveDataWriteSync(mGameDataHolder, true);
    al::setNerve(this, &NrvPhaseBossScenePhaseEnd);
    mIsPhaseEnd = true;

setPeaceFrames:
    switch (SingleModeDataFunction::getUnlockedPhase(this)) {
    case 3:
        SingleModeDataFunction::setDisasterModePostBossPeaceFrames(this, 10800);
        break;
    case 5:
        SingleModeDataFunction::setDisasterModePostBossPeaceFrames(this, 21600);
        break;
    case 8:
        SingleModeDataFunction::setDisasterModePostBossPeaceFrames(this, 32400);
        break;
    default:
        break;
    }
}

/**
 * Requests the BGM of the boss stage.
 */
void PhaseBossScene::requestStageBgmStart() {
    s32 phase = SingleModeDataFunction::getUnlockedPhase(this);
    const char* name = phase == 4 ? "Phase2" : phase == 6 ? "Phase3" : "Phase1";
    StageBgmNames* bgmNames = getStageBgmNames(mAudioDirector);
    bgmNames->mBgmScenarioName = name;
    bgmNames->mBgmStageName = name;
}

/**
 * Runs the phase end state.
 */
void PhaseBossScene::exePhaseEnd() {
    SingleModeScene::exeGameEnd();
}

/**
 * Runs the game end state.
 */
void PhaseBossScene::exeGameEnd() {
    SingleModeScene::exeGameEnd();
}

/**
 * Starts the scene, hiding the scene layout.
 */
void PhaseBossScene::appear() {
    SingleModeScene::appear();
    mSceneLayout->kill();
}

/**
 * Handles a game over during a boss fight by going back to the previous phase.
 */
void PhaseBossScene::handleGameOver() {
    s32 phase = SingleModeDataFunction::getUnlockedPhase(this);
    if (phase == 2 || phase == 4 || phase == 6 || phase == 9) {
        mIsPhaseEnd = true;
        mIsGameOver = true;
        SingleModeDataFunction::recordDisasterMode(GameDataHolderAccessor(mGameDataHolder),
                                                   SingleModeDataFunction::DisasterForceSetting_On,
                                                   this);
        SingleModeDataFunction::setUnlockedPhase(this, phase - 1);
        SaveDataAccessFunction::startSaveDataWriteSync(mGameDataHolder, true);
        rc::setControlUserFigureType(GameDataHolderAccessor(mGameDataHolder), 0, 0);
    }

    SingleModeScene::handleGameOver();
}

/**
 * Ends the scene.
 */
void PhaseBossScene::kill() {
    SingleModeScene::kill();
}

/**
 * Checks whether restart points are allowed.
 * @return false; boss fights have no restart points.
 */
bool PhaseBossScene::allowRestartPoint() const {
    return false;
}

/**
 * Checks whether this is a boss scene.
 * @return true.
 */
bool PhaseBossScene::isBossScene() const {
    return true;
}

/**
 * Checks whether the zone ids of the placements are checked.
 * @return false.
 */
bool PhaseBossScene::doZoneIDCheck() const {
    return false;
}
