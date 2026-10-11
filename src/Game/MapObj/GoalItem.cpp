#include "MapObj/GoalItem.hpp"

#include <attributes.h>
#include <math/seadMathCalcCommon.h>

#include "Camera/CameraLookAtPoint.hpp"
#include "Camera/CameraPoserFollowLimit.hpp"
#include "Camera/GoalCameraCollisionFilter.hpp"
#include "Enemy/ActorRailBrakeMover.hpp"
#include "Enemy/SuperBowser.hpp"
#include "Layout/GuideGameWindow.hpp"
#include "Layout/IslandMap.hpp"
#include "Layout/SingleModeSceneLayout.hpp"
#include "Layout/Switch/DemoSkipLayout.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"
#include "Library/Camera/CameraPoser_RS.hpp"
#include "Library/Camera/CameraTargetBase.hpp"
#include "Library/Camera/CameraTargetHolder.hpp"
#include "Library/Camera/CameraTicket.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/Collision/PartsConnectorUtil.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/Execute/ExecuteUtil.hpp"
#include "Library/HitSensor/HitSensorKeeper.hpp"
#include "Library/LiveActor/LiveActorFunc.hpp"
#include "Library/LiveActor/Util/ActorClippingUtil.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorSceneInfo.hpp"
#include "Library/LiveActor/Util/ActorSceneUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Play/Camera/CameraPoserFixActor.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Play/Placement/PlacementHolder.hpp"
#include "Library/Play/Placement/PlacementInfo.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Rail/RailUtil.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/StageSwitch/StageSwitchFunc.hpp"
#include "Library/Thread/Functor.hpp"
#include "MapObj/ActorStateDemoCameraProto.hpp"
#include "MapObj/DisasterModeController.hpp"
#include "MapObj/DoorLock.hpp"
#include "MapObj/Fury/GigaBellManager.hpp"
#include "MapObj/Fury/InkPatch.hpp"
#include "MapObj/GoalItemBindPuppeteer.hpp"
#include "MapObj/GoalItemHolder.hpp"
#include "MapObj/IGoalItemCollectListener.hpp"
#include "MapObj/IslandKeeper.hpp"
#include "MapObj/Lighthouse.hpp"
#include "MapObj/LuckyIslandHolder.hpp"
#include "MapObj/SinkedItem.hpp"
#include "MapObj/TimerManager.hpp"
#include "NPC/IslandHolder.hpp"
#include "Player/IUsePlayerPuppet.hpp"
#include "Player/Normal/PlayerActor.hpp"
#include "Player/Normal/PlayerKoopaJr.hpp"
#include "Project/AreaObj/AreaObj.hpp"
#include "Project/AreaObj/AreaObjGroup.hpp"
#include "Project/AreaObj/AreaObjUtil.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Camera/Main/CameraDirector_RS.hpp"
#include "Project/Collision/CollisionUtil.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Scene/ProjectItemDirector.hpp"
#include "Scene/SceneObjID.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/IslandDataList.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/DemoUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"

namespace {
NERVE_DECL(GoalItem, Wait)
NERVE_DECL(GoalItem, CameraPlay)
NERVE_DECL(GoalItem, CleanUpFinish)
NERVE_DECL(GoalItem, GuideMessage)
NERVE_DECL(GoalItem, Done)
NERVE_DECL(GoalItem, AppearAnim)

/**
 * @brief Idle state of an item that was already collected before.
 */
class GoalItemNrvCollectedIdle : public al::Nerve {
public:
    /**
     * @brief Run the nerve.
     * @param pKeeper The nerve keeper of the goal item.
     */
    void execute(al::NerveKeeper* pKeeper) const override {
        pKeeper->getParent<GoalItem>()->exeCleanUpFinish();
    }
};

NERVE_DECL(GoalItem, WaitInstantCollect)
NERVE_DECL(GoalItem, Disappear)
NERVE_DECL(GoalItem, Spin)

/**
 * @brief Slow down of the spin started by an attack.
 */
class GoalItemNrvSpinSpeedDown : public al::Nerve {
public:
    /**
     * @brief Run the nerve.
     * @param pKeeper The nerve keeper of the goal item.
     */
    void execute(al::NerveKeeper* pKeeper) const override {
        pKeeper->getParent<GoalItem>()->exeSpinSpeedDown();
    }

    /**
     * @brief End the nerve.
     * @param pKeeper The nerve keeper of the goal item.
     */
    void executeOnEnd(al::NerveKeeper* pKeeper) const override {
        pKeeper->getParent<GoalItem>()->endSpinSpeedDown();
    }
};

NERVE_DECL(GoalItem, AnimateMoveRail)
NERVE_DECL(GoalItem, PreAnimate)

/**
 * @brief Pre-animation of an item collected without the collect demo.
 */
class GoalItemNrvPreAnimateInstant : public al::Nerve {
public:
    /**
     * @brief Run the nerve.
     * @param pKeeper The nerve keeper of the goal item.
     */
    void execute(al::NerveKeeper* pKeeper) const override {
        pKeeper->getParent<GoalItem>()->exePreAnimate();
    }
};

NERVE_DECL(GoalItem, Animate)
NERVE_DECL(GoalItem, AnimateMoveRailFinish)
NERVE_DECL(GoalItem, CleanUpFinishNoBGM)
NERVE_DECL(GoalItem, WaitStartCollectDemo)
NERVE_DECL(GoalItem, CollectRepeated)

/**
 * @brief Collect of an item collected without the collect demo.
 */
class GoalItemNrvCollectInstant : public al::Nerve {
public:
    /**
     * @brief Run the nerve.
     * @param pKeeper The nerve keeper of the goal item.
     */
    void execute(al::NerveKeeper* pKeeper) const override {
        pKeeper->getParent<GoalItem>()->exeCollect();
    }
};

NERVE_DECL(GoalItem, Collect)
NERVE_DECL(GoalItem, WaitCameraIn)
NERVE_DECL(GoalItem, WaitCameraOut)
NERVE_DECL(GoalItem, FinishAnimateOceanCase)
NERVE_DECL(GoalItem, AnimateCodeMoveRail)
NERVE_DECL(GoalItem, AnimateOceanCaseCameraReturn)
NERVE_DECL(GoalItem, FinalizeOceanCase)
NERVE_DECL(GoalItem, WaitOceanBowserExit)

/**
 * @brief Wait for the disaster fade out after skipping Fury Bowser's exit.
 */
class GoalItemNrvWaitDisasterFadeOutDoneSkip : public al::Nerve {
public:
    /**
     * @brief Run the nerve.
     * @param pKeeper The nerve keeper of the goal item.
     */
    void execute(al::NerveKeeper* pKeeper) const override {
        pKeeper->getParent<GoalItem>()->exeWaitDisasterFadeOutDone();
    }
};

NERVE_DECL(GoalItem, WaitDisasterFadeOutDone)
NERVE_DECL(GoalItem, WaitOceanBowserExitGigaBell)

/**
 * @brief Wait for the Giga Bell return cutscene after Fury Bowser was driven away.
 */
class GoalItemNrvWaitOceanBowserExitGigaBellDamage : public al::Nerve {
public:
    /**
     * @brief Run the nerve.
     * @param pKeeper The nerve keeper of the goal item.
     */
    void execute(al::NerveKeeper* pKeeper) const override {
        pKeeper->getParent<GoalItem>()->exeWaitOceanBowserExitGigaBell();
    }
};

NERVE_DECL(GoalItem, WaitOceanBowserExitReturn)
NERVE_DECL(GoalItem, EndLighthouseCutscene)

/**
 * @brief Done state reached once the lighthouse cutscene finished.
 */
class GoalItemNrvDoneLighthouse : public al::Nerve {
public:
    /**
     * @brief Run the nerve.
     * @param pKeeper The nerve keeper of the goal item.
     */
    void execute(al::NerveKeeper* pKeeper) const override {
        pKeeper->getParent<GoalItem>()->exeDone();
    }
};

/**
 * @brief Wait for the lighthouse shine cutscene after the rail move was skipped.
 */
class GoalItemNrvWaitForLighthouseShineCutsceneSkip : public al::Nerve {
public:
    /**
     * @brief Run the nerve.
     * @param pKeeper The nerve keeper of the goal item.
     */
    void execute(al::NerveKeeper* pKeeper) const override {
        pKeeper->getParent<GoalItem>()->exeWaitForLighthouseShineCutscene();
    }
};

NERVE_DECL(GoalItem, LighthouseShoneCutscene)

/**
 * @brief Wait for the lighthouse shine cutscene without a rail move.
 */
class GoalItemNrvWaitForLighthouseShineCutsceneInstant : public al::Nerve {
public:
    /**
     * @brief Run the nerve.
     * @param pKeeper The nerve keeper of the goal item.
     */
    void execute(al::NerveKeeper* pKeeper) const override {
        pKeeper->getParent<GoalItem>()->exeWaitForLighthouseShineCutscene();
    }
};

/**
 * @brief Wait for Fury Bowser's exit while he is already leaving.
 */
class GoalItemNrvWaitOceanBowserExitLeaving : public al::Nerve {
public:
    /**
     * @brief Run the nerve.
     * @param pKeeper The nerve keeper of the goal item.
     */
    void execute(al::NerveKeeper* pKeeper) const override {
        pKeeper->getParent<GoalItem>()->exeWaitOceanBowserExit();
    }
};

NERVE_DECL(GoalItem, WaitForLighthouseShineCutscene)

NERVES_MAKE_NOSTRUCT(GoalItem, CleanUpFinish, WaitCameraOut, EndLighthouseCutscene, DoneLighthouse,
                     Wait, CameraPlay, GuideMessage, Done, AppearAnim, CollectedIdle,
                     WaitInstantCollect, Disappear, Spin, SpinSpeedDown, AnimateMoveRail,
                     PreAnimate, PreAnimateInstant, Animate, AnimateMoveRailFinish,
                     CleanUpFinishNoBGM, WaitStartCollectDemo, CollectRepeated, CollectInstant,
                     Collect, WaitCameraIn, FinishAnimateOceanCase, AnimateCodeMoveRail,
                     AnimateOceanCaseCameraReturn, FinalizeOceanCase, WaitOceanBowserExit,
                     WaitDisasterFadeOutDoneSkip, WaitDisasterFadeOutDone,
                     WaitOceanBowserExitGigaBell, WaitOceanBowserExitGigaBellDamage,
                     WaitOceanBowserExitReturn, WaitForLighthouseShineCutsceneSkip,
                     LighthouseShoneCutscene, WaitForLighthouseShineCutsceneInstant,
                     WaitOceanBowserExitLeaving, WaitForLighthouseShineCutscene)

typedef al::FunctorV0M<GoalItem*, void (GoalItem::*)()> GoalItemFunctor;

/**
 * @brief Create the init info of the first actor linked under a link name.
 * @param rInfo The actor init info.
 * @param pLinkName The link name.
 * @return The created init info, or nullptr when nothing is linked.
 */
inline al::ActorInitInfo* tryCreateLinksActorInitInfo(const al::ActorInitInfo& rInfo,
                                                      const char* pLinkName) {
    s32 linkNum = al::calcLinkChildNum(rInfo, pLinkName);
    if (linkNum == 0) {
        return nullptr;
    }

    auto* placementInfo = new al::PlacementInfo();
    auto* info = new al::ActorInitInfo();
    if (linkNum == 1) {
        al::getLinksInfoByIndex(placementInfo, rInfo.getPlacementInfo(), pLinkName, 0);
        info->initViewIdSelf(placementInfo, rInfo);
    }

    return info;
}

/**
 * @brief Get the controller ports allowed to skip a demo.
 * @return The main controller port as a port mask.
 */
inline sead::BitFlag16 getSkipPorts() {
    return sead::BitFlag16(1 << al::getMainControllerPort());
}

/**
 * @brief Check whether Fury Bowser is in his last phase 3 fight.
 * @param pController The disaster mode controller.
 * @return True for the last phase 3 Fury Bowser.
 */
inline bool isLastPhase3DarkBowser(DisasterModeController* pController) {
    bool isLast = false;
    if (pController->getSuperBowser() != nullptr) {
        isLast = pController->getSuperBowser()->isLastPhase3Bowser();
    }

    return isLast;
}

/**
 * @brief Move an item flying along its rail below the rail, by the same offset for every player.
 * @param pActor The item.
 * @param pPlayer The player that collected the item.
 */
inline void setTransBelowRail(al::LiveActor* pActor, al::LiveActor* pPlayer) {
    sead::Vector3f trans = al::getTrans(pActor);
    trans.y += rc::isPlayerMini(pPlayer) ? -200.0f : -200.0f;
    al::setTrans(pActor, trans);
}

/**
 * @brief Calculate the horizontal distance between two positions.
 * @param rA The first position.
 * @param rB The second position.
 * @return The distance on the XZ plane.
 */
inline f32 calcDistanceXZ(const sead::Vector3f& rA, const sead::Vector3f& rB) {
    return sead::Vector2f(rA.x - rB.x, rA.z - rB.z).length();
}

/**
 * @brief Hide the timer of a timer challenge.
 * @param pItem The goal item.
 */
inline void hideChallengeTimer(GoalItem* pItem) {
    auto* itemDirector =
        static_cast<ProjectItemDirector*>(pItem->getSceneInfo()->itemDirectorBase);
    if (itemDirector != nullptr) {
        auto* layout = static_cast<SingleModeSceneLayout*>(itemDirector->getSceneLayout());
        if (layout != nullptr) {
            layout->hideTimer(true);
        }
    }
}
}  // namespace

/**
 * @brief Get the position of the bound player, or of the item when nobody is bound.
 * @return The position.
 */
inline const sead::Vector3f& GoalItem::getBindTrans() const {
    if (mPuppeteer != nullptr && mPuppeteer->isBind()) {
        auto* puppet = mPuppeteer->getPlayerPuppet();
        if (puppet != nullptr) {
            return puppet->getTrans();
        }
    }

    return al::getTrans(this);
}

