#pragma once

#include "Library/Nerve/NerveStateBase.hpp"

class BrosMoveStepKeeper;

/** @brief Attack state shared by the Bros enemies: sign, attack, wait and turn back. */
class BrosStateAttack : public al::ActorStateBase {
public:
    BrosStateAttack(al::LiveActor* pHost, const BrosMoveStepKeeper* pMoveStepKeeper);

    void appear() override;
    void exeSign();
    void exeAttack();
    void exeWait();
    void exeTurn();
    void exeEnd();
    bool isAttackStep() const;
    bool isSignStep() const;

private:
    unsigned char _20[0x20];
};

static_assert(sizeof(BrosStateAttack) == 0x40);
