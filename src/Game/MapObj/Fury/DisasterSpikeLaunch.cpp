#include "MapObj/Fury/DisasterSpikeLaunch.hpp"
#include "MapObj/Fury/DisasterSpikeDirector.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"

namespace {
NERVE_DECL(DisasterSpikeLaunch, Shoot)
NERVES_MAKE_NOSTRUCT(DisasterSpikeLaunch, Shoot)
}

DisasterSpikeLaunch::DisasterSpikeLaunch(const char* name, DisasterSpikeDirector* director)
    : al::LiveActor(name) {
    mDirector = director;
}

void DisasterSpikeLaunch::init(const al::ActorInitInfo& info) {
    al::initNerve(this, &NrvDisasterSpikeLaunchShoot, 0);
    al::initActorWithArchiveName(this, info, "DisasterSpikeLaunch", nullptr);
    sead::Quatf quat = sead::Quatf::unit;
    al::makeQuatUpNoSupport(&quat, -sead::Vector3f::ey);
    al::setQuat(this, quat);
    al::setScaleAll(this, mDirector->getParam().mLaunchSpikeScale);
    makeActorDead();
}

void DisasterSpikeLaunch::appear(SuperBowser* bowser) {
    al::LiveActor::appear();
    mBowser = bowser;
    al::invalidateClipping(this);
    al::setNerve(this, &NrvDisasterSpikeLaunchShoot);
    al::hideModelIfShow(this);
}

void DisasterSpikeLaunch::forceKill() {
    al::tryKillEmitterAndParticleAll(this);
    kill();
}

void DisasterSpikeLaunch::exeShoot() {
    if (al::isFirstStep(this)) {
        al::tryEmitEffect(this, "Launch", nullptr);
        mStart = mDirector->getLaunchSpikePos(al::getRandom(10));
        sead::Vector3f direction = mStart - mDirector->getLaunchSpikeOrigin();
        direction.normalize();
        al::lerpVec(&direction, direction, sead::Vector3f::ey,
                    mDirector->getParam().mLaunchSpikeUpRate);
        direction.normalize();
        mEnd = mDirector->getParam().mLaunchSpikeShootDistance * direction + mStart;
        sead::Quatf quat = sead::Quatf::unit;
        al::makeQuatUpNoSupport(&quat, mStart - mEnd);
        al::setQuat(this, quat);
    }
    float rate = al::getNerveStep(this) / float(mDirector->getParam().mLaunchSpikeShootTime);
    rate = (rate + al::easeOut(rate)) * 0.5f;
    sead::Vector3f pos = sead::Vector3f::zero;
    al::lerpVec(&pos, mStart, mEnd, rate);
    al::setTrans(this, pos);
    int fadeStart = mDirector->getParam().mLaunchSpikeShootTime -
                    mDirector->getParam().mLaunchSpikeFadeTime;
    if (al::isGreaterEqualStep(this, fadeStart))
        mGlobalAlphaLastFrame = 1.0f - (al::getNerveStep(this) - fadeStart) /
                               float(mDirector->getParam().mLaunchSpikeFadeTime);
    if (al::isGreaterStep(this, mDirector->getParam().mLaunchSpikeShootTime))
        kill();
}