/**
 * @brief Construct the goal item.
 * @param pName The actor name.
 */
GoalItem::GoalItem(const char* pName) : al::LiveActor(pName) {
    mLighthouse = nullptr;
    mAreaOutStep = 0;
    for (s32 i = 0; i < 5; i++) {
        mIsRailMove[i] = false;
    }
}

/**
 * @brief Initialize the goal item, its linked objects, cameras and stage switches.
 * @param rInfo The actor init info.
 */
void GoalItem::init(const al::ActorInitInfo& rInfo) {
    const char* modelName = nullptr;
    const char* puppetActionName = nullptr;
    al::tryGetArg(&mIsCompletelyIdle, rInfo, "IsCompletelyIdle");
    alPlacementFunction::tryGetModelName(&modelName, rInfo);
    if (!mIsEmptyItem && mPairItem == nullptr) {
        if (modelName != nullptr) {
            al::initActorWithArchiveName(this, rInfo, modelName, nullptr);
        } else {
            al::initActorWithArchiveName(this, rInfo, "GoalItem", nullptr);
        }
    }

    mIsPhase0 = SingleModeDataFunction::isPhase0(this);
    alActorSystemFunction::tryCompletelyRemoveFromExecutorDraw(this, &mDrawers);
    al::trySetShadowLength(this, rInfo, nullptr);
    al::addTransOffsetLocalDir(this, 270.0f, 1);
    mRestartInfo = tryCreateLinksActorInitInfo(rInfo, "PlayerRestartPos");

    bool isLandedPosSet;
    if (al::calcLinkChildNum(rInfo, "PlayerLandedPos") == 1) {
        al::PlacementInfo placementInfo;
        al::getLinksInfoByIndex(&placementInfo, rInfo.getPlacementInfo(), "PlayerLandedPos", 0);
        al::tryGetTrans(&mLandedPos, placementInfo);
        al::tryGetFront(&mLandedFront, placementInfo);
        isLandedPosSet = true;
    } else {
        isLandedPosSet = false;
        mLandedFront = sead::Vector3f::zero;
        mLandedPos = sead::Vector3f::zero;
    }

    mIsLandedPosSet = isLandedPosSet;
    if (al::calcLinkChildNum(rInfo, "InkPatch") >= 1 && !mIsEmptyItem) {
        mInkPatch = new InkPatch("InkPatch");
        al::initLinksActor(mInkPatch, rInfo, "InkPatch", 0);
        mInkCameraInStep = 40;
        al::tryGetArg(&mInkCameraInStep, rInfo, "InkCameraInStep");
        mInkCameraHoldStep = 120;
        al::tryGetArg(&mInkCameraHoldStep, rInfo, "InkCameraHoldStep");
    } else if (mIsEmptyItem) {
        mInkPatch = mPairItem->mInkPatch;
    }

    if (al::calcLinkChildNum(rInfo, "DoorLock") == 1) {
        mDoorLock = new DoorLock("DoorLock");
        al::initLinksActor(mDoorLock, rInfo, "DoorLock", 0);
    }

    al::tryGetArg(&mIsClearGenericSaveLocation, rInfo, "IsClearGenericSaveLocation");
    if (al::calcLinkChildNum(rInfo, "CameraArea") != 0) {
        al::PlacementInfo placementInfo;
        al::getLinksInfoByIndex(&placementInfo, rInfo.getPlacementInfo(), "CameraArea", 0);
        al::AreaInitInfo areaInfo(placementInfo, rInfo.getStageSwitchDirector());
        if (mIsEmptyItem) {
            mCameraArea = mPairItem->mCameraArea;
        } else {
            mCameraArea = new al::AreaObj("CameraArea");
            mCameraArea->init(areaInfo);
            mCameraArea->invalidate();
            auto* areaGroup = al::tryFindAreaObjGroup(this, "CameraArea");
            if (areaGroup != nullptr) {
                areaGroup->resisterAreaObj(mCameraArea);
            }
        }
    }

    if (al::calcLinkChildNum(rInfo, "InkCameraLookAtPoint") >= 1) {
        auto* info = tryCreateLinksActorInitInfo(rInfo, "InkCameraLookAtPoint");
        mInkLookAtPoint = new CameraLookAtPoint("InkCameraLookAtPoint");
        mInkLookAtPoint->init(*info);
    }

    mMtxConnector = al::tryCreateMtxConnector(this, rInfo);
    al::tryGetArg(&mCollisionCheckDist, rInfo, "CollisionCheckDist");
    al::tryGetArg(&mAnimationFrameCount, rInfo, "AnimationFrameCount");
    al::tryGetStringArg(&puppetActionName, rInfo, "PuppetActionName");
    mPuppeteer = new GoalItemBindPuppeteer("GoalItemBindPuppeteer", this);
    mPuppeteer->setActionName("GoalItemGet");
    mCameraPos = al::getCameraPos_RS(getSceneCameraInfo(), 0);
    mCameraAt = al::getCameraAt_RS(getSceneCameraInfo(), 0);
    mProgramableCamera =
        al::initProgramableCamera_RS(this, rInfo, "GoalItem", &mCameraPos, &mCameraAt, nullptr);
    al::initNerve(this, &NrvGoalItemWait, 2);
    al::tryGetStringArg(&mSharedLabel, rInfo, "SharedLabel");
    if (mSharedLabel != nullptr) {
        s32 waitFrames = 0;
        s32 delayFrames = 60;
        al::tryGetArg(&waitFrames, rInfo, "WaitFrames");
        al::tryGetArg(&delayFrames, rInfo, "DelayFrames");
        mDemoCamera = new ActorStateDemoCameraProto(this, rInfo, mSharedLabel, waitFrames,
                                                    delayFrames, false);
        al::initNerveState(this, mDemoCamera, &NrvGoalItemCameraPlay, "CameraPlay");
        mDemoCamera->setNoDemo();
    }

    al::tryGetArg(&mIslandClearStartFrame, rInfo, "IslandClearStartFrame");
    al::tryGetArg(&mIslandClearEndFrame, rInfo, "IslandClearEndFrame");
    al::tryGetArg(&mIsNoIslandFlagCutscene, rInfo, "IsNoIslandFlagCutscene");
    al::tryGetArg(&mIsRetainAppearAngle, rInfo, "IsRetainAppearAngle");
    al::tryGetArg(&mIsAppearWait, rInfo, "IsAppearWait");
    mIsAppearWaitDefault = mIsAppearWait;
    al::tryGetArg(&mIsConsistentAngle, rInfo, "IsConsistentAngle");
    al::tryGetArg(&mIsSetDistance, rInfo, "IsSetDistance");
    al::tryGetArg(&mIsInkMeNot, rInfo, "IsInkMeNot");
    al::tryGetArg(&mIsDisasterShine, rInfo, "IsDisasterShine");
    al::tryGetArg(&mIsLuckyShine, rInfo, "IsLuckyShine");
    al::tryGetArg(&mIsCodeBasedPath, rInfo, "IsCodeBasedPath");
    al::tryGetArg(&mArcHeight, rInfo, "ArcHeight");
    al::tryGetArg(&mAreaOutStep, rInfo, "AreaOutStep");
    mAreaOutStep = 0;
    al::tryGetArg(&mInkReturnMoveCount, rInfo, "InkReturnMoveCount");
    al::tryGetArg(&mIsSetExitAngle, rInfo, "IsSetExitAngle");
    al::tryGetArg(&mInkInterpoleStep, rInfo, "InkInterpoleStep");
    if (mIsSetExitAngle) {
        al::tryGetArg(&mExitAngleH, rInfo, "ExitAngleH");
        al::tryGetArg(&mExitAngleV, rInfo, "ExitAngleV");
        al::tryGetArg(&mExitDirRate, rInfo, "ExitDirRate");
        al::tryGetArg(&mExitZoomInRate, rInfo, "ExitZoomInRate");
        al::tryGetArg(&mExitTgtOffsetY, rInfo, "ExitTgtOffsetY");
        al::tryGetArg(&mExitCamBlendCount, rInfo, "ExitCamBlendCount");
        al::tryGetArg(&mIsSetExitInterpolate, rInfo, "IsSetExitInterpolate");
    }

    if (al::calcLinkChildNum(rInfo, "BuriedObject") == 1) {
        auto* info = tryCreateLinksActorInitInfo(rInfo, "BuriedObject");
        mSinkedItem = new SinkedItem("BuriedObject");
        mSinkedItem->init(*info);
    }

    al::tryGetArg(&mIsSwitchOnGet, rInfo, "IsSwitchOnGet");
    if (mIsSwitchOnGet) {
        al::listenStageSwitchOnGoalItemGet(this,
                                           GoalItemFunctor(this, &GoalItem::onGoalItemGetSwitch));
        al::invalidateHitSensors(this);
    }

    al::tryGetArg(&mMoveCount, rInfo, "MoveCount");
    mMoveCount = 120;
    al::tryGetArg(&mShineId, rInfo, "ScenarioID");
    al::tryGetArg(&mCameraDistance, rInfo, "CameraDistance");
    if (mCameraDistance == 750.0f) {
        mCameraDistance = 680.0f;
    }

    s32 quadrant = 0;
    al::tryGetArg(&quadrant, rInfo, "Quadrant");
    al::tryGetArg(&mSafeAngle, rInfo, "SafeAngle");
    if (quadrant != 0) {
        mIslandId = IslandDataFunction::getIslandIDFromParam(quadrant);
    } else {
        mIslandId = mPlacementHolder->getZoneNo();
    }

    bool isUseObjectCamera = false;
    al::tryGetArg(&isUseObjectCamera, rInfo, "IsUseObjectCamera");
    if (isUseObjectCamera) {
        mObjectCamera = al::initObjectCamera_RS(this, rInfo, nullptr);
        al::tryGetArg(&mFocusCameraInStep, rInfo, "FocusCameraInStep");
        al::tryGetArg(&mFocusCameraOutStep, rInfo, "FocusCameraOutStep");
        al::tryGetArg(&mFocusCameraHoldStep, rInfo, "FocusCameraHoldStep");
        auto* poser = mObjectCamera->getPoser();
        poser->setInterpoleStep(mFocusCameraInStep);
        poser->setEndInterpoleStep(mFocusCameraOutStep);
    } else if (mIslandId < 0) {
        mObjectCamera = al::initObjectCameraManual_RS(this, "FixedActor", rInfo);
        auto* poser = mObjectCamera->getPoser();
        poser->setInterpoleStep(0);
        poser->setEndInterpoleStep(0);
        mIsFixedCamera = true;
    }

    al::tryGetArg(&mIsMultiCollect, rInfo, "IsMultiCollect");
    al::tryGetArg(&mIsTimerChallenge, rInfo, "IsTimerChallenge");
    al::tryGetArg(&mIsKeepVerticalAngle, rInfo, "IsKeepVerticalAngle");
    al::tryGetArg(&mIsReturnToOldCameraPos, rInfo, "IsReturnToOldCameraPos");

    s32 sensorNum = mHitSensorKeeper->getSensorNum();
    for (s32 i = 0; i < sensorNum; i++) {
        if (al::isSensorBindableAll(al::getHitSensor(this, i))) {
            break;
        }
    }

    if (al::isExistRail(rInfo)) {
        mRailMover = new ActorRailBrakeMover(this, 60.0f, nullptr);
        al::setRailPosToCoord(this, al::getRailCoord(this));
        mIsRailMove[mShineId - 1] = true;
    }

    al::tryGetArg(&mPhase2OffsetY, rInfo, "phase2OffsetY");
    al::tryGetArg(&mPhase2DistanceOffset, rInfo, "phase2DistanceOffset");
    al::tryGetArg(&mPhase3OffsetY, rInfo, "phase3OffsetY");
    al::tryGetArg(&mPhase3DistanceOffset, rInfo, "phase3DistanceOffset");
    al::tryGetArg(&mInkLinkId, rInfo, "InkLinkID");
    al::killPrePassLight(this, "体ポイントライト", -1);
    if (al::listenStageSwitchOn(this, "SwitchAppear", GoalItemFunctor(this, &GoalItem::appear)) ||
        al::listenStageSwitchOnOff(this, "SwitchTimerAppear",
                                   GoalItemFunctor(this, &GoalItem::appear),
                                   GoalItemFunctor(this, &GoalItem::hide)) ||
        al::listenStageSwitchOn(this, "SwitchIdleAppear",
                                GoalItemFunctor(this, &GoalItem::appearIdle))) {
        makeActorDead();
    } else {
        makeActorAppeared();
    }

    al::listenStageSwitchOn(this, "SwitchKill", GoalItemFunctor(this, &GoalItem::hide));
    al::listenStageSwitchOn(this, "SwitchTryKill", GoalItemFunctor(this, &GoalItem::tryHide));

    s32 disasterModeSetting;
    al::tryGetArg(&disasterModeSetting, rInfo, "DisasterModeSetting");
    mDisasterModeSetting = disasterModeSetting;
    al::setNerve(this, &NrvGoalItemWait);
    if (!mIsEmptyItem && mPairItem == nullptr && mIsMultiCollect) {
        mPairItem = new GoalItem("GoalItemEmpty");
        mPairItem->mIsEmptyItem = true;
        mPairItem->mPairItem = this;
        al::initActorWithArchiveName(mPairItem, rInfo, "GoalItemEmpty", nullptr);
        mPairItem->init(rInfo);
        mPairItem->makeActorDead();
    }

    mDisasterFadeoutFunctor = new GoalItemFunctor(this, &GoalItem::disasterFadeoutDoneFunc);
    mIsNekoShine = SingleModeDataFunction::isNekoShine(this, mIslandId, mShineId);
}

/**
 * @brief Called when the goal item get switch turns on: makes the item collectable.
 */
void GoalItem::onGoalItemGetSwitch() {
    if (mIsSwitchOnGet) {
        al::validateHitSensors(this);
        mIsSwitchOnGet = false;
    }
}

