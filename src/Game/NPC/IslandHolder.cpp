#include "NPC/IslandHolder.hpp"

#include <cstring>

#include "Layout/GuideBalloon.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorModelUtil.hpp"
#include "Library/LiveActor/Util/ActorSceneInfo.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Play/Placement/PlacementHolder.hpp"
#include "Library/Play/Placement/PlacementInfo.hpp"
#include "Library/Resource/ResourceFunction.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/StageSwitch/StageSwitchFunc.hpp"
#include "Library/Thread/Functor.hpp"
#include "MapObj/GoalItem.hpp"
#include "MapObj/Lighthouse.hpp"
#include "Project/Action/Common/ActionPadAndCameraCtrl.hpp"
#include "Project/AreaObj/AreaObj.hpp"
#include "Project/AreaObj/AreaObjUtil.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/GameDataHolderWriter.hpp"
#include "Util/DemoUtil.hpp"

namespace {
    /// Collector holding the static nerves plus the per-island demo nerves (see addDemoActions).
    alNerveFunction::NerveActionCollector* sDemoCollector = nullptr;

    /// Offset of the entrance guide balloon from the island.
    sead::Vector3f sGuideBalloonOffset(0.0f, 500.0f, 0.0f);

    NERVE_ACTION_IMPL(IslandHolder, Idle)
    NERVE_ACTION_IMPL(IslandHolder, DelayRise)
    NERVE_ACTION_IMPL(IslandHolder, Rise)
    NERVE_ACTIONS_MAKE_STRUCT(IslandHolder, Idle, DelayRise, Rise)

// Nerve action created at runtime whose name carries the island's suffix ("DemoRise<Suffix>").
#define ISLAND_DEMO_NERVE_ACTION(Action, ActionFunc)                                               \
    class IslandHolderNrv##Action : public al::NerveAction {                                       \
    public:                                                                                        \
        explicit IslandHolderNrv##Action(const char* pSuffix) {                                    \
            mName.format(#Action "%s", pSuffix);                                                   \
        }                                                                                          \
                                                                                                   \
        void execute(al::NerveKeeper* pKeeper) const override {                                    \
            pKeeper->getParent<IslandHolder>()->exe##ActionFunc();                                 \
        }                                                                                          \
                                                                                                   \
        const char* getActionName() const override { return mName.cstr(); }                       \
                                                                                                   \
    private:                                                                                       \
        sead::FixedSafeString<128> mName;                                                          \
    };

    ISLAND_DEMO_NERVE_ACTION(DemoRise, Rise)
    ISLAND_DEMO_NERVE_ACTION(DemoDelayRise, DelayRise)
    ISLAND_DEMO_NERVE_ACTION(DemoEndRise, Idle)

#undef ISLAND_DEMO_NERVE_ACTION
}  // namespace

/**
 * Creates the shared nerve collector and copies the static island nerves into it.
 */
void IslandHolder::initDemoActions() {
    sDemoCollector = new alNerveFunction::NerveActionCollector();

    al::NerveAction* pAction = NrvIslandHolder.collector.mStartAction;
    if (NrvIslandHolder.collector.mNumActions > 0) {
        sDemoCollector->addNerve(pAction);
        for (s32 i = 1; i < NrvIslandHolder.collector.mNumActions; i++) {
            pAction = pAction->mNextNode;
            sDemoCollector->addNerve(pAction);
        }
    }

    if (pAction != nullptr) {
        pAction->mNextNode = nullptr;
    }
}

/**
 * Registers the rise demo nerves of one island in the shared collector.
 * @param pSuffix island suffix appended to the demo nerve names
 */
void IslandHolder::addDemoActions(const char* pSuffix) {
    new IslandHolderNrvDemoRise(pSuffix);
    new IslandHolderNrvDemoDelayRise(pSuffix);
    new IslandHolderNrvDemoEndRise(pSuffix);
}

/**
 * Creates the island holder.
 * @param pName actor name
 */
