#include "MapObj/DrcTouchChecker.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Screen/ScreenFunction.hpp"

DrcTouchChecker::DrcTouchChecker(al::SceneCameraInfo* pCameraInfo) : mCameraInfo(pCameraInfo) {}

void DrcTouchChecker::update() {
    int port = rc::calcTouchPanelPortByPortNum(al::getMainControllerPort());
    if (al::isPadHoldTouch(port)) {
        mIsTouching = true;
        sead::Vector2f screenPos = sead::Vector2f::zero;
        al::calcTouchScreenPos(&screenPos, port);
        sead::Vector3f worldPos;
        al::calcWorldPosFromScreenPos(&worldPos, mCameraInfo, screenPos,
                                     al::getCameraLookAtSub(this), 1);
        sead::Vector3f direction = worldPos - al::getCameraPosSub(this);
        al::normalizeOrDirZ(&direction);
        mTouchDirection = direction;
    } else {
        mIsTouching = false;
    }
}

bool DrcTouchChecker::isTouchSphere(const sead::Vector3f& rCenter, float radius,
                                  float* pDistance) const {
    if (!mIsTouching)
        return false;
    const sead::Vector3f& cameraPos = al::getCameraPosSub(this);
    if (!al::checkHitHalfLineSphere(rCenter, cameraPos, mTouchDirection, radius))
        return false;
    *pDistance = (cameraPos - rCenter).length();
    return true;
}

al::SceneCameraInfo* DrcTouchChecker::getSceneCameraInfo() const {
    return mCameraInfo;
}