/**
 * @brief Make the item disappear if it is idle.
 */
void GoalItem::hide() {
    if (canCollect() || (!mIsAppearCamera && al::isNerve(this, &NrvGoalItemAppearAnim))) {
        al::setNerve(this, &NrvGoalItemDisappear);
    }
}

/**
 * @brief Appear the item without its appear animation.
 */
void GoalItem::appearIdle() {
    if (!mIsMultiCollect) {
        if (SingleModeDataFunction::isScenarioComplete(this, mIslandId - 1, mShineId - 1)) {
            al::tryOnStageSwitch(this, "SwitchDemoEndAnimateOn");
            al::tryOnStageSwitch(this, "SwitchDemoAnimateEndOn");
            if (mAnimEndFunctor != nullptr) {
                (*mAnimEndFunctor)();
            }

            kill();
            return;
        }
    } else if (SingleModeDataFunction::isScenarioComplete(this, mIslandId - 1, mShineId - 1)) {
        if (!mIsEmptyItem) {
            mPairItem->appearIdle();
            return;
        }
    } else if (mIsEmptyItem) {
        return;
    }

    al::setTrans(this, mInitTrans);
    al::setQuat(this, mInitQuat);
    mIsAppearWait = false;
    al::LiveActor::appear();
    al::hideModelIfShow(this);
    al::tryStartAction(this, "Before");
    al::setNerve(this, &NrvGoalItemWait);
    al::tryOffStageSwitch(this, "SwitchDemoAnimateEndOn");
    al::tryOffStageSwitch(this, "SwitchCollectOn");
}

/**
 * @brief Kill the item if it is idle.
 */
void GoalItem::tryHide() {
    if (canCollect()) {
        makeActorDead();
    }
}

/**
 * @brief Called once the disaster fade out is done.
 */
void GoalItem::disasterFadeoutDoneFunc() {
    mIsDisasterFadeoutDone = true;
}

/**
 * @brief Register the item to its holder and lighthouse once everything is placed.
 */
void GoalItem::initAfterPlacement() {
    if (mMtxConnector != nullptr) {
        al::attachMtxConnectorToCollision(mMtxConnector, this, 50.0f, mCollisionCheckDist);
    }

    if (isCollected()) {
        mIsCollected = true;
        mIsCollectPending = true;
        al::setNerve(this, &NrvGoalItemCleanUpFinish);
        if (mIsEmptyItem) {
            if (mInkPatch != nullptr) {
                mInkPatch->kill();
            }
        } else {
            makeActorDead();
            if (mInkPatch != nullptr) {
                mInkPatch->kill();
            }
        }
    } else if (mInkPatch != nullptr) {
        mInkPatch->appear();
    }

    if (al::isExistSceneObj(this, SceneObjID_GoalItemHolder)) {
        mHolder = al::tryGetSceneObj<GoalItemHolder>(this, SceneObjID_GoalItemHolder);
        if (!mIsEmptyItem) {
            mHolder->registerGoalItem(this, 0, mIslandId);
        }
    }

    s32 phase = SingleModeDataFunction::getUnlockedPhase(this);
    if (mIslandId >= 1) {
        auto* island = static_cast<IslandHolder*>(
            al::getSceneObj<IslandKeeper>(this, SceneObjID_IslandKeeper)->findIsland(mIslandId));
        if (island != nullptr) {
            mLighthouse = island->getLighthouse();
        }
    }

    if (phase == 1 || phase == 3 || phase == 5) {
        if (mIsInkMeNot) {
            makeActorDead();
        }

        if (mLighthouse != nullptr) {
            mLighthouse->setInkMeNot(mIsInkMeNot);
        }
    } else {
        mIsInkMeNot = false;
    }

    al::hideSilhouetteModel(this);
    mInitTrans = al::getTrans(this);
    mInitQuat = al::getQuat(this);
    if (mShineId == 1 && mInkLinkId >= 0 && mLighthouse != nullptr) {
        mLighthouse->setInkLinkID(mInkLinkId);
    }

    if (mInkLookAtPoint != nullptr && mLighthouse != nullptr) {
        mLighthouse->setupCameraLookAtPoint(mInkLookAtPoint, mInkReturnMoveCount);
    }

    if (mSinkedItem != nullptr && mLighthouse != nullptr) {
        mLighthouse->setSinkedItem(mSinkedItem);
    }
}

/**
 * @brief Continue the collect sequence after Fury Bowser left.
 */
void GoalItem::finishBowserExit() {
    if (mHolder != nullptr && mHolder->getNextGoalItemGuideMessage(this) != nullptr) {
        al::setNerve(this, &NrvGoalItemGuideMessage);
        return;
    }

    al::setNerve(this, &NrvGoalItemDone);
}

/**
 * @brief Request the end of the collect animation demo.
 */
void GoalItem::requestEndAnimDemo() {
    al::tryOnStageSwitch(this, "SwitchDemoAnimateEndOn");
    if (mAnimEndFunctor != nullptr) {
        (*mAnimEndFunctor)();
    }
}

/**
 * @brief Set the stage switches of the collect animation demo.
 * @param isOn Whether the demo is starting.
 */
void GoalItem::setStageSwitchAnimState(bool isOn) {
    if (isOn) {
        al::tryOnStageSwitchInstant(this, "SwitchDemoOn");
        al::tryOnStageSwitch(this, "SwitchDemoAnimateEndOn");
        if (mAnimEndFunctor != nullptr) {
            (*mAnimEndFunctor)();
        }

        auto* islandKeeper = IslandKeeper::tryGetIslandKeeper(this);
        if (islandKeeper != nullptr) {
            islandKeeper->setIslandLODDisable(mIslandId, true);
        }
    } else {
        al::tryOffStageSwitchInstant(this, "SwitchDemoOn");
        auto* islandKeeper = IslandKeeper::tryGetIslandKeeper(this);
        if (islandKeeper != nullptr) {
            islandKeeper->setIslandLODDisable(mIslandId, false);
        }
    }
}

/**
 * @brief Appear the item, playing its appear animation when needed.
 */
void GoalItem::appear() {
    if (!mIsMultiCollect) {
        if (SingleModeDataFunction::isScenarioComplete(this, mIslandId - 1, mShineId - 1)) {
            al::tryOnStageSwitch(this, "SwitchDemoEndAnimateOn");
            al::tryOnStageSwitch(this, "SwitchDemoAnimateEndOn");
            if (mAnimEndFunctor != nullptr) {
                (*mAnimEndFunctor)();
            }

            kill();
            return;
        }
    } else if (SingleModeDataFunction::isScenarioComplete(this, mIslandId - 1, mShineId - 1)) {
        if (!mIsEmptyItem) {
            mPairItem->appear();
            return;
        }
    } else if (mIsEmptyItem) {
        return;
    }

    al::setTrans(this, mInitTrans);
    al::setQuat(this, mInitQuat);
    mIsAppearWait = mIsAppearWaitDefault;
    al::LiveActor::appear();
    if (mIsAppearWait) {
        al::hideModelIfShow(this);
    }

    if (mObjectCamera != nullptr) {
        mIsAppearCamera = true;
    }

    bool isAppearAnim = false;
    if (!mIsAppearWait || mObjectCamera != nullptr) {
        isAppearAnim = !mIsCompletelyIdle;
    }

    if (mPlayer != nullptr) {
        if (!mIsMultiCollect) {
            al::tryStartAction(this, "After");
            al::setNerve(this, &NrvGoalItemCollectedIdle);
        } else {
            al::startVisAnimAndSetFrameAndStop(this, "GoalItemGet", 0.0f);
            if (!isAppearAnim) {
                al::setNerve(this, &NrvGoalItemWait);
            } else {
                al::hideModelIfShow(this);
                al::setNerve(this, &NrvGoalItemAppearAnim);
            }
        }
    } else if (!isAppearAnim) {
        al::setNerve(this, &NrvGoalItemWait);
    } else {
        al::hideModelIfShow(this);
        al::setNerve(this, &NrvGoalItemAppearAnim);
    }

    al::tryOffStageSwitch(this, "SwitchDemoAnimateEndOn");
    al::tryOffStageSwitch(this, "SwitchCollectOn");
}

/**
 * @brief Appear the item immediately, without any animation.
 */
void GoalItem::quickAppear() {
    if (mIsMultiCollect) {
        if (SingleModeDataFunction::isScenarioComplete(this, mIslandId - 1, mShineId - 1)) {
            if (!mIsEmptyItem) {
                mPairItem->quickAppear();
                return;
            }
        } else if (mIsEmptyItem) {
            return;
        }
    }

    al::stopSeByName(this, "PgAppear");
    al::LiveActor::appear();
    if (mPlayer == nullptr) {
        al::tryStartAction(this, "Before");
        return;
    }

    if (mIsMultiCollect) {
        al::showModelIfHide(this);
        al::startVisAnimAndSetFrameAndStop(this, "GoalItemGet", 0.0f);
        al::tryStartAction(this, "Before");
        al::setNerve(this, &NrvGoalItemWait);
    } else {
        al::tryStartAction(this, "After");
        al::setNerve(this, &NrvGoalItemCollectedIdle);
    }
}

/**
 * @brief Appear the item hidden.
 */
void GoalItem::appearHidden() {
    if (mIsMultiCollect) {
        if (SingleModeDataFunction::isScenarioComplete(this, mIslandId - 1, mShineId - 1)) {
            if (!mIsEmptyItem) {
                al::setTrans(mPairItem, al::getTrans(this));
                mPairItem->appearHidden();
                return;
            }
        } else if (mIsEmptyItem) {
            return;
        }
    }

    al::LiveActor::appear();
    al::hideModelIfShow(this);
    al::setNerve(this, &NrvGoalItemWait);
}

/**
 * @brief Appear the item to be collected right away by the player.
 * @param isAddDemoPlayer Whether the nearest player takes part in the demo.
 */
void GoalItem::appearCollect(bool isAddDemoPlayer) {
    if (mIsMultiCollect) {
        if (SingleModeDataFunction::isScenarioComplete(this, mIslandId - 1, mShineId - 1)) {
            if (!mIsEmptyItem) {
                rc::requestEndDemoInGameCutscene(this);
                rc::resetImmediateSwitchFlag(this);
                al::setTrans(mPairItem, al::getTrans(this));
                mPairItem->appearHidden();
                return;
            }
        } else if (mIsEmptyItem) {
            return;
        }
    }

    al::LiveActor::appear();
    al::invalidateClipping(this);
    al::hideModelIfShow(this);
    if (mHolder != nullptr) {
        mHolder->setCurrentGoalItem(this);
    }

    if (isAddDemoPlayer) {
        rc::addDemoPlayer(static_cast<PlayerActor*>(al::findNearestPlayerActor(this)));
    }

    rc::setDemoAudioType(this, alSeFunction::DemoType(1));
    al::setNerve(this, &NrvGoalItemWaitInstantCollect);
}

/**
 * @brief Kill the item if it is idle.
 */
void GoalItem::makeActorDead() {
    if (canCollect()) {
        al::LiveActor::makeActorDead();
    }
}

/**
 * @brief Check whether the item is idle and can be collected.
 * @return True while waiting or spinning.
 */
bool GoalItem::canCollect() const {
    return al::isNerve(this, &NrvGoalItemWait) || al::isNerve(this, &NrvGoalItemSpin) ||
           al::isNerve(this, &NrvGoalItemSpinSpeedDown);
}

/**
 * @brief Kill the item and its empty twin.
 */
void GoalItem::makeActorDeadAll() {
    if (canCollect()) {
        al::LiveActor::makeActorDead();
    }

    if (mPairItem != nullptr && mIsMultiCollect && mPairItem->canCollect()) {
        mPairItem->makeActorDead();
    }
}

/**
 * @brief Check whether the scenario of the item is complete.
 * @return True once the item was collected.
 */
bool GoalItem::isCollected() {
    return SingleModeDataFunction::isScenarioComplete(this, mIslandId - 1, mShineId - 1);
}

/**
 * @brief Start a demo.
 * @param demoType The demo type.
 */
void GoalItem::startDemoActor(s32 demoType) {
    if (al::isAlive(this) && !(al::isClipped(this) && al::isInvalidClipping(this))) {
        mIsDemo = true;
    }
}

/**
 * @brief End a demo.
 * @param demoType The demo type.
 */
void GoalItem::endDemoActor(s32 demoType) {
    mIsDemo = false;
    if (al::isAlive(this)) {
        al::isClipped(this);
    }
}

/**
 * @brief Appear the item at a checkpoint.
 */
void GoalItem::appearCheckpoint() {
    appear();
}

/**
 * @brief Get the vertical offset of the item.
 * @return The vertical offset.
 */
f32 GoalItem::getVerticalOffset() {
    return 235.0f;
}

/**
 * @brief Set the direction the camera looks at the item from.
 * @param rFront The front direction.
 * @param isKeep Whether the angle is kept.
 * @param angle The angle.
 */
void GoalItem::setFront(sead::Vector3f& rFront, bool isKeep, f32 angle) {
    mIsFrontSet = true;
    mFront = rFront;
    mIsFrontAngleSet = isKeep;
    mFrontAngle = angle;
}

/**
 * @brief Update the item every frame.
 */
void GoalItem::control() {
    if (mPuppeteer != nullptr) {
        mPuppeteer->update();
    }

    if (mMtxConnector == nullptr) {
        return;
    }

    if (al::isNerve(this, &NrvGoalItemAnimateMoveRail) ||
        al::isNerve(this, &NrvGoalItemPreAnimate) ||
        al::isNerve(this, &NrvGoalItemPreAnimateInstant) || al::isNerve(this, &NrvGoalItemAnimate) ||
        al::isNerve(this, &NrvGoalItemAnimateMoveRailFinish)) {
        return;
    }

    al::connectPoseQT(this, mMtxConnector);
}

