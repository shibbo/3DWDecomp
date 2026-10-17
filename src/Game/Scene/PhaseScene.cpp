#include "Scene/PhaseScene.hpp"

#include "Layout/IslandMap.hpp"
#include "Library/LiveActor/Common/LiveActorKit.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "Scene/SceneObjID.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "System/GameDataHolderWriter.hpp"
#include "System/SaveDataAccessFunction.hpp"

namespace {
NERVE_DECL(PhaseScene, GameEnd)
NERVE_DECL(PhaseScene, PhaseEnd)
NERVES_MAKE_NOSTRUCT(PhaseScene, GameEnd, PhaseEnd)
} // namespace

PhaseScene::PhaseScene() : SingleModeScene("PhaseScene") {}
PhaseScene::PhaseScene(const char* pName) : SingleModeScene(pName) {}
PhaseScene::~PhaseScene() = default;

void PhaseScene::initIslandMap(const al::LayoutInitInfo& rLayoutInfo,
                               const al::ActorInitInfo& rActorInfo) {
    switch (SingleModeDataFunction::getUnlockedPhase(this)) {
    case 1:
    case 3:
    case 5:
    case 7:
    case 8:
    case 10:
        mIslandMap = new IslandMap("IslandMap", rLayoutInfo, rActorInfo, mGameDataHolder,
                                   mLiveActorKit->getCameraDirector_RS(),
                                   mLiveActorKit->getPlayerHolder(), mPlayerAliveWatcher,
                                   mWipeWorldJump, mLiveActorKit->getGraphicsSystemInfo());
        al::setSceneObj(this, mIslandMap, SceneObjID_IslandMap);
        break;
    default:
        mIslandMap = nullptr;
        break;
    }
}

void PhaseScene::preInitPlacement(const al::ActorInitInfo& rInfo) {
    if (mUnlockedPhase == 0)
        SingleModeDataFunction::setHasSeenCutscene(GameDataHolderWriter(mGameDataHolder), 2);
    SingleModeScene::preInitPlacement(rInfo);
}

void PhaseScene::initAreaObj(const al::ActorInitInfo& rInfo) {
    SingleModeScene::initAreaObj(rInfo);
}

void PhaseScene::appear() {
    mStartingPhase =
        SingleModeDataFunction::getUnlockedPhase(GameDataHolderAccessor(mGameDataHolder));
    SingleModeScene::appear();
}

void PhaseScene::handlePhaseEnd() {
    const al::IUseSceneObjHolder* holder;
    s32 phase = mStartingPhase;
    switch (phase) {
    case 0:
        SingleModeDataFunction::reportPhaseClearEvent(this, this, 0);
        SingleModeDataFunction::clearHasSeenCutscene(GameDataHolderWriter(mGameDataHolder), 2);
        holder = this;
        goto advancePhase;
    case 7:
    case 10: {
        SingleModeDataFunction::clearAllCheckpoints(GameDataHolderWriter(mGameDataHolder));
        SingleModeDataFunction::setCurValidIslandVisited(GameDataHolderWriter(mGameDataHolder), 0);
        SingleModeDataFunction::recordDisasterMode(GameDataHolderWriter(mGameDataHolder),
                                                   SingleModeDataFunction::DisasterForceSetting_Off,
                                                   this);
        if (phase == 7)
            SingleModeDataFunction::clearHasSeenCutscene(this, 2);
        SingleModeDataFunction::reportPhaseClearEvent(this, this, phase);
        SingleModeDataFunction::setUnlockedPhase(this, 8);
        SingleModeDataFunction::setDisasterModePostBossPeaceFrames(this, 32400);
        goto finishGame;
    }
    case 8:
        SingleModeDataFunction::setPhase4DarkBowserHitPointPreBoss(this);
        SingleModeDataFunction::setUnlockedPhase(this, 9);
        break;
    case 5:
        SingleModeDataFunction::setPhase3DarkBowserHitPointPreBoss(this);
        SingleModeDataFunction::setUnlockedPhase(this, 6);
        break;
    default:
        holder = this;
        if (phase >= 9) {
            SingleModeDataFunction::setUnlockedPhase(holder, 8);
        } else {
        advancePhase:
            SingleModeDataFunction::setUnlockedPhase(holder, phase + 1);
        }
        break;
    }
    {
        GameDataHolderAccessor guideData(this);
        SingleModeDataFunction::resetGuideMessageSeen(guideData, 5, true);
        SingleModeDataFunction::recordDisasterMode(GameDataHolderWriter(mGameDataHolder),
                                                   SingleModeDataFunction::DisasterForceSetting_Off,
                                                   this);
        al::setNerve(this, &NrvPhaseScenePhaseEnd);
        SaveDataAccessFunction::startSaveDataWriteSync(mGameDataHolder, true);
        return;
    }
finishGame: {
    GameDataHolderAccessor guideData(this);
    SingleModeDataFunction::resetGuideMessageSeen(guideData, 5, true);
    SingleModeDataFunction::recordDisasterMode(GameDataHolderWriter(mGameDataHolder),
                                               SingleModeDataFunction::DisasterForceSetting_Off,
                                               this);
    mGameDataHolder->updatePlayerFigures(3);
    al::setNerve(this, &NrvPhaseSceneGameEnd);
    SaveDataAccessFunction::startSaveDataWriteSync(mGameDataHolder, true);
    return;
}
}

void PhaseScene::exePhaseEnd() { SingleModeScene::exeGameEnd(); }

void PhaseScene::exeGameEnd() { SingleModeScene::exeGameEnd(); }

bool PhaseScene::isChangePhase() const { return isPhaseEnd(); }

bool PhaseScene::isPhaseEnd() const { return al::isNerve(this, &NrvPhaseScenePhaseEnd); }

bool PhaseScene::isGameEnd() const { return al::isNerve(this, &NrvPhaseSceneGameEnd); }
