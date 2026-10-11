#pragma once

#include "Library/LiveActor/LiveActor.hpp"
#include "Library/LiveActor/Common/LiveActorGroup.hpp"

class TrampleSwitchTogether;

class TrampleSwitchTogetherWatcher : public al::LiveActor {
public:
    TrampleSwitchTogetherWatcher(const char* pName);
    virtual ~TrampleSwitchTogetherWatcher();
    virtual void init(const al::ActorInitInfo& rInfo);

    void exeWatch();
    void exeEnd();

    al::DeriveActorGroup<TrampleSwitchTogether>* mSwitches = nullptr; // 0x148
};
