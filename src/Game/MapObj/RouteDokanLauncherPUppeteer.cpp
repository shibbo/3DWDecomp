#include "MapObj/RouteDokanLauncherPuppeteer.hpp"
#include "MapObj/BindWarpEffect.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorFlagUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Util/PlayerPuppetUtil.hpp"

namespace {
    NERVE_DECL(RouteDokanLauncherPuppeteer, InWait);
    NERVE_DECL(RouteDokanLauncherPuppeteer, BindForce);
    NERVES_MAKE_NOSTRUCT(RouteDokanLauncherPuppeteer, InWait, BindForce)
}

RouteDokanLauncherPuppeteer::RouteDokanLauncherPuppeteer(const char* pName,
                                                       const al::ActorInitInfo& rInfo)
    : BindPuppeteer(pName) {
    mWarpEffect = new BindWarpEffect;
    al::initCreateActorNoPlacementInfo(mWarpEffect, rInfo);
    initNerve(&NrvRouteDokanLauncherPuppeteerInWait, 0);
}

void RouteDokanLauncherPuppeteer::startBindForce(al::HitSensor* pPlayer, al::HitSensor* pBinder,
                                                const sead::Quatf& rQuat,
                                                const sead::Vector3f& rTrans) {
    startBind(pPlayer, pBinder);
    IUsePlayerPuppet* puppet = getPlayerPuppet();
    mWarpEffect->start(rc::getPuppetSensor(puppet), rTrans, false);
    rc::setPuppetTrans(puppet, rTrans);
    rc::setPuppetQuat(puppet, rQuat);
    rc::startPuppetAction(puppet, "RouteDokanLaunchWait");
    rc::hidePuppet(puppet);
    rc::hidePuppetSilhouette(puppet);
    al::setNerve(this, &NrvRouteDokanLauncherPuppeteerBindForce);
}

void RouteDokanLauncherPuppeteer::startBindInStart(al::HitSensor* pPlayer, al::HitSensor* pBinder,
                                                  const sead::Quatf& rQuat,
                                                  const sead::Vector3f& rTrans) {
    startBind(pPlayer, pBinder);
    IUsePlayerPuppet* puppet = getPlayerPuppet();
    rc::setPuppetTrans(puppet, rTrans);
    rc::setPuppetQuat(puppet, rQuat);
    rc::startPuppetAction(puppet, "RouteDokanLaunchWait");
    rc::hidePuppet(puppet);
    rc::hidePuppetSilhouette(puppet);
    al::setNerve(this, &NrvRouteDokanLauncherPuppeteerInWait);
}

bool RouteDokanLauncherPuppeteer::isEnableStart() const {
    return isBind() && al::isNerve(this, &NrvRouteDokanLauncherPuppeteerInWait);
}

void RouteDokanLauncherPuppeteer::exeBindForce() {
    if (al::isDead(mWarpEffect))
        al::setNerve(this, &NrvRouteDokanLauncherPuppeteerInWait);
}

void RouteDokanLauncherPuppeteer::exeInWait() {}
