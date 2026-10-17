#include "Layout/GhostMiiNameplate.hpp"
#include "Util/LayoutUtil.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/LiveActor/Util/ActorModelUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
namespace {
const sead::Vector3f sNameplateOffset(0.0f, 180.0f, 0.0f);
}
/** @brief Creates the name plate associated with a ghost actor.
 * @param pActor Ghost actor to follow.
 * @param rInfo Layout initialization context. */
GhostMiiNameplate::GhostMiiNameplate(al::LiveActor* pActor, const al::LayoutInitInfo& rInfo)
    : al::SimpleLayoutAppearWaitEnd("ネームプレート", "MiiNameplate", rInfo, nullptr, false),
      mHost(pActor) {}
/** @brief Sets the name and appearance flags.
 * @param pName Name displayed by the plate.
 * @param isFriend Whether to use the friend color.
 * @param isTimeAttack Whether to display the time indicator. */
void GhostMiiNameplate::init(const char16_t* pName, bool isFriend, bool isTimeAttack) {
    mName = pName;
    mIsFriend = isFriend;
    mIsTimeAttack = isTimeAttack;
    al::setPaneString(this, "TxtName", pName, 0, -1);
}
/** @brief Shows the plate with the configured time and friend animations. */
void GhostMiiNameplate::appear() {
    al::SimpleLayoutAppearWaitEnd::appear();
    al::startAction(this, mIsTimeAttack ? "TimeOn" : "TimeOff", "Time");
    al::startAction(this, mIsFriend ? "FriendColor" : "NormalColor", "Friend");
}
/** @brief Tracks the ghost and hides the root pane while it is behind the camera. */
void GhostMiiNameplate::control() {
    if (isHostBehindCamera()) {
        al::hidePaneRoot(this);
        return;
    }
    if (al::isHidePaneRoot(this)) {
        al::showPaneRoot(this);
        al::startAction(this, mIsTimeAttack ? "TimeOn" : "TimeOff", "Time");
        al::startAction(this, mIsFriend ? "FriendColor" : "NormalColor", "Friend");
    }
    rc::updateLayoutPosAtOwner(this, mHost, sNameplateOffset, 100.0f);
}
/** @brief Tests the ghost's position against the camera's look direction. */
bool GhostMiiNameplate::isHostBehindCamera() const {
    sead::Vector3f lookDir;
    al::calcCameraLookDir(&lookDir, mHost);
    return (al::getTrans(mHost) - al::getCameraPos(mHost)).dot(lookDir) < 0.0f;
}
/** @brief Updates visibility based on clipping and distance from the camera.
 * @param pNameplate Name plate to update.
 * @param isClipped Whether to hide the active plate immediately. */
void GhostMiiNameplateFunction::updateShowHideFromCameraDistance(GhostMiiNameplate* pNameplate,
                                                               bool isClipped) {
    if (al::isHideModel(pNameplate->getHost())) return;
    if (isClipped) {
        if (pNameplate->isAlive()) pNameplate->kill();
        return;
    }
    float distance = (al::getCameraPos(pNameplate->getHost()) - al::getTrans(pNameplate->getHost())).length();
    if (distance < 4400.0f) {
        if (!pNameplate->isAlive()) pNameplate->appear();
    } else if (distance > 4000.0f && pNameplate->isAlive() &&
               !al::isHidePaneRoot(pNameplate) && pNameplate->isWait()) {
        pNameplate->end();
    }
}
