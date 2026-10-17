#pragma once

#include "Project/Camera/Core/IUseCameraDirector.hpp"
#include <math/seadVector.h>

class DrcTouchChecker : public al::IUseCamera {
public:
    explicit DrcTouchChecker(al::SceneCameraInfo* pCameraInfo);
    void update();
    bool isTouchSphere(const sead::Vector3f& rCenter, float radius, float* pDistance) const;
    al::SceneCameraInfo* getSceneCameraInfo() const override;

private:
    bool mIsTouching = false;
    sead::Vector3f mTouchDirection = sead::Vector3f::ez;
    al::SceneCameraInfo* mCameraInfo;
};

static_assert(sizeof(DrcTouchChecker) == 0x20);
