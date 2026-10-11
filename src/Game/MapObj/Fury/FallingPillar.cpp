#include "MapObj/Fury/FallingPillar.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/HitReactionKeeper.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"

namespace {
NERVE_DECL(FallingPillar, Wait);
NERVE_DECL(FallingPillar, Fall);
NERVE_DECL(FallingPillar, Land);
NERVE_DECL(FallingPillar, FallDelay);
NERVES_MAKE_NOSTRUCT(FallingPillar, Wait)
NERVES_MAKE_STRUCT(FallingPillar, Fall, Land, FallDelay)
}

FallingPillar::FallingPillar(const char* pName) : al::LiveActor(pName) {}
FallingPillar::~FallingPillar() {}

void FallingPillar::init(const al::ActorInitInfo& info) {
    al::initNerve(this, &NrvFallingPillarWait, 0);
    al::initActorWithArchiveName(this, info, "FallingPillar", nullptr);
    makeActorAppeared();
    al::hideModelIfShow(this);
    createPillarParts(info);
    al::tryGetArg(&mFallDelayFrames, info, "FallDelayFrames");
    if (al::calcLinkChildNum(info, "FallingPillarKey") > 0)
        al::getLinksMatrix(&mLandingMtx, info, "FallingPillarKey");
    al::invalidateClipping(mTop);
    al::invalidateClipping(mBottom);
}

void FallingPillar::createPillarParts(const al::ActorInitInfo& info) {
    mBottom = new al::LiveActor("FallingPillarBottom");
    al::initMapPartsActorNoPlacementInfo(mBottom, info, "keeslu01FixedPart238");
    al::setTrans(mBottom, al::getTrans(this));
    al::setQuat(mBottom, al::getQuat(this));
    mBottom->makeActorAppeared();
    mTop = new al::LiveActor("FallingPillarTop");
    al::initMapPartsActorNoPlacementInfo(mTop, info, "keeslu01FixedPart237");
    al::setTrans(mTop, al::getTrans(this));
    al::setQuat(mTop, al::getQuat(this));
    mTop->makeActorAppeared();
}

void FallingPillar::appear() {
    al::LiveActor::appear();
    mTop->appear();
    mBottom->appear();
}

void FallingPillar::kill() {
    al::LiveActor::kill();
    mTop->kill();
    mBottom->kill();
}

void FallingPillar::exeWait() {}

void FallingPillar::exeFallDelay() {
    if (al::isGreaterEqualStep(this, mFallDelayFrames))
        al::setNerve(this, &NrvFallingPillar.Fall);
}

void FallingPillar::exeFall() {
    if (al::isFirstStep(this)) {
        al::setTrans(mTop, al::getTrans(this));
        al::setQuat(mTop, al::getQuat(this));
    }
    if (al::isLessEqualStep(this, mFallFrames)) {
        float rate = sead::Mathf::pow(float(al::getNerveStep(this)) / float(mFallFrames), mFallExponent);
        sead::Vector3f trans = sead::Vector3f::zero;
        al::lerpVec(&trans, al::getTrans(this), mLandingMtx.getTranslation(), rate);
        al::setTrans(mTop, trans);
        sead::Quatf quat = sead::Quatf::unit;
        sead::Quatf target = sead::Quatf::unit;
        mLandingMtx.toQuat(target);
        al::slerpQuat(&quat, al::getQuat(this), target, rate);
        al::setQuat(mTop, quat);
    } else {
        al::setNerve(this, &NrvFallingPillar.Land);
    }
}

void FallingPillar::exeLand() {
    if (al::isFirstStep(this)) {
        sead::Vector3f offset = sead::Vector3f::ey * 3000.0f;
        sead::Quatf quat = sead::Quatf::unit;
        mLandingMtx.toQuat(quat);
        al::rotateVectorQuat(&offset, quat);
        mSplashPos = mLandingMtx.getTranslation() + offset;
        al::tryEmitEffect(this, "Splash", &mSplashPos);
        getHitReactionKeeper()->start("Land", nullptr, nullptr, nullptr);
    }
}

void FallingPillar::restore() {
    al::setTrans(mTop, al::getTrans(this));
    al::setQuat(mTop, al::getQuat(this));
    al::setNerve(this, &NrvFallingPillarWait);
}

void FallingPillar::fall() {
    al::invalidateClipping(this);
    al::setNerve(this, &NrvFallingPillar.FallDelay);
}

bool FallingPillar::isFallen() {
    return al::isNerve(this, &NrvFallingPillar.FallDelay) ||
           al::isNerve(this, &NrvFallingPillar.Fall) ||
           al::isNerve(this, &NrvFallingPillar.Land);
}
