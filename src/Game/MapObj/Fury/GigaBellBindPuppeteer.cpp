#include "MapObj/Fury/GigaBellBindPuppeteer.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Player/Normal/PlayerActor.hpp"
#include "Player/Normal/PlayerProperty.hpp"
#include "Util/PlayerPuppetUtil.hpp"
#include "Util/PlayerUtil.hpp"

namespace {
    NERVE_DECL(GigaBellBindPuppeteer, Wait);
    NERVE_DECL(GigaBellBindPuppeteer, Animate);
    NERVES_MAKE_NOSTRUCT(GigaBellBindPuppeteer, Wait, Animate)
}

GigaBellBindPuppeteer::GigaBellBindPuppeteer(const char* pName) : BindPuppeteer(pName) {
    initNerve(&NrvGigaBellBindPuppeteerWait, 0);
}

void GigaBellBindPuppeteer::setActionName(const char* pName) {
    mActionName = pName;
}

void GigaBellBindPuppeteer::startBind(al::HitSensor* pPlayerSensor, al::HitSensor* pBinderSensor) {
    BindPuppeteer::startBind(pPlayerSensor, pBinderSensor);
    rc::killAllDoubleMarioExceptWithScore(pPlayerSensor);
    rc::invalidatePuppetSensors(getPlayerPuppet());
    al::setNerve(this, &NrvGigaBellBindPuppeteerAnimate);
    if (mActionName) {
        mPlayer = static_cast<PlayerActor*>(al::getSensorHost(pPlayerSensor));
        mPlayer->getProperty()->mVelocity = sead::Vector3f::zero;
        mHasAction = al::isExistAction(mPlayer);
    }
}

void GigaBellBindPuppeteer::endBind(const PlayerBindEndParam* pParam) {
    rc::validatePuppetSensors(getPlayerPuppet());
    BindPuppeteer::endBind(pParam);
    al::setNerve(this, &NrvGigaBellBindPuppeteerWait);
}

bool GigaBellBindPuppeteer::isEndAnimate() {
    return rc::isPuppetActionEnd(getPlayerPuppet());
}

void GigaBellBindPuppeteer::update() {
    updateNerve();
}

void GigaBellBindPuppeteer::exeWait() {}

void GigaBellBindPuppeteer::exeAnimate() {
    if (al::isFirstStep(this) && mHasAction)
        rc::startPuppetAction(getPlayerPuppet(), mActionName);
    if (mPlayer)
        mPlayer->getProperty()->mVelocity = sead::Vector3f::zero;
}
