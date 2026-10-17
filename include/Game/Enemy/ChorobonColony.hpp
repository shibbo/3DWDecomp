#pragma once
#include "Library/LiveActor/LiveActor.hpp"
namespace al { class KeyPoseKeeper; }

class ChorobonColony : public al::LiveActor {
public:
    explicit ChorobonColony(const char* pName);
    void init(const al::ActorInitInfo& rInfo) override;
    void makeActorAppeared() override;
    void killBySwitch();
    void start();
    void exeMove();
    void exeDelay();
    void exeWait();
    void exeStop();
private:
    al::KeyPoseKeeper* mKeyPoseKeeper = nullptr;
    s32 mMoveTime = 0;
    s32 mDelayTime = 0;
    s32 mWaitTime = 0;
};
static_assert(sizeof(ChorobonColony) == 0x160);
