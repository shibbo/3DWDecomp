#pragma once

#include <math/seadVector.h>

#include "Library/Layout/LayoutActor.hpp"

namespace al {
class LayoutInitInfo;
}

class TimerClockNumber : public al::LayoutActor {
public:
    TimerClockNumber(const al::LayoutInitInfo& rInfo, s32 time);

    void appearWithWorldPos(const sead::Vector3f& rWorldPos);
    void updatePosition();
    void exeAppear();
    al::SceneCameraInfo* getSceneCameraInfo() const override;

private:
    sead::Vector3f mWorldPos = sead::Vector3f::zero;
    al::SceneCameraInfo* mSceneCameraInfo;
};
