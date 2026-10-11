#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class KoopaLastFireBreath : public al::LiveActor {
public:
    explicit KoopaLastFireBreath(const char* pName);
    ~KoopaLastFireBreath() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    void appearAttack(const sead::Vector3f& rStart, const sead::Vector3f& rTarget, float speed);
    void exeMove();
    void exeCollided();
};
