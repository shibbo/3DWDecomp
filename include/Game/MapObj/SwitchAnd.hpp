#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class SwitchAnd : public al::LiveActor {
public:
    SwitchAnd(const char* pName);
    virtual ~SwitchAnd();
    virtual void init(const al::ActorInitInfo& rInfo);

    void notifyInputSwitchOn();

    int mRemainingSwitches = 0;       // 0x144
    bool mAllowInstantSwitch = false; // 0x148
};
