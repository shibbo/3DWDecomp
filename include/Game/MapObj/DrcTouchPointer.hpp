#pragma once

#include <prim/seadSafeString.h>

#include "Library/LiveActor/LiveActor.hpp"

namespace al {
class ArrowHitInfo;
class HitSensor;
class ScreenPointer;
class ScreenPointTarget;
}  // namespace al

namespace rc {
class Stamp;
}  // namespace rc

class DrcAssistDirector;
class DrcTouchAssistInfo;
class DrcTouchEffectTraceTracker;
class TouchPointTransparent;

/**
 * @brief The touch-screen / gyro pointer actor: follows the touch position on the stage, sends
 *        touch messages to what it points at and carries items and stamps.
 */
class DrcTouchPointer : public al::LiveActor {
public:
    enum GyroTouchState {
        GyroTouchState_None = 0,
        GyroTouchState_NoTouch = 1,
        GyroTouchState_Touch = 2,
    };

    DrcTouchPointer(const char* pName, const DrcTouchAssistInfo* pTouchInfo, bool isMiddleRange,
                    bool isLongRange, DrcAssistDirector* pDirector,
                    DrcTouchEffectTraceTracker* pTraceTracker);

    void startAction(const char* pActionName, bool isKeepTrace);
    void hideModelIfShow();
    void forcePrevPos();
    void init(const al::ActorInitInfo& rInfo) override;
    bool isPointer(const al::ScreenPointer* pPointer) const;
    bool isSensor(const al::HitSensor* pSensor) const;
    void setCharacter(s32 character);
    void setInvalidChar();
    void control() override;
    bool isDisappearNerve() const;
    bool isGyroNoTouchNerve() const;
    bool isPhysicallyTouchingScreen() const;
    void startAppear(bool isGyro);
    bool calcHitPosAndNormal();
    void initialAppear();
    bool isItemGrab() const;
    al::ScreenPointTarget* getHitTarget(s32 index) const;
    void startNewStamp();
    void startStampGrab(al::LiveActor* pStamp, bool isKeepRotation);
    void setDisappearNerve(bool isKeepGyro);
    void handleTouchRelease(s32 frame);
    void tryResetActiveTime();
    bool isReverseTransparent() const;
    void changeGyroState(GyroTouchState state);
    void startSnapshotMode();
    void endSnapshotMode();
    bool isHoldStamp();
    s32 getHitTargetTableSize() const;
    void startDisappearGyro();
    void startDisappear();
    bool isVisible() const;
    bool isVisibleAndNotHidden() const;
    bool isCompletelyAlive() const;
    bool isDisappearing() const;
    bool tryThrowReleaseStamp();
    void startDisappearForce();
    void startRelease();
    void exeDisAppear();
    bool handleGyroButtonInput(bool& rIsPress, bool& rIsRelease);
    void exeFadeToGyroNoTouch();
    void updateTransparentGyro();
    void exeWaitGyroNoTouch();
    void exeHiddenGyroNoTouch();
    void exeHiddenGyroTouch();
    void exeWait();
    void exeGrabItemStart();
    void exeGrabItem();
    void exeReleaseItem();
    void exeThrowItem();
    void exeStroke();
    void exeHold();
    void exeKnock();
    void exeBurnStart();
    void exeBurn();
    void exeTouchObj();
    bool isEnableGrabItem(const al::LiveActor* pActor) const;

    const sead::Vector3f& getHitNormal() const { return mHitNormal; }

    bool isGyroStarted() const { return mIsGyroStarted; }

    bool isSnapshotMode() const { return mIsSnapshotMode; }

private:
    void showModel();
    void tryDeleteTraceEffect();
    void stopSklAnim();
    void sendMsgReleaseItem();

    s32 mHitFlags = 0;                                      // 0x144
    TouchPointTransparent* mTransparent = nullptr;          // 0x148
    const DrcTouchAssistInfo* mTouchInfo;                   // 0x150
    al::ScreenPointer* mScreenPointer = nullptr;            // 0x158
    al::HitSensor* mHitSensor = nullptr;                    // 0x160
    al::LiveActor* mGrabActor = nullptr;                    // 0x168
    rc::Stamp* mGrabCandidate = nullptr;                    // 0x170
    al::ScreenPointTarget* mHitTarget = nullptr;            // 0x178
    const al::ArrowHitInfo* mHitArrowInfo;                  // 0x180
    sead::Vector3f mHitPos = sead::Vector3f::zero;          // 0x188
    sead::Vector3f mPrevHitPos;                             // 0x194
    sead::Vector3f mHitNormal = sead::Vector3f::zero;       // 0x1A0
    sead::Vector3f mPrevHitNormal;                          // 0x1AC
    f32 mSlideSpeed = 0.0f;                                 // 0x1B8
    f32 mCheckLength = 5000.0f;                             // 0x1BC
    s32 mSeLocalVariable = -1;                              // 0x1C0
    s32 mKnockStep = 0;                                     // 0x1C4
    sead::FixedSafeString<32> mKnockActionName{"Knock"};    // 0x1C8
    bool mIsSlowDisappear = false;                          // 0x200
    bool mIsGyroStarted = false;                            // 0x201
    s32 mThrowCooldown = 0;                                 // 0x204
    s32 mGyroTimer = 0;                                     // 0x208
    GyroTouchState mGyroState = GyroTouchState_None;        // 0x20C
    s32 mCharacter = -1;                                    // 0x210
    DrcAssistDirector* mDirector;                           // 0x218
    DrcTouchEffectTraceTracker* mTraceTracker;              // 0x220
    bool mIsSnapshotMode = false;                           // 0x228
    bool mIsSnapshotTouched = false;                        // 0x229
    bool mIsModelActive = false;                            // 0x22A
    bool mIsSingleMode = false;                             // 0x22B
};

static_assert(sizeof(DrcTouchPointer) == 0x230);
