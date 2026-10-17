#pragma once

#include <math/seadMatrix.h>
#include <math/seadQuat.h>
#include <math/seadVector.h>

#include "Raidon/RaidonBase.hpp"

namespace al {
class AudioGeneralPurposeAreaChecker;
class ComboCounter;
class HitSensor;
class ScreenPointer;
class ScreenPointTarget;
class SensorMsg;
}  // namespace al

class ActorStateSupportStroke;
class PlayerBindEndParam;
class RaidonEndState;
class RaidonGoalState;
class RaidonPuppeteer;
class RaidonRideAnimState;
class RaidonRideStartState;
class RaidonWaitState;

/// Plessie on land: up to four players ride her from the start to the goal position.
class Raidon : public RaidonBase {
public:
    explicit Raidon(const char* pName);

    void init(const al::ActorInitInfo& rInfo) override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;
    void control() override;
    void calcAnim() override;

    void setPuppetQT();
    void exeWait();
    void exeGetOn();
    void exeStart();
    void exeRide();
    void exeAbyss();
    void exeGoal();
    void exeGetOff();
    void exeEnd();
    void endBind(const PlayerBindEndParam* pParam);

    void updatePuppetInput() override;
    void updateHandleAndAccel() override;
    void updateGroundUpVec() override;
    void updateOnGround() override;
    void updateMatrialCode() override;
    void startPuppetActionAll(const char* pActionName) override;
    void setPuppetInputBlendAnimWeight() override;
    void setInputBlendAnimWeight() override;
    bool isAllGetOffPlayer() const override;
    void startPuppetSe(const char* pName) override;
    void updateStart() override;
    void updateRide() override;
    void clearGroundCount() override;

    /** @return Whether Plessie touched the ground within the last few frames. */
    bool isOnGroundRaidon() const override { return mGroundCount > 0; }

    /** @return Whether Plessie stands on a water or sand floor. */
    bool isInWater() const override { return mIsInWater; }

    /** @return Center of the screen blur used while dashing. */
    const sead::Vector3f& getDashBlurCenter() const override { return mDashBlurCenter; }

    /** @return Front direction Plessie runs to. */
    const sead::Vector3f& getBaseFrontDir() const override { return mBaseFrontDir; }

    /** @return Up direction of the ground below Plessie. */
    const sead::Vector3f& getGroundUpVec() const override { return mGroundUpVec; }

    /** @return Rotation Plessie was placed with. */
    const sead::Quatf& getBaseQuat() const override { return mBaseQuat; }

    /** @return Position Plessie stops at. */
    const sead::Vector3f& getGoalPosition() const override { return mGoalPosition; }

    /** @return Whether a goal position was linked in the placement. */
    bool isEnableGoalPosition() const override { return mIsEnableGoalPosition; }

    /** @return Averaged steering input of the riders. */
    f32 getHandle() const override { return mHandle; }

    /** @return Averaged acceleration input of the riders. */
    f32 getAccel() const override { return mAccel; }

    /** @return Current Y rotation offset in degrees. */
    f32 getRotateY() const override { return mRotateY; }

    /** @return Whether the stage BGM must be kept while riding. */
    bool isNotChangeBgm() const override { return mIsNotChangeBgm; }

private:
    RaidonWaitState* mWaitState = nullptr;                    // 0x170
    RaidonRideStartState* mRideStartState = nullptr;          // 0x178
    RaidonRideAnimState* mRideAnimState = nullptr;            // 0x180
    RaidonGoalState* mGoalState = nullptr;                    // 0x188
    RaidonEndState* mEndState = nullptr;                      // 0x190
    RaidonPuppeteer* mPuppeteers = nullptr;                   // 0x198
    ActorStateSupportStroke* mStateSupportStroke;             // 0x1a0
    s32 mPuppeteerNumMax = 0;                                 // 0x1a8
    s32 mPuppeteerNum = 0;                                    // 0x1ac
    al::HitSensor* mFirstPlayerSensor = nullptr;              // 0x1b0
    al::ComboCounter* mTrampleComboCounter;                   // 0x1b8
    al::ComboCounter* mInvincibleComboCounter;                // 0x1c0
    sead::Matrix34f mScreenWetMtx = sead::Matrix34f::ident;   // 0x1c8
    const char* mMaterialCodeName = nullptr;                  // 0x1f8
    sead::Quatf mBaseQuat = sead::Quatf::unit;                // 0x200
    sead::Vector3f mBaseTrans = sead::Vector3f::zero;         // 0x210
    sead::Vector3f mBaseFrontDir = sead::Vector3f::ez;        // 0x21c
    sead::Vector3f mBaseSideDir = sead::Vector3f::ex;         // 0x228
    sead::Vector3f mGroundUpVec = sead::Vector3f::ey;         // 0x234
    sead::Vector3f mDashBlurCenter = sead::Vector3f::zero;    // 0x240
    sead::Vector3f mGoalPosition = {0.0f, 0.0f, 60000.0f};    // 0x24c
    f32 mHandle = 0.0f;                                       // 0x258
    f32 mRotateY = 0.0f;                                      // 0x25c
    f32 mAccel = 0.0f;                                        // 0x260
    s32 mGroundCount = 3;                                     // 0x264
    s32 mDashTimer = 0;                                       // 0x268
    s32 mHitTimer = 0;                                        // 0x26c
    s32 mMultiJumpTimer = 0;                                  // 0x270
    bool mIsInWater = false;                                  // 0x274
    bool mIsEnableGoalPosition = false;                       // 0x275
    bool mIsNotChangeBgm = false;                             // 0x276
    al::AudioGeneralPurposeAreaChecker* mFallAreaChecker = nullptr;  // 0x278
    void* _280 = nullptr;
};

static_assert(sizeof(Raidon) == 0x288);
