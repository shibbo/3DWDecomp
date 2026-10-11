#pragma once

#include <basis/seadTypes.h>
#include <math/seadMatrix.h>
#include <math/seadVector.h>

#include "Library/Scene/IUseSceneObjHolder.hpp"
#include "Project/Camera/Core/IUseCameraDirector.hpp"

namespace al {
struct ActorInitInfo;
class HitSensor;
class IUseCamera;
class LiveActor;
class PlayerHolder;
class SceneCameraInfo;
class SceneObjHolder;
class ScreenPointer;
}  // namespace al

namespace rc {
class StampDirector;
}  // namespace rc

class DrcTouchAssistInfo;
class DrcTouchEffectTraceTracker;
class DrcTouchPointer;
class PlayerAliveWatcher;
class TouchEffect;

/**
 * @brief Touch-screen (DRC) assist of one pad port: drives the touch pointer actor and its
 *        layout effect from the touch panel or from the Joy-Con gyro pointer.
 */
class DrcAssistDirector : public al::IUseSceneObjHolder, public al::IUseCamera {
public:
    /// Frames a touch is kept alive after the panel / gyro button is released.
    static constexpr s32 cTouchKeepFrame = 5;

    DrcAssistDirector(s32 port, bool isLongRange);

    al::SceneObjHolder* getSceneObjHolder() const override;
    virtual void initAfterPlacementSceneObj(const al::ActorInitInfo& rInfo,
                                            DrcTouchEffectTraceTracker* pTraceTracker);
    al::SceneCameraInfo* getSceneCameraInfo() const override;

    bool isDrcDirector(const al::HitSensor* pSensor) const;
    bool isDrcDirector(const al::ScreenPointer* pPointer) const;
    al::LiveActor* getPlayer() const;
    static void controllerEventCallback(s32 event, void* pUserData);
    void checkPreStartPose();
    bool doCheckPoseImpl(bool isCheckDiff);
    void update();
    void disappearTouchPointerEffect();
    s32 isPoseExtreme(sead::Vector3f* pAccel, const sead::Vector3f& rThreshold,
                      const sead::Vector3f& rLimit) const;
    void setJoyConBaseAngle(const sead::Vector2f& rScreenPos);
    void setJoyConBaseAngle(const sead::Vector3f& rWorldPos);
    void calcJoyConBaseAngle();
    void calcJoyConDiffAngle();
    void calcOffscreenAmount(sead::Vector2f& rAmount, const sead::Vector2f& rPos,
                             const sead::Vector2f& rMin, const sead::Vector2f& rMax);
    void updateZeroPushAmount(const sead::Vector2f& rOffscreen, const sead::Vector2f& rPos,
                              const sead::Vector2f& rMin, const sead::Vector2f& rMax);
    void validate();
    void invalidate(bool isForce);
    void disappearTouchPointer();
    void invalidateImmediate();
    void disappearTouchPointerImmediately();
    bool isValid() const;
    bool isTouch() const;
    bool isTouch(const sead::Vector3f& rPos, f32 radius) const;
    bool isTouchTrigger(const sead::Vector3f& rPos, f32 radius) const;
    bool isEnableTouchPointer() const;
    bool isEnableTouchPointerGrabItem(const al::LiveActor* pActor) const;
    s32 getTouchControlMode();
    void incTouchControlMode();
    void toggleTouchDisabled();
    void setTouchDisabled(bool isDisabled);
    bool isTouchDisabled();
    DrcTouchAssistInfo* getTouchAssistInfo();
    const DrcTouchAssistInfo* getTouchAssistInfo() const;
    bool isCursorFadedOut() const;
    void releaseTouchPointerHoldItem();
    void toggleTouchEffect();
    const sead::Vector2f& getTouchIconPos() const;
    const sead::Vector3f& getTouchPointerPosition() const;
    const sead::Vector3f& getTouchPointerNormal() const;
    DrcTouchPointer* getTouchPointer() const;
    void calcTouchPointerUpDir(sead::Vector3f* pUp);
    bool tryCalcTouchPointerSlideDirOnScreen(sead::Vector2f* pDir);
    bool tryCalcTouchPointerSlideDirOnWorld(sead::Vector3f* pDir, const al::IUseCamera* pCamera);
    void deleteTouchPointerEffect();
    void startSnapshotMode();
    void endSnapshotMode();
    bool isPlayerHoldingStamp();
    void calcScreenAngle(s32 port, const sead::Vector3f* pAngle, sead::Vector3f* pOut) const;
    void unrotatePoseMtx(s32 port, f32 angle, sead::Matrix34f* pMtx) const;
    void reverseCalcScreenAngle(s32 port, const sead::Vector3f* pAngle,
                                sead::Vector3f* pOut) const;
    void getPadTypeFrontAxis(s32 port, sead::Vector3f* pAxis) const;

