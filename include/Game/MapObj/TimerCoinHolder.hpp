#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/LiveActor/Common/LiveActorGroup.hpp"
class TimerCoin;
class TimerCoinHolder : public al::LiveActor {
public:
    TimerCoinHolder(const char*);
    ~TimerCoinHolder() override;
    void init(const al::ActorInitInfo&) override;
    void appear() override;
    void exeCountDown();
    void forceReset();
    bool isComplete() const { return mIsComplete; }
private:
    al::DeriveActorGroup<TimerCoin>* mCoins;
    int mCoinCount = 0;
    int mCollectedCount = 0;
    int mTimerFrame = 600;
    bool mIsComplete = false;
    bool mIsSingleMode = false;
};
