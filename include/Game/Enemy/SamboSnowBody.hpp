#pragma once

#include <basis/seadTypes.h>

#include "Library/LiveActor/LiveActor.hpp"

/** @brief One stacked snow ball of a snow Pokey, linked to the head that controls it. */
class SamboSnowBody : public al::LiveActor {
public:
    explicit SamboSnowBody(const char* pName);

    bool isAttacked();
    void requestBlowDown();
    void requestSupportFreeze(const al::LiveActor* pActor);
    void requestEndSupportFreeze();

    u8 _144[0xc];
    bool* mHeadIsStacked;                    // 0x150
    bool* mHeadIsRequestAttack;              // 0x158
    bool* mHeadIsRequestBlowDown;            // 0x160
    al::LiveActor** mHeadSupportFreezeActor;  // 0x168

private:
    u8 mUnreconstructed[0x18];
};
static_assert(sizeof(SamboSnowBody) == 0x188);
