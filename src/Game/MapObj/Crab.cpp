#include "MapObj/Crab.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Audio/System/AudioVolumeCtrl.hpp"
#include "Library/Item/ItemUtil.hpp"
#include "Util/ItemUtil.hpp"
namespace {
    NERVE_DECL(Crab, Wait);
    NERVE_DECL(Crab, Disappear);
    NERVE_DECL(Crab, Walk);
    NERVES_MAKE_NOSTRUCT(Crab, Wait, Disappear, Walk)
}
Crab::Crab(const char* name) : al::LiveActor(name) {}
Crab::~Crab() {}
void Crab::init(const al::ActorInitInfo& info) {
    mPositive = al::isHalfProbability();
    al::initActor(this, info);
    mCenter = al::getTrans(this);
    sead::Vector3f end;
    if (al::tryGetLinksTrans(&end, info, "MoveNext")) {
        if (al::separateScalarAndDirection(&mMaxRange, &mSide, end - al::getTrans(this))) {
            al::calcSideDir(&mSide, this);
            mMinRange = mMaxRange = 0.0f;
        } else {
            mCenter = (end + al::getTrans(this)) * 0.5f;
            mMinRange = sead::Mathf::max(0.0f, mMaxRange - 50.0f);
            al::resetPosition(this, mCenter, false);
            al::makeQuatSideUp(al::getQuatPtr(this), mSide, sead::Vector3f::ey);
        }
    } else {
        al::calcSideDir(&mSide, this);
        mMinRange = 400.0f;
        mMaxRange = 450.0f;
    }
    al::initNerve(this, &NrvCrabWait, 0);
    mItemId = rc::tryInitItemByHostInfo(this, info, 1);
    makeActorAppeared();
}
bool Crab::receiveMsg(const al::SensorMsg* msg, al::HitSensor*, al::HitSensor*) {
    if ((al::isMsgItemGetAll(msg) || al::isMsgExplosion(msg) ||
         al::isMsgPlayerFireBallAttack(msg) || al::isMsgPlayerBoomerangAttack(msg)) &&
        !al::isNerve(this, &NrvCrabDisappear)) al::setNerve(this, &NrvCrabDisappear);
    return false;
}
bool Crab::receiveMsgScreenPoint(const al::SensorMsg* msg, al::ScreenPointer*, al::ScreenPointTarget*) {
    if (al::isMsgTouchAssistTrig(msg) || al::isMsgTouchAssist(msg)) {
        if (al::isNerve(this, &NrvCrabDisappear)) return false;
        al::setNerve(this, &NrvCrabDisappear);
        return true;
    }
    return false;
}
void Crab::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait");
        mWaitTime = al::getRandom(20, 40);
    }
    if (al::isMicBreathInputOn(this)) {
        al::setNerve(this, &NrvCrabDisappear);
        return;
    }
    if (al::isGreaterEqualStep(this, mWaitTime)) {
        setNextTargetPos();
        al::setNerve(this, &NrvCrabWalk);
    }
}
void Crab::setNextTargetPos() {
    float range = al::getRandom(mMinRange, mMaxRange);
    range *= 0.5f;
    float distance = mPositive ? range : -range;
    mTarget = mSide * distance + mCenter;
    mPositive = !mPositive;
}
void Crab::exeWalk() {
    if (al::isFirstStep(this)) {
        mStart = al::getTrans(this);
        al::startAction(this, "Run");
        mWalkTime = (mStart - mTarget).length() * 0.125f;
    }
    if (al::isMicBreathInputOn(this)) {
        al::setNerve(this, &NrvCrabDisappear);
        return;
    }
    al::lerpVec(al::getTransPtr(this), mStart, mTarget, al::calcNerveRate(this, int(mWalkTime)));
    if (al::isGreaterEqualStep(this, int(mWalkTime))) al::setNerve(this, &NrvCrabWait);
}
void Crab::exeDisappear() {
    if (al::isFirstStep(this)) al::startAction(this, "Disappear");
    if (al::isActionEnd(this)) {
        if (mItemId != -1) {
            sead::Vector3f front;
            al::calcFrontDir(&front, this);
            const sead::Vector3f& trans = al::getTrans(this);
            al::appearItem(this, sead::Vector3f::ey * 50.0f + trans, front);
        }
        kill();
    }
}