IslandHolder::IslandHolder(const char* pName) : al::LiveActor(pName) {
    mLinkedActors.allocBuffer(4, nullptr);
    mDemoRiseName.format("DemoRise%s", getName());
    mDemoDelayRiseName.format("DemoDelayRise%s", getName());
    mDemoEndRiseName.format("DemoEndRise%s", getName());
}

/**
 * Loads the island model and rise parameters and hooks up the rise stage switches.
 * @param rInfo placement / init info
 */
void IslandHolder::init(const al::ActorInitInfo& rInfo) {
    al::initNerveAction(this, "Idle", sDemoCollector, 0);

    const char* pSuffix = nullptr;
    al::tryGetStringArg(&pSuffix, rInfo, "SuffixName");
    if (pSuffix != nullptr && pSuffix[0] == '\0') {
        pSuffix = nullptr;
    }

    al::initActorWithArchiveName(this, rInfo, "ZoneHolder", pSuffix);

    al::StringTmp<256> path("ObjectData/ZoneHolder");
    al::Resource* pResource = al::findOrCreateResource(path, nullptr);
    if (pResource != nullptr) {
        mActionPadAndCameraCtrl =
            al::ActionPadAndCameraCtrl::tryCreate(this, al::getTransPtr(this), pResource);
    }

    al::tryGetArg(&mIsAllowEchoSounds, rInfo, "AllowEchoSounds");
    al::tryGetArg(&mRiseTime, rInfo, "IslandRiseTime");
    al::tryGetArg(&mRiseDelayTime, rInfo, "IslandRiseDelayTime");
    al::tryGetArg(&mRiseStartY, rInfo, "IslandRiseStartY");
    al::tryGetArg(&mRiseSeFrameStart, rInfo, "IslandRiseSeFrameStart");
    if (mRiseSeFrameStart > mRiseDelayTime) {
        mRiseSeFrameStart = mRiseDelayTime;
    }

    al::tryGetObjectName(&mObjectName, rInfo);
    al::listenStageSwitchOnKill(this, al::Functor(this, &IslandHolder::hideZone));
    al::invalidateClipping(this);
    makeActorAppeared();

    mFixedPart = nullptr;
    mGuideBalloon = new GuideBalloon("ガイドバルーン", "SpeechBalloon", al::getLayoutInitInfo(rInfo),
                                     al::getTransPtr(this), sGuideBalloonOffset,
                                     getSceneInfo()->isSingleMode, getSceneInfo()->demoDirector);

    al::listenStageSwitchOn(this, "SwitchStartRise",
                            al::Functor(this, &IslandHolder::onStartRiseSwitch));
    al::listenStageSwitchOn(this, "SwitchEndRise", al::Functor(this, &IslandHolder::onEndRiseSwitch));
}

/**
 * Hides every actor of the zone.
 */
void IslandHolder::hideZone() {
    for (s32 i = 0; i < mZoneActors.size(); i++) {
        mZoneActors[i]->hideActor();
    }
}

/**
 * Stage switch callback: starts the island rise demo.
 */
void IslandHolder::onStartRiseSwitch() {
    if (mLighthouse != nullptr) {
        mLighthouse->killInkPillar();
    }

    if (mZoneActors.size() != 0) {
        rc::addDemoActor(this);
        mRiseOffsetY = mRiseStartY;
        startDemoOnActors();
        al::startNerveAction(this, mDemoDelayRiseName.cstr());
    }
}

/**
 * Stage switch callback: finishes the island rise immediately.
 */
void IslandHolder::onEndRiseSwitch() {
    if (mRiseOffsetY != 0.0f) {
        al::tryKillEmitterAndParticleAll(this);
        al::tryOnStageSwitch(this, "SwitchRiseDoneOn");
        endDemoOnActors();
        if (mLighthouse != nullptr) {
            mLighthouse->startInkPillar();
        }
    }

    mRiseOffsetY = 0.0f;
    al::startNerveAction(this, "Idle");
}

