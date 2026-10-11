#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include <math/seadMatrix.h>

class FallingPillar : public al::LiveActor {
public:
    explicit FallingPillar(const char* pName);
    ~FallingPillar() override;
    void init(const al::ActorInitInfo&) override;
    void appear() override;
    void kill() override;
    void createPillarParts(const al::ActorInitInfo&);
    void exeWait();
    void exeFallDelay();
    void exeFall();
    void exeLand();
    void restore();
    void fall();
    bool isFallen();
private:
    al::LiveActor* mBottom = nullptr;
    al::LiveActor* mTop = nullptr;
    sead::Matrix34f mLandingMtx = sead::Matrix34f::ident;
    sead::Vector3f mSplashPos = sead::Vector3f::zero;
    int mFallDelayFrames = 0;
    int mFallFrames = 175;
    float mFallExponent = 4.0f;
};
static_assert(sizeof(FallingPillar) == 0x1a0);
