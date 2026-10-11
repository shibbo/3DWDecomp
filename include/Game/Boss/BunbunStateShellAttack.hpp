#pragma once

#include "Library/Nerve/NerveStateBase.hpp"

namespace al {
class ActorInitInfo;
class HitSensor;
class LiveActor;
class SensorMsg;
}  // namespace al

/** @brief Bunbun's shell attack: it hides in its shell and spins towards the player. */
class BunbunStateShellAttack : public al::ActorStateBase {
public:
    BunbunStateShellAttack(al::LiveActor* pHost, const al::ActorInitInfo& rInfo);

    bool isEnableAttack() const;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf);

private:
    // Allocated as 0x58 bytes by Bunbun::init; its fields remain unreconstructed.
    unsigned char mStateData[0x38];
};

static_assert(sizeof(BunbunStateShellAttack) == 0x58);
