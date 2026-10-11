#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/LiveActor/Common/LiveActorGroup.hpp"
class ChikaChikaBlock;
class ChikaChikaBlockWatcher : public al::LiveActor {
public:
    ChikaChikaBlockWatcher(const char*);
    ~ChikaChikaBlockWatcher() override;
    void init(const al::ActorInitInfo&) override;
    void exeWatch();
    void startSwitchOffSign();
    void startSwitch();
    void exeAllAppear();
    void exeDisAppear();
    bool isStopBgm();
    int getSwitchInterval() const { return mSwitchInterval; }
private:
    al::DeriveActorGroup<ChikaChikaBlock>* mBlocks = nullptr;
    int mSwitchInterval = 120;
    int mFrame = 0;
    bool mIsSyncBgm = false;
};
static_assert(sizeof(ChikaChikaBlockWatcher) == 0x160);
