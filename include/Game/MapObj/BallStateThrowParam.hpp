#pragma once

class BallStateThrowParam {
public:
    BallStateThrowParam();
    BallStateThrowParam(float, float, float, float, float, float, float, float, int, bool, bool);

    float mLaunchSpeed;
    float mLaunchSpeedUp;
    float mThrowGravity;
    float mGravity;
    float mWaterGravity;
    float mVelocityScale;
    float mWaterVelocityScale;
    float mAirRotationSpeed;
    int mThrowFrames;
    bool mRotateOnAir;
    bool mIsAutoThrow;
};

static_assert(sizeof(BallStateThrowParam) == 0x28);
