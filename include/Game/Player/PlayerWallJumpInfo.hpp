#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>

#include "Player/IUsePlayerLandingObserver.hpp"
#include "Player/IUsePlayerWallJumpInfo.hpp"

/// Where the player wall jumped first and last since it left the ground.
class PlayerWallJumpInfo : public IUsePlayerWallJumpInfo, public IUsePlayerLandingObserver {
public:
    PlayerWallJumpInfo();
    void startWallJump(const sead::Vector3f& rPos, const sead::Vector3f& rNormal);

    bool isWallJumping() const override;
    bool isInAirAfterWallJump() const override;
    const sead::Vector3f& getLastWallJumpPos() const override;
    const sead::Vector3f& getLastWallNormal() const override;
    const sead::Vector3f& getFirstWallJumpPos() const override;
    const sead::Vector3f& getFirstWallJumpNormal() const override;
    void notifyLanding() override;

private:
    u8 _10[0x48 - 0x10];
};
static_assert(sizeof(PlayerWallJumpInfo) == 0x48);
