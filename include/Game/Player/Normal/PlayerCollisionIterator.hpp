#pragma once

#include <basis/seadTypes.h>

#include "Player/IUsePlayerCollision.hpp"

/// Walks over the sides (floor, walls, ceiling) a player collision can touch.
class PlayerCollisionIterator {
public:
    PlayerCollisionIterator(const IUsePlayerCollision* pCollision);

    bool isOn() const;
    void getInfo(IUsePlayerCollision::Info* pInfo) const;
    PlayerCollisionIterator& operator++();
    bool isEnd() const;

private:
    const IUsePlayerCollision* mCollision;  // 0x0
    s32 mIndex;                             // 0x8
};