/**
 * @brief Attack the sensors touching the item.
 * @param pSelf The sensor of the item.
 * @param pOther The touched sensor.
 */
void GoalItem::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (((al::isSensorKoopaJr(pOther) && al::isSensorName(pOther, "Body")) ||
         al::isSensorKickKoura(pOther)) &&
        al::isNerve(this, &NrvGoalItemWait)) {
        al::setNerve(this, &NrvGoalItemSpin);
    }

    if (al::isSensorDoorKey(pOther) || al::isSensorNpc(pOther) ||
        al::isSensorHostName(pOther, "BallNeko") ||
        (al::isSensorPackunWithPot(pOther) && al::isSensorName(pOther, "Push"))) {
        al::sendMsgPush(pOther, pSelf);
    }
}

/**
 * @brief Check whether the item can react to attacks.
 * @return True while waiting or spinning.
 */
bool GoalItem::canReact() const {
    return al::isNerve(this, &NrvGoalItemWait) || al::isNerve(this, &NrvGoalItemSpin) ||
           al::isNerve(this, &NrvGoalItemSpinSpeedDown);
}

/**
 * @brief Receive a message.
 * @param pMsg The message.
 * @param pOther The sending sensor.
 * @param pSelf The receiving sensor.
 * @return Whether the message was handled.
 */
bool GoalItem::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                          al::HitSensor* pSelf) {
    if (al::isNerve(this, &NrvGoalItemCollectedIdle) ||
        al::isNerve(this, &NrvGoalItemCleanUpFinishNoBGM)) {
        return false;
    }

    if (al::isMsgBindSteal(pMsg)) {
        return true;
    }

    if (al::isSensorMapObj(pSelf)) {
        if (al::isMsgItemGetDirectAll(pMsg) || rc::isMsgPlayerCheckpointTouch(pMsg)) {
            if (mDoorLock != nullptr && al::isAlive(mDoorLock)) {
                return false;
            }

            if (mHolder != nullptr && mHolder->getCurrentGoalItem() != nullptr) {
                return false;
            }

            if (!canCollect() || !rc::isReallyPlayerActor(pOther)) {
                return false;
            }

            auto* timerManager = TimerManager::tryGetTimerManager(this);
            if (timerManager != nullptr && !mIsTimerChallenge) {
                timerManager->forceCancelCurrent(false, false);
            }

            al::invalidateClipping(this);
            if (mHolder != nullptr) {
                mHolder->setCurrentGoalItem(this);
            }

            if (mIsCollected) {
                mPlayer = al::getSensorHost(pOther);
                al::setAppearItemAttackerSensor(this, pOther);
                al::setNerve(this, &NrvGoalItemCollectRepeated);
            } else {
                al::setNerve(this, &NrvGoalItemWaitStartCollectDemo);
            }

            return true;
        }

        if (!canCollect()) {
            return false;
        }

        if (al::isMsgEnemyAttackFire(pMsg) || al::isMsgEnemyAttackBoomerang(pMsg) ||
            al::isMsgLaserAttack(pMsg) || al::isMsgBallAttackHold(pMsg) ||
            al::isMsgExplosion(pMsg) || al::isMsgKeyThrow(pMsg) || al::isMsgBallAttack(pMsg) ||
            al::isMsgNekoAttack(pMsg) || rc::isMsgPackunThrowAttack(pMsg)) {
            if (al::isNerve(this, &NrvGoalItemWait) ||
                al::isNerve(this, &NrvGoalItemSpinSpeedDown)) {
                al::setNerve(this, &NrvGoalItemSpin);
            }

            return true;
        }

        if (al::isMsgPlayerFireBallAttack(pMsg) || al::isMsgPlayerBoomerangAttack(pMsg) ||
            al::isMsgPlayerHipDropAll(pMsg) || al::isMsgPlayerClimbAttack(pMsg) ||
            al::isMsgPlayerSpinAttack(pMsg) || al::isMsgPlayerTailAttack(pMsg)) {
            if (al::isNerve(this, &NrvGoalItemWait) ||
                al::isNerve(this, &NrvGoalItemSpinSpeedDown)) {
                rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
                if (al::isNerve(this, &NrvGoalItemSpinSpeedDown)) {
                    mIsSpinRestart = true;
                }

                al::setNerve(this, &NrvGoalItemSpin);
            }

            return true;
        }

        return false;
    }

    if (al::isMsgBindStart(pMsg)) {
        if (!rc::isAnyActiveDemo(this) || !rc::isActiveDemoInGameCutscene(this)) {
            return false;
        }

        if (pOther != nullptr && mPlayer == nullptr) {
            mPlayer = al::getSensorHost(pOther);
        }

        if (!al::isNerve(this, &NrvGoalItemWaitInstantCollect) &&
            !al::isNerve(this, &NrvGoalItemWaitStartCollectDemo)) {
            return false;
        }

        al::tryDeleteEffectAndParticle(this, "Before");
        if (mIsCollected) {
            return false;
        }

        rc::hidePlayerHoldingItem(mPlayer, true, true);
        rc::cancelAllPlayersForDemo(this);
        if (mHolder != nullptr) {
            mHolder->setCollectDemo(true);
        }

        if (al::isNerve(this, &NrvGoalItemWaitInstantCollect)) {
            al::setNerve(this, &NrvGoalItemCollectInstant);
        } else {
            al::setNerve(this, &NrvGoalItemCollect);
        }

        if (al::isSingleMode(this)) {
            auto* controller = DisasterModeController::tryGetController(this);
            sead::Vector3f playerTrans = al::getTrans(mPlayer);
            s32 playerPos[3] = {static_cast<s32>(playerTrans.x), static_cast<s32>(playerTrans.y),
                                static_cast<s32>(playerTrans.z)};
            if (!SingleModeDataFunction::reportShineEvent(this, this, 5)) {
                SingleModeDataFunction::setPlayReportData(this, preport::Key(30), playerPos, 3);
                // The figure name is really a sead::SafeString in the game.
                SingleModeDataFunction::setPlayReportData(
                    this, preport::Key(32),
                    *reinterpret_cast<sead::SafeString*>(
                        const_cast<char*>(rc::getPlayerRealFigureName(mPlayer))));
                if (mIslandId >= 1) {
                    SingleModeDataFunction::setPlayReportData(this, preport::Key(35),
                                                              mIslandId - 1);
                } else {
                    SingleModeDataFunction::setPlayReportData(this, preport::Key(35), mIslandId);
                }

                SingleModeDataFunction::setPlayReportData(this, preport::Key(62), mShineId);
                SingleModeDataFunction::setPlayReportData(this, preport::Key(42),
                                                          static_cast<s32>(controller->isDisasterMode()));
                SingleModeDataFunction::endPlayReport(this);
            }
        }

        return !mIsCollected;
    }

    if (al::isMsgBindInit(pMsg)) {
        if (mPlayer == nullptr) {
            return false;
        }

        alActorSystemFunction::addBackToExecutorDraw(this, &mDrawers);
        auto* player = mPlayer;
        rc::pausePlayerEquip(player);
        if (!al::isNerve(this, &NrvGoalItemCollectInstant)) {
            rc::addDemoPlayer(static_cast<PlayerActor*>(player));
        }

        alLiveActorFunction::setAlphaCtrlOn(mPlayer, false);
        IslandMap::setIslandWarpEnable(this, false);
        mPuppeteer->startBind(pOther, pSelf);
        rc::cancelInvincibleMarioForce(pOther, true, true);
        return true;
    }

    return false;
}

/**
 * @brief Receive a message from a screen pointer.
 * @param pMsg The message.
 * @param pPointer The screen pointer.
 * @param pTarget The pointed target.
 * @return Whether the message was handled.
 */
bool GoalItem::receiveMsgScreenPointSM(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                       al::ScreenPointTarget* pTarget) {
    if (!al::isMsgTouchAssist(pMsg)) {
        return false;
    }

    if (!al::isNerve(this, &NrvGoalItemWait) && !al::isNerve(this, &NrvGoalItemSpinSpeedDown)) {
        return false;
    }

    al::setNerve(this, &NrvGoalItemSpin);
    return true;
}

/**
 * @brief Check whether the item flies to the lighthouse along its rail.
 * @return True when the rail of the scenario is used.
 */
bool GoalItem::isRailMove() {
    if (mShineId < 1) {
        return false;
    }

    return mIsRailMove[mShineId - 1];
}

/**
 * @brief Disappear with a poof.
 */
void GoalItem::exeDisappear() {
    if (al::isFirstStep(this)) {
        al::stopAllSeId(this, "Before", 10, nullptr);
        al::tryDeleteEmitterAndParticleAll(this);
        al::hideModelIfShow(this);
    } else if (al::isStep(this, 2)) {
        al::tryEmitEffect(this, "Poof", nullptr);
    }

    if (al::isStep(this, 30)) {
        makeActorDead();
    }
}

/**
 * @brief Show the item and start its rise animation.
 */
void GoalItem::playAppearRiseAnim() {
    al::showModelIfHide(this);
    sead::Vector3f cameraPos = al::getCameraPos_RS(this, 0);
    if ((cameraPos - al::getTrans(this)).length() < 2500.0f) {
        al::startSe(this, "PgAppear", nullptr);
    }

    al::tryStartAction(this, "AppearRise");
}

/**
 * @brief Play the appear animation, focusing the camera on the item if needed.
 */
void GoalItem::exeAppearAnim() {
    if (al::isFirstStep(this)) {
        if (mIsAppearCamera) {
            if (!mIsFixedCamera) {
                if (!rc::requestStartDemoInGameCutscene(this)) {
                    al::setNerve(this, &NrvGoalItemAppearAnim);
                    return;
                }

                rc::setDemoFullEffectUpdate(this, true);
                rc::setDemoAudioType(this, alSeFunction::DemoType(3));
                rc::addDemoActor(this);
                auto* poser = static_cast<al::CameraPoserFixActor*>(mObjectCamera->getPoser());
                if (al::isEqualString(poser->getName(), "FixedActor")) {
                    poser->setTargetActor(this);
                    poser->storeCamera(al::getCameraPos_RS(this, 0), al::getCameraAt_RS(this, 0));
                    if (mIsConsistentAngle) {
                        sead::Vector3f dir = al::getCameraPos_RS(this, 0) - al::getCameraAt_RS(this, 0);
                        dir.normalize();
                        poser->setDirectAngle(dir);
                    }
                }

                poser->setEndInterpoleStep(60);
                poser->mIsReturnEnd = false;
                poser->setTargetActor(this);
                al::invalidateClipping(this);
                al::startCamera_RS(this, mObjectCamera, -1);
                al::setNerve(this, &NrvGoalItemWaitCameraIn);
                return;
            }

            mIsAppearCamera = false;
        }

        playAppearRiseAnim();
    }

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvGoalItemWait);
    }
}

/**
 * @brief Wait for the bind of a player collecting the item without its demo.
 */
void GoalItem::exeWaitInstantCollect() {
    rc::requestBindAllPlayerAcceptReviveBubble(this, al::getHitSensor(this, "Bind"));
}

/**
 * @brief Wait for the collect demo to start.
 */
void GoalItem::exeWaitStartCollectDemo() {
    if (al::isFirstStep(this)) {
        if (!rc::requestStartDemoInGameCutscene(this)) {
            al::setNerve(this, &NrvGoalItemWaitStartCollectDemo);
            return;
        }

        rc::setDemoFullEffectUpdate(this, true);
        rc::addDemoActor(this);
        rc::setDemoAudioType(this, alSeFunction::DemoType(1));
        rc::requestBindAllPlayerAcceptReviveBubble(this, al::getHitSensor(this, "Bind"));
    }
}

/**
 * @brief Wait to be collected.
 */
void GoalItem::exeWait() {
    if (al::isFirstStep(this)) {
        if (!al::isActionPlaying(this, "Before") || mIsLuckyShine) {
            al::tryStartAction(this, "Before");
        }

        al::showModelIfHide(this);
        mIsSpinEnd = false;
        if (mIsPhase0) {
            al::tryEmitEffect(this, "BeforeFlashy", nullptr);
            mPlayer = rc::findNearestActivePlayerActor(this);
            mIsBeforeFlashy = true;
        } else {
            al::tryDeleteEffect(this, "BeforeFlashy");
            mIsBeforeFlashy = false;
        }
    }

    if (mIsDemo) {
        mIsWaitEffectDeleted = true;
        if (al::isActionPlaying(this, "Before")) {
            al::tryDeleteEffectAndParticle(this, "Before");
        }
    } else if (mIsWaitEffectDeleted) {
        al::tryEmitEffect(this, "Before", nullptr);
        mIsWaitEffectDeleted = false;
    }

    if (!mIsPhase0) {
        return;
    }

    f32 distance = calcDistanceXZ(al::getTrans(this), al::getTrans(mPlayer));
    if (mIsBeforeFlashy) {
        if (distance < 4500.0f) {
            al::tryDeleteEffect(this, "BeforeFlashy");
            mIsBeforeFlashy = false;
        }
    } else if (distance > 4600.0f) {
        al::tryEmitEffect(this, "BeforeFlashy", nullptr);
        mIsBeforeFlashy = true;
    }
}

/**
 * @brief Spin after an attack.
 */
void GoalItem::exeSpin() {
    if (al::isFirstStep(this)) {
        al::startHitReaction(this, "撫でる");
        if (mIsSpinRestart) {
            mIsSpinRestart = false;
        } else {
            al::tryStartAction(this, "Before");
        }

        al::setActionFrameRate(this, 11.0f);
    }

    if (al::isGreaterEqualStep(this, 20)) {
        al::setNerve(this, &NrvGoalItemSpinSpeedDown);
    }
}

/**
 * @brief Slow the spin down back to the wait animation.
 */
