#include "MapObj/GoalItemBindPuppeteer.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Player/Normal/PlayerActor.hpp"
#include "Player/Normal/PlayerProperty.hpp"
#include "Util/PlayerPuppetUtil.hpp"
#include "Util/PlayerUtil.hpp"

namespace {
    NERVE_DECL(GoalItemBindPuppeteer, Begin);
    NERVE_DECL(GoalItemBindPuppeteer, End);
    NERVE_DECL(GoalItemBindPuppeteer, Animate);
    NERVES_MAKE_NOSTRUCT(GoalItemBindPuppeteer, Begin, Animate, End)
}

GoalItemBindPuppeteer::GoalItemBindPuppeteer(const char* pName, GoalItem* pItem)
    : BindPuppeteer(pName), mGoalItem(pItem) {
    initNerve(&NrvGoalItemBindPuppeteerBegin, 0);
}

void GoalItemBindPuppeteer::setActionName(const char* pName) {
    mActionName = pName;
}

void GoalItemBindPuppeteer::startBind(al::HitSensor* pPlayerSensor, al::HitSensor* pBinderSensor) {
    BindPuppeteer::startBind(pPlayerSensor, pBinderSensor);
    rc::killAllDoubleMarioExceptWithScore(pPlayerSensor);
    rc::invalidatePuppetSensors(getPlayerPuppet());
    al::calcDirBetweenSensorsH(&mBindDirection, pPlayerSensor, pBinderSensor);
    if (al::isNearZero(mBindDirection))
        mBindDirection.set(sead::Vector3f::ez);
    al::setNerve(this, &NrvGoalItemBindPuppeteerAnimate);
    if (mActionName) {
        mPlayer = static_cast<PlayerActor*>(al::getSensorHost(pPlayerSensor));
        mPlayer->getProperty()->mVelocity = sead::Vector3f::zero;
        mHasAction = al::isExistAction(mPlayer);
    }
}

void GoalItemBindPuppeteer::endBind(const PlayerBindEndParam* pParam) {
    rc::validatePuppetSensors(getPlayerPuppet());
    BindPuppeteer::endBind(pParam);
    al::setNerve(this, &NrvGoalItemBindPuppeteerEnd);
}

bool GoalItemBindPuppeteer::isEndAnimate() {
    return rc::isPuppetActionEnd(getPlayerPuppet());
}

void GoalItemBindPuppeteer::update() {
    updateNerve();
}

void GoalItemBindPuppeteer::exeBegin() {}
void GoalItemBindPuppeteer::exeEnd() {}

void GoalItemBindPuppeteer::exeAnimate() {
    if (al::isFirstStep(this) && mActionName && mHasAction)
        rc::startPuppetAction(getPlayerPuppet(), mActionName);
    if (mPlayer)
        mPlayer->getProperty()->mVelocity = sead::Vector3f::zero;
}
