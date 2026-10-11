#pragma once

#include "Library/LiveActor/LiveActor.hpp"

namespace al { class FixMapParts; }

class WheelWatcher : public al::LiveActor {
public:
    WheelWatcher(const char* pName);
    virtual ~WheelWatcher();
    virtual void init(const al::ActorInitInfo& rInfo);

    void stop();
    void exeWait();

    al::FixMapParts** mWheels = nullptr; // 0x148
    int mWheelCount = 0; // 0x150
};
