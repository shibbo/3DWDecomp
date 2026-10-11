#include "MapObj/Fury/GigaBellManager.hpp"

#include <cmath>
#include <math/seadMathCalcCommon.h>
#include <math/seadQuat.h>

#include "Camera/DummyCameraTarget.hpp"
#include "Demo/DemoCutscene.hpp"
#include "Enemy/SuperBowser.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/LiveActor/Util/ActorSceneInfo.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Play/Layout/WipeSimple.hpp"
#include "Library/Play/Placement/PlacementInfo.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Yaml/ByamlIter.hpp"
#include "MapObj/DisasterModeController.hpp"
#include "MapObj/Fury/GigaBell3in1.hpp"
#include "MapObj/GoalItemHolder.hpp"
#include "MapObj/LuckyIslandHolder.hpp"
#include "Player/Normal/PlayerActor.hpp"
#include "Project/Camera/Main/CameraDirector_RS.hpp"
#include "Raidon/RaidonSurf.hpp"
#include "Scene/SceneObjID.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/GameDataHolderWriter.hpp"
#include "Util/DemoUtil.hpp"
#include "Util/PlayerUtil.hpp"

namespace {
NERVE_DECL(GigaBellManager, Watch)

/**
 * @brief Wait for the bell to open after a bell's own unlock cutscene.
 */
class GigaBellManagerNrvWaitOpenBellSelf : public al::Nerve {
public:
    /**
     * @brief Run the nerve.
     * @param pKeeper The nerve keeper of the manager.
     */
    void execute(al::NerveKeeper* pKeeper) const override {
        pKeeper->getParent<GigaBellManager>()->exeWaitOpenBell();
    }
};

NERVE_DECL(GigaBellManager, PlessieChase)
NERVE_DECL(GigaBellManager, WaitDemo)
NERVE_DECL(GigaBellManager, WaitOpenBell)
NERVE_DECL(GigaBellManager, WaitResetBell)
NERVE_DECL(GigaBellManager, PlessieChaseReset)

NERVES_MAKE_NOSTRUCT(GigaBellManager, Watch, WaitOpenBellSelf, PlessieChase, WaitDemo, WaitOpenBell,
                     WaitResetBell, PlessieChaseReset)

/// Scene object id of the manager.
constexpr s32 cSceneObjIdGigaBellManager = 50;
/// Scene object id of the Lucky Island holder.
constexpr s32 cSceneObjIdLuckyIslandHolder = 38;
/// Cutscene id of the Giga Bell unlock explanation.
constexpr s32 cCutsceneIdGigaBellUnlock = 9;
/// Cutscene id of the Giga Bell return explanation.
constexpr s32 cCutsceneIdGigaBellReturn = 12;

/**
 * @brief Cubic ease out.
 * @param t The rate, between 0 and 1.
 * @return The eased rate.
 */
inline f32 calcEaseOutCubic(f32 t) {
    f32 inv = 1.0f - t;
    return 1.0f - inv * (inv * inv);
}
}  // namespace

/**
 * @brief Get the manager of the scene.
 * @param pHolder Object with access to the scene object holder.
 * @return The manager, or nullptr when the scene has none.
 */
GigaBellManager* GigaBellManager::tryGetManager(const al::IUseSceneObjHolder* pHolder) {
    return al::tryGetSceneObj<GigaBellManager>(pHolder, cSceneObjIdGigaBellManager);
}

/**
 * @brief Construct the manager.
 * @param pName The actor name.
 */
GigaBellManager::GigaBellManager(const char* pName) : al::LiveActor(pName) {
    mIsSkipDisasterDemo = false;
    mExplainCutscenes.allocBuffer(1, nullptr);
}

/**
 * @brief Initialize the manager, its bells, cutscenes and fade wipes.
 * @param rInfo The actor init info.
 */
void GigaBellManager::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "GigaBellManager", nullptr);
    al::initNerve(this, &NrvGigaBellManagerWatch, 0);
    initFromYaml();
    al::hideModelIfShow(this);
    al::invalidateClipping(this);
    al::tryGetArg(&mLockCountMin, rInfo, "LockCountMin");
    al::tryGetArg(&mLockCountMax, rInfo, "LockCountMax");
    al::tryGetArg(&mLockCountIncrement, rInfo, "LockCountIncrement");

    s32 lockCount = SingleModeDataFunction::getGigaBellLockCount(this);
    if (lockCount == -1) {
        lockCount = mLockCountMin;
    } else if (lockCount < 0) {
        lockCount = mLockCountIncrement - lockCount;
    }

    setLockCount(lockCount);

    s32 bellNum = al::calcLinkChildNum(rInfo, "GigaBell");
    mGigaBellNum = bellNum;
    mGigaBells = new GigaBell*[bellNum];
    for (s32 i = 0; i < mGigaBellNum; i++) {
        mGigaBells[i] = new GigaBell("GigaBell");
        mGigaBells[i]->setManager(this);
        al::initLinksActor(mGigaBells[i], rInfo, "GigaBell", i);
    }

    if (DisasterModeController::isLastBowserBattle(this)) {
        mGigaBell3in1 = new GigaBell3in1("GigaBell3in1");
        mGigaBell3in1->init(rInfo);
        mGigaBell3in1->setGigaBellManager(this);
        mGigaBell3in1->makeActorDead();
    } else {
        auto* cutscene = new DemoCutscene("DemoSingleModeGenericDialogueGB",
                                          static_cast<alSeFunction::DemoType>(2));
        cutscene->createWipeFade(rInfo, true, 30);
        mExplainCutscenes.pushBack(cutscene);
        mExplainCutscenes.front()->setDemoName("DemoSingleModeGenericDialogueGB");
        mExplainCutscenes.unsafeAt(0)->init(rInfo);
    }

    mCameraTarget = new DummyCameraTarget("DummyCameraTarget");
    mCameraTarget->init(rInfo);
    al::setTrans(mCameraTarget, sead::Vector3f::zero);
    mCameraTarget->makeActorDead();
    al::setSceneObj(this, this, cSceneObjIdGigaBellManager);

    if (al::calcLinkChildNum(rInfo, "TransformationCutscene") > 0) {
        al::PlacementInfo placementInfo;
        al::getLinksInfoByIndex(&placementInfo, rInfo, "TransformationCutscene", 0);
        al::ActorInitInfo childInfo;
        childInfo.initNoViewId(&placementInfo, rInfo);
        mCollectDemo = new DemoCutscene("DemoGigaMarioTransformation",
                                        static_cast<alSeFunction::DemoType>(2));
        mCollectDemo->init(childInfo);
        mCollectDemo->setEndSceneFlag();
    }

    mWipeFadeBlack = new al::WipeSimple("黒フェードワイプ", "WipeFadeBlack",
                                        al::getLayoutInitInfo(rInfo), "Demo");
    mWipeFadeWhite = new al::WipeSimple("黒フェードワイプ", "WipeFadeWhite",
                                        al::getLayoutInitInfo(rInfo), nullptr);
    makeActorAppeared();
}

