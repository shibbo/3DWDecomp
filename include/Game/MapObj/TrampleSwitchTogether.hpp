#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class TrampleSwitchTogether : public al::LiveActor {
public:
    TrampleSwitchTogether(const char* pName);
    virtual ~TrampleSwitchTogether();
    virtual void init(const al::ActorInitInfo& rInfo);
    virtual void control();
    virtual bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf);

    void exeOffWait();
    void exeOn();
    void exeOnWait();
    void exeOff();
    void exeSuccess();
    bool isOnWait() const;
    void success();

    int mPressFrames = 0; // 0x144
};