void GoalItem::exeSpinSpeedDown() {
    if (al::isFirstStep(this)) {
        al::setActionFrameRate(this, 10.0f);
    }

    al::setActionFrameRate(this, (al::getNerveStep(this) / -140.0f + 1.0f) * 10.0f + 1.0f);
    if (al::isGreaterEqualStep(this, 140)) {
        al::tryDeleteEffect(this, "Touch");
        al::setActionFrameRate(this, 1.0f);
        al::setNerve(this, &NrvGoalItemWait);
        mIsSpinEnd = true;
    }
}

/**
 * @brief End the spin slow down.
 */
void GoalItem::endSpinSpeedDown() {
    al::tryDeleteEffect(this, "Touch");
}

/**
 * @brief Move the focus camera in and play the appear animation.
 */
void GoalItem::exeWaitCameraIn() {
    auto* poser = mObjectCamera->getPoser();
    if (al::isLessEqualStep(this, mFocusCameraInStep)) {
        if (al::isStep(this, mFocusCameraInStep)) {
            playAppearRiseAnim();
            s32 frameMax = al::getActionFrameMax(this, "AppearRise");
            if (mFocusCameraHoldStep <= frameMax) {
                mFocusCameraHoldStep = frameMax;
            }
        }
    } else if (al::isActionPlaying(this, "AppearRise") && al::isActionEnd(this)) {
        al::startAction(this, "Before");
    }

    if (al::isGreaterEqualStep(this, mFocusCameraInStep + mFocusCameraHoldStep)) {
        mIsAppearWait = false;
        if (al::isEqualString(poser->getName(), "FixedActor")) {
            static_cast<al::CameraPoserFixActor*>(poser)->setReturnGoalItemAppear(
                this, mFocusCameraOutStep, true);
        }

        al::setNerve(this, &NrvGoalItemWaitCameraOut);
    }
}

/**
 * @brief Move the focus camera back out.
 */
void GoalItem::exeWaitCameraOut() {
    auto* poser = mObjectCamera->getPoser();
    rc::invalidatePlayerInput(this, 2);
    if (!al::isGreaterEqualStep(this, mFocusCameraOutStep)) {
        return;
    }

    rc::requestEndDemoInGameCutscene(this);
    if (al::isEqualString(poser->getName(), "FixedActor")) {
        if (mIsRetainAppearAngle &&
            !static_cast<al::CameraPoserFixActor*>(poser)->mIsReturnEnd) {
            return;
        }

        al::validateClipping(this);
        al::endCamera_RS(this, mObjectCamera, 30, false);
        al::setNerve(this, &NrvGoalItemWait);
        return;
    }

    al::requestCaptureScreenCover(this, 4);
    al::setNerve(this, &NrvGoalItemWait);
    al::endCamera_RS(this, mObjectCamera, -1, false);
}

/**
 * @brief Collect an item that was already collected before.
 */
void GoalItem::exeCollectRepeated() {
    if (al::isFirstStep(this)) {
        al::tryOnStageSwitch(this, "SwitchCollectOn");
        if (mIsTimerChallenge) {
            hideChallengeTimer(this);
        }

        if (mIsCollected) {
            al::appearItemTiming(this, "RepeatGet");
            if (rc::isPlayerMini(mPlayer)) {
                rc::tryChangeToSuperMario(mPlayer);
            }
        }
    }

    al::tryOnStageSwitch(this, "SwitchDemoEndAnimateOn");
    al::tryOnStageSwitch(this, "SwitchDemoAnimateEndOn");
    if (mAnimEndFunctor != nullptr) {
        (*mAnimEndFunctor)();
    }

    if (al::tryStartAction(this, "GoalItemGetNoBGM")) {
        al::setNerve(this, &NrvGoalItemCleanUpFinishNoBGM);
        return;
    }

    al::setNerve(this, &NrvGoalItemCollectedIdle);
}

/**
 * @brief Collect the item, saving the completed scenario.
 */
void GoalItem::exeCollect() {
    if (al::isFirstStep(this)) {
        al::tryOnStageSwitch(this, "SwitchCollectOn");
        if (mIsTimerChallenge) {
            hideChallengeTimer(this);
        }

        if (!mIsCollected && mPlayer != nullptr) {
            auto* controller = DisasterModeController::tryGetController(this);
            if (controller != nullptr) {
                controller->pause(false);
            }
        }

        if (SingleModeDataFunction::getScenarioType(this, mIslandId, mShineId) == 4) {
            auto* islandMap = al::tryGetSceneObj<IslandMap>(this, SceneObjID_IslandMap);
            if (islandMap != nullptr) {
                islandMap->setSpecialShineIconComplete(this, true);
            }
        }

        al::stopBgm(this, "AfterBattleSingleMode", -1, -1);
    }

    al::tryOnStageSwitch(this, "SwitchDemoEndAnimateOn");
    if (!mIsCollected || !mIsMultiCollect) {
        if (mShineId >= 1) {
            if (mCollectListener != nullptr) {
                mCollectListener->goalItemCollectCallback();
                mCollectListener = nullptr;
            }

            SingleModeDataFunction::completeScenario(this, {mIslandId - 1, mShineId - 1});
            SingleModeDataFunction::recordDisasterMode(
                this, SingleModeDataFunction::DisasterForceSetting(mDisasterModeSetting), this);
            if (mRestartInfo != nullptr) {
                SingleModeDataFunction::setGoalItemCheckpointPass(this, mIslandId, mShineId);
            }

            if (mIsPhase0) {
                SingleModeDataFunction::setUnlockedPhase(this, 1);
                SingleModeDataFunction::resetGuideMessageSeen(this, 5, true);
                SingleModeDataFunction::setAutoForeshadow(this, false);
                SingleModeDataFunction::recordDisasterMode(
                    this, SingleModeDataFunction::DisasterForceSetting_Off, this);
                auto* layout = SingleModeSceneLayout::tryGetSingleModeSceneLayout(this);
                if (layout != nullptr) {
                    layout->setDisablePause(true);
                }
            }

            if (mIsClearGenericSaveLocation) {
                SingleModeDataFunction::clearGenericPlayerRespawnPosition(this);
            }

            if (mIslandId <= 0) {
                auto* islandKeeper = IslandKeeper::tryGetIslandKeeper(this);
                if (islandKeeper != nullptr) {
                    islandKeeper->tryUpdateLastIslandScenario();
                }

                if (SingleModeDataFunction::isLuckyShine(this, mIslandId, mShineId)) {
                    auto* luckyIslandHolder =
                        al::tryGetSceneObj<LuckyIslandHolder>(this, SceneObjID_LuckyIslandHolder);
                    if (luckyIslandHolder != nullptr) {
                        luckyIslandHolder->setIslandFlagActive();
                    }
                }
            }
        }

        rc::hideGuideGameWindow(this);
        auto* koopaJr = PlayerKoopaJr::tryGetPlayerKoopaJr(this);
        if (koopaJr != nullptr) {
            al::hideModelIfShow(koopaJr);
            al::hideSilhouetteModelIfShow(koopaJr);
        }

        if (al::isNerve(this, &NrvGoalItemCollectInstant)) {
            al::setNerve(this, &NrvGoalItemPreAnimateInstant);
        } else {
            al::setNerve(this, &NrvGoalItemPreAnimate);
        }
    }

    if (mIsCollected) {
        return;
    }

    if ((mIslandClearEndFrame == -1 || mIslandClearStartFrame == -1) && mHolder != nullptr) {
        mHolder->setClearLayoutText(mIslandId, mShineId);
        mHolder->appearClearLayout();
    }

    al::startBgm(this, "SingleCourseClear", -1, 0, -1, -1);
    al::changeLineAutoStopMode(this, "SingleCourseClear", true);
}

/**
 * @brief Prepare the camera and the player for the collect animation.
 */
void GoalItem::exePreAnimate() {
    if (al::isFirstStep(this)) {
        if (al::isNerve(this, &NrvGoalItemPreAnimateInstant)) {
            rc::changeActiveDemoAudioType(this, alSeFunction::DemoType(1));
        }

        mTargetDistance = (al::getCameraAt_RS(this, 0) - al::getCameraPos_RS(this, 0)).length();
        auto* ticket = getCameraDirector_RS()->getCurrentTicket();
        if (ticket != nullptr) {
            auto* poser = ticket->getPoser();
            poser->getTargetHolder()->getTarget(0)->calcTrans(&mTargetTrans);
            if (al::isEqualString(poser->getName(), "Follow") ||
                al::isEqualString(poser->getName(), "Parallel")) {
                mTargetDistance =
                    static_cast<CameraPoserFollowLimit*>(poser)->getDistanceWithAngle(2.0f);
            }
        }

        auto* controller = DisasterModeController::tryGetController(this);
        if (controller != nullptr) {
            controller->setBlackSunFloating(true);
            rc::addDemoActor(controller);
        }

        auto* island = static_cast<IslandHolder*>(
            al::getSceneObj<IslandKeeper>(this, SceneObjID_IslandKeeper)->findIsland(mIslandId));
        if (island != nullptr) {
            mLighthouse = island->getLighthouse();
            if (mLighthouse != nullptr) {
                rc::addDemoActor(mLighthouse);
            }
        }

        setCameraDirection();
        IUsePlayerPuppet* puppet = nullptr;
        if (mPuppeteer != nullptr && mPuppeteer->isBind()) {
            puppet = mPuppeteer->getPlayerPuppet();
            if (puppet != nullptr) {
                al::setTrans(this, puppet->getTrans());
                al::makeQuatFrontUp(al::getQuatPtr(this), mCameraDir, sead::Vector3f::ey);
            }
        }

        al::setProgramableCameraPos_RS(mProgramableCamera, &mCameraPos);
        al::setProgramableCameraAt_RS(mProgramableCamera, &mCameraAt);
        al::startCamera_RS(this, mProgramableCamera, 30);
        al::invalidateClipping(this);
        mIsAnimating = true;
        if (mHolder != nullptr) {
            mHolder->setClearLayoutText(mIslandId, mShineId);
            mHolder->appearClearLayout();
        }

        auto* koopaJr = PlayerKoopaJr::tryGetPlayerKoopaJr(this);
        if (koopaJr != nullptr) {
            rc::addDemoActor(koopaJr);
            koopaJr->startGoalItemDemo(puppet != nullptr ? puppet->getTrans() :
                                                           al::getTrans(this));
        }

        if (controller != nullptr) {
            controller->forceKillSuperBowserAttacks();
        }

        al::showModelIfHide(this);
        if (rc::isPlayerMini(mPlayer)) {
            al::tryStartAction(this, "GoalItemGetUpMini");
        } else {
            al::tryStartAction(this, "GoalItemGet");
        }
    }

    if (mIsCameraDirSet) {
        mCameraPosTarget.x = mCameraAt.x + mCameraDir.x * mCameraDistance;
        mCameraPosTarget.z = mCameraAt.z + mCameraDistance * mCameraDir.z;
        f32 y = mCameraAt.y;
        mCameraPosTarget.y = y + sead::Mathf::sin(mCameraAngleV * sead::Mathf::deg2rad(1.0f)) *
                                     mCameraDistance;
        al::lerpVec(&mCameraPos, mCameraPos, mCameraPosTarget, 0.1f);
        mCameraDistance = al::lerpValue(1.0f, mCameraDistance, 680.0f);
    }

    if (al::isGreaterEqualStep(this, 40)) {
        al::setNerve(this, &NrvGoalItemAnimate);
        turnPlayerToCamera(1.0f);
        return;
    }

    updateCameraDirection();
}

/**
 * @brief Decide the direction the collect camera looks at the item from.
 */
void GoalItem::setCameraDirection() {
    auto& lookAt =
        const_cast<sead::LookAtCamera&>(mActorSceneInfo->cameraDirector->getLookAtMain());
    if (mCollectedBySensor) {
        if (mPuppeteer->getPlayerPuppet() != nullptr) {
            sead::Vector3f trans = al::getTrans(this);
            trans.y += -50.0f;
            sead::Vector3f offset = trans - al::getTrans(mPlayer);
            al::setTrans(mPlayer, trans);
            mPuppeteer->getPlayerPuppet()->setTrans(trans);
            sead::Vector3f lookAtPos = lookAt.getPos() + offset;
            sead::Vector3f lookAtAt = lookAt.getAt() + offset;
            lookAt.setPos(lookAtPos);
            lookAt.setAt(lookAtAt);
            mCameraPos = lookAt.getPos();
            mCameraPosTarget = mCameraPos;
            mStartCameraPos = al::getCameraPos_RS(this, 0);
        } else {
            sead::Vector3f front;
            al::calcFrontDir(&front, mPlayer);
            const sead::Vector3f& playerTrans = al::getTrans(mPlayer);
            sead::Vector3f pos = front * 680.0f + playerTrans;
            mCameraPos = pos;
            mCameraPosTarget = pos;
            mStartCameraPos = pos;
        }
    } else {
        mCameraPos = lookAt.getPos();
        mCameraPosTarget = mCameraPos;
        mStartCameraPos = al::getCameraPos_RS(this, 0);
    }

    mCameraAt = al::getTrans(this);
    if (!mIsKeepVerticalAngle) {
        mCameraPos.y = mCameraAt.y;
    }

    mCameraDir = mCameraPos - mCameraAt;
    mStartCameraAt = al::getCameraAt_RS(this, 0);
    if (!mIsSetDistance || mCameraDir.length() < 680.0f) {
        mCameraDistance = 680.0f;
    }

    sead::Vector3f dir = mCameraAt - al::getCameraPos_RS(this, 0);
    if (!mIsKeepVerticalAngle) {
        dir.y = 0.0f;
    }

    dir.normalize();
    sead::Vector3f leftDir;
    sead::Vector3f rightDir;
    GoalCameraCollisionFilter filter(this);
    mIsCameraDirSet = false;
    leftDir = dir;
    al::rotateVectorDegreeY(&leftDir, -8.0f);
    leftDir.normalize();
    rightDir = dir;
    al::rotateVectorDegreeY(&rightDir, 8.0f);
    rightDir.normalize();
    if (mIsFrontAngleSet) {
        f32 angle = mFrontAngle * sead::Mathf::deg2rad(1.0f);
        f32 sin = sead::Mathf::sin(angle);
        f32 cos = sead::Mathf::cos(mFrontAngle * sead::Mathf::deg2rad(1.0f));
        mCameraDir.set(sin, 0.0f, cos);
        if (dir.dot(mCameraDir) > 0.0f) {
            mCameraDir.set(-sin, -0.0f, -cos);
        }

        mIsCameraDirSet = true;
    } else {
        sead::Vector3f arrow = -dir * mTargetDistance;
        if (alCollisionUtil::checkStrikeArrow(this, mCameraAt, arrow, &filter, nullptr) ||
            alCollisionUtil::checkStrikeArrow(this, mCameraAt, -leftDir * mTargetDistance,
                                              &filter, nullptr) ||
            alCollisionUtil::checkStrikeArrow(this, mCameraAt, -rightDir * mTargetDistance,
                                              &filter, nullptr)) {
            if (mIsFrontSet) {
                mCameraDir.set(mFront.x, mFront.y, mFront.z);
            } else {
                f32 sin = sead::Mathf::sin(mSafeAngle * sead::Mathf::deg2rad(1.0f));
                f32 cos = sead::Mathf::cos(mSafeAngle * sead::Mathf::deg2rad(1.0f));
                mCameraDir.set(sin, 0.0f, cos);
            }

            mIsCameraDirSet = true;
        }
    }

    if (!mIsKeepVerticalAngle) {
        mCameraDir.y = 0.0f;
    }

    mCameraDir.normalize();
    if (mIsKeepVerticalAngle) {
        mCameraAngleV = sead::Mathf::rad2deg(atan2f(mCameraDir.y, mCameraDir.z));
    } else {
        mCameraAngleV = 25.0f;
    }

    mCameraAngleH = sead::Mathf::rad2deg(atan2f(mCameraDir.x, mCameraDir.z));
    mCameraAngleH = al::wrapValue(mCameraAngleH, 360.0f);
}