/**
 * @brief Read the Plessie chase parameters from the actor's InitPlessieChase file.
 */
void GigaBellManager::initFromYaml() {
    al::ByamlIter rootIter;
    al::ByamlIter iter;
    if (!al::tryGetActorInitFileIter(&rootIter, this, "InitPlessieChase", nullptr)) {
        return;
    }

    rootIter.tryGetIterByKey(&iter, "Giga Bell Base Offset");
    iter.tryGetFloatByKey(&mBaseSocketOffset.x, "Base Socket Offset X");
    iter.tryGetFloatByKey(&mBaseSocketOffset.y, "Base Socket Offset Y");
    iter.tryGetFloatByKey(&mBaseSocketOffset.z, "Base Socket Offset Z");
    iter.tryGetFloatByKey(&mBaseSocketOffsetFirst.x, "Base Socket Offset First X");
    iter.tryGetFloatByKey(&mBaseSocketOffsetFirst.y, "Base Socket Offset First Y");
    iter.tryGetFloatByKey(&mBaseSocketOffsetFirst.z, "Base Socket Offset First Z");

    rootIter.tryGetIterByKey(&iter, "Giga Bell Movement Pattern Default");
    iter.tryGetFloatByKey(&mMoveDefault.moveTime, "Path Default Move Time");
    iter.tryGetFloatByKey(&mMoveDefault.moveDistance, "Path Default Move Distance");
    iter.tryGetFloatByKey(&mMoveDefault.moveHeight, "Path Default Move Height");
    iter.tryGetFloatByKey(&mMoveDefault.offsetY, "Path Default Offset Y");

    rootIter.tryGetIterByKey(&iter, "Giga Bell Movement Pattern Swing");
    iter.tryGetFloatByKey(&mMoveSwing.moveTime, "Path Swing Move Time");
    iter.tryGetFloatByKey(&mMoveSwing.moveDistance, "Path Swing Move Distance");
    iter.tryGetFloatByKey(&mMoveSwing.moveHeight, "Path Swing Move Height");
    iter.tryGetFloatByKey(&mMoveSwing.offsetY, "Path Swing Offset Y");

    rootIter.tryGetIterByKey(&iter, "Giga Bell Movement Pattern Figure Eight");
    iter.tryGetFloatByKey(&mMoveFigureEight.moveTime, "Path Figure Eight Move Time");
    iter.tryGetFloatByKey(&mMoveFigureEight.moveDistance, "Path Figure Eight Move Distance");
    iter.tryGetFloatByKey(&mMoveFigureEight.moveHeight, "Path Figure Eight Move Height");
    iter.tryGetFloatByKey(&mMoveFigureEight.offsetY, "Path Figure Eight Offset Y");

    rootIter.tryGetIterByKey(&iter, "Giga Bell Movement Pattern Bow Tie");
    iter.tryGetFloatByKey(&mMoveBowTie.moveTime, "Path Bow Tie Move Time");
    iter.tryGetFloatByKey(&mMoveBowTie.moveDistance, "Path Bow Tie Move Distance");
    iter.tryGetFloatByKey(&mMoveBowTie.moveHeight, "Path Bow Tie Move Height");
    iter.tryGetFloatByKey(&mMoveBowTie.offsetY, "Path Bow Tie Offset Y");

    rootIter.tryGetIterByKey(&iter, "Giga Bell Movement Pattern Rose");
    iter.tryGetFloatByKey(&mMoveRose.moveTime, "Path Rose Move Time");
    iter.tryGetFloatByKey(&mMoveRose.moveDistance, "Path Rose Move Distance");
    iter.tryGetFloatByKey(&mMoveRose.moveHeight, "Path Rose Move Height");
    iter.tryGetFloatByKey(&mMoveRose.coefficient, "Path Rose Coefficient");
    iter.tryGetFloatByKey(&mMoveRose.rotationOffset, "Path Rose Rotation Offset");
    iter.tryGetFloatByKey(&mMoveRose.smoothness, "Path Rose Smoothness");
    iter.tryGetFloatByKey(&mMoveRose.offsetY, "Path Rose Offset Y");

    rootIter.tryGetIterByKey(&iter, "Giga Bell Movement Pattern Circle");
    iter.tryGetFloatByKey(&mMoveCircle.moveTime, "Path Circle Move Time");
    iter.tryGetFloatByKey(&mMoveCircle.moveDistance, "Path Circle Move Distance");
    iter.tryGetFloatByKey(&mMoveCircle.moveHeight, "Path Circle Move Height");
    iter.tryGetFloatByKey(&mMoveCircle.offsetY, "Path Circle Offset Y");

    rootIter.tryGetIterByKey(&iter, "Giga Bell Movement Pattern Atom");
    iter.tryGetFloatByKey(&mMoveAtom.moveTime, "Path Atom Move Time");
    iter.tryGetFloatByKey(&mMoveAtom.moveDistance, "Path Atom Move Distance");
    iter.tryGetFloatByKey(&mMoveAtom.moveHeight, "Path Atom Move Height");
    iter.tryGetFloatByKey(&mMoveAtom.offsetY, "Path Atom Offset Y");
    iter.tryGetFloatByKey(&mMoveAtom.rotateSpeed, "Path Atom Rotate Speed");

    mKnockback.initFromYaml(&rootIter, "Bowser Knockback");
    mKnockbackFinalHit.initFromYaml(&rootIter, "Bowser Final Hit Knockback");
    mKnockbackP10.initFromYaml(&rootIter, "Bowser Knockback P10");
    mKnockbackFinalHitP10.initFromYaml(&rootIter, "Bowser Final Hit Knockback P10");

    rootIter.tryGetIterByKey(&iter, "Final Hit");
    iter.tryGetIntByKey(&mFinalHitCamera.camera1PlayTime, "Camera 1 Play Time");
    iter.tryGetIntByKey(&mFinalHitCamera.camera1FreezeTime, "Camera 1 Freeze Time");
    iter.tryGetIntByKey(&mFinalHitCamera.camera2PlayTime, "Camera 2 Play Time");
    iter.tryGetIntByKey(&mFinalHitCamera.camera2FreezeTime, "Camera 2 Freeze Time");
    iter.tryGetIntByKey(&mFinalHitCamera.camera3PlayTime, "Camera 3 Play Time");
    iter.tryGetIntByKey(&mFinalHitCamera.cameraInterpBackFrames, "Camera Interp Back Frames");
    iter.tryGetFloatByKey(&mFinalHitCamera.camera2AngleV, "Camera 2 Angle V");
    iter.tryGetFloatByKey(&mFinalHitCamera.camera2Distance, "Camera 2 Distance");
    iter.tryGetFloatByKey(&mFinalHitCamera.camera2Offset.x, "Camera 2 Offset X");
    iter.tryGetFloatByKey(&mFinalHitCamera.camera2Offset.y, "Camera 2 Offset Y");
    iter.tryGetFloatByKey(&mFinalHitCamera.camera2Offset.z, "Camera 2 Offset Z");
    iter.tryGetFloatByKey(&mFinalHitCamera.camera2AngleHOffset, "Camera 2 Angle H Offset");
    iter.tryGetFloatByKey(&mFinalHitCamera.camera3AngleV, "Camera 3 Angle V");
    iter.tryGetFloatByKey(&mFinalHitCamera.camera3Distance, "Camera 3 Distance");
    iter.tryGetFloatByKey(&mFinalHitCamera.camera3Offset.x, "Camera 3 Offset X");
    iter.tryGetFloatByKey(&mFinalHitCamera.camera3Offset.y, "Camera 3 Offset Y");
    iter.tryGetFloatByKey(&mFinalHitCamera.camera3Offset.z, "Camera 3 Offset Z");
    iter.tryGetFloatByKey(&mFinalHitCamera.camera3AngleHOffset, "Camera 3 Angle H Offset");
}

