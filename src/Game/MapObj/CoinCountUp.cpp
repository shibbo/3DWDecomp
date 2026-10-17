#include "MapObj/CoinCountUp.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
namespace {
    NERVE_DECL(CoinCountUp, Up);
    NERVE_DECL(CoinCountUp, UpQuick);
    NERVE_DECL(CoinCountUp, DelayQuickStandby);
    NERVE_DECL(CoinCountUp, UpFront);
    NERVES_MAKE_NOSTRUCT(CoinCountUp, Up, UpQuick, DelayQuickStandby, UpFront)
}
CoinCountUp::CoinCountUp(const char* pName) : al::LiveActor(pName) {}
CoinCountUp::~CoinCountUp() {}
void CoinCountUp::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "Coin", "CountUp");
    al::initNerve(this, &NrvCoinCountUpUp, 0);
    makeActorDead();
}
void CoinCountUp::appear() {
    al::LiveActor::appear();
    al::setRotate(this, sead::Vector3f::zero);
    al::setNerve(this, &NrvCoinCountUpUp);
}
void CoinCountUp::kill() {
    al::startHitReactionDisappear(this);
    al::LiveActor::kill();
}
void CoinCountUp::appearQuick() {
    al::LiveActor::appear();
    al::setRotate(this, sead::Vector3f::zero);
    al::setNerve(this, &NrvCoinCountUpUpQuick);
}
void CoinCountUp::appearDelayQuick(int delay) {
    al::LiveActor::appear();
    if (delay == 0) {
        al::setRotate(this, sead::Vector3f::zero);
        al::setNerve(this, &NrvCoinCountUpUpQuick);
    } else {
        mDelayFrames = delay;
        if (!al::isHideModel(this))
            al::hideModel(this);
        al::setNerve(this, &NrvCoinCountUpDelayQuickStandby);
    }
}
void CoinCountUp::appearFront(const sead::Vector3f& rFront) {
    al::LiveActor::appear();
    sead::Vector3f velocity = rFront * 16.0f;
    velocity.y += 16.0f;
    al::setVelocity(this, velocity);
    al::setNerve(this, &NrvCoinCountUpUpFront);
}
void CoinCountUp::exeDelayQuickStandby() {
    if (al::isGreaterEqualStep(this, mDelayFrames)) {
        al::showModel(this);
        al::setNerve(this, &NrvCoinCountUpUpQuick);
    }
}
void CoinCountUp::exeUp() {
    if (al::isFirstStep(this)) {
        al::setVelocity(this, sead::Vector3f(0.0f, 16.0f, 0.0f));
        al::startHitReactionAppear(this);
    }
    al::getVelocityPtr(this)->y += -0.6f;
    al::addRotateAndRepeatY(this, 9.5f);
    if (al::isStep(this, 32))
        kill();
}
void CoinCountUp::exeUpQuick() {
    if (al::isFirstStep(this)) {
        al::setVelocity(this, sead::Vector3f(0.0f, 23.0f, 0.0f));
        al::startHitReactionAppear(this);
    }
    al::getVelocityPtr(this)->y += -1.2f;
    al::addRotateAndRepeatY(this, 9.5f);
    if (al::isStep(this, 21))
        kill();
}
void CoinCountUp::exeUpFront() {
    al::getVelocityPtr(this)->y += -0.6f;
    al::addRotateAndRepeatY(this, 9.5f);
    if (al::isStep(this, 32))
        kill();
}