/**
 * Puts every actor owned by the island in demo mode.
 */
void IslandHolder::startDemoOnActors() {
    for (s32 j = 0; j < cScenarioLayerNum + 1; j++) {
        for (s32 i = 0; i < mScenarioActors[j].size(); i++) {
            mScenarioActors[j][i]->startDemoActor(0);
        }
    }

    for (s32 j = 0; j < cScenarioLayerNum; j++) {
        for (s32 i = 0; i < mOtherScenarioActors[j].size(); i++) {
            mOtherScenarioActors[j][i]->startDemoActor(0);
        }
    }

    for (s32 i = 0; i < mZoneActors.size(); i++) {
        mZoneActors[i]->startDemoActor(0);
    }

    for (s32 i = 0; i < mLinkedActors.size(); i++) {
        mLinkedActors[i]->startDemoActor(0);
    }
}

/**
 * Takes every actor owned by the island out of demo mode.
 */
void IslandHolder::endDemoOnActors() {
    for (s32 j = 0; j < cScenarioLayerNum + 1; j++) {
        for (s32 i = 0; i < mScenarioActors[j].size(); i++) {
            mScenarioActors[j][i]->endDemoActor(0);
        }
    }

    for (s32 j = 0; j < cScenarioLayerNum; j++) {
        for (s32 i = 0; i < mOtherScenarioActors[j].size(); i++) {
            mOtherScenarioActors[j][i]->endDemoActor(0);
        }
    }

    for (s32 i = 0; i < mZoneActors.size(); i++) {
        mZoneActors[i]->endDemoActor(0);
    }

    for (s32 i = 0; i < mLinkedActors.size(); i++) {
        mLinkedActors[i]->endDemoActor(0);
    }
}

/**
 * Nothing to do while the island rests.
 */
void IslandHolder::exeIdle() {}

/**
 * Waits before the rise, starting the rise sound on the configured frame.
 */
void IslandHolder::exeDelayRise() {
    if (al::isStep(this, mRiseSeFrameStart)) {
        al::tryStartSe(this, mDemoRiseName, nullptr);
    }

    if (al::isGreaterEqualStep(this, mRiseDelayTime)) {
        al::startNerveAction(this, mDemoRiseName.cstr());
    }
}

/**
 * Raises the island from mRiseStartY to its placed height over mRiseTime frames.
 */
void IslandHolder::exeRise() {
    if (al::isFirstStep(this)) {
        mRiseOffsetY = mRiseStartY;
        if (mActionPadAndCameraCtrl != nullptr) {
            mActionPadAndCameraCtrl->startAction(mDemoRiseName.cstr());
        }
    }

    if (mActionPadAndCameraCtrl != nullptr) {
        mActionPadAndCameraCtrl->update(al::getNerveStep(this), 1.0f);
    }

    if (al::isGreaterEqualStep(this, mRiseTime)) {
        mRiseOffsetY = 0.0f;
        al::startNerveAction(this, mDemoEndRiseName.cstr());
        al::tryOnStageSwitch(this, "SwitchRiseDoneOn");
        endDemoOnActors();
        if (mLighthouse != nullptr) {
            mLighthouse->startInkPillar();
        }

        return;
    }

    mRiseOffsetY = al::lerpValue(al::getNerveStep(this) / static_cast<f32>(mRiseTime),
                                 mRiseStartY, 0.0f);
}

/**
 * Allocates the actor and area lists.
 * @param zoneActorNum capacity of the zone actor list
 * @param unused unused
 */
void IslandHolder::allocBuffer(s32 zoneActorNum, s32 unused) {
    mZoneActors.allocBuffer(zoneActorNum, nullptr);
    for (s32 i = 0; i < cScenarioLayerNum; i++) {
        mScenarioActors[i].allocBuffer(512, nullptr);
        mOtherScenarioActors[i].allocBuffer(512, nullptr);
        mScenarioAreas[i].allocBuffer(512, nullptr);
        mOtherScenarioAreas[i].allocBuffer(512, nullptr);
    }

    mScenarioActors[cScenarioLayerNum].allocBuffer(512, nullptr);
    mScenarioAreas[cScenarioLayerNum].allocBuffer(512, nullptr);
    mGoalItems.allocBuffer(80, nullptr);
}

