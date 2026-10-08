#pragma once

#include <container/seadPtrArray.h>
#include <math/seadQuat.h>
#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"

namespace al {
class AreaObj;
class CameraTicket;
class FunctorBase;
class ModelDrawerBase;
class MtxConnector;
}  // namespace al

class ActorRailBrakeMover;
class ActorStateDemoCameraProto;
class CameraLookAtPoint;
class DoorLock;
class GoalItemBindPuppeteer;
class GoalItemHolder;
class IGoalItemCollectListener;
class IGoalItemListener;
class InkPatch;
class Lighthouse;
class SinkedItem;

/**
 * @brief A Shine (Power Star / Cat Shine) that ends a course or an island scenario when collected.
 */
class GoalItem : public al::LiveActor {
public:
    explicit GoalItem(const char* pName);

    void init(const al::ActorInitInfo& rInfo) override;
    void onGoalItemGetSwitch();
    void hide();
    void appearIdle();
    void tryHide();
    void disasterFadeoutDoneFunc();
    void initAfterPlacement() override;
    void finishBowserExit();
    void requestEndAnimDemo();
    void setStageSwitchAnimState(bool isOn);
    void appear() override;
    void quickAppear();
    void appearHidden();
    void appearCollect(bool isAddDemoPlayer);
    void makeActorDead() override;
    bool canCollect() const;
    void makeActorDeadAll();
    bool isCollected();
    void startDemoActor(s32 demoType) override;
    void endDemoActor(s32 demoType) override;
    void appearCheckpoint();
    f32 getVerticalOffset();
    void setFront(sead::Vector3f& rFront, bool isKeep, f32 angle);
    void control() override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool canReact() const;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    bool receiveMsgScreenPointSM(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                 al::ScreenPointTarget* pTarget) override;
    bool isRailMove();
    void exeDisappear();
    void playAppearRiseAnim();
    void exeAppearAnim();
    void exeWaitInstantCollect();
    void exeWaitStartCollectDemo();
    void exeWait();
    void exeSpin();
    void exeSpinSpeedDown();
    void endSpinSpeedDown();
    void exeWaitCameraIn();
    void exeWaitCameraOut();
    void exeCollectRepeated();
    void exeCollect();
    void exePreAnimate();
    void setCameraDirection();
    bool turnPlayerToCamera(f32 rate);
    void updateCameraDirection();
    void exeAnimate();
    bool handleDisasterMode();
    bool tryPlayGoalToLighthouseCutscene();
    void setWaitForLighthouseShineCutscene(bool isInstant, bool isSkip);
    void exeCameraPlay();
    void exeFinishAnimateOceanCase();
    void exeAnimateOceanCaseCameraReturn();
    void exeFinalizeOceanCase();
    void finishBindPlayer(bool isBindEnded);
    void exeAnimateMoveRail();
    void jumpRailToFinish();
    void exeAnimateCodeMoveRail();
    void exeAnimateMoveRailFinish();
    void exeWaitOceanBowserExit();
    void exeWaitDisasterFadeOutDone();
    void endBindPlayer();
    void exeWaitOceanBowserExitGigaBell();
    void exeWaitOceanBowserExitReturn();
    void exeCleanUpFinish();
    void exeCleanUpFinishNoBGM();
    void tryTriggerGoalItemLightStart(s32 step);
    void lighthouseSequenceAlmostEnded();
    void lighthouseSequenceCompletelyEnded(bool isInstant);
    void finishGoalItemCutscene();
    void completeFinish();
    void exeEndLighthouseCutscene();
    void lighthouseLightSequenceSkipped();
    void lighthouseLightShone();
    void exeLighthouseShoneCutscene();
    void lighthouseDarkBowserGone(bool isGone);
    void exeWaitForLighthouseShineCutscene();
    void exeInkCameraPlay();
    void exeGuideMessage();
    void exeDone();
    bool quickFinish();
    void addGoalItemToDemo();

    void setFromShards() { mFromShards = true; }

    void setCollectListener(IGoalItemCollectListener* pListener) { mCollectListener = pListener; }

    void setCollectedBySensor() { mCollectedBySensor = true; }

    void disableCollectionFlag() { mCollectionFlag = false; }

    s32 getIslandId() const { return mIslandId; }

    s32 getShineId() const { return mShineId; }

    bool isNekoShine() const { return mIsNekoShine; }

    bool isDisasterShine() const { return mIsDisasterShine; }

    void setRegistration(s32 index, s32 islandId) {
        mIndex = index;
        mIslandId = islandId;
    }

    void setIslandId(s32 islandId) { mIslandId = islandId; }

    void setLighthouse(Lighthouse* pLighthouse) { mLighthouse = pLighthouse; }

