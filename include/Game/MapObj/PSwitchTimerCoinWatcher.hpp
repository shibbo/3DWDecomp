#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include <container/seadBuffer.h>
class TimerCoinHolder;
class TrampleSwitch;
class PSwitchTimerCoinWatcher : public al::LiveActor {
public:
    explicit PSwitchTimerCoinWatcher(const char*);
    ~PSwitchTimerCoinWatcher() override;
    void init(const al::ActorInitInfo&) override;
    void appear() override;
    void startClipped() override;
    void endClipped() override;
    void switchAppear();
    void exeWait();
    void exeSuccess();
    void exeWaitToReset();
    void exeWaitToInstantReset();
private:
    sead::Buffer<TimerCoinHolder*> mCoinHolders;
    TrampleSwitch* mSwitch = nullptr;
    int mResetDelay = 0;
    bool mHasViewGroup = false;
};
static_assert(sizeof(PSwitchTimerCoinWatcher) == 0x168);
