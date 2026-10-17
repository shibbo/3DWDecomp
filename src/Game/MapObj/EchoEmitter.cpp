#include "MapObj/EchoEmitter.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"

namespace {
    NERVE_DECL(EchoEmitter, Wait);
    NERVE_DECL(EchoEmitter, Keep);
    NERVE_DECL(EchoEmitter, Stop);
    NERVES_MAKE_NOSTRUCT(EchoEmitter, Wait, Keep, Stop)
}

EchoEmitter::EchoEmitter(const char* pName) : al::LiveActor(pName) {}
EchoEmitter::~EchoEmitter() {}

void EchoEmitter::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "EchoModel", nullptr);
    al::initNerve(this, &NrvEchoEmitterWait, 0);
    makeActorDead();
}

void EchoEmitter::start(const sead::Vector3f& rPosition, float radius, int life) {
    mUser = nullptr;
    al::resetPosition(this, rPosition, false);
    al::setNerve(this, &NrvEchoEmitterWait);
    mLife = life;
    mStartDistance = 0.0f;
    mEndDistance = 0.0f;
    mStartRadius = radius * 0.5f;
    mEndRadius = radius * 2.0f;
    mDistance = 0.0f;
    mRadius = radius * 0.5f;
    mIntensity = 1.0f;
    appear();
}

void EchoEmitter::startKeep(const sead::Vector3f& rPosition, float radius, int life) {
    mUser = nullptr;
    al::resetPosition(this, rPosition, false);
    al::setNerve(this, &NrvEchoEmitterKeep);
    mLife = life;
    mDistance = 0.0f;
    mKeepRadius = radius;
    mIntensity = 1.0f;
    appear();
}

void EchoEmitter::exeWait() {
    mDistance = al::calcNerveEaseOutValue(this, mLife / 3, mStartDistance, mEndDistance);
    mRadius = al::calcNerveEaseOutValue(this, mLife / 3, mStartRadius, mEndRadius);
    mIntensity = al::calcNerveValue(this, mLife / 2, mLife, 1.0f, 0.0f);
    if (al::isGreaterEqualStep(this, mLife)) {
        al::setNerve(this, &NrvEchoEmitterStop);
        mRadius = 10.0f;
        kill();
    }
}

void EchoEmitter::exeKeep() {
    mIntensity = al::calcNerveEaseInOutValue(this, mLife, 1.0f, 0.0f);
    mRadius = al::lerpValue(0.1f, mRadius, mKeepRadius);
    if (al::isGreaterEqualStep(this, mLife)) {
        al::setNerve(this, &NrvEchoEmitterStop);
        mRadius = 10.0f;
        kill();
    }
}

void EchoEmitter::exeStop() {
    mDistance = 0.0f;
    mRadius = 10.0f;
    mIntensity = 0.0f;
}

int EchoEmitter::getLife() const {
    if (al::isNerve(this, &NrvEchoEmitterWait))
        return mLife - al::getNerveStep(this);
    return 0;
}
