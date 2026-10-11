#pragma once

#include "Library/Nerve/NerveStateBase.hpp"
#include <math/seadVector.h>

namespace al {
class ActorInitInfo;
template <class T> class DeriveActorGroup;
}
class TargetFinder;
class KoopaFireBall;

struct KoopaLastStateAttackFireParam {
    int mAttackInterval = 0;
    int mValue04 = -1;
    int mValue08;
};

class KoopaLastStateAttackFire : public al::ActorStateBase {
public:
    KoopaLastStateAttackFire(al::LiveActor* pActor, const al::ActorInitInfo& rInfo,
                            const KoopaLastStateAttackFireParam* pParam);

private:
    const KoopaLastStateAttackFireParam* mParam;
    TargetFinder* mTargetFinder;
    al::DeriveActorGroup<KoopaFireBall>* mFireBalls;
    sead::Vector3f mFrontDir;
    sead::Vector3f mUpDir;
    int mValue50;
};
