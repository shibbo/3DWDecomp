#pragma once

#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"

class SuperBowserPlessieChaseFireFlameState;

/** @brief Fireball Fury Bowser spits in sweeps and rings during the Plessie chase. */
class SuperBowserFireFlamePlessieChase : public al::LiveActor {
public:
    SuperBowserFireFlamePlessieChase(const char* pName,
                                     SuperBowserPlessieChaseFireFlameState* pState);

    void startAttack(const sead::Vector3f& rPos, const sead::Vector3f& rDir,
                     const sead::Vector3f& rUp, f32 effectScale, f32 speed,
                     f32 disappearOffset, bool, f32, bool);

private:
    u8 _148[0x168 - 0x148];
};
static_assert(sizeof(SuperBowserFireFlamePlessieChase) == 0x168);
