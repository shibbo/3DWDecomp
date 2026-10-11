#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class TimerStageSwitch : public al::LiveActor {
public:
    TimerStageSwitch(const char* pName);
    virtual ~TimerStageSwitch();
    virtual void init(const al::ActorInitInfo& rInfo);
    virtual void control();

    void start();

    int mFrame = 0;  // 0x144
    int mTimer = 0;  // 0x148
};