/**
 * @brief Turn the bound player towards the camera.
 * @param rate The interpolation rate.
 * @return Whether the player faces the camera.
 */
bool GoalItem::turnPlayerToCamera(f32 rate) {
    if (!mPuppeteer->isBind()) {
        return false;
    }

    auto* puppet = mPuppeteer->getPlayerPuppet();
    if (puppet == nullptr) {
        return false;
    }

    sead::Vector3f front = puppet->getFrontVec();
    f32 angle = al::wrapValue(sead::Mathf::rad2deg(atan2f(front.x, front.z)), 360.0f);
    angle = al::lerpValue(rate, angle, mCameraAngleH);
    f32 radian = angle * sead::Mathf::deg2rad(1.0f);
    front.set(sead::Mathf::sin(radian), 0.0f, sead::Mathf::cos(radian));
    puppet->setFrontVec(front);
    return al::isNearZero(angle - mCameraAngleH, 0.01f);
}

/**
 * @brief Move the collect camera towards the player.
 */
void GoalItem::updateCameraDirection() {
    if (mPuppeteer->isBind()) {
        mCameraAtTarget = mPuppeteer->getPlayerPuppet()->getTrans();
        mCameraAtTarget.y += rc::isPlayerMini(mPlayer) ? 130.0f : 235.0f;
    }

    al::lerpVec(&mCameraAt, mCameraAt, mCameraAtTarget, 1.0f);
    mCameraPosTarget.x = mCameraAt.x + mCameraDir.x * mCameraDistance;
    mCameraPosTarget.z = mCameraAt.z + mCameraDistance * mCameraDir.z;
    f32 y = mCameraAt.y;
    mCameraPosTarget.y =
        y + sead::Mathf::sin(mCameraAngleV * sead::Mathf::deg2rad(1.0f)) * mCameraDistance;
    turnPlayerToCamera(1.0f);
    al::lerpVec(&mCameraPos, mCameraPos, mCameraPosTarget, 1.0f);
}

/**
 * @brief Play the collect animation.
 */
void GoalItem::exeAnimate() {
    if (al::isFirstStep(this)) {
        mIsExistAnimateAction = al::isExistAction(this, "Animate");
        if (mIsExistAnimateAction) {
            al::startAction(this, "Animate");
        }

        al::tryOnStageSwitch(this, "SwitchDemoStartAnimateOn");
    }

    if (al::isStep(this, 20)) {
        al::emitEffect(this, "Shine", nullptr);
    }

    updateCameraDirection();
    auto* koopaJr = PlayerKoopaJr::tryGetPlayerKoopaJr(this);
    if (koopaJr != nullptr) {
        sead::Vector3f trans = getBindTrans();

        if (rc::isPlayerMini(mPlayer)) {
            trans.y += -105.0f;
        }

        al::resetPosition(koopaJr, trans, false);
    }

    if (!al::isActionEnd(this)) {
        return;
    }

    al::requestCaptureScreenCover(this, 2);
    al::onUseCameraClippingPos(this);
    if (mIslandId < 0) {
        bool isPlessieChase = rc::isPlessieChase(SingleModeDataFunction::getUnlockedPhase(this));
        auto* controller = DisasterModeController::tryGetController(this);
        if (controller == nullptr || !controller->isDisasterMode() ||
            (isPlessieChase | controller->isSuperHardMode())) {
            al::setNerve(this, &NrvGoalItemFinishAnimateOceanCase);
            return;
        }

        rc::hidePlayerHoldingItem(mPlayer, false, false);
        if (!handleDisasterMode()) {
            al::setNerve(this, &NrvGoalItemFinishAnimateOceanCase);
            return;
        }
    } else if (!mIsNoIslandFlagCutscene && tryPlayGoalToLighthouseCutscene()) {
        al::requestCaptureScreenCover(this, 4);
        al::endCamera_RS(this, mProgramableCamera, -1, false);
        if (mHolder != nullptr) {
            mHolder->endClearLayout();
        }

        mIsAnimating = false;
        if (mIsPhase0 && rc::getControlUserFigureType(this, 0) == 1) {
            rc::setControlUserFigureType(GameDataHolderAccessor(this), 0, 0);
        }

        if (mIsCodeBasedPath) {
            if (mLighthouse != nullptr) {
                mLighthouse->setFinishBlocked(true);
            }

            al::setNerve(this, &NrvGoalItemAnimateCodeMoveRail);
        } else if (isRailMove()) {
            if (mLighthouse != nullptr) {
                mLighthouse->setFinishBlocked(true);
            }

            al::setNerve(this, &NrvGoalItemAnimateMoveRail);
            return;
        } else {
            setWaitForLighthouseShineCutscene(true, false);
            if (mHolder != nullptr) {
                mHolder->setCollectDemo(false);
            }

            return;
        }
    }

    al::tryOnStageSwitchInstant(this, "SwitchDemoOn");
    al::tryOnStageSwitch(this, "SwitchDemoAnimateEndOn");
    if (mAnimEndFunctor != nullptr) {
        (*mAnimEndFunctor)();
    }

    auto* islandKeeper = IslandKeeper::tryGetIslandKeeper(this);
    if (islandKeeper != nullptr) {
        islandKeeper->setIslandLODDisable(mIslandId, true);
    }
}

/**
 * @brief Handle Fury Bowser after collecting the item during disaster mode.
 * @return Whether Fury Bowser has to be driven away first.
 */
bool GoalItem::handleDisasterMode() {
    auto* controller = DisasterModeController::tryGetController(this);
    if (controller == nullptr) {
        return false;
    }

    bool isLastPhase3Bowser = isLastPhase3DarkBowser(controller);
    if (!controller->isDisasterMode() || (isLastPhase3Bowser | controller->isSuperHardMode())) {
        return false;
    }

    al::changeLineAutoStopMode(this, "SingleCourseClear", false);
    controller->endImmediate();
    controller->setGoalItemEndImmediate();
    if (mIslandId >= 0) {
        return false;
    }

    if (al::isNerve(this, &NrvGoalItemWaitOceanBowserExit) ||
        al::isNerve(this, &NrvGoalItemWaitOceanBowserExitReturn)) {
        return false;
    }

    if (!mIsPhase0) {
        controller->startBgmRequest(DisasterModeController::BGM_REQUEST_NONE, false);
        al::startBgm(this, "DarkBowserScaredShort", -1, 0, -1, -1);
        al::changeLineAutoStopMode(this, "DarkBowserScaredShort", true);
    }

    if (controller->isSuperBowserLeaving()) {
        al::setNerve(this, &NrvGoalItemWaitOceanBowserExitLeaving);
    } else {
        al::setNerve(this, &NrvGoalItemWaitOceanBowserExit);
    }

    al::invalidateClipping(this);
    return true;
}

/**
 * @brief Start the cutscene of the item flying to the island lighthouse.
 * @return Whether the cutscene was started.
 */
bool GoalItem::tryPlayGoalToLighthouseCutscene() {
    auto* island = static_cast<IslandHolder*>(
        al::getSceneObj<IslandKeeper>(this, SceneObjID_IslandKeeper)->findIsland(mIslandId));
    if (island == nullptr) {
        return false;
    }

    mLighthouse = island->getLighthouse();
    if (mLighthouse == nullptr) {
        return false;
    }

    if (!mLighthouse->isOkayToChangeFlag()) {
        return false;
    }

    if (mIsPhase0) {
        al::startBgm(this, "DisasterKoopaAfterPhase0", -1, 0, -1, -1);
        al::changeLineAutoStopMode(this, "DisasterKoopaAfterPhase0", true);
    }

    mLighthouse->setCameraArea(mCameraArea, mAreaOutStep);
    f32 offsetY;
    f32 distanceOffset;
    if (mLighthouse->getFinishedMainNum() == 1) {
        offsetY = mPhase2OffsetY;
        distanceOffset = mPhase2DistanceOffset;
    } else if (mLighthouse->getFinishedMainNum() >= 2) {
        offsetY = mPhase3OffsetY;
        distanceOffset = mPhase3DistanceOffset;
    } else {
        offsetY = 0.0f;
        distanceOffset = 0.0f;
    }

    mLighthouse->setOffsets(offsetY, distanceOffset);
    mLighthouse->setCurrentScenarioID(mShineId);
    mProgramableCamera->getPoser()->setEndInterpoleStep(0);
    mLighthouse->startGoalItemArrivalCutscene(this);
    mLighthouse->setInkPatch(mInkPatch, mInkCameraInStep, mInkCameraHoldStep);
    mLighthouse->setLandedActor(mPlayer);
    mLighthouse->setExitAngleSettings(mIsSetExitAngle, mExitAngleH, mExitAngleV, mExitDirRate,
                                      mExitZoomInRate, mIsSetExitInterpolate, mInkInterpoleStep,
                                      mExitTgtOffsetY, 60);
    bool isKeep;
    if (mIsLandedPosSet) {
        mLighthouse->setLandedPos(mLandedPos);
        isKeep = mIsLandedPosSet;
    } else {
        isKeep = false;
    }

    mLighthouse->setReturnPos(mTargetTrans, mTargetDistance, isKeep);
    return true;
}

/**
 * @brief Wait for the lighthouse cutscene once the item reached the lighthouse.
 * @param isInstant Whether the item did not fly along a rail.
 * @param isSkip Whether the flight was skipped.
 */
void GoalItem::setWaitForLighthouseShineCutscene(bool isInstant, bool isSkip) {
    if (isSkip) {
        al::setNerve(this, &NrvGoalItemWaitForLighthouseShineCutsceneSkip);
        mLighthouse->setGoalItemArrived(true);
        al::tryDeleteEffectAndParticle(this, "GetTrail");
    } else {
        if (isInstant) {
            al::setNerve(this, &NrvGoalItemWaitForLighthouseShineCutsceneInstant);
        } else {
            al::setNerve(this, &NrvGoalItemWaitForLighthouseShineCutscene);
        }

        mLighthouse->setGoalItemArrived(false);
        al::startSe(this, "PgGoalItemFlyEnd", nullptr);
    }

    al::tryDeleteEffect(this, "BeforeFlashy");
}

/**
 * @brief Play the shared demo camera.
 */
void GoalItem::exeCameraPlay() {}

/**
 * @brief Finish the collect animation of an item outside of the islands.
 */
void GoalItem::exeFinishAnimateOceanCase() {
    if (al::isFirstStep(this)) {
        al::tryOnStageSwitch(this, "SwitchDemoAnimateEndOn");
        if (mAnimEndFunctor != nullptr) {
            (*mAnimEndFunctor)();
        }
    }

    if (!al::isGreaterEqualStep(this, 1)) {
        return;
    }

    if (mIsReturnToOldCameraPos) {
        al::setNerve(this, &NrvGoalItemAnimateOceanCaseCameraReturn);
        mCodeRailRate = 0.0f;
        mReturnCameraPos = al::getCameraPos_RS(this, 0);
        mReturnCameraAt = al::getCameraAt_RS(this, 0);
    } else {
        al::requestCaptureScreenCover(this, 2);
        al::setNerve(this, &NrvGoalItemFinalizeOceanCase);
        al::endCamera_RS(this, mProgramableCamera, -1, false);
    }

    if (mHolder != nullptr) {
        mHolder->endClearLayout();
        mHolder->setCollectDemo(false);
    }

    mIsAnimating = false;
}

/**
 * @brief Move the camera back to where it was before the collect animation.
 */
void GoalItem::exeAnimateOceanCaseCameraReturn() {
    f32 rate = mCodeRailRate + 0.05f;
    if (rate > 1.0f) {
        rate = 1.0f;
    }

    mCodeRailRate = rate;
    mCameraPos = mReturnCameraPos * (1.0f - rate) + mStartCameraPos * rate;
    mCameraAt = mReturnCameraAt * (1.0f - rate) + mStartCameraAt * rate;
    if (rate == 1.0f) {
        al::requestCaptureScreenCover(this, 2);
        al::endCamera_RS(this, mProgramableCamera, -1, false);
        al::setNerve(this, &NrvGoalItemFinalizeOceanCase);
    }
}

