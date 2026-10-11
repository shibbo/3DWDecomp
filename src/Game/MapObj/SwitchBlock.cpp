#include "MapObj/SwitchBlock.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/KeyPose/KeyPoseKeeperUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"

namespace {
    NERVE_DECL(SwitchBlock, Wait);
    NERVE_DECL(SwitchBlock, Move);
    NERVES_MAKE_NOSTRUCT(SwitchBlock, Wait, Move)
}

SwitchBlock::SwitchBlock(const char* pName) : al::LiveActor(pName) {
}

void SwitchBlock::init(const al::ActorInitInfo& rInfo) {
    al::initMapPartsActor(this, rInfo, nullptr, 0);
    al::initNerve(this, &NrvSwitchBlockWait, 1);
    mKeyPoseKeeper = al::createKeyPoseKeeper(rInfo);
    if (al::getKeyPoseCount(mKeyPoseKeeper) == 1) {
        makeActorDead();
        return;
    }
    float radius = 0.0f;
    al::calcKeyMoveClippingInfo(&mClippingCenter, &radius, mKeyPoseKeeper, 0.0f);
    radius += al::getClippingRadius(this);
    al::setClippingInfo(this, radius, &mClippingCenter);
    makeActorAppeared();
}

void SwitchBlock::exeWait() {
}

void SwitchBlock::exeMove() {
    if (al::isFirstStep(this)) {
        auto* keeper = mKeyPoseKeeper;
        sead::Vector3f dir = sead::Vector3f::ey;
        al::calcDirToNextKey(&dir, keeper);
        if (al::isNearDirection(dir, sead::Vector3f::ey)) {
            al::startHitReaction(this, "上昇");
        } else {
            al::startHitReaction(this, "下降");
        }
    }
    float rate = al::getNerveStep(this) / 30.0f;
    al::calcLerpKeyTrans(al::getTransPtr(this), mKeyPoseKeeper, rate);
    if (al::isGreaterEqualStep(this, 30)) {
        auto* keeper = mKeyPoseKeeper;
        sead::Vector3f dir = sead::Vector3f::ey;
        al::calcDirToNextKey(&dir, keeper);
        if (al::isNearDirection(dir, sead::Vector3f::ey)) {
            al::startHitReaction(this, "上昇完了");
        } else {
            al::startHitReaction(this, "下降完了");
        }
        al::nextKeyPose(mKeyPoseKeeper);
        al::setNerve(this, &NrvSwitchBlockWait);
    }
}

void SwitchBlock::startMove() {
    if (al::isNerve(this, &NrvSwitchBlockWait)) {
        al::setNerve(this, &NrvSwitchBlockMove);
    }
}

bool SwitchBlock::isMove() const {
    return al::isNerve(this, &NrvSwitchBlockMove);
}

SwitchBlock::~SwitchBlock() {
}
