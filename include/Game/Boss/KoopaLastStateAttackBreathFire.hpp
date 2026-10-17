#pragma once

#include "Library/Nerve/NerveStateBase.hpp"

namespace al {
class ActorInitInfo;
template<class T> class DeriveActorGroup;
}

class KoopaFireBall;

class KoopaLastStateAttackBreathFire : public al::ActorStateBase {
public:
    KoopaLastStateAttackBreathFire(al::LiveActor* pActor, const al::ActorInitInfo& rInfo);
    ~KoopaLastStateAttackBreathFire() override;
    void appear() override;
    void killAllFire();
    void exeClimbEnd();
    void exeWait();
    void exePrepare();
    void exeAttack();
    void exeEnd();
    void exeRestartWait();


private:
    al::DeriveActorGroup<KoopaFireBall>* mFireBalls = nullptr;
    int mStartWait = 0;
};

static_assert(sizeof(KoopaLastStateAttackBreathFire) == 0x30);