/**
 * Hides the zone until its appear switch turns on, if it has one.
 */
void IslandHolder::endInit() {
    mIsHidingZone = al::listenStageSwitchOnAppear(
        this, al::FunctorV1M<IslandHolder*, void (IslandHolder::*)(bool), bool>(
                  this, &IslandHolder::showZone, true));
    if (mIsHidingZone) {
        hideZone();
        mIsHidingZone = false;
    }
}

/**
 * Shows the zone and selects the island's active scenario.
 * @param isPlayAppear whether to play the appear effect and sound
 */
void IslandHolder::showZone(bool isPlayAppear) {
    if (isPlayAppear) {
        if (al::isEffectExist(this, "AppearS")) {
            al::tryEmitEffect(this, "AppearS", &mAppearEffectPos);
        }

        al::tryStartSe(this, "Appear", nullptr);
    }

    for (s32 i = 0; i < mZoneActors.size(); i++) {
        mZoneActors[i]->showActor();
    }

    s32 scenarioId;
    if (mIslandId > 0) {
        SingleModeDataFunction::setIslandUnlocked(GameDataHolderWriter(this), mIslandId);
        scenarioId =
            SingleModeDataFunction::getCurActiveScenarioIndex(GameDataHolderAccessor(this),
                                                              mIslandId - 1);
    } else {
        scenarioId = -1;
    }

    setScenarioID(scenarioId, true);
    mIsHidingZone = false;
}

/**
 * Shows the actors and enables the areas of a scenario, hiding those of the others.
 * @param scenarioId scenario to show, or a negative value to show every scenario layer
 * @param isNoRespawn whether to keep the actors from respawning
 */
void IslandHolder::setScenarioID(s32 scenarioId, bool isNoRespawn) {
    s32 id = scenarioId < 3 ? scenarioId : 3;

    for (s32 i = 0; i < mZoneActors.size(); i++) {
        al::LiveActor* pActor = mZoneActors[i];
        pActor->changeScenarioID(id, isNoRespawn);
        if (!isNoRespawn && mZoneActors[i]->mPlacementHolder->getLayerId() != 3) {
            pActor->respawn();
        }
    }

    if (id < 0) {
        for (s32 j = 0; j < cScenarioLayerNum + 1; j++) {
            for (s32 i = 0; i < mScenarioActors[j].size(); i++) {
                al::LiveActor* pActor = mScenarioActors[j][i];
                pActor->showActor();
                if (!isNoRespawn) {
                    pActor->respawn();
                }
            }

            for (s32 i = 0; i < mScenarioAreas[j].size(); i++) {
                mScenarioAreas[j][i]->mIsDisabled = false;
            }
        }

        mScenarioId = -1;
        return;
    }

    s32 prevId = mScenarioId;
    mScenarioId = id;
    bool isChanged = prevId != id;
    if (isChanged && mLighthouse != nullptr) {
        mLighthouse->activateScenarioAnim(true);
    }

    for (s32 j = 0; j < cScenarioLayerNum; j++) {
        for (s32 i = 0; i < mScenarioActors[j].size(); i++) {
            al::LiveActor* pActor = mScenarioActors[j][i];
            if (j == id) {
                if (isChanged) {
                    pActor->showActor();
                }

                if (!isNoRespawn) {
                    pActor->respawn();
                }
            } else if (isChanged) {
                pActor->hideActor();
            }
        }

        for (s32 i = 0; i < mOtherScenarioActors[j].size(); i++) {
            al::LiveActor* pActor = mOtherScenarioActors[j][i];
            if (j == id) {
                if (isChanged) {
                    pActor->hideActor();
                }
            } else {
                if (isChanged) {
                    pActor->showActor();
                }

                if (!isNoRespawn) {
                    pActor->respawn();
                }
            }
        }

        for (s32 i = 0; i < mScenarioAreas[j].size(); i++) {
            mScenarioAreas[j][i]->mIsDisabled = j != id;
        }

        for (s32 i = 0; i < mOtherScenarioAreas[j].size(); i++) {
            mOtherScenarioAreas[j][i]->mIsDisabled = j == id;
        }
    }

    ActorArray& rLateActors = mScenarioActors[cScenarioLayerNum];
    for (s32 i = 0; i < rLateActors.size(); i++) {
        al::LiveActor* pActor = rLateActors[i];
        if (mScenarioId >= 2) {
            if (!isNoRespawn) {
                pActor->respawn();
            }

            if (isChanged) {
                pActor->showActor();
            }
        } else {
            pActor->hideActor();
        }
    }

    AreaArray& rLateAreas = mScenarioAreas[cScenarioLayerNum];
    for (s32 i = 0; i < rLateAreas.size(); i++) {
        rLateAreas[i]->mIsDisabled = mScenarioId < 2;
    }
}