/**
 * @brief Set the number of goal items needed to unlock the bells and save it.
 * @param lockCount The requested count, clamped to the placement's limits.
 */
void GigaBellManager::setLockCount(s32 lockCount) {
    s32 count = sead::Mathi::clamp(lockCount, mLockCountMin, mLockCountMax);
    if (count > SingleModeDataFunction::getGigaBellLockCount(this)) {
        SingleModeDataFunction::setGigaBellUnlocked(this, false);
    }

    mLockCount = count;
    SingleModeDataFunction::setGigaBellLockCount(this, mLockCount);
}

/**
 * @brief Read a knockback parameter set.
 * @param pIter The parent iterator.
 * @param pKey Key of the parameter set.
 */
void GigaBellManager::PlessieChaseKnockbackParam::initFromYaml(al::ByamlIter* pIter,
                                                               const char* pKey) {
    al::ByamlIter iter;
    pIter->tryGetIterByKey(&iter, pKey);
    iter.tryGetFloatByKey(&height, "Height");
    iter.tryGetFloatByKey(&distance, "Distance");
    iter.tryGetIntByKey(&frames, "Frames");
    iter.tryGetFloatByKey(&slideDistance, "SlideDistance");
    iter.tryGetIntByKey(&slideFrames, "SlideFrames");
    iter.tryGetIntByKey(&delayFrames, "DelayFrames");
    iter.tryGetFloatByKey(&rotationDeg, "RotationDeg");
}

/**
 * @brief Watch the goal item count and unlock the bells once enough were collected.
 */
void GigaBellManager::exeWatch() {
    auto* controller = DisasterModeController::tryGetController(this);
    if (controller->getSuperBowser()->isLastPhase3Bowser()) {
        return;
    }

    if (!controller->isDisasterMode() && !controller->isDisasterNerve()) {
        mLockState = SingleModeDataFunction::getGoalItemsCollected(this) >= mLockCount ?
                         GigaBell::LockState::LockedSufficientGoalItems :
                         GigaBell::LockState::Locked;
    }

    if (mLockState == GigaBell::LockState::Unlocked) {
        if (mIsUnlocked) {
            return;
        }

        if (shouldPlayUnlockCutscene()) {
            if (rc::isAnyActiveDemo(this)) {
                return;
            }

            if (!controller->getSuperBowser()->hasLanded()) {
                return;
            }

            rc::setDemoAudioType(this, static_cast<alSeFunction::DemoType>(2));
            if (!rc::requestStartDemoInGameCutscene(this)) {
                return;
            }

            al::requestCaptureScreenCover(this, 4);
            getSceneInfo()->cameraDirector->storeCamera();
            mIsCameraReturn = true;
            rc::setUpdatePausedWatchersInDemo(this);
            rc::addDemoActor(this);
            for (s32 i = 0; i < mGigaBellNum; i++) {
                rc::addDemoActor(mGigaBells[i]);
            }

            rc::addDemoActor(controller);
            controller->forceKillSuperBowserLaser();
        }

        s32 closestIndex = getIndexOfGigaBellClosestToPlayer();
        for (s32 i = 0; i < mGigaBellNum; i++) {
            mGigaBells[i]->setIsBellInCutscene(i == closestIndex);
        }

        mIsUnlocked = true;
    } else if (mIsUnlocked) {
        mIsUnlocked = false;
    }

    for (s32 i = 0; i < mGigaBellNum; i++) {
        mGigaBells[i]->trySetLockState(mLockState);
    }
}

