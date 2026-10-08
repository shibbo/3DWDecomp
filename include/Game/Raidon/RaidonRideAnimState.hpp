#pragma once

#include "Library/Nerve/NerveStateBase.hpp"

class RaidonBase;

/// Animation state of Plessie while she is ridden (run, swim, jump, dash...).
class RaidonRideAnimState : public al::HostStateBase<RaidonBase> {
public:
    /// Bits of mMoveEffectFlags: which looping swim effects are currently emitted.
    enum MoveEffect : s32 {
        MoveEffect_Front = 1 << 0,
        MoveEffect_Back = 1 << 1,
        MoveEffect_Right = 1 << 2,
        MoveEffect_Left = 1 << 3,
        MoveEffect_Neutral = 1 << 4,
    };

    RaidonRideAnimState(const char* pName, RaidonBase* pHost);
    ~RaidonRideAnimState() override;
    void appear() override;
    void kill() override;
    void endMove();
    void control() override;
    void exeSwim();
    void updateSwimSound();
    void updateMoveEffect();
    void exeRun();
    void exeJumpStart();
    void exeJumpLoop();
    void exeFall();
    void exeLand();
    void exeBound();
    void exeHit();
    void exeDash();
    bool requestJump();
    void requestBound();
    void requestHit();
    void requestDash();
    void tryEmitMoveEffect(bool isEmit, const char* pName, s32 flag);

    /** @return Whether Plessie was landing when she reached the goal area. */
    bool isSwim() const { return mIsSwim; }

private:
    s32 mAirCount = 0;         // 0x20
    f32 mHandle = 0.0f;        // 0x24
    s32 mMoveEffectFlags = 0;  // 0x28
    bool mIsSwim = false;      // 0x2c
};

static_assert(sizeof(RaidonRideAnimState) == 0x30);