    s32 getPort() const { return mPort; }

    s32 getTouchPadPort() const { return mTouchPadPort; }

    rc::StampDirector* getStampDirector() const { return mStampDirector; }

    void setStampDirector(rc::StampDirector* pStampDirector) { mStampDirector = pStampDirector; }

    void setPlayerAliveWatcher(PlayerAliveWatcher* pWatcher) { mPlayerAliveWatcher = pWatcher; }

    void setPlayerHolder(al::PlayerHolder* pHolder) { mPlayerHolder = pHolder; }

    void setIgnoreTriggerFrames(s32 frames) { mIgnoreTriggerFrames = frames; }

    bool isPlayerExist() const { return mIsPlayerExist; }

private:
    bool isTriggerTouchPanel() const;
    bool isHoldTouchPanel() const;

    PlayerAliveWatcher* mPlayerAliveWatcher = nullptr;                  // 0x10
    al::PlayerHolder* mPlayerHolder = nullptr;                          // 0x18
    DrcTouchPointer* mTouchPointer = nullptr;                           // 0x20
    DrcTouchAssistInfo* mTouchAssistInfo = nullptr;                     // 0x28
    al::SceneCameraInfo* mSceneCameraInfo = nullptr;                    // 0x30
    al::SceneObjHolder* mSceneObjHolder = nullptr;                      // 0x38
    sead::Vector2f mTouchScreenPos = {0.0f, 0.0f};                      // 0x40
    sead::Vector2f mPrevTouchScreenPos = {0.0f, 0.0f};                  // 0x48
    sead::Vector2f mTouchIconPos = {-1000.0f, -1000.0f};                // 0x50
    sead::Vector2f mZeroPushAmount = {0.0f, 0.0f};                      // 0x58
    s32 mTouchFrame = 0;                                                // 0x60
    sead::Matrix34f mBaseMtx = sead::Matrix34f::ident;                  // 0x64
    sead::Vector3f mBaseAngle = {0.0f, 0.0f, 0.0f};                     // 0x94
    sead::Vector3f mDiffAngle = {0.0f, 0.0f, 0.0f};                     // 0xA0
    f32 mMinPoseDiff = 3.40282347e+38f;                                 // 0xAC
    bool mIsRequestCalib = false;                                       // 0xB0
    bool mIsPrevGyroOrHold = false;                                     // 0xB1
    bool mIsPoseChecked = false;                                        // 0xB2
    bool mIsFirstUpdate = true;                                         // 0xB3
    bool mIsCheckPose = false;                                          // 0xB4
    bool mIsValid = true;                                               // 0xB5
    bool mIsDisappearGyro;                                              // 0xB6
    bool mIsPrevGyroTouch;                                              // 0xB7
    bool mIsPlayerExist;                                                // 0xB8
    bool mIsLongRange;                                                  // 0xB9
    bool mIsMiddleRange = false;                                        // 0xBA
    bool mIsEnableUpdate = true;                                        // 0xBB
    bool mIsSnapshotMode = false;                                       // 0xBC
    TouchEffect* mTouchEffect;                                          // 0xC0
    s32 mPort;                                                          // 0xC8
    s32 mTouchPadPort;                                                  // 0xCC
    s32 mIgnoreTriggerFrames;                                           // 0xD0
    rc::StampDirector* mStampDirector;                                  // 0xD8
};

static_assert(sizeof(DrcAssistDirector) == 0xE0);
