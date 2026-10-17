#include "MapObj/GustWind.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Yaml/ByamlIter.hpp"
#include "Library/Yaml/ByamlUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/PlayerUtil.hpp"
namespace {
NERVE_ACTION_IMPL(GustWind, Wait);
NERVE_ACTION_IMPL(GustWind, BlowSign);
NERVE_ACTION_IMPL(GustWind, Blow);
NERVE_ACTION_IMPL(GustWind, BlowEnd);
NERVE_ACTIONS_MAKE_STRUCT(GustWind, Wait, BlowSign, Blow, BlowEnd)
}
GustWind::GustWind(const char* name) : al::LiveActor(name) {}
GustWind::~GustWind() {}
void GustWind::init(const al::ActorInitInfo& info) {
    al::initNerveAction(this, "Wait", &NrvGustWind.collector, 0);
    al::initMapPartsActor(this, info, nullptr, 0);
    al::tryGetMatrixTR(&mBaseMtx, info);
    al::ByamlIter range(al::tryGetMapPartsResourceYaml(info, "WindRange"));
    if (range.isValid()) {
        al::tryGetByamlF32(&mFrontStart, range, "FrontStart");
        al::tryGetByamlF32(&mFrontEnd, range, "FrontEnd");
        al::tryGetByamlF32(&mSideStart, range, "SideStart");
        al::tryGetByamlF32(&mSideEnd, range, "SideEnd");
    }
    al::tryGetArg(&mWaitTime, info, "WaitTime");
    al::tryGetArg(&mBlowTime, info, "BlowTime");
    al::tryGetArg(&mIsNeverBlow, info, "IsNeverBlow");
    if (mIsNeverBlow) al::startNerveAction(this, "Blow");
    sead::Vector3f min(-mSideEnd, 0.0f, -mSideEnd);
    sead::Vector3f max(mSideEnd, mFrontEnd, mSideEnd);
    al::multVecPose(&min, this, min);
    al::multVecPose(&max, this, max);
    mClippingCenter = (min + max) * 0.5f;
    al::setClippingInfo(this, (min - max).length() * 0.5f, &mClippingCenter);
    makeActorAppeared();
}
void GustWind::attackSensor(al::HitSensor* sender, al::HitSensor* receiver) {
    float power = calcBlowPowerRate();
    if (!(power > 0.0f)) return;
    if (al::isSensorPlayer(receiver) && rc::isPlayerGiant(receiver)) return;
    sead::Vector3f up;
    al::calcUpDir(&up, this);
    sead::Vector3f position;
    al::multVecInvQuat(&position, this, al::getSensorPos(receiver));
    float frontRate = al::lerpValue(position.y, mFrontStart, mFrontEnd, 1.0f, 0.0f);
    float sideX = sead::Mathf::abs(position.x);
    float sideZ = sead::Mathf::abs(position.z);
    float sideDistance = sideX < sideZ ? sideZ : sideX;
    float sideRate = al::lerpValue(sideDistance, mSideStart, mSideEnd, 1.0f, 0.0f);
    if (al::isSensorPlayer(receiver)) {
        sead::Vector3f force = (power * (frontRate * 0.6f * sideRate)) * up;
        rc::sendMsgGroundSnapOffForce(receiver, sender);
        rc::sendMsgAddForce(receiver, sender, force);
    }
    sead::Vector3f force = (power * (frontRate * 3.0f * sideRate)) * up;
    rc::sendMsgGustWind(receiver, sender, force);
}
float GustWind::calcBlowPowerRate() const {
    if (al::isNerve(this, NrvGustWind.Blow.data())) return al::calcNerveRate(this, 20);
    if (al::isNerve(this, NrvGustWind.BlowEnd.data())) return al::calcNerveValue(this, 45, 1.0f, 0.0f);
    return 0.0f;
}
void GustWind::exeWait() {
    if (al::isGreaterEqualStep(this, mWaitTime)) al::startNerveAction(this, "BlowSign");
}
void GustWind::exeBlowSign() {
    if (al::isGreaterEqualStep(this, 90)) al::startNerveAction(this, "Blow");
}
void GustWind::exeBlow() {
    if (!mIsNeverBlow && al::isGreaterEqualStep(this, mBlowTime)) al::startNerveAction(this, "BlowEnd");
}
void GustWind::exeBlowEnd() {
    if (al::isGreaterEqualStep(this, 45)) al::startNerveAction(this, "Wait");
}
