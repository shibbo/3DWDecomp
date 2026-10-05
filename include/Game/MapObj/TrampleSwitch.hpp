#pragma once

#include "Library/LiveActor/LiveActor.hpp"

namespace al { class MtxConnector; }

class TrampleSwitch : public al::LiveActor {
public:
    TrampleSwitch(const char* pName);
    virtual ~TrampleSwitch() {}
    virtual void init(const al::ActorInitInfo& rInfo);
    virtual void initAfterPlacement();
    virtual void control();
    virtual bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSender, al::HitSensor* pReceiver);

    void offSwitch();
    void resetSwitch();
    void exeOffWait();
    void exeOn();
    void exeOnWait();
    void exeOff();
    bool isTrigSwitchOn();
    bool isEarlyTrigOn();

    al::MtxConnector* mConnector = nullptr; // 0x148
    bool mIsReusable = false;              // 0x150
    bool mIsHeldOn = false;                // 0x151
};
