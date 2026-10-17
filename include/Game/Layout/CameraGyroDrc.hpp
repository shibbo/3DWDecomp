#pragma once
#include "Library/Layout/LayoutActor.hpp"
namespace al { class LayoutInitInfo; }
class ButtonTouch;
/** @brief Touch button for switching the camera's gyro control. */
class CameraGyroDrc : public al::LayoutActor {
public:
    explicit CameraGyroDrc(const al::LayoutInitInfo& rInfo);
    void startAppear();
    void startEnd();
    void reset();
    void exeAppear();
    void exeWait();
    bool isTriggerDecide() const;
    void exeEnd();
    void setGyroOn();
    void setGyroOff();
private:
    bool mIsGyroOn = false;
    ButtonTouch* mButton = nullptr;
};
static_assert(sizeof(CameraGyroDrc) == 0x130);