/**
 * Keeps a copy of the init info used to place the player at the island's start.
 * @param rInfo init info of the island start
 */
void IslandHolder::setIslandStartPos(const al::ActorInitInfo& rInfo) {
    if (mIslandStartInfo != nullptr) {
        return;
    }

    auto* pStartInfo = new al::ActorInitInfo();
    mIslandStartInfo = pStartInfo;
    auto* pPlacementInfo = new al::PlacementInfo(rInfo.getPlacementInfo());
    pStartInfo->initViewIdHost(pPlacementInfo, rInfo);
}

/**
 * Enables the entrance camera area around the island's start position.
 */
void IslandHolder::turnOnCameraArea() {
    sead::Vector3f trans;
    al::getTrans(&trans, mIslandStartInfo->getPlacementInfo());
    mStartCameraArea = al::getStartCameraArea(this, trans, getCameraDirector_RS());
    if (mStartCameraArea == nullptr) {
        return;
    }

    if (mStartCameraArea->mIsValid && !mStartCameraArea->mIsDisabled && mStartCameraArea->_66) {
        return;
    }

    if (!mStartCameraArea->mIsSpawnEntranceCamera || mStartCameraArea->_69) {
        return;
    }

    mStartCameraArea->validate();
    mStartCameraArea->setStartPos(trans);
}

/**
 * Enables or disables the LOD models of the zone and scenario actors.
 * @param isDisable whether to disable LOD
 */
void IslandHolder::setLODDisable(bool isDisable) {
    for (s32 i = 0; i < mZoneActors.size(); i++) {
        al::setLodDisabled(mZoneActors[i], isDisable);
    }

    for (s32 j = 0; j < cScenarioLayerNum + 1; j++) {
        for (s32 i = 0; i < mScenarioActors[j].size(); i++) {
            al::setLodDisabled(mScenarioActors[j][i], isDisable);
        }
    }
}

/**
 * Sets the island's display name.
 * @param pName name to copy
 */
void IslandHolder::setIslandName(const char* pName) {
    strcpy(mIslandName, pName);
}

/**
 * Unused.
 */
void IslandHolder::hideMovingActors() {}

/**
 * Links an actor that moves with the island.
 * @param pActor actor to link
 */
void IslandHolder::linkActor(al::LiveActor* pActor) {
    mLinkedActors.pushBack(pActor);
    pActor->setGlobalYOffsetRef(&mRiseOffsetY);
}

/**
 * Sorts a placed actor into the zone or scenario lists by its layer.
 * @param pActor actor placed on the island
 */
