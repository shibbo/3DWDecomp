#pragma once

#include "Library/Nerve/NerveStateBase.hpp"

namespace al {
class LiveActor;
}

/** @brief Body attack (jump at the player) state of the climbing Goombas. */
class KuriboClimbStateBodyAttack : public al::ActorStateBase {
public:
    KuriboClimbStateBodyAttack(al::LiveActor* pHost);

    void appear() override;
    void kill() override;
    bool isWait() const;
    void exeWait();
    void exeAttackJumpStart();
    void exeAttackJump();
    void exeAttackStart();
    void exeAttackWait();
    void exeAttack();
    void exeAttackEnd();

    /** @brief Requests the attack to resume after the host was frozen in the middle of it. */
    void requestResume() { mIsRequestResume = true; }

private:
    bool mIsRequestResume;
    u8 _21[7];
};

static_assert(sizeof(KuriboClimbStateBodyAttack) == 0x28);
