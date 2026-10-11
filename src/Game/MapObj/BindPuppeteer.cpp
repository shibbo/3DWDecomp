#include "MapObj/BindPuppeteer.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/PlayerPuppetUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"

BindPuppeteer::BindPuppeteer(const char* pName) : al::NerveExecutor(pName) {}

void BindPuppeteer::startBind(al::HitSensor* pPlayerSensor, al::HitSensor* pBinderSensor) {
    mPlayerPuppet = rc::startPuppet(pBinderSensor, pPlayerSensor);
    mControlUserId = rc::findControlUserId(pPlayerSensor);
}

void BindPuppeteer::endBind(const PlayerBindEndParam* pParam) {
    rc::endBindAndPuppetNull(&mPlayerPuppet, pParam);
}

void BindPuppeteer::endBindOnGround() {
    rc::endBindOnGroundAndPuppetNull(&mPlayerPuppet);
}

void BindPuppeteer::endBindSquat() {
    rc::endBindSquatAndPuppetNull(&mPlayerPuppet);
}

void BindPuppeteer::endBindForceAbyss() {
    rc::endBindForceAbyssAndPuppetNull(&mPlayerPuppet);
}

void BindPuppeteer::cancelBind() {
    mPlayerPuppet = nullptr;
}

void BindPuppeteer::setNullPlayerPuppet() {
    mPlayerPuppet = nullptr;
}

al::LiveActor* BindPuppeteer::getTargetActor() {
    return al::getSensorHost(rc::getPuppetSensor(mPlayerPuppet));
}

s32 BindPuppeteer::getControlUserId() const {
    return mControlUserId;
}

IUsePlayerPuppet* BindPuppeteer::getPlayerPuppet() const {
    return mPlayerPuppet;
}
