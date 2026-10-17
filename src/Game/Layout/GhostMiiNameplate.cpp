#include "Layout/GhostMiiNameplate.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorModelUtil.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Util/LayoutUtil.hpp"
#include <math/seadVector.hpp>
namespace {
const sead::Vector3f sNameplateOffset(0.0f, 180.0f, 0.0f);
}
GhostMiiNameplate::GhostMiiNameplate(al::LiveActor* host, const al::LayoutInitInfo& info)
    : al::SimpleLayoutAppearWaitEnd("ネームプレート", "MiiNameplate", info, nullptr, false), mHost(host) {}
void GhostMiiNameplate::init(const char16_t* name, bool isFriend, bool isShadow) {
    mUserName = name;
    mIsFriend = isFriend;
    mIsShadow = isShadow;
    al::setPaneString(this, "TxtName", name, 0, -1);
}
void GhostMiiNameplate::appear() {
    al::SimpleLayoutAppearWaitEnd::appear();
    al::startAction(this, mIsShadow ? "TimeOn" : "TimeOff", "Time");
    al::startAction(this, mIsFriend ? "FriendColor" : "NormalColor", "Friend");
}
void GhostMiiNameplate::control() {
    if (isHostBehindCamera()) { al::hidePaneRoot(this); return; }
    if (al::isHidePaneRoot(this)) {
        al::showPaneRoot(this);
        al::startAction(this, mIsShadow ? "TimeOn" : "TimeOff", "Time");
        al::startAction(this, mIsFriend ? "FriendColor" : "NormalColor", "Friend");
    }
    rc::updateLayoutPosAtOwner(this, mHost, sNameplateOffset, 100.0f);
}
bool GhostMiiNameplate::isHostBehindCamera() const {
    sead::Vector3f look;
    al::calcCameraLookDir(&look, mHost);
    return (al::getTrans(mHost) - al::getCameraPos(mHost)).dot(look) < 0.0f;
}
namespace GhostMiiNameplateFunction {
void updateShowHideFromCameraDistance(GhostMiiNameplate* nameplate, bool forceHide) {
    if (al::isHideModel(nameplate->getHost())) return;
    if (forceHide) {
        if (nameplate->isAlive()) nameplate->kill();
        return;
    }
    float distance = (al::getCameraPos(nameplate->getHost()) - al::getTrans(nameplate->getHost())).length();
    if (distance < 4400.0f) {
        if (!nameplate->isAlive()) nameplate->appear();
    } else if (distance > 4000.0f && nameplate->isAlive()
               && !al::isHidePaneRoot(nameplate) && nameplate->isWait()) nameplate->end();
}
}