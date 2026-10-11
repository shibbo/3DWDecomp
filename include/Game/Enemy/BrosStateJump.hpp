#pragma once

#include "Library/Nerve/NerveStateBase.hpp"

namespace al {
class KeyPoseKeeper;
}

class BrosMoveStepKeeper;

/** @brief Jump state shared by the Bros enemies: jumps from one key pose to the next. */
class BrosStateJump : public al::ActorStateBase {
public:
    BrosStateJump(al::LiveActor* pHost, al::KeyPoseKeeper* pKeyPoseKeeper,
                  const BrosMoveStepKeeper* pMoveStepKeeper);

    void appear() override;
    void exeStart();
    void exeLoop();
    void exeLand();
    bool isLand() const;

private:
    unsigned char _20[0x28];
};

static_assert(sizeof(BrosStateJump) == 0x48);
