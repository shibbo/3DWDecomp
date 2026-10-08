#pragma once

#include <basis/seadTypes.h>

#include "Library/LiveActor/LiveActor.hpp"

class BossBunretsu;

/** @brief One body chunk of the splitting boss (BossBunretsu). */
class BossBunretsuChip : public al::LiveActor {
public:
    BossBunretsuChip(BossBunretsu* pParent, s32 index, const char* pName);

    void startBreakup();
    void startGather();
    void endGather();
    void onDeath();
    void onDamage();

private:
    u8 _144[0x180 - 0x144];
};
static_assert(sizeof(BossBunretsuChip) == 0x180);
