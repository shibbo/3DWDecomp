#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>

#include "Player/IUsePlayerActualMove.hpp"

struct PlayerProperty;

/// How far the player really moved in the last frame.
class PlayerActualMove : public IUsePlayerActualMove {
public:
    PlayerActualMove(const PlayerProperty* pProperty);
    void calc();
    void reset();

    const sead::Vector3f& getActualMove() const override;

private:
    u8 _8[0x30 - 0x8];
};
static_assert(sizeof(PlayerActualMove) == 0x30);
