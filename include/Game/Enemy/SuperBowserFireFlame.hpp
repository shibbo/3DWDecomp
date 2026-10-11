#pragma once

#include "Library/LiveActor/LiveActor.hpp"

/** @brief Fireball spat by Fury Bowser. */
class SuperBowserFireFlame : public al::LiveActor {
public:
    explicit SuperBowserFireFlame(const char* pName);

private:
    u8 _148[0x150 - 0x148];
};
static_assert(sizeof(SuperBowserFireFlame) == 0x150);
