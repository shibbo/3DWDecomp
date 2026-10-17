#pragma once

#include <basis/seadTypes.h>
#include <container/seadPtrArray.h>

#include "Library/LiveActor/LiveActor.hpp"

namespace al {
class ComboCounter;
class PadRumbleKeeper;
}  // namespace al
class HoldColliderControl;

/** @brief Trampoline left behind by a Skipper; players bounce on it and can carry it around. */
class SkipperTrampoline : public al::LiveActor {
public:
    explicit SkipperTrampoline(const char* pName);

    void init(const al::ActorInitInfo& rInfo) override;
    void initColliderControl();
    void makeActorAppeared() override;
    void kill() override;
    void tryJumpPlayer();
    void control() override;
    bool isNerveCarry() const;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    bool isEnd() const;
    void setRelease();
    bool isInvalidNerve() const;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;

    void exeWait();
    void updateVelocity();
    void exeWaitCarry();
    void setHoldPos();
    void exeSink();
    void exeReaction();
    void exeReactionFall();
    void exeReactionCarry();
    void exeFall();
    void exeLand();
    void exeRecoverSign();
    void exeRecoverStart();
    void exeRecoverSignCarry();
    void exeRecoverStartCarry();
    void exeRecover();
    void exeEnd();
    void exeReactionHit();
    void updateCollider() override;

private:
    typedef sead::FixedPtrArray<al::HitSensor, 16> SensorArray;

    SensorArray mBindSensors;                             // 0x148
    al::HitSensor* mHolderSensor = nullptr;               // 0x1d8
    f32 mJumpSpeed = 40.0f;                               // 0x1e0
    f32 mJumpSpeedHigh = 50.0f;                           // 0x1e4
    s32 mStableStep = 0;                                  // 0x1e8
    al::ComboCounter* mComboCounter;                      // 0x1f0
    HoldColliderControl* mHoldColliderControl;            // 0x1f8
    f32 mColliderRadius = 50.0f;                          // 0x200
    SensorArray mSinkSensors;                             // 0x208
    al::PadRumbleKeeper* mPadRumbleKeeper = nullptr;      // 0x298
};

static_assert(sizeof(SkipperTrampoline) == 0x2a0);
