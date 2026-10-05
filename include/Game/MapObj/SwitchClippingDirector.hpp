#pragma once

#include "Library/LiveActor/LiveActor.hpp"
#include <container/seadPtrArray.h>

class SwitchClippingDirector : public al::LiveActor {
public:
    SwitchClippingDirector(const char* pName);
    virtual ~SwitchClippingDirector();
    virtual void init(const al::ActorInitInfo& rInfo);
    virtual void appear();
    virtual void kill();

    struct Target {
        al::LiveActor* actor;
        bool wasDead;
    };
    sead::PtrArray<Target> mTargets;  // 0x148
};
