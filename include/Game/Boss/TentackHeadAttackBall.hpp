#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class TentackHeadAttackBall : public al::LiveActor {
public:
    explicit TentackHeadAttackBall(const char* pName);
    ~TentackHeadAttackBall() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void kill() override;
    void control() override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSelf,
                    al::HitSensor* pOther) override;
    void shot(const al::LiveActor* pSource, const sead::Vector3f& rTarget);

private:
    int mLifeFrame = 0;
};