    /**
     * @brief Set the listener notified when the item's animation ends.
     * @param pListener The listener (stored in the slot also typed as an end functor; both are
     * called through the first virtual function).
     */
    void setAnimEndListener(IGoalItemListener* pListener) {
        mAnimEndFunctor = reinterpret_cast<al::FunctorBase*>(pListener);
    }

private:
    /**
     * @brief Mark the item (and its empty twin) as collected.
     */
    void setCollected() {
        mIsCollected = true;

        if (!mIsEmptyItem && mIsMultiCollect) {
            mPairItem->mIsCollected = true;
            mPairItem->mPlayer = mPlayer;
        }
    }

    /**
     * @brief Get the position of the bound player, or of the item when nobody is bound.
     * @return The position.
     */
    const sead::Vector3f& getBindTrans() const;

    al::CameraTicket* mProgramableCamera = nullptr;              // 0x148
    sead::Vector3f mCameraPos = sead::Vector3f::zero;            // 0x150
    sead::Vector3f mCameraAt = sead::Vector3f::zero;             // 0x15c
    s32 mIslandId = -5;                                          // 0x168
    s32 mShineId = -1;                                           // 0x16c
    bool mIsNekoShine = false;                                   // 0x170
    bool mIsDisasterShine = false;                               // 0x171
    bool mIsLuckyShine = false;                                  // 0x172
    GoalItemHolder* mHolder = nullptr;                           // 0x178
    GoalItemBindPuppeteer* mPuppeteer = nullptr;                 // 0x180
    s32 mAnimationFrameCount = -1;                               // 0x188
    bool mIsExistAnimateAction = false;                          // 0x18c
    s32 mIslandClearStartFrame = -1;                             // 0x190
    s32 mIslandClearEndFrame = -1;                               // 0x194
    al::LiveActor* mPlayer = nullptr;                            // 0x198
    al::MtxConnector* mMtxConnector = nullptr;                   // 0x1a0
    f32 mCollisionCheckDist;                                     // 0x1a8
    al::ActorInitInfo* mRestartInfo = nullptr;                   // 0x1b0
    s32 mIndex = -1;                                             // 0x1b8
    ActorStateDemoCameraProto* mDemoCamera = nullptr;            // 0x1c0
    void* _1c8 = nullptr;                                        // 0x1c8
    const char* mSharedLabel = nullptr;                          // 0x1d0
    bool mIsCollected = false;                                   // 0x1d8
    sead::Vector3f mCameraDir = sead::Vector3f::zero;            // 0x1dc
    sead::Vector3f mCameraPosTarget = sead::Vector3f::zero;      // 0x1e8
    sead::Vector3f mCameraAtTarget = sead::Vector3f::zero;       // 0x1f4
    f32 mCameraAngleH = 0.0f;                                    // 0x200
    f32 mCameraDistance = 680.0f;                                // 0x204
    f32 mCameraAngleV;                                           // 0x208
    al::CameraTicket* mObjectCamera = nullptr;                   // 0x210
    s32 mFocusCameraInStep = 0;                                  // 0x218
    s32 mFocusCameraHoldStep = 0;                                // 0x21c
    s32 mFocusCameraOutStep = 0;                                 // 0x220
    s32 mAreaOutStep;                                            // 0x224
    Lighthouse* mLighthouse;                                     // 0x228
    bool mIsMultiCollect = false;                                // 0x230
    bool mIsAnimating = false;                                   // 0x231
    bool mIsAppearCamera = false;                                // 0x232
    bool mIsNoIslandFlagCutscene = false;                        // 0x233
    bool mIsCollectPending = false;                              // 0x234
    bool mIsSpinEnd = false;                                     // 0x235
    bool mIsTimerChallenge = false;                              // 0x236
    bool _237 = false;                                           // 0x237
    bool mCollectionFlag = false;                                // 0x238
    bool mIsLandedPosSet = false;                                // 0x239
    bool mIsRetainAppearAngle = false;                           // 0x23a
    bool mIsDemo = false;                                        // 0x23b
    bool mIsWaitEffectDeleted = false;                           // 0x23c
    s32 mDisasterModeSetting = 0;                                // 0x240
    InkPatch* mInkPatch = nullptr;                               // 0x248
    s32 mInkCameraInStep = 0;                                    // 0x250
    s32 mInkCameraHoldStep = 0;                                  // 0x254
    DoorLock* mDoorLock = nullptr;                               // 0x258
    al::AreaObj* mCameraArea = nullptr;                          // 0x260
    sead::Vector3f mLandedPos;                                   // 0x268
    sead::Vector3f mLandedFront;                                 // 0x274
    bool mIsAppearWaitDefault;                                   // 0x280
    bool mIsAppearWait = false;                                  // 0x281
    bool mIsConsistentAngle = false;                             // 0x282
    bool mIsPhase0 = false;                                      // 0x283
    bool mIsBeforeFlashy = false;                                // 0x284
    bool mIsSetDistance = false;                                 // 0x285
    bool mIsSetExitAngle = false;                                // 0x286
    bool mIsRailMove[5];                                         // 0x287
    bool mIsFixedCamera = false;                                 // 0x28c
    bool mIsCameraDirSet;                                        // 0x28d
    bool mIsInkMeNot = false;                                    // 0x28e
    bool mIsFrontSet = false;                                    // 0x28f
    bool mIsCodeBasedPath = false;                               // 0x290
    bool mIsKeepVerticalAngle = false;                           // 0x291
    bool mIsReturnToOldCameraPos = false;                        // 0x292
    bool mIsSetExitInterpolate = true;                           // 0x293
    bool _294;                                                   // 0x294
    bool mCollectedBySensor = false;                             // 0x295
    f32 mExitAngleH = 0.0f;                                      // 0x298
    f32 mExitAngleV = 20.0f;                                     // 0x29c
    f32 mExitDirRate = 0.14f;                                    // 0x2a0
    f32 mExitZoomInRate = 0.12f;                                 // 0x2a4
    f32 mExitTgtOffsetY = 180.0f;                                // 0x2a8
    s32 mExitCamBlendCount = 60;                                 // 0x2ac
    s32 mMoveCount = 50;                                         // 0x2b0
    sead::Vector3f mInitTrans;                                   // 0x2b4
    sead::Quatf mInitQuat = sead::Quatf::unit;                   // 0x2c0
    sead::Vector3f mTargetTrans;                                 // 0x2d0
    f32 mTargetDistance;                                         // 0x2dc
    f32 mSafeAngle = 0.0f;                                       // 0x2e0
    ActorRailBrakeMover* mRailMover;                             // 0x2e8
    sead::Vector3f mFront = sead::Vector3f::zero;                // 0x2f0
    f32 _2fc = 0.0f;                                             // 0x2fc
    f32 mArcHeight = 3000.0f;                                    // 0x300
    sead::Vector3f mCodeRailStart;                               // 0x304
    sead::Vector3f mCodeRailEnd;                                 // 0x310
    f32 mCodeRailRate;                                           // 0x31c
    f32 mCodeRailSpeed;                                          // 0x320
    sead::Vector3f mStartCameraPos;                              // 0x324
    sead::Vector3f mStartCameraAt;                               // 0x330
    sead::Vector3f mReturnCameraPos;                             // 0x33c
    sead::Vector3f mReturnCameraAt;                              // 0x348
    sead::PtrArray<al::ModelDrawerBase> mDrawers;                // 0x358
    GoalItem* mPairItem = nullptr;                               // 0x368
    bool _370 = false;                                           // 0x370
    bool _371 = false;                                           // 0x371
    bool mIsEmptyItem = false;                                   // 0x372
    bool mIsClearGenericSaveLocation = false;                    // 0x373
    bool mIsSwitchOnGet = false;                                 // 0x374
    bool mIsFrontAngleSet = false;                               // 0x375
    f32 mFrontAngle = 0.0f;                                      // 0x378
    f32 mPhase2OffsetY = 0.0f;                                   // 0x37c
    f32 mPhase2DistanceOffset = 0.0f;                            // 0x380
    f32 mPhase3OffsetY = 0.0f;                                   // 0x384
    f32 mPhase3DistanceOffset = 0.0f;                            // 0x388
    s32 mInkInterpoleStep = 120;                                 // 0x38c
    s32 mInkLinkId = -1;                                         // 0x390
    CameraLookAtPoint* mInkLookAtPoint = nullptr;                // 0x398
    void* _3a0;                                                  // 0x3a0
    SinkedItem* mSinkedItem = nullptr;                           // 0x3a8
    al::FunctorBase* mAnimEndFunctor = nullptr;                  // 0x3b0
    IGoalItemCollectListener* mCollectListener = nullptr;        // 0x3b8
    bool mFromShards = false;                                    // 0x3c0
    bool mIsDisasterFadeoutDone = false;                         // 0x3c1
    bool mIsSpinRestart = false;                                 // 0x3c2
    bool mIsCompletelyIdle = false;                              // 0x3c3
    s32 mInkReturnMoveCount = 120;                               // 0x3c4
    al::FunctorBase* mDisasterFadeoutFunctor = nullptr;          // 0x3c8
};

static_assert(sizeof(GoalItem) == 0x3d0);
