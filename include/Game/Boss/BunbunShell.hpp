#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class BunbunShell : public al::LiveActor {
public:
    explicit BunbunShell(const char* pName);
    ~BunbunShell() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void appear() override;
    void control() override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) override;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer*, al::ScreenPointTarget*) override;
    void reflectSensor(al::HitSensor* pOther, al::HitSensor* pSelf);
    void exeAppear();
    void exeSpin();
private:
    int mSpinStep = 0;
    int mTrampleCooldown = 0;
};
static_assert(sizeof(BunbunShell) == 0x150);