/**
 * @brief Check whether unlocking the bells plays the unlock cutscene.
 * @return True in the first phase or while the bells were never unlocked.
 */
bool GigaBellManager::shouldPlayUnlockCutscene() {
    return SingleModeDataFunction::getUnlockedPhase(this) == 1 ||
           !SingleModeDataFunction::isGigaBellUnlocked(this);
}

/**
 * @brief Find the bell closest to a position.
 * @param rPos The position.
 * @return Index of the closest bell, or -1 when there are none.
 */
inline s32 GigaBellManager::calcIndexOfGigaBellClosestTo(const sead::Vector3f& rPos) const {
    s32 closestIndex = -1;
    f32 closestDistance = -1.0f;
    for (s32 i = 0; i < mGigaBellNum; i++) {
        sead::Vector3f diff = al::getTrans(mGigaBells[i]);
        diff -= rPos;
        f32 distance = diff.squaredLength();
        if (distance < closestDistance || closestIndex == -1) {
            closestDistance = distance;
            closestIndex = i;
        }
    }

    return closestIndex;
}

/**
 * @brief Find the bell closest to the player.
 * @return Index of the closest bell, or -1 when there are none.
 */
s32 GigaBellManager::getIndexOfGigaBellClosestToPlayer() {
    sead::Vector3f playerPos = al::getTrans(al::tryFindNearestPlayerActor(mGigaBells[0]));
    return calcIndexOfGigaBellClosestTo(playerPos);
}

/**
 * @brief Wait while a bell cutscene plays.
 */
void GigaBellManager::exeWaitDemo() {}

/**
 * @brief Wait for the unlocked bell to open, then hand over to the next demo.
 */
void GigaBellManager::exeWaitOpenBell() {
    if (!al::isGreaterEqualStep(this, 1)) {
        return;
    }

    auto* luckyIslandHolder =
        al::tryGetSceneObj<LuckyIslandHolder>(this, cSceneObjIdLuckyIslandHolder);
    if (luckyIslandHolder != nullptr && luckyIslandHolder->canStartLuckyIslandDemo()) {
        luckyIslandHolder->queueLuckyIslandDemo();
    } else {
        bool isSelf = al::isNerve(this, &NrvGigaBellManagerWaitOpenBellSelf);
        auto* controller = DisasterModeController::tryGetController(this);
        if (isSelf) {
            if (controller != nullptr) {
                al::addDemoActor(controller);
            }
        } else if (controller == nullptr || !controller->setPostCutsceneDisasterFreezeTime()) {
            rc::requestEndDemoInGameCutscene(this);
        }
    }

    al::setNerve(this, &NrvGigaBellManagerWatch);
}

/**
 * @brief Wait for the bells to reset after their return cutscene.
 */
void GigaBellManager::exeWaitResetBell() {
    if (!al::isGreaterEqualStep(this, 1)) {
        return;
    }

    if (mIsReturnDemoStarted) {
        rc::requestEndDemoInGameCutscene(this);
        mIsReturnDemoStarted = false;
    }

    if (mIsSkipDisasterDemo) {
        mIsSkipDisasterDemo = false;
    } else {
        auto* controller = DisasterModeController::tryGetController(this);
        if (controller != nullptr) {
            al::addDemoActor(controller);
        }
    }

    al::setNerve(this, &NrvGigaBellManagerWatch);
}

/**
 * @brief Plessie chase in progress.
 */
void GigaBellManager::exePlessieChase() {}

/**
 * @brief Place Plessie and the player back at the start of the Plessie chase.
 */
void GigaBellManager::exePlessieChaseReset() {
    if (al::isFirstStep(this)) {
        auto* player = static_cast<PlayerActor*>(rc::getActivePlayer(this));
        auto* raidon = al::tryGetSceneObj<RaidonSurf>(this, SceneObjID_RaidonSurf);
        if (mSuperBowser->getLastPhase3CurrentIndex() == 0 ||
            (mSuperBowser->getLastPhase3CurrentIndex() == 10 && mIsPlessieChaseFinal)) {
            sead::Vector3f pos = mGigaBells[0]->getPlessieChasePos();
            sead::Vector3f front = mGigaBells[0]->getPlessieChaseFront();
            if (raidon != nullptr) {
                sead::Vector3f spawnPos = sead::Vector3f::zero;
                sead::Vector3f spawnFront = sead::Vector3f::ez;
                raidon->getClosestSpawnPosFront(front * 3000.0f + pos, &spawnPos, &spawnFront);
                al::offCollide(raidon);
                al::setTrans(raidon, spawnPos);
                al::setVelocity(raidon, sead::Vector3f::zero);
                sead::Quatf quat = sead::Quatf::unit;
                al::makeQuatFrontUp(&quat, spawnFront, sead::Vector3f::ey);
                al::setQuat(raidon, quat);
            }

            if (!player->isRaidonExist()) {
                rc::setPlayerTrans(player, pos);
                rc::setPlayerFrontVec(player, front);
            }
        } else {
            sead::Vector3f spawnFront = mSuperBowser->getCurrentSpawnInfo()->mFrontDir;
            sead::Vector3f front = -spawnFront;
            f32 distance = mIsPlessieChaseFinal ? 10000.0f : 15000.0f;
            sead::Vector3f pos = mSuperBowser->getCurrentSpawnInfo()->mTrans +
                                 spawnFront * distance + sead::Vector3f::ey * 500.0f;
            if (raidon != nullptr) {
                al::offCollide(raidon);
                al::setTrans(raidon, pos);
                al::setVelocity(raidon, sead::Vector3f::zero);
                sead::Quatf quat = sead::Quatf::unit;
                al::makeQuatFrontUp(&quat, front, sead::Vector3f::ey);
                al::setQuat(raidon, quat);
            }

            if (!player->isRaidonExist()) {
                rc::setPlayerTrans(player, sead::Vector3f::ey * 250.0f + pos);
                rc::setPlayerFrontVec(player, front);
            }
        }

        return;
    }

    if (al::isStep(this, 1)) {
        auto* raidon = al::tryGetSceneObj<RaidonSurf>(this, SceneObjID_RaidonSurf);
        if (raidon != nullptr) {
            al::onCollide(raidon);
        }

        if (mIsPlessieChaseFinal) {
            mSuperBowser->doPlessieChaseHit(true);
        }

        al::setCameraReset(this, true);
        al::showModelIfHide(mSuperBowser);
        al::setNerve(this, &NrvGigaBellManagerPlessieChase);
    }
}

