#pragma once

#include "Library/LiveActor/LiveActor.hpp"

namespace al { class MtxConnector; }

class SwitchRotateSwitch : public al::LiveActor {
public:
    SwitchRotateSwitch(const char* pName);
    virtual ~SwitchRotateSwitch();
    virtual void init(const al::ActorInitInfo& rInfo);
    virtual void initAfterPlacement();
    virtual void control();
    virtual bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf);

    bool isOnSwitch() const;
    void requestOffSwitch(int frames);
    void exeOffWait();
    void exeOn();
    void exeOnWait();
    void exeOff();

    al::MtxConnector* mMtxConnector = nullptr; // 0x148
    int mOffDelay = -1; // 0x150
};
