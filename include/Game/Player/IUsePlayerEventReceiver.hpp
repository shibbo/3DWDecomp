#pragma once

#include <basis/seadTypes.h>

/// Notifications about the player's state changes (implemented by PlayerActor; the base
/// handlers do nothing).
class IUsePlayerEventReceiver {
public:
    virtual void onAbyss();
    virtual void onDying();
    virtual void onVanishDying();
    virtual void onForceDying();
    virtual void onDamage();
    virtual void onRevive();
    virtual void onWarpStart();
    virtual void onWarpEnd();
    virtual void onInvincibleStart();
    virtual void onInvincibleBgmEnd(u32);
    virtual void onInvincibleEnd();
    virtual void onInvincibleRestart();
    virtual void onInvincibleGetStar();
    virtual void onInvincibleCancel(bool);
    virtual void onGiantStart();
    virtual void onGiantEnd();
    virtual void onGiantRestart();
    virtual void onGiantCancel();
    virtual void onGigaStart();
    virtual void onGigaStartByBell();
    virtual void onGigaEnd();
    virtual void onGigaRestart();
    virtual void onGigaCancel();
    virtual void onLanding();
    virtual void onWallJump();
    virtual void onWallFall();
    virtual void onBodyAttackStart();
    virtual void onBodyAttackLanding();
    virtual void onBodyAttackStickWall();
    virtual void onHipDropStart();
    virtual void onHipDropLand();
    virtual void onHipDropLandLoop();
    virtual void onClimbAttackStart();
    virtual void onClimbAttackEnd();
    virtual void onSuperDash();
    virtual void onSuperDashLand();
    virtual void onSetDashTime(s32);
    virtual void onSetModifiedDashTime(s32);
    virtual void onSetFlingPoleDashTime(s32);
    virtual void onChangeAction();
    virtual void onRaccoonDogFallStart();
    virtual void onPunchHit();
    virtual void onWallClimbReady();
    virtual void onWallHit();
    virtual void onSpinAttackStart();
    virtual void onSpinAttackEnd();
    virtual void onKickGroundInWater();
    virtual void onKnockDown();
    virtual void onManekinekoDropStart();
    virtual void onManekinekoDropFallStart();
    virtual void onManekinekoDropLand();
    virtual void onManekinekoDropEndNotice();
    virtual void onManekinekoDropEnd();
    virtual void onHoldedEnd();
    virtual void onSwimDiveLand();
    virtual void onForceKill();
    virtual void requestFlingPoleFlagClear();
    virtual void requestDashFlagClear();
    virtual void onCancelJumpAudio();
};