/**
 * @brief Finalize the collect of an item outside of the islands.
 */
void GoalItem::exeFinalizeOceanCase() {
    auto* controller = DisasterModeController::tryGetController(this);
    if (al::isFirstStep(this)) {
        al::hideModelIfShow(this);
        mIsCollectPending = false;
        _237 = false;
        _294 = false;
        if (!mHolder->getSkipLayout()->isAlive()) {
            mHolder->getSkipLayout()->startHidden();
        }

        if (controller != nullptr) {
            bool isLastPhase3Bowser = isLastPhase3DarkBowser(controller);
            if (!controller->isDisasterMode() ||
                (isLastPhase3Bowser | controller->isSuperHardMode())) {
                al::changeLineAutoStopMode(this, "SingleCourseClear", false);
            }
        }
    }

    rc::invalidatePlayerInput(this, 2);
    al::requestCaptureScreenCover(this, 4);
    al::tryDeleteEmitterAndParticleAll(this);
    al::hideModelIfShow(this);
    finishBindPlayer(false);
    rc::requestEndDemoInGameCutscene(this);
    if (mHolder != nullptr && mHolder->getNextGoalItemGuideMessage(this) != nullptr) {
        al::setNerve(this, &NrvGoalItemGuideMessage);
    } else {
        al::setNerve(this, &NrvGoalItemDone);
        if (controller != nullptr) {
            controller->resume(true);
        }
    }

    rc::hidePlayerHoldingItem(mPlayer, false, false);
}

/**
 * @brief Finish the bind of the player and mark the item as collected.
 * @param isBindEnded Whether the bind already ended.
 */
void GoalItem::finishBindPlayer(bool isBindEnded) {
    if (!isBindEnded) {
        endBindPlayer();
    }

    al::tryOnStageSwitch(this, "SwitchDemoAnimateEndOn");
    setCollected();
}

/**
 * @brief Fly to the lighthouse along the rail.
 */
void GoalItem::exeAnimateMoveRail() {
    if (al::isFirstStep(this)) {
        rc::changeActiveDemoAudioType(this, alSeFunction::DemoType(3));
        al::startSe(this, "PgGoalItemFlyStart", nullptr);
        al::tryOnStageSwitchInstant(this, "SwitchDemoOn");
        al::tryOnStageSwitch(this, "SwitchDemoAnimateEndOn");
        if (mAnimEndFunctor != nullptr) {
            (*mAnimEndFunctor)();
        }

        auto* islandKeeper = IslandKeeper::tryGetIslandKeeper(this);
        if (islandKeeper != nullptr) {
            islandKeeper->setIslandLODDisable(mIslandId, true);
        }

        if (mHolder != nullptr) {
            mHolder->killEffect();
            mHolder->setCollectDemo(false);
        }

        al::tryDeleteEmitterAndParticleAll(this);
        al::showModelIfHide(this);
        al::setRailPosToCoord(this, 0.0f);
        mRailMover->setSpeedByTime(mMoveCount, false);
        al::syncRailTrans(this);
        al::turnToRailDir(this, 180.0f);
    }

    if (!al::isGreaterEqualStep(this, 1)) {
        return;
    }

    if (al::tryGetSceneObj<GoalItemHolder>(this, SceneObjID_GoalItemHolder)
            ->getSkipLayout()
            ->isSkip(getSkipPorts())) {
        auto* controller = DisasterModeController::tryGetController(this);
        if (controller != nullptr && controller->isDisasterMode()) {
            _370 = true;
            controller->setDemoSkipped();
        }

        al::stopAllSeId(this, "PgGoalItemFlyStart", 30, nullptr);
        jumpRailToFinish();
        return;
    }

    if (al::isStep(this, 1)) {
        al::changeLineAutoStopMode(this, "SingleCourseClear", false);
        auto* controller = DisasterModeController::tryGetController(this);
        bool isLastPhase3Bowser = isLastPhase3DarkBowser(controller);
        if (controller != nullptr && controller->isDisasterMode() &&
            !controller->isSuperHardMode() && !(isLastPhase3Bowser | mIsPhase0)) {
            controller->startBgmRequest(DisasterModeController::BGM_REQUEST_NONE, false);
            al::startBgm(this, "DarkBowserScaredLong", -1, 0, -1, -1);
            al::changeLineAutoStopMode(this, "DarkBowserScaredLong", true);
        }

        al::syncRailTrans(this);
        al::turnToRailDir(this, 180.0f);
        if (!mIsFixedCamera) {
            al::tryEmitEffect(this, "GetTrail", nullptr);
        }
    }

    f32 prevY = al::getTrans(this).y;
    bool isEnd = mRailMover->moveSyncRailByTime();
    sead::Vector3f trans = al::getTrans(this);
    trans.y = prevY + (al::getTrans(this).y - prevY) * 0.5f;
    al::setTrans(this, trans);
    al::turnToRailDir(this, 180.0f);
    setTransBelowRail(this, mPlayer);
    if (isEnd) {
        setWaitForLighthouseShineCutscene(false, false);
    }
}

/**
 * @brief Jump to the end of the flight to the lighthouse.
 */
void GoalItem::jumpRailToFinish() {
    al::changeLineAutoStopMode(this, "SingleCourseClear", false);
    auto* controller = DisasterModeController::tryGetController(this);
    bool isLastPhase3Bowser = isLastPhase3DarkBowser(controller);
    if (controller != nullptr && controller->isDisasterMode() && !controller->isSuperHardMode() &&
        !(isLastPhase3Bowser | mIsPhase0)) {
        controller->startBgmRequest(DisasterModeController::BGM_REQUEST_NONE, false);
        al::startBgm(this, "DarkBowserScaredLong", -1, 0, -1, -1);
        al::changeLineAutoStopMode(this, "DarkBowserScaredLong", true);
    }

    if (isRailMove()) {
        al::setRailPosToCoord(this, al::getRailTotalLength(this));
        al::syncRailTrans(this);
    }

    al::requestCaptureScreenCover(this, 2);
    setWaitForLighthouseShineCutscene(false, true);
}

/**
 * @brief Fly to the lighthouse along an arc computed in code.
 */
void GoalItem::exeAnimateCodeMoveRail() {
    if (al::isFirstStep(this)) {
        rc::changeActiveDemoAudioType(this, alSeFunction::DemoType(3));
        al::startSe(this, "PgGoalItemFlyStart", nullptr);
        al::tryOnStageSwitchInstant(this, "SwitchDemoOn");
        al::tryOnStageSwitch(this, "SwitchDemoAnimateEndOn");
        if (mAnimEndFunctor != nullptr) {
            (*mAnimEndFunctor)();
        }

        auto* islandKeeper = IslandKeeper::tryGetIslandKeeper(this);
        if (islandKeeper != nullptr) {
            islandKeeper->setIslandLODDisable(mIslandId, true);
        }

        if (mHolder != nullptr) {
            mHolder->killEffect();
            mHolder->setCollectDemo(false);
        }

        al::showSilhouetteModel(this);
        al::tryDeleteEmitterAndParticleAll(this);
        al::startAction(this, "Idle");
        al::showModelIfHide(this);
        if (mLighthouse != nullptr) {
            mCodeRailRate = 0.0f;
            mCodeRailStart = al::getTrans(this);
            f32 time;
            if (isRailMove()) {
                al::setRailPosToCoord(this, mCodeRailRate);
                mRailMover->getPosition(&mCodeRailEnd, mCodeRailRate);
                time = mMoveCount * 0.6f;
            } else {
                mCodeRailEnd = al::getTrans(mLighthouse);
                mCodeRailEnd.y += 2000.0f;
                time = mMoveCount;
            }

            mCodeRailSpeed = 1.0f / time;
            al::tryEmitEffect(this, "BeforeFlashy", nullptr);
        }
    }

    if (!al::isGreaterEqualStep(this, 1)) {
        return;
    }

    if (al::isStep(this, 1)) {
        al::changeLineAutoStopMode(this, "SingleCourseClear", false);
        auto* controller = DisasterModeController::tryGetController(this);
        bool isLastPhase3Bowser = isLastPhase3DarkBowser(controller);
        if (controller != nullptr && controller->isDisasterMode() &&
            !controller->isSuperHardMode() && !(isLastPhase3Bowser | mIsPhase0) &&
            !mIsFixedCamera) {
            controller->startBgmRequest(DisasterModeController::BGM_REQUEST_NONE, false);
            al::startBgm(this, "DarkBowserScaredLong", -1, 0, -1, -1);
            al::changeLineAutoStopMode(this, "DarkBowserScaredLong", true);
        }

        al::tryEmitEffect(this, "GetTrail", nullptr);
    }

    if (al::tryGetSceneObj<GoalItemHolder>(this, SceneObjID_GoalItemHolder)
            ->getSkipLayout()
            ->isSkip(getSkipPorts())) {
        auto* controller = DisasterModeController::tryGetController(this);
        if (controller != nullptr && controller->isDisasterMode()) {
            _370 = true;
            controller->setDemoSkipped();
        }

        jumpRailToFinish();
        return;
    }

    mCodeRailRate = mCodeRailSpeed + mCodeRailRate;
    if (mCodeRailRate >= 1.0f) {
        mCodeRailRate = 1.0f;
        al::hideSilhouetteModel(this);
        if (isRailMove()) {
            al::setNerve(this, &NrvGoalItemAnimateMoveRailFinish);
        } else {
            setWaitForLighthouseShineCutscene(false, false);
        }
    }

    f32 rate = al::easeIn(mCodeRailRate);
    sead::Vector3f trans = mCodeRailStart * (1.0f - rate) + mCodeRailEnd * rate;
    trans.y += sead::Mathf::cos((rate + -0.5f) * sead::Mathf::pi()) * mArcHeight + -200.0f;
    al::setTrans(this, trans);
}

/**
 * @brief Finish the flight along the rail after the arc computed in code.
 */
void GoalItem::exeAnimateMoveRailFinish() {
    if (al::isFirstStep(this)) {
        mCodeRailRate = 0.0f;
        al::setRailPosToCoord(this, 0.0f);
        mRailMover->setSpeedByTime(mMoveCount * 0.4f, false);
        al::syncRailTrans(this);
        al::turnToRailDir(this, 180.0f);
    }

    if (al::tryGetSceneObj<GoalItemHolder>(this, SceneObjID_GoalItemHolder)
            ->getSkipLayout()
            ->isSkip(getSkipPorts())) {
        auto* controller = DisasterModeController::tryGetController(this);
        if (controller != nullptr && controller->isDisasterMode()) {
            _370 = true;
            controller->setDemoSkipped();
        }

        jumpRailToFinish();
        return;
    }

    rc::invalidatePlayerInput(this, 2);
    bool isEnd = mRailMover->moveSyncRailByTime();
    al::turnToRailDir(this, 180.0f);
    setTransBelowRail(this, mPlayer);
    if (isEnd) {
        setWaitForLighthouseShineCutscene(false, false);
    }
}

/**
 * @brief Wait for Fury Bowser to leave after collecting the item outside of the islands.
 */
void GoalItem::exeWaitOceanBowserExit() {
    auto* controller = DisasterModeController::tryGetController(this);
    auto* superBowser = controller->getSuperBowser();
    if (al::isFirstStep(this)) {
        al::tryGetSceneObj<LuckyIslandHolder>(this, SceneObjID_LuckyIslandHolder)
            ->disableLuckyIslandCollision();
        al::hideModelIfShow(this);
        al::tryDeleteEmitterAndParticleAll(this);
        if (mHolder != nullptr) {
            mHolder->endClearLayout();
            mHolder->killEffect();
            mHolder->setCollectDemo(false);
        }

        mIsAnimating = false;
        if (al::isActiveCamera(mObjectCamera)) {
            al::endCamera_RS(this, mObjectCamera, -1, false);
        }

        if (al::isActiveCamera(mProgramableCamera)) {
            al::endCamera_RS(this, mProgramableCamera, -1, false);
        }

        if (!al::isNerve(this, &NrvGoalItemWaitOceanBowserExit)) {
            if (mHolder != nullptr) {
                mHolder->getSkipLayout()->end();
            }

            controller->endInstantly(true, false);
            controller->setFadeInDoneFunctor(mDisasterFadeoutFunctor);
            mIsDisasterFadeoutDone = false;
            if (controller->getSuperBowser() != nullptr) {
                controller->getSuperBowser()->tryDamageDarkBowser();
            }

            controller->startBgmRequest(DisasterModeController::BGM_REQUEST_PROSPERITY, false);
            al::changeLineAutoStopMode(this, "DarkBowserScaredShort", false);
            al::setNerve(this, &NrvGoalItemWaitDisasterFadeOutDoneSkip);
            return;
        }

        al::tryOnStageSwitch(this, "SwitchDemoAnimateEndOn");
        setCollected();
        controller->getSuperBowser()->startBowserExitCamera(false, false, true);
        auto* poser = static_cast<al::CameraPoserFixActor*>(mObjectCamera->getPoser());
        poser->setInterpoleStep(0);
        poser->setEndInterpoleStep(0);
        poser->storeCamera(al::getCameraPos_RS(this, 0), al::getCameraAt_RS(this, 0));
        rc::addDemoActor(superBowser);
    }

    rc::invalidatePlayerInput(this, 2);
    rc::isActiveDemo(this);
    bool isSkip;
    if (controller->isDisasterMode() && mHolder->getSkipLayout()->isSkip(getSkipPorts())) {
        isSkip = true;
        controller->endInstantly(true, false);
        mIsDisasterFadeoutDone = false;
        controller->setFadeInDoneFunctor(mDisasterFadeoutFunctor);
        al::stopAllSeFromUser(controller->getSuperBowser(), 30);
        controller->isBowserHidden();
    } else {
        if (!controller->isBowserHidden()) {
            return;
        }

        isSkip = false;
    }

    mHolder->getSkipLayout()->end();
    if (!mIsPhase0 && mIslandId < 0) {
        controller->startBgmRequest(DisasterModeController::BGM_REQUEST_PROSPERITY, false);
        al::changeLineAutoStopMode(this, "DarkBowserScaredShort", false);
    }

    if (!isSkip) {
        mIsDisasterFadeoutDone = true;
    }

    al::setNerve(this, &NrvGoalItemWaitDisasterFadeOutDone);
}

