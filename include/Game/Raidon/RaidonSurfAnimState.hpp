#pragma once

#include <math/seadVector.h>

#include "Library/Nerve/NerveStateBase.hpp"

namespace al {
class IUseNerve;
class Nerve;
}  // namespace al

class RaidonSurf;

/// Drives Plessie's swim/run/jump/dive animations, effects and sounds while she is ridden.
class RaidonSurfAnimState : public al::HostStateBase<RaidonSurf> {
public:
    /// Bits of mMoveEffectFlags: which looping swim effects are currently emitted.
    enum MoveEffect : s32 {
        MoveEffect_Front = 1 << 0,
        MoveEffect_Right = 1 << 2,
        MoveEffect_Left = 1 << 3,
        MoveEffect_Neutral = 1 << 4,
        MoveEffect_Trail = 1 << 5,
        MoveEffect_Ripple = 1 << 6,
    };

    RaidonSurfAnimState(const char* pName, RaidonSurf* pHost);
    ~RaidonSurfAnimState() override;
    void appear() override;
    void endMove();
    void kill() override;
    void control() override;
    bool isDive() const;
    void playSplash();
    void exeSwim();
    void updateSwimSound();
    void updateMoveEffect();
    void changeToNerve(al::IUseNerve* pUser, const al::Nerve* pNerve);
    void exeRun();
    void exeSlide();
    void exeJumpStart();
    void exeJumpLoop();
    void exeFall();
    void exeLand();
    void playJumpSound();
    void exeBound();
    void exeHit();
    void exeDash();
    void exeDiveStart();
    void exeDiveLoop();
    void exeSurfaceStart();
    void exeSurfaceLoop();
    void exeDiveJump();
    void exeSurfaceEnd();
    void exeDamage();
    void exePlessieChaseBellHit();
    bool requestJump();
    void requestBound();
    void requestHit();
    void requestDash();
    bool isHit() const;
    void requestFall();
    void requestIdle();
    void requestDive();
    void requestDamage();
    void requestPlessieChaseBellHit();
    void forceDiveEnd();
    bool isDiveJump() const;
    bool isUnderwater() const;
    void tryEmitMoveEffect(bool isEmit, const char* pName, s32 flag);
    bool isJumping();

    /** @return Whether a jump can be requested. */
    bool isEnableJump() const { return _2f; }

    /** @return Whether a jump was requested and not started yet. */
    bool isRequestJump() const { return mIsRequestJump; }

    /** @return Whether Plessie waits to dive after surfacing. */
    bool isWaitDoDive() const { return mIsWaitDoDive; }

private:
    s32 mAirCount = 0;               // 0x20
    f32 mHandle = 0.0f;              // 0x24
    s32 mMoveEffectFlags = 0;        // 0x28
    bool mIsLanding = false;         // 0x2c
    bool mIsEnableDive = true;       // 0x2d
    bool mIsOnSlideGround = false;   // 0x2e
    bool _2f = true;                 // 0x2f
    bool mIsRequestDive = false;     // 0x30
    bool mIsRequestJump = false;     // 0x31
    bool mIsRequestDiveJump = false; // 0x32
    bool _33 = false;
    sead::Vector3f mSplashPos = sead::Vector3f::zero;  // 0x34
    bool mIsInWaterPrev = false;     // 0x40
    bool mIsWaitDoDive = false;       // 0x41
    bool mIsWaitEnterWaterSe = false;  // 0x42
};
