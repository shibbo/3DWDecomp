#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class CoinCountUp : public al::LiveActor {
public:
    CoinCountUp(const char*);
    ~CoinCountUp() override;
    void init(const al::ActorInitInfo&) override;
    void appear() override;
    void kill() override;
    void appearQuick();
    void appearDelayQuick(int);
    void appearFront(const sead::Vector3f&);
    void exeDelayQuickStandby();
    void exeUp();
    void exeUpQuick();
    void exeUpFront();
private:
    int mDelayFrames = 0;
};
