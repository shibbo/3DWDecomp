#include "MapObj/LanternSwingMovement.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include <cmath>
#include <math/seadMathCalcCommon.h>

namespace {
    NERVE_DECL(LanternSwingMovement, Move);
    NERVE_DECL(LanternSwingMovement, End);
    NERVES_MAKE_NOSTRUCT(LanternSwingMovement, Move, End)
}

LanternSwingMovement::LanternSwingMovement() : al::NerveExecutor("減衰スイング挙動") {
    initNerve(&NrvLanternSwingMovementMove, 0);
}

void LanternSwingMovement::exeMove() {
    if (al::isGreaterEqualStep(this, mDuration)) {
        al::setNerve(this, &NrvLanternSwingMovementEnd);
        return;
    }
    updateSwing();
}

void LanternSwingMovement::updateSwing() {
    float angle = 360.0f * mPhase / mPeriod;
    mRotation = std::sin(sead::Mathf::deg2rad(angle)) * mAmplitude;
    mPhase = al::modi(mPhase + mPeriod + 1, mPeriod);
}

void LanternSwingMovement::exeEnd() {}

void LanternSwingMovement::startSwing() {
    mPhase = 0;
    al::setNerve(this, &NrvLanternSwingMovementMove);
}

void LanternSwingMovement::setParam(int duration, float amplitude, int period) {
    mDuration = duration;
    mAmplitude = amplitude;
    mPeriod = period;
}

float LanternSwingMovement::getCurrentRotate() {
    return mRotation * al::calcNerveEaseOutValue(this, mDuration, 1.0f, 0.0f);
}

bool LanternSwingMovement::isEnd() const {
    return al::isNerve(this, &NrvLanternSwingMovementEnd);
}