/**
 * @brief Get a bell.
 * @param index Index of the bell.
 * @return The bell.
 */
GigaBell* GigaBellManager::getGigaBell(s32 index) {
    return mGigaBells[index];
}

/**
 * @brief Unlock the bells once enough goal items were collected and queue their cutscene.
 * @return Whether the unlock cutscene was queued.
 */
bool GigaBellManager::tryQueueCutscene() {
    disasterModeStart();
    if (mLockState != GigaBell::LockState::Unlocked || mIsUnlocked) {
        return false;
    }

    bool isQueued = false;
    if (shouldPlayUnlockCutscene()) {
        rc::requestEndDemoInGameCutscene(this);
        rc::setImmediateSwitchFlag(this);
        rc::requestStartDemoInGameCutscene(this);
        rc::setDemoAudioType(this, static_cast<alSeFunction::DemoType>(2));
        rc::setUpdatePausedWatchersInDemo(this);
        if (!getSceneInfo()->cameraDirector->isStoredCamera()) {
            getSceneInfo()->cameraDirector->storeCamera();
            mIsCameraReturn = true;
        }

        rc::addDemoActor(this);
        for (s32 i = 0; i < mGigaBellNum; i++) {
            rc::addDemoActor(mGigaBells[i]);
        }

        isQueued = true;
    }

    s32 closestIndex = getIndexOfGigaBellClosestToPlayer();
    for (s32 i = 0; i < mGigaBellNum; i++) {
        mGigaBells[i]->setIsBellInCutscene(i == closestIndex);
    }

    for (s32 i = 0; i < mGigaBellNum; i++) {
        if (i == closestIndex && shouldPlayUnlockCutscene()) {
            mGigaBells[i]->startUnlockCutscene();
        } else {
            mGigaBells[i]->trySetLockState(mLockState);
        }
    }

    mIsUnlocked = true;
    return isQueued;
}

/**
 * @brief Update the lock state when disaster mode starts.
 */
void GigaBellManager::disasterModeStart() {
    s32 goalItemsCollected = SingleModeDataFunction::getGoalItemsCollected(this);
    s32 lockCount = mLockCount;
    auto* goalItemHolder =
        al::tryGetSceneObj<GoalItemHolder>(this, SceneObjID_GoalItemHolder);
    if (goalItemHolder != nullptr && goalItemHolder->getCurrentGoalItem() != nullptr) {
        return;
    }

    if (goalItemsCollected < lockCount) {
        mLockState = GigaBell::LockState::LockedDisasterMode;
    } else {
        mLockState = GigaBell::LockState::Unlocked;
        SingleModeDataFunction::setHasSeenCutscene(this, cCutsceneIdGigaBellUnlock);
    }
}

/**
 * @brief Queue the cutscene of the bells returning to their pedestals.
 * @param isCheckOnly Only check whether the cutscene can play.
 * @param isSkipDisasterDemo Keep the disaster controller out of the demo afterwards.
 * @param isStartDemo Start the in-game cutscene demo.
 * @return Whether the cutscene was (or can be) queued.
 */
bool GigaBellManager::tryQueueReturnCutscene(bool isCheckOnly, bool isSkipDisasterDemo,
                                             bool isStartDemo) {
    if (SingleModeDataFunction::getUnlockedPhase(this) != 1) {
        return false;
    }

    if (SingleModeDataFunction::hasSeenCutscene(this, cCutsceneIdGigaBellReturn)) {
        return false;
    }

    if (mLockState != GigaBell::LockState::Unlocked) {
        return false;
    }

    if (isCheckOnly) {
        mIsSkipDisasterDemo = true;
        return true;
    }

    if (isStartDemo) {
        if (!rc::requestStartDemoInGameCutscene(this)) {
            return false;
        }

        mIsReturnDemoStarted = true;
    }

    if (isSkipDisasterDemo) {
        mIsSkipDisasterDemo = true;
    }

    al::setNerve(this, &NrvGigaBellManagerWaitDemo);
    if (!getSceneInfo()->cameraDirector->isStoredCamera()) {
        getSceneInfo()->cameraDirector->storeCamera();
        mIsCameraReturn = true;
    }

    s32 closestIndex = getIndexOfGigaBellClosestToPlayer();
    for (s32 i = 0; i < mGigaBellNum; i++) {
        mGigaBells[i]->startReturnCutscene(i == closestIndex);
        rc::addDemoActor(mGigaBells[i]);
    }

    SingleModeDataFunction::setHasSeenCutscene(this, cCutsceneIdGigaBellReturn);
    rc::setUpdatePausedWatchersInDemo(this);
    return true;
}