/**
 * @brief Wait for the disaster fade out once Fury Bowser left.
 */
void GoalItem::exeWaitDisasterFadeOutDone() {
    if (!mIsDisasterFadeoutDone) {
        return;
    }

    al::requestCaptureScreenCover(this, 3);
    auto* controller = DisasterModeController::tryGetController(this);
    auto* bellManager = GigaBellManager::tryGetManager(this);
    if (al::isNerve(this, &NrvGoalItemWaitDisasterFadeOutDoneSkip)) {
        if (bellManager != nullptr && bellManager->tryQueueReturnCutscene(false, true, false)) {
            al::setNerve(this, &NrvGoalItemWaitOceanBowserExitGigaBell);
            return;
        }

        rc::requestEndDemoInGameCutscene(this);
        finishBindPlayer(false);
        al::validateClipping(this);
        finishBowserExit();
        return;
    }

    controller->getSuperBowser()->disappear(true);
    if (bellManager != nullptr && bellManager->tryQueueReturnCutscene(false, true, false)) {
        al::setNerve(this, &NrvGoalItemWaitOceanBowserExitGigaBellDamage);
        return;
    }

    rc::requestEndDemoInGameCutscene(this);
    endBindPlayer();
    al::setNerve(this, &NrvGoalItemWaitOceanBowserExitReturn);
}

/**
 * @brief End the bind of the player holding the item.
 */
void GoalItem::endBindPlayer() {
    if (mPuppeteer->isBind()) {
        IslandMap::setIslandWarpEnable(this, true);
        al::tryOffStageSwitchInstant(this, "SwitchDemoOn");
        auto* islandKeeper = IslandKeeper::tryGetIslandKeeper(this);
        if (islandKeeper != nullptr) {
            islandKeeper->setIslandLODDisable(mIslandId, false);
        }

        mPuppeteer->getPlayerPuppet()->startAction("Fall");
        if (mPlayer != nullptr) {
            alLiveActorFunction::setAlphaCtrlOn(mPlayer, true);
            rc::clearScannedAmiiboList(mPlayer);
            if (rc::isPlayerClimbWhite(mPlayer)) {
                rc::changeToClimbMarioForce(mPlayer);
            } else if (rc::isPlayerRaccoonDogWhite(mPlayer)) {
                rc::changeToRaccoonDogMarioForce(mPlayer);
            } else {
                rc::tryChangeToSuperMario(mPlayer);
            }
        }

        rc::resumePlayerEquip(
            al::getSensorHost(mPuppeteer->getPlayerPuppet()->getMsgTargetSensor()));
        mPuppeteer->endBind(nullptr);
    }

    auto* koopaJr = PlayerKoopaJr::tryGetPlayerKoopaJr(this);
    if (koopaJr != nullptr) {
        koopaJr->endGoalItemDemo();
    }

    setCollected();
}

/**
 * @brief Wait for the Giga Bell return cutscene.
 */
void GoalItem::exeWaitOceanBowserExitGigaBell() {
    if (al::isFirstStep(this) && al::isNerve(this, &NrvGoalItemWaitOceanBowserExitGigaBellDamage)) {
        DisasterModeController::tryGetController(this)->getSuperBowser()->endBowserExitCamera(
            false);
    }

    if (!GigaBellManager::tryGetManager(this)->isCutsceneDone()) {
        return;
    }

    if (al::isNerve(this, &NrvGoalItemWaitOceanBowserExitGigaBell)) {
        finishBindPlayer(false);
    } else {
        endBindPlayer();
    }

    al::requestCaptureScreenCover(this, 4);
    rc::requestEndDemoInGameCutscene(this);
    al::validateClipping(this);
    finishBowserExit();
}

/**
 * @brief Return from Fury Bowser's exit camera.
 */
void GoalItem::exeWaitOceanBowserExitReturn() {
    if (!al::isFirstStep(this)) {
        return;
    }

    finishBowserExit();
    DisasterModeController::tryGetController(this)->getSuperBowser()->endBowserExitCamera(false);
    al::validateClipping(this);
}

/**
 * @brief Clean up once the item was collected.
 */
void GoalItem::exeCleanUpFinish() {
    if (al::isFirstStep(this)) {
        al::tryDeleteEmitterAndParticleAll(this);
        al::hideModelIfShow(this);
        if (isCollected()) {
            al::setNerve(this, &NrvGoalItemDone);
        }
    }

    if (al::isNerve(this, &NrvGoalItemCollectedIdle)) {
        mIsCollectPending = false;
    }
}

/**
 * @brief Clean up once an item collected again was collected, keeping the BGM.
 */
void GoalItem::exeCleanUpFinishNoBGM() {
    if (al::isFirstStep(this)) {
        mIsCollectPending = false;
    }

    if (al::isActionEnd(this)) {
        al::hideModelIfShow(this);
        al::tryDeleteEffect(this, "Touch");
        if (isCollected()) {
            al::setNerve(this, &NrvGoalItemDone);
        }
    }
}

/**
 * @brief Play the sound of the lighthouse light start.
 * @param step The step of the lighthouse cutscene.
 */
void GoalItem::tryTriggerGoalItemLightStart(s32 step) {
    if (step != 45) {
        return;
    }

    const char* seName = "PgGoalItemLightStartNoInk";
    if (mLighthouse != nullptr && mLighthouse->isInked()) {
        seName = "PgGoalItemLightStart";
    }

    al::startSe(this, seName, nullptr);
}

/**
 * @brief Called when the lighthouse sequence is almost over.
 */
void GoalItem::lighthouseSequenceAlmostEnded() {
    al::setNerve(this, &NrvGoalItemEndLighthouseCutscene);
}

/**
 * @brief Called when the lighthouse sequence is over.
 * @param isInstant Whether the sequence ended without a screen capture.
 */
void GoalItem::lighthouseSequenceCompletelyEnded(bool isInstant) {
    if (!isInstant) {
        al::requestCaptureScreenCover(this, 4);
    }

    al::tryDeleteEmitterAndParticleAll(this);
    al::hideModelIfShow(this);
    al::hideSilhouetteModel(this);
    if (!al::isNerve(this, &NrvGoalItemEndLighthouseCutscene)) {
        finishBindPlayer(false);
    }

    rc::requestEndDemoInGameCutscene(this);
    rc::hidePlayerHoldingItem(mPlayer, false, false);
}

/**
 * @brief Called when the lighthouse cutscene of the item is over.
 */
void GoalItem::finishGoalItemCutscene() {
    completeFinish();
    al::setNerve(this, &NrvGoalItemDoneLighthouse);
}

/**
 * @brief Complete the collect sequence.
 */
void GoalItem::completeFinish() {
    al::validateClipping(this);
    mHolder->setCurrentGoalItem(nullptr);
    rc::unHideGuideGameWindow(this);
    if (mIsCollectPending) {
        mIsCollectPending = false;
    } else if (mHolder != nullptr && mHolder->getNextGoalItemGuideMessage(this) == nullptr) {
        mHolder->appearWindowProcessing();
    }

    auto* bellManager = al::tryGetSceneObj<GigaBellManager>(this, SceneObjID_GigaBellManager);
    if (bellManager != nullptr) {
        bellManager->shineCollected();
    }

    auto* controller = DisasterModeController::tryGetController(this);
    if (controller == nullptr || controller->isDisasterModeAnim()) {
        return;
    }

    if (controller->isPreDisasterModeDone()) {
        controller->preDisasterMode();
        controller->tryPlayFirstDisasterModeCutscene();
    }
}

/**
 * @brief End the lighthouse cutscene.
 */
void GoalItem::exeEndLighthouseCutscene() {
    if (al::isFirstStep(this)) {
        finishBindPlayer(false);
    }
}

/**
 * @brief Called when the lighthouse light sequence was skipped.
 */
void GoalItem::lighthouseLightSequenceSkipped() {
    auto* controller = DisasterModeController::tryGetController(this);
    al::stopAllSeFromUser(this, 30);
    if (controller != nullptr && controller->isDisasterMode()) {
        al::stopAllSeFromUser(controller->getSuperBowser(), 30);
    }
}

/**
 * @brief Called when the lighthouse shone its light.
 */
void GoalItem::lighthouseLightShone() {
    if (al::isNerve(this, &NrvGoalItemWaitForLighthouseShineCutsceneSkip)) {
        return;
    }

    al::setNerve(this, &NrvGoalItemLighthouseShoneCutscene);
}

/**
 * @brief Handle Fury Bowser once the lighthouse shone.
 */
void GoalItem::exeLighthouseShoneCutscene() {
    if (al::isFirstStep(this)) {
        handleDisasterMode();
    }
}

/**
 * @brief Called when Fury Bowser left the lighthouse.
 * @param isGone Whether Fury Bowser is gone for good.
 */
void GoalItem::lighthouseDarkBowserGone(bool isGone) {
    auto* controller = DisasterModeController::tryGetController(this);
    if (controller == nullptr || controller->isSuperHardMode() || mIsPhase0 || mIsFixedCamera) {
        return;
    }

    if (isGone) {
        controller->startBgmRequest(DisasterModeController::BGM_REQUEST_END, false);
    } else {
        controller->startBgmRequest(DisasterModeController::BGM_REQUEST_PROSPERITY, false);
    }

    al::changeLineAutoStopMode(this, "DarkBowserScaredLong", false);
}

/**
 * @brief Wait for the lighthouse cutscene to shine its light.
 */
void GoalItem::exeWaitForLighthouseShineCutscene() {
    DisasterModeController::tryGetController(this);
    if (!al::isFirstStep(this)) {
        return;
    }

    if (al::isNerve(this, &NrvGoalItemWaitForLighthouseShineCutsceneInstant)) {
        al::tryOnStageSwitchInstant(this, "SwitchDemoOn");
        al::tryOnStageSwitch(this, "SwitchDemoAnimateEndOn");
        if (mAnimEndFunctor != nullptr) {
            (*mAnimEndFunctor)();
        }

        auto* islandKeeper = IslandKeeper::tryGetIslandKeeper(this);
        if (islandKeeper != nullptr) {
            islandKeeper->setIslandLODDisable(mIslandId, true);
        }
    }

    al::hideModelIfShow(this);
    mIsCollectPending = false;
    _237 = false;
    _294 = false;
    if (mLighthouse != nullptr) {
        mLighthouse->setFinishBlocked(false);
    }

    if (al::isNerve(this, &NrvGoalItemWaitForLighthouseShineCutsceneSkip)) {
        handleDisasterMode();
    }
}

/**
 * @brief Play the ink camera.
 */
void GoalItem::exeInkCameraPlay() {
    al::isFirstStep(this);
}

/**
 * @brief Show the guide message following the collect.
 */
void GoalItem::exeGuideMessage() {
    rc::invalidatePlayerInput(this, 2);
    if (al::isFirstStep(this)) {
        al::invalidateClipping(this);
    }

    auto* controller = DisasterModeController::tryGetController(this);
    if (controller != nullptr && controller->isWipeActive()) {
        al::setNerve(this, &NrvGoalItemGuideMessage);
        return;
    }

    if (!al::isGreaterEqualStep(this, 1)) {
        return;
    }

    if (al::isStep(this, 1)) {
        if (mHolder != nullptr && mHolder->getNextGoalItemGuideMessage(this) != nullptr) {
            rc::appearGuideGameWindowWithConfirm(this, mHolder->getNextGoalItemGuideMessage(this),
                                                 true);
        }

        if (SingleModeDataFunction::getUnlockedPhase(this) == 5 &&
            SingleModeDataFunction::isFirstPhase3BossDefeated(this)) {
            SingleModeDataFunction::setGuideMessageSeen(GameDataHolderAccessor(this), 15, true);
        }
    }

    if (rc::isGuideGameWindowWaitConfirm(this)) {
        return;
    }

    if (mHolder != nullptr) {
        mHolder->appearWindowProcessing();
    }

    rc::unHideGuideGameWindow(this);
    al::validateClipping(this);
    auto* disasterController = DisasterModeController::tryGetController(this);
    if (disasterController != nullptr) {
        disasterController->resume(true);
        if (SingleModeDataFunction::getGoalItemsCollected(this) ==
            SingleModeDataFunction::getMaxCollectableGoalItems()) {
            disasterController->tryJumpToRainWithFlash();
        }
    }

    al::setNerve(this, &NrvGoalItemDone);
}

/**
 * @brief Done state: complete the collect sequence.
 */
void GoalItem::exeDone() {
    if (al::isFirstStep(this) && !al::isNerve(this, &NrvGoalItemDoneLighthouse)) {
        completeFinish();
    }
}

/**
 * @brief Finish the item immediately, moving the player to it.
 * @return Whether the item could be finished.
 */
bool GoalItem::quickFinish() {
    if (al::isNerve(this, &NrvGoalItemCollectedIdle)) {
        return false;
    }

    if (canCollect()) {
        if (!al::isAlive(this)) {
            appear();
        }

        auto* player = rc::getActivePlayer(this);
        if (player != nullptr) {
            rc::setPlayerTrans(player, al::getTrans(this));
        }
    }

    return true;
}

/**
 * @brief Add the item (or its empty twin) to the current demo.
 */
void GoalItem::addGoalItemToDemo() {
    if (mIsMultiCollect && isCollected() && !mIsEmptyItem) {
        rc::addDemoActor(mPairItem);
        return;
    }

    rc::addDemoActor(this);
}
