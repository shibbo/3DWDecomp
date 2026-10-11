#pragma once

#include "Library/Nerve/NerveExecutor.hpp"

class LanternSwingMovement : public al::NerveExecutor {
public:
    LanternSwingMovement();
    void exeMove();
    void exeEnd();
    void updateSwing();
    void startSwing();
    void setParam(int duration, float amplitude, int period);
    float getCurrentRotate();
    bool isEnd() const;

private:
    int mDuration = 300;
    float mAmplitude = 80.0f;
    int mPeriod = 80;
    int mPhase = 0;
    float mRotation = 0.0f;
};
