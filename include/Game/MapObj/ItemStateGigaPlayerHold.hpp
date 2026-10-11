#pragma once

#include <math/seadMatrix.h>

#include "MapObj/ItemStatePlayerHold.hpp"

/// Player hold state for items carried by a giga (mega) player.
class ItemStateGigaPlayerHold : public ItemStatePlayerHold {
public:
    ItemStateGigaPlayerHold(al::LiveActor* pActor, const ItemStatePlayerHoldParam* pParam,
                            bool, bool);
    void exeHold();
    void calcPlayerHoldMtx(sead::Matrix34f* pMtx);
};

static_assert(sizeof(ItemStateGigaPlayerHold) == 0x58);
