#include "MapObj/CloudBonusLauncherBindPuppeteer.hpp"
#include "MapObj/BindWarpEffect.hpp"
#include "MapObj/WarpObjUtil.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Math/ParabolicPath.hpp"
#include "Util/PlayerPuppetUtil.hpp"
namespace {
NERVE_DECL(CloudBonusLauncherBindPuppeteer, Nothing);
NERVE_DECL(CloudBonusLauncherBindPuppeteer, BindForce);
NERVE_DECL(CloudBonusLauncherBindPuppeteer, InWait);
NERVE_DECL(CloudBonusLauncherBindPuppeteer, Launch);
NERVE_DECL(CloudBonusLauncherBindPuppeteer, BonusStartWarp);
NERVES_MAKE_NOSTRUCT(CloudBonusLauncherBindPuppeteer, Nothing, BindForce, InWait)
NERVES_MAKE_STRUCT(CloudBonusLauncherBindPuppeteer, Launch, BonusStartWarp)
}
CloudBonusLauncherBindPuppeteer::CloudBonusLauncherBindPuppeteer(const char* name, const al::ActorInitInfo& info)
    : BindPuppeteer(name), mWarpEffect(new BindWarpEffect) {
    al::initCreateActorNoPlacementInfo(mWarpEffect, info);
    initNerve(&NrvCloudBonusLauncherBindPuppeteerNothing, 0);
    mPath = new al::ParabolicPath;
}
void CloudBonusLauncherBindPuppeteer::startBindForce(al::HitSensor* player, al::HitSensor* binder, const sead::Vector3f& end) {
    mBinderSensor = binder;
    startBind(player, binder);
    auto* puppet = getPlayerPuppet();
    rc::startPuppetAction(puppet, "Wait");
    rc::hidePuppet(puppet);
    rc::hidePuppetSilhouette(puppet);
    mWarpEffect->start(player, end, false);
    al::setNerve(this, &NrvCloudBonusLauncherBindPuppeteerBindForce);
}
void CloudBonusLauncherBindPuppeteer::startBindInStart(al::HitSensor* player, al::HitSensor* binder) {
    mBinderSensor = binder;
    startBind(player, binder);
    auto* puppet = getPlayerPuppet();
    rc::startPuppetAction(puppet, "Wait");
    rc::hidePuppet(puppet);
    rc::hidePuppetSilhouette(puppet);
    al::setNerve(this, &NrvCloudBonusLauncherBindPuppeteerInWait);
}
void CloudBonusLauncherBindPuppeteer::launchStart(float speed) {
    auto* puppet = getPlayerPuppet();
    auto* launcher = al::getSensorHost(mBinderSensor);
    rc::showPuppet(puppet);
    rc::startPuppetAction(puppet, "RouteDokanBazookaFly");
    sead::Vector3f front;
    al::calcQuatFront(&front, launcher);
    rc::setPuppetFrontVec(puppet, front);
    sead::Vector3f up;
    al::calcQuatUp(&up, launcher);
    rc::setPuppetUpVec(puppet, up);
    rc::setPuppetVelocity(puppet, sead::Vector3f::zero);
    mLaunchSpeed = speed;
    al::setNerve(this, &NrvCloudBonusLauncherBindPuppeteer.Launch);
}
void CloudBonusLauncherBindPuppeteer::launchEnd() { al::setNerve(this, &NrvCloudBonusLauncherBindPuppeteerNothing); }
void CloudBonusLauncherBindPuppeteer::startBonusStartWarp(const sead::Matrix34f& mtx, int index, int count) {
    auto* puppet = getPlayerPuppet();
    rc::setPuppetMtx(puppet, &mtx);
    rc::setPuppetUpVec(puppet, sead::Vector3f::ey);
    rc::setPuppetVelocity(puppet, sead::Vector3f(0.0f, 0.0f, 0.0f));
    const sead::Vector3f& trans = rc::getPuppetTrans(puppet);
    sead::Vector3f offset;
    WarpObjUtil::getJumpOutLocalTrans(&offset, index, count);
    offset.rotate(mtx);
    sead::Vector3f end = offset + trans - rc::getPuppetUpVec(puppet) * 180.0f;
    sead::Vector3f start = end - rc::getPuppetFrontVec(puppet) * 1500.0f - rc::getPuppetUpVec(puppet) * 1000.0f;
    rc::setPuppetTrans(puppet, start);
    mPath->initFromUpVector(start, end, rc::getPuppetUpVec(puppet), 1600.0f);
    al::setNerve(this, &NrvCloudBonusLauncherBindPuppeteer.BonusStartWarp);
}
void CloudBonusLauncherBindPuppeteer::startBonusEndBind(al::HitSensor* player, al::HitSensor* binder) {
    mBinderSensor = binder;
    startBind(player, binder);
    auto* puppet = getPlayerPuppet();
    rc::startPuppetAction(puppet, "Wait");
    rc::hidePuppet(puppet);
    rc::hidePuppetSilhouette(puppet);
}
bool CloudBonusLauncherBindPuppeteer::tryTemporaryWarp() {
    if (mTemporaryWarp) return false;
    auto* puppet = getPlayerPuppet();
    sead::Vector3f trans = rc::getPuppetTrans(puppet);
    trans.y += -3000.0f;
    rc::setPuppetTrans(puppet, trans);
    mTemporaryWarp = true;
    return true;
}
bool CloudBonusLauncherBindPuppeteer::isBonusStartWarp() const {
    return al::isNerve(this, &NrvCloudBonusLauncherBindPuppeteer.Launch) || al::isNerve(this, &NrvCloudBonusLauncherBindPuppeteer.BonusStartWarp);
}
void CloudBonusLauncherBindPuppeteer::exeNothing() {}
void CloudBonusLauncherBindPuppeteer::exeBindForce() {
    if (mWarpEffect->isEnd()) al::setNerve(this, &NrvCloudBonusLauncherBindPuppeteerInWait);
}
void CloudBonusLauncherBindPuppeteer::exeInWait() {
    if (al::isFirstStep(this)) {
        mInWait = true;
        auto* puppet = getPlayerPuppet();
        rc::setPuppetTrans(puppet, al::getActorTrans(mBinderSensor));
        al::setNerve(this, &NrvCloudBonusLauncherBindPuppeteerNothing);
    }
}
void CloudBonusLauncherBindPuppeteer::exeLaunch() {
    auto* puppet = getPlayerPuppet();
    const sead::Vector3f& trans = rc::getPuppetTrans(puppet);
    rc::setPuppetTrans(puppet, mLaunchSpeed * sead::Vector3f::ey + trans);
}
void CloudBonusLauncherBindPuppeteer::exeBonusStartWarp() {
    auto* puppet = getPlayerPuppet();
    if (al::isFirstStep(this)) rc::resetPuppetDynamics(puppet);
    sead::Vector3f trans;
    mPath->calcPosition(&trans, al::calcNerveRate(this, 95));
    rc::setPuppetTrans(puppet, trans);
    if (al::isStep(this, 94)) mPreviousTrans = rc::getPuppetTrans(puppet);
    else if (al::isGreaterEqualStep(this, 95)) {
        rc::setPuppetVelocity(puppet, rc::getPuppetTrans(puppet) - mPreviousTrans);
        al::setNerve(this, &NrvCloudBonusLauncherBindPuppeteerNothing);
    }
}
