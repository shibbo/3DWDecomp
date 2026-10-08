#pragma once

#include "Library/Nerve/NerveStateBase.hpp"

namespace al {
class ActorInitInfo;
}

class KoopaLastStateAttackBreathFire : public al::ActorStateBase {
public:
    KoopaLastStateAttackBreathFire(al::LiveActor* pActor, const al::ActorInitInfo& rInfo);
    void killAllFire();

private:
    void* mUnk20;
    void* mUnk28;
};

static_assert(sizeof(KoopaLastStateAttackBreathFire) == 0x30);
