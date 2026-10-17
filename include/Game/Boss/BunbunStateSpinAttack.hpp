#pragma once

#include <basis/seadTypes.h>

#include "Library/Nerve/NerveStateBase.hpp"

namespace al {
class ActorInitInfo;
class LiveActor;
}  // namespace al

class BunbunStateSpinAttack : public al::ActorStateBase {
public:
    BunbunStateSpinAttack(al::LiveActor* pHost, const al::ActorInitInfo& rInfo);

    bool isValidAttackSensor() const;
    bool isEnableAttack() const;
    void setRumbleScale(f32 scale);
    void endAttack();
    void requestReactionAnim();
    void setInvalidateClippingFlag();
    void setKouraPos();
    void appearKoura();

private:
    // The state is allocated as 0xb8 bytes by Bunbun::init; its fields remain unreconstructed.
    unsigned char mStateData[0x98];
};
