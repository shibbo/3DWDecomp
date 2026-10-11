#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>

#include "Player/IUsePlayerHeightChecker.hpp"

class IUsePlayerCollisionCheckArrow;
class PlayerConstParam;

/// Measures the player's height above the ground with a collision arrow.
class PlayerHeightChecker : public IUsePlayerHeightChecker {
public:
    PlayerHeightChecker(const PlayerConstParam* pConstParam);
    void update(const sead::Vector3f& rTrans, const sead::Vector3f& rUp);
    bool isAboveGround() const override;
    f32 getHeight() const override;

    void setCollisionCheckArrow(IUsePlayerCollisionCheckArrow* pArrow) { mCheckArrow = pArrow; }

private:
    const PlayerConstParam* mConstParam;  // 0x8
    IUsePlayerCollisionCheckArrow* mCheckArrow;  // 0x10
    u8 _18[0x20 - 0x18];
};
static_assert(sizeof(PlayerHeightChecker) == 0x20);
