#pragma once

#include "Library/LiveActor/LiveActor.hpp"

namespace al {
class LiveActorGroup;
class MtxConnector;
class FlashingTimer;
}

class TimeLimitStepSwitch : public al::LiveActor {
public:
    TimeLimitStepSwitch(const char* pName);
    virtual ~TimeLimitStepSwitch();
    virtual void init(const al::ActorInitInfo& rInfo);
    virtual void initAfterPlacement();
    virtual void control();
    virtual bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf);

    void showModelStep();
    void hideModelStep();
    void exeOffWait();
    void exeOn();
    void exeOnWait();
    void exeOff();

    al::LiveActorGroup* mSteps = nullptr; // 0x148
    al::MtxConnector* mMtxConnector = nullptr; // 0x150
    int mAppearTime = 600; // 0x158
    al::FlashingTimer* mTimer = nullptr; // 0x160
};
