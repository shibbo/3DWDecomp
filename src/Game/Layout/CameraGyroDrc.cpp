#include "Layout/CameraGyroDrc.hpp"
#include "Layout/ButtonTouch.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
namespace {
NERVE_DECL(CameraGyroDrc, Appear);
NERVE_DECL(CameraGyroDrc, End);
NERVE_DECL(CameraGyroDrc, Wait);
NERVES_MAKE_NOSTRUCT(CameraGyroDrc, Appear, End, Wait)
}
/** @brief Creates a hidden gyro switch with gyro control initially off.
 * @param rInfo Layout initialization context. */
CameraGyroDrc::CameraGyroDrc(const al::LayoutInitInfo& rInfo)
    : al::LayoutActor("カメラジャイロ切替レイアウト") {
    al::initLayoutActor(this, rInfo, "CameraGyroDRC", nullptr);
    mButton = new ButtonTouch("カメラ切り替えボタン", this, nullptr);
    initNerve(&NrvCameraGyroDrcAppear, 0);
    al::startAction(this, "Off", "OnOff");
    kill();
}
/** @brief Starts showing the switch if it is inactive. */
void CameraGyroDrc::startAppear() {
    if (isAlive()) return;
    al::startAction(this, "Hide", nullptr);
    al::setNerve(this, &NrvCameraGyroDrcAppear);
    appear();
}
/** @brief Starts the exit animation if the switch is active. */
void CameraGyroDrc::startEnd() {
    if (!isAlive() || al::isNerve(this, &NrvCameraGyroDrcEnd)) return;
    al::setNerve(this, &NrvCameraGyroDrcEnd);
}
/** @brief Restores the off state. */
void CameraGyroDrc::reset() { setGyroOff(); }
/** @brief Starts the entrance animation and advances to input handling. */
void CameraGyroDrc::exeAppear() {
    if (al::isFirstStep(this))
        al::startAction(this, "Appear", "AppearEnd");
    al::isActionEnd(this, nullptr);
    al::setNerve(this, &NrvCameraGyroDrcWait);
}
/** @brief Updates touch input and toggles gyro state when the button is decided. */
void CameraGyroDrc::exeWait() {
    if (al::isFirstStep(this)) mButton->reset();
    mButton->update();
    if (mButton->isDecideEnd()) mButton->reset();
    if (isTriggerDecide()) {
        mIsGyroOn = !mIsGyroOn;
        if (mIsGyroOn) al::startSe(this, "GyroOn", nullptr);
        else al::startSe(this, "GyroOff", nullptr);
        if (mIsGyroOn) al::startAction(this, "On", "OnOff");
        else al::startAction(this, "Off", "OnOff");
    }
}
/** @brief Checks whether the touch button was just decided. */
bool CameraGyroDrc::isTriggerDecide() const { return mButton->isTrigerDecide(); }
/** @brief Plays the exit animation and hides the switch when it ends. */
void CameraGyroDrc::exeEnd() {
    if (al::isFirstStep(this)) al::startAction(this, "End", "AppearEnd");
    if (al::isActionEnd(this, "AppearEnd")) kill();
}
/** @brief Enables gyro control and selects the on animation. */
void CameraGyroDrc::setGyroOn() {
    mIsGyroOn = true;
    al::startAction(this, "On", "OnOff");
}
/** @brief Disables gyro control and selects the off animation. */
void CameraGyroDrc::setGyroOff() {
    mIsGyroOn = false;
    al::startAction(this, "Off", "OnOff");
}
