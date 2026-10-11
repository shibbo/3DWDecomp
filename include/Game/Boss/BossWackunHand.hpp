#pragma once

#include <basis/seadTypes.h>

#include "Library/LiveActor/LiveActor.hpp"

class BossWackun;

/** @brief The hand that pushes BossWackun's body (layout not reconstructed yet). */
class BossWackunHand : public al::LiveActor {
public:
    explicit BossWackunHand(BossWackun* pBoss);

    void startWait();
    void startMove(s32 targetIndex);
    bool isEndMove() const;
    void startControled();

private:
    u8 _144[0x24];
};
static_assert(sizeof(BossWackunHand) == 0x168);
