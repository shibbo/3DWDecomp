#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class BunbunStateSpinAttack;

class BunbunArm : public al::LiveActor {
public:
    BunbunArm(const char* pName, BunbunStateSpinAttack* pSpinAttack);
    ~BunbunArm() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool isHitCylinder(al::HitSensor* pSelf, al::HitSensor* pOther) const;

private:
    BunbunStateSpinAttack* mSpinAttack;
};
