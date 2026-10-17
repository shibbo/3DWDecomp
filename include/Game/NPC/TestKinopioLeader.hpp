#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class GreenStar;
class TestKinopioLeader : public al::LiveActor {
public:
    explicit TestKinopioLeader(const char* pName);
    ~TestKinopioLeader() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMessage, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    void exeAppear();
    void exeStandBy();
    void exeTakeOut();
    void exeWait();
    void exeMove();
    void exeRouteDokan();

private:
    int mState;
    float mMoveSpeed;
    GreenStar* mGreenStar;
};