/**
 * @brief Lock the other bells once one of them was collected.
 * @param pBell The collected bell.
 */
void GigaBellManager::gigaBellCollected(GigaBell* pBell) {
    for (s32 i = 0; i < mGigaBellNum; i++) {
        if (mGigaBells[i] != pBell) {
            mGigaBells[i]->startLockedPlayerGiga();
        }
    }
}

/**
 * @brief Raise the number of goal items needed to unlock the bells.
 */
void GigaBellManager::incrementLockCount() {
    s32 lockCount = mLockCount + mLockCountIncrement;
    setLockCount(lockCount < mLockCountMax ? lockCount : mLockCountMax);
}

/**
 * @brief Find the bell closest to a position.
 * @param pos The position.
 * @return The closest bell, or nullptr when there are none.
 */
GigaBell* GigaBellManager::getGigaBellClosestTo(sead::Vector3f pos) {
    s32 closestIndex = calcIndexOfGigaBellClosestTo(pos);
    return closestIndex != -1 ? mGigaBells[closestIndex] : nullptr;
}

/**
 * @brief End the unlock cutscene.
 * @param isSelf Whether the cutscene was the bell's own one.
 */
void GigaBellManager::endCutscene(bool isSelf) {
    if (isSelf) {
        al::setNerve(this, &NrvGigaBellManagerWaitOpenBellSelf);
    } else {
        al::setNerve(this, &NrvGigaBellManagerWaitOpenBell);
    }
}

/**
 * @brief End the return cutscene.
 */
void GigaBellManager::endReturnCutscene() {
    al::addDemoActor(this);
    al::setNerve(this, &NrvGigaBellManagerWaitResetBell);
}

/**
 * @brief Get the number of bells.
 * @return The number of bells.
 */
s32 GigaBellManager::getGigaBellCount() {
    return mGigaBellNum;
}

/**
 * @brief Unlock the bells right away in super hard mode once enough Cat Shines were collected.
 */
void GigaBellManager::shineCollected() {
    auto* controller = DisasterModeController::tryGetController(this);
    if (controller == nullptr || !controller->isSuperHardMode()) {
        return;
    }

    if (SingleModeDataFunction::isGigaBellUnlocked(this)) {
        return;
    }

    if (SingleModeDataFunction::getGoalItemsCollected(this) < mLockCount) {
        return;
    }

    mLockState = GigaBell::LockState::Unlocked;
    SingleModeDataFunction::setHasSeenCutscene(this, cCutsceneIdGigaBellUnlock);
    al::requestCaptureScreenCover(this, 4);
}

/**
 * @brief Unlock all bells immediately.
 */
void GigaBellManager::forceUnlockBells() {
    mLockState = GigaBell::LockState::Unlocked;
    mIsUnlocked = true;
    for (s32 i = 0; i < mGigaBellNum; i++) {
        mGigaBells[i]->forceUnlock();
    }
}

/**
 * @brief Get the black fade wipe of the cutscenes.
 * @return The wipe.
 */
al::WipeSimple* GigaBellManager::getWipeFadeBlack() {
    return mWipeFadeBlack;
}

/**
 * @brief Get the white fade wipe of the cutscenes.
 * @return The wipe.
 */
al::WipeSimple* GigaBellManager::getWipeFadeWhite() {
    return mWipeFadeWhite;
}

/**
 * @brief Close the white fade wipe.
 */
void GigaBellManager::startCloseFadeWhite() {
    mWipeFadeWhite->startClose(10);
}

/**
 * @brief Check whether the lock count was raised above its minimum.
 * @return The result.
 */
bool GigaBellManager::hasLockCountBeenIncremented() {
    return mLockCount > mLockCountMin;
}

/**
 * @brief Check whether no bell cutscene is in progress.
 * @return True while the manager watches the goal items.
 */
bool GigaBellManager::isCutsceneDone() {
    return al::isNerve(this, &NrvGigaBellManagerWatch);
}

/**
 * @brief Check whether a bell plays its unlock cutscene.
 * @return The result.
 */
bool GigaBellManager::isInUnlockCutscene() {
    for (s32 i = 0; i < mGigaBellNum; i++) {
        if (mGigaBells[i]->isInUnlockCutscene()) {
            return true;
        }
    }

    return false;
}

/**
 * @brief Reset the bells once their return cutscene was seen.
 */
void GigaBellManager::resetGigaBells() {
    if (SingleModeDataFunction::getUnlockedPhase(this) == 1 &&
        SingleModeDataFunction::hasSeenCutscene(this, cCutsceneIdGigaBellReturn) &&
        mLockState == GigaBell::LockState::Unlocked) {
        for (s32 i = 0; i < mGigaBellNum; i++) {
            mGigaBells[i]->startReset();
        }
    }
}

/**
 * @brief Get the next explanation cutscene to play for a layer.
 * @param layerId The layer of the bell.
 * @param pCutsceneId Set to the id of the cutscene, or -1.
 * @return The cutscene, or nullptr when there is none to play.
 */
DemoCutscene* GigaBellManager::getNextExplainationCutscene(s32 layerId, s32* pCutsceneId) {
    if (SingleModeDataFunction::getUnlockedPhase(this) == layerId && layerId == 1 &&
        !SingleModeDataFunction::hasSeenCutscene(this, cCutsceneIdGigaBellUnlock)) {
        *pCutsceneId = cCutsceneIdGigaBellUnlock;
        DemoCutscene* cutscene = getExplainationCutscene(0);
        cutscene->setCutsceneId(cCutsceneIdGigaBellUnlock);
        return cutscene;
    }

    *pCutsceneId = -1;
    return nullptr;
}

/**
 * @brief Get an explanation cutscene.
 * @param index Index of the cutscene.
 * @return The cutscene, or nullptr.
 */
