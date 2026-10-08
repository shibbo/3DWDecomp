#pragma once

#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"
#include "MapObj/IGoalItemListener.hpp"
#include "MapObj/IUseTimer.hpp"

namespace al {
class AreaObj;
class CollisionObj;
class CameraTicket;
class MtxConnector;
}  // namespace al

class GoalItem;
class SingleModeSceneLayout;

/**
 * @brief A floor switch that starts a timer challenge when ground pounded.
 *
 * Optionally shows a camera on the challenge goal (a fixed camera, a fixed actor camera or a
 * camera area), plays a challenge BGM and spawns a goal item once the challenge is completed.
 */
class TrampleSwitchTimer : public al::LiveActor, public rc::IUseTimer, public IGoalItemListener {
public:
    TrampleSwitchTimer(const char* pName);

    void init(const al::ActorInitInfo& rInfo) override;
    void offSwitch();
    void resetSwitch();
    void killBySwitch();
    void stopBySwitch();
    void allEnemiesDead();
    void goalComplete();
    void initAfterPlacement() override;
    void control() override;
    void makeActorAppeared() override;
    void goalItemAnimIsDone() override;
    void resetSwitchCancel(bool isReset);
    void forceCancel() override;
    void reset() override;
    bool canCancel() const override;
    void exeOffWait();
    void exeOn();
    void exeWaitCameraIn();
    void exeWaitCameraArea();
    void exeWaitCameraOut();
    void endCameraAtReturnAngles(f32 step);
    void exeWaitGameplayCamera();
    void exeOnWait();
    void stop(bool isForceReset);
    void exeOff();
    void exeKill();
    bool isTrigSwitchOn();
    bool isEarlyTrigOn();
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;

private:
    bool isScenarioComplete() const;
    bool isPlayingBgm() const;
    void pauseFieldBgm();
    void hideTimer();
    void releaseTimer();
    void setScenarioCompleteAnim();
    bool isCameraReturnDone() const;
    void freezeCameraInput(bool isFreeze);

    al::CollisionObj* mCollisionObj = nullptr;           // 0x160
    al::MtxConnector* mConnector = nullptr;              // 0x168
    bool mIsReusable = false;                            // 0x170
    bool mIsHeldOn = false;                              // 0x171
    s32 mTimeFrameCount = -1;                            // 0x174
    s32 mTimer = 0;                                      // 0x178
    s32 mFocusCameraInStep = 0;                          // 0x17c
    s32 mFocusCameraOutStep = 0;                         // 0x180
    s32 mFocusCameraHoldStep = 0;                        // 0x184
    s32 mFreezeBeforeStart = 0;                          // 0x188
    bool mIsPressed;                                     // 0x18c
    bool mIsPressedNext;                                 // 0x18d
    bool mIsScenarioComplete = false;                    // 0x18e
    al::CameraTicket* mCameraTicket = nullptr;           // 0x190
    SingleModeSceneLayout* mSceneLayout = nullptr;       // 0x198
    bool mIsShowTimer = false;                           // 0x1a0
    bool mIsUseCamera = false;                           // 0x1a1
    const char* mBgmName = nullptr;                      // 0x1a8
    al::AreaObj* mCameraArea = nullptr;                  // 0x1b0
    s32 mAreaCameraHoldStep = 120;                       // 0x1b8
    bool mIsAllEnemiesDead = false;                      // 0x1bc
    bool mHasReturnAngles = false;                       // 0x1bd
    sead::Vector3f mStoredCameraAt = sead::Vector3f::ez;    // 0x1c0
    sead::Vector3f mStoredCameraPos = sead::Vector3f::zero; // 0x1cc
    f32 mCamAngleH = 0.0f;                               // 0x1d8
    f32 mCamAngleV = 0.0f;                               // 0x1dc
    f32 mCamReturnDist = 0.0f;                           // 0x1e0
    s32 mIslandId = -1;                                  // 0x1e4
    s32 mScenarioId = -1;                                // 0x1e8
    bool mIsSingleMode = false;                          // 0x1ec
    GoalItem* mGoalItem = nullptr;                       // 0x1f0
    bool mIsMysteryBox = false;                          // 0x1f8
    bool mIsFixedCamera = false;                         // 0x1f9
    bool mIsFixedActorCamera = false;                    // 0x1fa
    bool mIsPreserveCamera = false;                      // 0x1fb
    bool mIsNoIslandScenario = false;                    // 0x1fc
    bool mIsNoResetOnSwitchOff = false;                  // 0x1fd
    sead::Vector3f mFixedLookAt = sead::Vector3f::ez;    // 0x200
    al::HitSensor* mTrampleSensor = nullptr;             // 0x210
};

static_assert(sizeof(TrampleSwitchTimer) == 0x218);