void IslandHolder::addActor(al::LiveActor* pActor) {
    if (pActor->mIsFarLodModel) {
        return;
    }

    if (pActor->canLinkYOffset()) {
        pActor->setGlobalYOffsetRef(&mRiseOffsetY);
    }

    s32 layerId = pActor->mPlacementHolder->getLayerId();
    if (layerId >= 4 && layerId <= 7) {
        mScenarioActors[layerId - 4].pushBack(pActor);
    } else if (layerId >= 9 && layerId <= 12) {
        mOtherScenarioActors[layerId - 9].pushBack(pActor);
    } else if (layerId == 8) {
        mScenarioActors[cScenarioLayerNum].pushBack(pActor);
    } else {
        mZoneActors.pushBack(pActor);
    }

    if (al::isEqualString(pActor->getName(), "GoalItem")) {
        mGoalItems.pushBack(static_cast<GoalItem*>(pActor));
    } else if (al::isEqualString(pActor->getName(), "IslandFlag")) {
        mIslandFlag = pActor;
    } else if (al::isEqualString(pActor->getName(), "Lighthouse")) {
        mLighthouse = static_cast<Lighthouse*>(pActor);
    } else if (al::isEqualString(pActor->getName(), "keeslu01FixedPart")) {
        mFixedPart = pActor;
        mGuideBalloon->setTrans(al::getTransPtr(pActor));
    }
}

/**
 * Checks whether the island has a scenario that is open but not completed.
 * @param islandId island index
 * @return true if a new scenario is open
 */
bool IslandHolder::isNewScenarioOpen(s32 islandId) {
    if (mFixedPart == nullptr) {
        return false;
    }

    s32 scenarioNum = SingleModeDataFunction::getScenarioNum(GameDataHolderAccessor(this), islandId);
    s32 activeIndex =
        SingleModeDataFunction::getCurActiveScenarioIndex(GameDataHolderAccessor(this), islandId);

    s32 i;
    for (i = 0; i < scenarioNum; i++) {
        if (!SingleModeDataFunction::isScenarioComplete(GameDataHolderAccessor(this), islandId, i)) {
            break;
        }
    }

    return activeIndex >= i;
}

/**
 * Shows or hides the entrance guide balloon.
 * @param isShow whether to show the balloon
 */
void IslandHolder::showEntranceBalloon(bool isShow) {
    if (mFixedPart == nullptr) {
        return;
    }

    if (isShow) {
        mGuideBalloon->startShowNew();
    } else {
        mGuideBalloon->endShow();
    }
}

/**
 * Sorts an area placed on the island into the scenario area lists by its layer.
 * @param pArea area placed on the island
 * @return always false
 */
bool IslandHolder::tryAddArea(al::AreaObj* pArea) {
    s32 layerId = al::tryGetLayerID(pArea->getPlacementInfo());
    if (layerId >= 4 && layerId <= 7) {
        mScenarioAreas[layerId - 4].pushBack(pArea);
    } else if (layerId >= 9 && layerId <= 12) {
        mOtherScenarioAreas[layerId - 9].pushBack(pArea);
    } else if (layerId == 8) {
        mScenarioAreas[cScenarioLayerNum].pushBack(pArea);
    }

    return false;
}

/**
 * Finds one of the island's goal items.
 * @param shineId shine index of the goal item
 * @return the goal item, or nullptr
 */
GoalItem* IslandHolder::findGoalItem(s32 shineId) {
    for (s32 i = 0; i < mGoalItems.size(); i++) {
        GoalItem* pGoalItem = mGoalItems.unsafeAt(i);
        if (pGoalItem->getShineId() == shineId) {
            return pGoalItem;
        }
    }

    return nullptr;
}

/**
 * Starts the island rise for the intro.
 */
void IslandHolder::startIntro() {
    if (mLighthouse != nullptr) {
        mLighthouse->killInkPillar();
    }

    if (mZoneActors.size() != 0) {
        rc::addDemoActor(this);
        mRiseOffsetY = mRiseStartY;
        startDemoOnActors();
        al::startNerveAction(this, mDemoDelayRiseName.cstr());
    }
}