DemoCutscene* GigaBellManager::getExplainationCutscene(s32 index) {
    return mExplainCutscenes[index];
}

/**
 * @brief Start the Plessie chase of the last battle.
 * @param pBowser Fury Bowser.
 */
void GigaBellManager::startPlessieChase(SuperBowser* pBowser) {
    mSuperBowser = pBowser;
    for (s32 i = 0; i < mGigaBellNum; i++) {
        mGigaBells[i]->kill();
    }

    mGigaBell3in1->appear();
    al::setNerve(this, &NrvGigaBellManagerPlessieChase);
}

/**
 * @brief Ring the bells during the Plessie chase.
 */
void GigaBellManager::hitBellPlessieChase() {
    mPlessieChaseHitCount++;
    mSuperBowser->doPlessieChaseHit(false);
    auto* raidon = al::tryGetSceneObj<RaidonSurf>(this, SceneObjID_RaidonSurf);
    raidon->plessieChaseHitBells(al::getHitSensor(mGigaBell3in1, "Body"));
}

/**
 * @brief Hit the first bell not yet hit during the Plessie chase.
 * @param pOther The sensor hitting the bell.
 * @param pSelf The bell's sensor.
 */
void GigaBellManager::tryHitNextBellPlessieChase(al::HitSensor* pOther, al::HitSensor* pSelf) {
    for (s32 i = 0; i < mGigaBellNum; i++) {
        if (!mGigaBells[i]->hasBeenHitPlessieChase()) {
            mGigaBells[i]->plessieChaseHit(pOther, pSelf);
            return;
        }
    }
}

/**
 * @brief Pick the movement pattern of the bells from the Plessie chase progress.
 */
void GigaBellManager::updateMoveType() {
    if (mIsDebugMoveType) {
        mMoveType = mDebugMoveType;
        return;
    }

    switch (mPlessieChaseHitCount) {
    case 1:
        mMoveType = MoveType::Circle;
        break;
    case 2:
        mMoveType = MoveType::Atom;
        break;
    default:
        mMoveType = MoveType::Default;
        break;
    }
}

/**
 * @brief Compute the offset of a bell on its movement pattern.
 * @param index Index of the bell.
 * @return The offset.
 */
sead::Vector3f GigaBellManager::calcMoveOffset(s32 index) {
    f32 moveTime = mMoveDefault.moveTime;
    if (mMoveType == MoveType::FigureEight) {
        moveTime = mMoveFigureEight.moveTime;
    } else if (mMoveType == MoveType::BowTie) {
        moveTime = mMoveBowTie.moveTime;
    } else if (mMoveType == MoveType::Rose) {
        moveTime = mMoveRose.moveTime;
    } else if (mMoveType == MoveType::Circle) {
        moveTime = mMoveCircle.moveTime;
    } else if (mMoveType == MoveType::Atom) {
        moveTime = mMoveAtom.moveTime;
    }

    sead::Vector3f offset = sead::Vector3f::zero;
    f32 indexRate = static_cast<f32>(index) / static_cast<f32>(mGigaBellNum);
    f32 time = static_cast<f32>(mMoveFrame) / static_cast<s32>(moveTime);
    f32 rate = time + indexRate;

    switch (mMoveType) {
    case MoveType::Default: {
        f32 angle = sead::Mathf::deg2rad(indexRate * 360.0f + 90.0f);
        offset.x += mMoveDefault.moveDistance * cosf(angle);
        offset.y += mMoveDefault.moveDistance * sinf(angle);
        if (!mSuperBowser->isIgnoreBellOffsetY()) {
            offset.y += mMoveDefault.offsetY;
        }

        break;
    }
    case MoveType::Swing: {
        offset += sead::Vector3f::ex *
                  (cosf(sead::Mathf::deg2rad(rate * 360.0f)) * mMoveSwing.moveDistance);
        f32 height = mMoveSwing.moveDistance * mMoveSwing.moveHeight;
        offset += sead::Vector3f::ey * (height * cosf(sead::Mathf::deg2rad((rate + rate) * 360.0f)));
        offset.y += mMoveSwing.offsetY;
        break;
    }
    case MoveType::FigureEight: {
        offset += sead::Vector3f::ex *
                  (cosf(sead::Mathf::deg2rad(rate * 360.0f)) * mMoveFigureEight.moveDistance);
        f32 height = mMoveFigureEight.moveDistance * mMoveFigureEight.moveHeight;
        offset += sead::Vector3f::ey * (height * sinf(sead::Mathf::deg2rad((rate + rate) * 360.0f)));
        offset.y += mMoveFigureEight.offsetY;
        break;
    }
    case MoveType::BowTie: {
        f32 c = cosf(sead::Mathf::deg2rad(rate * 360.0f));
        f32 x = al::sign(c) * calcEaseOutCubic(c > 0.0f ? c : -c);
        offset += sead::Vector3f::ex * (mMoveBowTie.moveDistance * x);
        f32 height = mMoveBowTie.moveDistance * mMoveBowTie.moveHeight;
        offset += sead::Vector3f::ey * (height * sinf(sead::Mathf::deg2rad((rate + rate) * 360.0f)));
        offset.y += mMoveBowTie.offsetY;
        break;
    }
    case MoveType::Rose: {
        f32 coefficient = mMoveRose.coefficient;
        f32 roseRate = rate;
        s32 petalNum = sead::Mathf::round(coefficient);
        if (coefficient == petalNum) {
            f32 k = petalNum;
            f32 petalRate = (rate - sead::Mathf::floor(rate * k) / k) * k;
            f32 k2 = k + k;
            f32 baseRate = sead::Mathf::floor(rate * k2) / k2;
            f32 t = k2 * (rate - baseRate);
            f32 linear = t * mMoveRose.smoothness;
            f32 ease = petalRate < 0.5f ? al::easeOut(t) : al::easeIn(t);
            roseRate = baseRate + (linear + ease) / (mMoveRose.smoothness + 1.0f) / k2;
        }

        f32 radius = sinf(sead::Mathf::deg2rad(mMoveRose.rotationOffset +
                                               roseRate * mMoveRose.coefficient * 180.0f));
        f32 theta = sead::Mathf::deg2rad(roseRate * 180.0f);
        f32 x = radius * cosf(theta);
        f32 y = radius * sinf(theta);
        f32 distance = mMoveRose.moveDistance;
        offset += sead::Vector3f::ex * (x * distance);
        offset += sead::Vector3f::ey * (distance * y) * mMoveRose.moveHeight;
        offset.y += mMoveRose.offsetY;
        f32 angle = sead::Mathf::deg2rad(rate * 360.0f);
        offset.x += distance * cosf(angle);
        offset.y += mMoveRose.moveDistance * sinf(angle);
        break;
    }
    case MoveType::Circle: {
        f32 angle = sead::Mathf::deg2rad(rate * 360.0f);
        offset.x += mMoveCircle.moveDistance * cosf(angle);
        offset.y += mMoveCircle.moveDistance * sinf(angle) * mMoveCircle.moveHeight;
        offset.y += mMoveCircle.offsetY;
        break;
    }
    case MoveType::Atom: {
        f32 angle = sead::Mathf::deg2rad(time * 360.0f);
        offset.x = mMoveAtom.moveDistance * cosf(angle);
        offset.z = mMoveAtom.moveDistance * sinf(angle) * mMoveAtom.moveHeight;
        al::rotateVectorDegree(&offset, offset, sead::Vector3f::ez, indexRate * 360.0f);
        sead::Quatf quat = sead::Quatf::unit;
        sead::Vector3f up(cosf(sead::Mathf::deg2rad(time * mMoveAtom.rotateSpeed * 360.0f)),
                          sinf(sead::Mathf::deg2rad(time * mMoveAtom.rotateSpeed * 360.0f)),
                          0.0f);
        al::makeQuatFrontUp(&quat, sead::Vector3f::ez, up);
        offset.rotate(quat);
        offset.y += mMoveAtom.offsetY;
        break;
    }
    default:
        break;
    }

    return offset;
}

