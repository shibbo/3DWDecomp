#pragma once

class BallStateFallParam {
public:
    BallStateFallParam();
    BallStateFallParam(float, float, float, float, float, float,
                       float, float, float, float, float, float);

    float _00;
    float _04;
    float _08;
    float mReboundRate;
    float mWaterReboundRate;
    float mReboundMinSpeed;
    float mAirRotationSpeed;
    float mGravity;
    float mWaterGravity;
    float mVelocityScale;
    float mWaterVelocityScale;
    float mReboundFriction;
};

static_assert(sizeof(BallStateFallParam) == 0x30);
