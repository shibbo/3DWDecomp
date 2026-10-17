#pragma once

#include "Library/Nerve/NerveStateBase.hpp"

class BrosMoveStepKeeper;

/** @brief Attack state shared by the Bros enemies: sign, attack, wait and turn back. */
class BrosStateAttack : public al::ActorStateBase {
public:
    BrosStateAttack(al::LiveActor* pHost, const BrosMoveStepKeeper* pMoveStepKeeper);

    /** @brief Destroys the attack state. */
    ~BrosStateAttack() override = default;
    void appear() override;
    void exeSign();
    void exeAttack();
    void exeWait();
    void exeTurn();
    void exeEnd();
    bool isAttackStep() const;
    bool isSignStep() const;

private:
    struct Actions {
        const char* pSign;
        const char* pAttack;
        const char* pSecondSign;
        const char* pSecondAttack;
        const char* pWait;
        const char* pEnd;
        const char* pTurn;
    };
    const BrosMoveStepKeeper* mMoveStepKeeper;
    Actions* mActions;
    Actions* mTowerActions = nullptr;
    s32 mAttacksLeft = 2;
    f32 mTurnDegree = 0.0f;
};

static_assert(sizeof(BrosStateAttack) == 0x40);
