#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class Bottle : public al::LiveActor {
public:
    explicit Bottle(const char* pName);
    ~Bottle() override;
    void init(const al::ActorInitInfo& rInfo) override;
    al::LiveActor* getInnerActor() const;
    al::LiveActor* getCapActor() const;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMessage, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    void setBreak();
    void exeWaitGoalPole();
    void exeBreak();
};
