#pragma once

#include "Library/Nerve/NerveStateBase.hpp"
#include <math/seadVector.h>

class BallStateRollingParam {
public:
    BallStateRollingParam();
    BallStateRollingParam(float gravity, float velocityScale, float stopSpeed, int stopFrames);

    float mGravity;
    float mVelocityScale;
    float mStopSpeed;
    int mStopFrames;
};

class BallStateRolling : public al::ActorStateBase {
public:
    BallStateRolling(al::LiveActor* pActor, const BallStateRollingParam* pParam);
    void appear() override;
    void exeRolling();
    bool isOnAir();

private:
    const BallStateRollingParam* mParam;
    sead::Vector3f mPreviousPos{0.0f, 0.0f, 0.0f};
    int mSlowFrames = 0;
    int mAirFrames = 0;
};

static_assert(sizeof(BallStateRolling) == 0x40);
