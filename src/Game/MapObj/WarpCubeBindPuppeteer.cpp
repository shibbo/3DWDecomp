#include "MapObj/WarpCubeBindPuppeteer.hpp"
#include "MapObj/BindWarpEffect.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/PlayerPuppetUtil.hpp"
#include "Util/PlayerUtil.hpp"
namespace {
    NERVE_DECL(WarpCubeBindPuppeteer, InWait);
    NERVE_DECL(WarpCubeBindPuppeteer, BindForce);
    NERVES_MAKE_NOSTRUCT(WarpCubeBindPuppeteer, InWait, BindForce)
}
WarpCubeBindPuppeteer::WarpCubeBindPuppeteer(const char* pName, const al::ActorInitInfo& rInfo)
    : BindPuppeteer(pName) {
    mWarpEffect = new BindWarpEffect();
    al::initCreateActorNoPlacementInfo(mWarpEffect, rInfo);
    initNerve(&NrvWarpCubeBindPuppeteerInWait, 0);
}
void WarpCubeBindPuppeteer::startBindForce(al::HitSensor* pPlayer, al::HitSensor* pBinder,
                                          const sead::Vector3f& rDestination) {
    BindPuppeteer::startBind(pPlayer, pBinder);
    auto* puppet = getPlayerPuppet();
    rc::startPuppetAction(puppet, "Wait");
    rc::hidePuppet(puppet);
    rc::hidePuppetSilhouette(puppet);
    rc::invalidatePlayerEffect(al::getSensorHost(rc::getPuppetSensor(puppet)));
    mWarpEffect->start(pPlayer, rDestination + sead::Vector3f(0.0f, 100.0f, 0.0f), false);
    al::setNerve(this, &NrvWarpCubeBindPuppeteerBindForce);
}
void WarpCubeBindPuppeteer::startBindInStart(al::HitSensor* pPlayer, al::HitSensor* pBinder) {
    BindPuppeteer::startBind(pPlayer, pBinder);
    auto* puppet = getPlayerPuppet();
    rc::startPuppetAction(puppet, "Wait");
    rc::hidePuppet(puppet);
    rc::hidePuppetSilhouette(puppet);
    rc::invalidatePlayerEffect(al::getSensorHost(rc::getPuppetSensor(puppet)));
    al::setNerve(this, &NrvWarpCubeBindPuppeteerInWait);
}
bool WarpCubeBindPuppeteer::tryCancelBind(const al::SensorMsg* pMsg, al::HitSensor* pSensor) {
    if (!al::isMsgBindCancel(pMsg))
        return false;
    if (!isBind())
        return false;
    int userId = rc::findControlUserId(pSensor);
    if (getControlUserId() != userId)
        return false;
    cancelBind();
    if (mWarpEffect->isMoving())
        mWarpEffect->kill();
    return true;
}
void WarpCubeBindPuppeteer::endBind(const PlayerBindEndParam* pParam) {
    rc::validatePlayerEffect(al::getSensorHost(rc::getPuppetSensor(getPlayerPuppet())));
    BindPuppeteer::endBind(pParam);
}
void WarpCubeBindPuppeteer::endBindOnGround() {
    rc::validatePlayerEffect(al::getSensorHost(rc::getPuppetSensor(getPlayerPuppet())));
    BindPuppeteer::endBindOnGround();
}
void WarpCubeBindPuppeteer::endBindSquat() {
    rc::validatePlayerEffect(al::getSensorHost(rc::getPuppetSensor(getPlayerPuppet())));
    BindPuppeteer::endBindSquat();
}
void WarpCubeBindPuppeteer::exeBindForce() {
    if (mWarpEffect->isEnd())
        al::setNerve(this, &NrvWarpCubeBindPuppeteerInWait);
}
void WarpCubeBindPuppeteer::exeInWait() {}
bool WarpCubeBindPuppeteer::isEnableStart() const {
    return al::isNerve(this, &NrvWarpCubeBindPuppeteerInWait);
}
