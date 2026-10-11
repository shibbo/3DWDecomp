#pragma once

#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"
#include "MapObj/IUseTimer.hpp"

namespace al {
class AreaObj;
class CameraTicket;
}  // namespace al

class ActorRailBrakeMover;
class DemoSkipLayout;
class DummyCameraTarget;
class SingleModeSceneLayout;

/**
 * @brief A ring gate that Plessie swims through to start a race against the timer.
 *
 * Passing the gate starts the timer and optionally shows a camera on the race goal (either a
 * fixed focus camera, a camera area or a rail camera flying along the course).
 */
class TimerGate : public al::LiveActor, public rc::IUseTimer {
public:
    TimerGate(const char* pName);

    void init(const al::ActorInitInfo& rInfo) override;
    void goalComplete();
    void resetSwitch();
    void stop();
    void initAfterPlacement() override;
    void setGateActions();
    void finishCameraMove();
    void reset() override;
    bool canCancel() const override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    void makeActorAppeared() override;
    void makeActorDead() override;
    void forceCancel() override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;

    void exeWait();
    void exeSpin();
    void endSpin();
    void exeOn();
    void cancel();
    void exeOnWait();
    void exeOff();
    void exeWaitCameraIn();
    void exeWaitCameraArea();
    void exeWaitCameraAreaToIn();
    void exeWaitCameraInHold();
    void exeWaitCameraReturn();
    void exeWaitCameraOut();
    void exeWaitCameraFadeOut();
    void exeRailMove();
    void exeRailFadeOut();
    void exeRailMoveDone();

private:
    bool isScenarioComplete() const;

    al::CameraTicket* mCameraTicket = nullptr;          // 0x158
    void* _160 = nullptr;                               // 0x160
    SingleModeSceneLayout* mSceneLayout = nullptr;      // 0x168
    bool mIsShowTimer = false;                          // 0x170
    s32 mTimeFrameCount = -1;                           // 0x174
    s32 mTimer;                                         // 0x178
    bool mIsUseCamera = false;                          // 0x17c
    s32 mFocusCameraInStep = 0;                         // 0x180
    s32 mFocusCameraOutStep;                            // 0x184
    s32 mFocusCameraHoldStep;                           // 0x188
    al::AreaObj* mCameraArea = nullptr;                 // 0x190
    s32 mAreaCameraHoldStep = 0;                        // 0x198
    s32 mAreaCameraOutStep = 0;                         // 0x19c
    bool mIsSetReturnAngles = false;                    // 0x1a0
    f32 mReturnAngleH = 0.0f;                           // 0x1a4
    f32 mReturnAngleV = 20.0f;                          // 0x1a8
    bool mIsSingleMode = false;                         // 0x1ac
    bool mIsSpinEnd = false;                            // 0x1ad
    s32 mIslandId = -1;                                 // 0x1b0
    s32 mScenarioId = -1;                               // 0x1b4
    ActorRailBrakeMover* mRailMover = nullptr;          // 0x1b8
    DummyCameraTarget* mDummyTarget = nullptr;          // 0x1c0
    sead::Vector3f mReturnStartPos;                     // 0x1c8
    sead::Vector3f mReturnStartAt;                      // 0x1d4
    sead::Vector3f mReturnPos;                          // 0x1e0
    sead::Vector3f mReturnAt;                           // 0x1ec
    f32 mReturnRate;                                    // 0x1f8
    s32 mReturnEndWait;                                 // 0x1fc
    al::CameraTicket* mRailCameraTicket = nullptr;      // 0x200
    sead::Vector3f mRailCameraPos = sead::Vector3f::zero;  // 0x208
    sead::Vector3f mRailCameraAt = sead::Vector3f::zero;   // 0x214
    sead::Vector3f mRailFront = {0.0f, 0.0f, 1.0f};     // 0x220
    sead::Vector3f mRailCameraUp = {0.0f, 1.0f, 0.0f};  // 0x22c
    s32 mRailCameraDuration = 200;                      // 0x238
    f32 mRailCameraDistance = 500.0f;                   // 0x23c
    s32 mRailCameraInStep = 0;                          // 0x240
    s32 mRailCameraOutStep = 0;                         // 0x244
    f32 mRailAngleH = 0.0f;                             // 0x248
    f32 mRailAngleV = 0.0f;                             // 0x24c
    s32 mGoalItemDelay = 0;                             // 0x250
    DemoSkipLayout* mSkipLayout = nullptr;              // 0x258
    bool _260 = false;                                  // 0x260
    bool mIsSkipped = false;                            // 0x261
};

static_assert(sizeof(TimerGate) == 0x268);
