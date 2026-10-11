#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class KoopaLastStateAttackFire;
struct KoopaLastStateAttackFireParam;

class KoopaLastAttackFire : public al::LiveActor {
public:
    explicit KoopaLastAttackFire(const char* pName);
    ~KoopaLastAttackFire() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    void exeAttack();

private:
    KoopaLastStateAttackFireParam* mAttackParam = nullptr;
    KoopaLastStateAttackFire* mAttackState = nullptr;
};