/**
 * @brief Get the number of bell hits of the Plessie chase.
 * @return The hit count.
 */
s32 GigaBellManager::getPlessieChaseHitCount() {
    return mPlessieChaseHitCount;
}

/**
 * @brief Check the level of the Plessie chase.
 * @param level The level, starting at 1.
 * @return Whether the chase is at that level.
 */
bool GigaBellManager::isPlessieChaseLv(s32 level) {
    return mPlessieChaseHitCount == level - 1;
}

/**
 * @brief Get the offset of the bells from Fury Bowser's socket.
 * @return The offset.
 */
sead::Vector3f GigaBellManager::getPlessieChaseBellBaseOffset() {
    return mSuperBowser->isPlessieChaseFirstTier() ? mBaseSocketOffsetFirst : mBaseSocketOffset;
}

/**
 * @brief Restart the Plessie chase.
 * @param hitCount The bell hit count to restart from.
 * @param tier The tier of Fury Bowser's chase.
 * @param isFinal Whether the chase restarts at its final hit.
 */
void GigaBellManager::plessieChaseReset(s32 hitCount, s32 tier, bool isFinal) {
    if (!mSuperBowser->isLastPhase3Bowser()) {
        return;
    }

    mIsPlessieChaseFinal = isFinal;
    mPlessieChaseHitCount = hitCount;
    mGigaBell3in1->reset();
    mSuperBowser->resetPlessieChase(tier);
    al::setNerve(this, &NrvGigaBellManagerPlessieChaseReset);
}

/**
 * @brief Check whether the bells can be hit during the Plessie chase.
 * @return The result.
 */
bool GigaBellManager::canHitBellPlessieChase() {
    return mSuperBowser->canPlessieChaseHit();
}

/**
 * @brief Repeat the effect of a final Plessie chase hit.
 * @param index Index of the final hit.
 */
void GigaBellManager::doPlessieChaseHitRepeat(s32 index) {
    if (mLastHitBell == nullptr) {
        mLastHitBell = mGigaBells[0];
    }

    switch (index) {
    case 0:
        al::startSe(mLastHitBell, "PlessieChaseHitFinal1");
        break;
    case 1:
        al::startSe(mLastHitBell, "PlessieChaseHitFinal2");
        mLastHitBell->doPlessieChaseHitEffect();
        break;
    case 2:
        al::startSe(mLastHitBell, "PlessieChaseHitFinal3");
        mLastHitBell->doPlessieChaseHitEffect();
        break;
    default:
        break;
    }
}

/**
 * @brief Remember the last bell hit during the Plessie chase.
 * @param pBell The bell.
 */
void GigaBellManager::setLastHitBell(GigaBell* pBell) {
    mLastHitBell = pBell;
}

/**
 * @brief Get the fused bell of the last battle.
 * @return The fused bell, or nullptr.
 */
GigaBell3in1* GigaBellManager::getGigaBell3in1() {
    return mGigaBell3in1;
}

/**
 * @brief Toggle the debug loop of the bell collection.
 */
void GigaBellManager::toggleLoopCollect() {
    mIsLoopCollect = !mIsLoopCollect;
}

/**
 * @brief Get the knockback of the current Plessie chase hit.
 * @return The knockback parameters.
 */
GigaBellManager::PlessieChaseKnockbackParam GigaBellManager::getCurrentKnockbackParam() {
    if (mSuperBowser->isPlessieChaseTierJump()) {
        if (mPlessieChaseHitCount == 3) {
            return mKnockbackFinalHitP10;
        }

        return mKnockbackP10;
    }

    if (mPlessieChaseHitCount == 3) {
        return mKnockbackFinalHit;
    }

    return mKnockback;
}
