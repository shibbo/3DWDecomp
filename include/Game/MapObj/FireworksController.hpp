#pragma once
#include "Library/LiveActor/LiveActor.hpp"

class FireworksEffectObj;

class FireworksController : public al::LiveActor {
public:
    FireworksController(const char* pName);
    ~FireworksController() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void kill() override;
    void exeWait();
    void exeLoop();

private:
    int mFrame = 0;
    int mCount = 0;
    FireworksEffectObj** mEffects = nullptr;
    int* mLaunchFrames = nullptr;
    int mLoopFrames = 300;
    int mStartDelay = 0;
};
