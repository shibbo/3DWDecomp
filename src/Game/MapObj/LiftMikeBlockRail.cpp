#include "MapObj/LiftMikeBlockRail.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Math/MatrixUtil.hpp"
#include "Library/Joint/JointControllerKeeper.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Audio/System/AudioVolumeCtrl.hpp"
#include "Project/Block/BlockRailPartsGroup.hpp"
#include "Project/Block/BlockRailRider.hpp"
#include "Project/Block/BlockRailUtil.hpp"
namespace {
NERVE_DECL(LiftMikeBlockRail, Wait);
NERVE_DECL(LiftMikeBlockRail, Move);
NERVE_DECL(LiftMikeBlockRail, Vibration);
NERVE_DECL(LiftMikeBlockRail, Back);
NERVES_MAKE_NOSTRUCT(LiftMikeBlockRail, Vibration)
NERVES_MAKE_STRUCT(LiftMikeBlockRail, Wait, Move, Back)
float absValue(float v) { return v > 0.0f ? v : -v; }
}
LiftMikeBlockRail::LiftMikeBlockRail(const char* name) : al::LiveActor(name) {}
LiftMikeBlockRail::~LiftMikeBlockRail() {}
void LiftMikeBlockRail::init(const al::ActorInitInfo& info) {
    al::initActorWithArchiveName(this, info, "LiftMikeSlide", nullptr);
    al::initNerve(this, &NrvLiftMikeBlockRail.Wait, 0);
    mRailGroup = new al::BlockRailPartsGroup;
    mRailGroup->init(info);
    if (mRailGroup->getPartsNum() == 0) { makeActorDead(); return; }
    float radius = 250.0f;
    al::calcBlockRailClippingSphere(&mClippingCenter, &radius, mRailGroup, al::getTrans(this), 250.0f);
    al::setClippingInfo(this, radius, &mClippingCenter);
    mRailRider = new al::BlockRailRider;
    mRailRider->mIsLeaveAtEnd = false;
    al::tryGetArg(&mGuideBalloonType, info, "GuideBalloonType");
    al::setBlockRailRiderToNearestPos(mRailRider, mRailGroup, al::getTrans(this));
    sead::Vector3f direction;
    al::calcPosAndDirBlockRailRider(al::getTransPtr(this), &direction, mRailRider);
    int goAxis = 3;
    al::tryGetArg(&goAxis, info, "GoAxis");
    sead::Vector3f axis;
    al::tryGetLocalSignAxis(&axis, info, goAxis);
    if (direction.dot(axis) < 0.0f) mRailRider->reverse();
    al::initJointControllerKeeper(this, 2);
    al::initJointLocalYRotator(this, &mPropellerAngle, "Propeller");
    al::initJointLocalMtxController(this, &mVibrationMtx, "LiftMikeSlide");
    makeActorAppeared();
}
void LiftMikeBlockRail::control() {
    mPropellerAngle += mSpeed * 2.0f;
    mPropellerAngle = al::wrapValue(mPropellerAngle, 360.0f);
    sead::Vector3f up = sead::Vector3f::ey;
    al::rotateVectorDegree(&up, up, sead::Vector3f::ex, mVibrationStrength * 2.5f);
    al::rotateVectorDegree(&up, up, sead::Vector3f::ey, mVibrationAngle);
    float height = sead::Mathf::cos(sead::Mathf::deg2rad(mVibrationAngle)) * mVibrationStrength * 3.0f;
    al::makeMtxUpSidePos(&mVibrationMtx, up, sead::Vector3f::ex, sead::Vector3f(0.0f, height, 0.0f));
}
void LiftMikeBlockRail::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait");
        al::validateClipping(this);
        al::startSe(this, "LiftLand", nullptr);
    }
    mVibrationAngle = al::wrapValue(mVibrationAngle + 40.0f, 360.0f);
    mVibrationStrength = al::lerpValue(0.12f, mVibrationStrength, 0.0f);
    mSpeed *= 0.9f;
    if (static_cast<u32>(al::getNerveStep(this)) >= 90) {
        mSpeed = 4.0f;
        al::invalidateClipping(this);
        al::setNerve(this, &NrvLiftMikeBlockRail.Move);
    }
}
void LiftMikeBlockRail::exeMove() {
    if (al::isFirstStep(this)) al::startAction(this, "Move");
    mSpeed = sead::Mathf::min(30.0f, mSpeed + 4.0f);
    al::moveBlockRailRiderAndCalcPos(al::getTransPtr(this), mRailRider, mSpeed);
    float rate = absValue(mSpeed / 30.0f);
    float pitch = rate * 0.7f + 0.5f;
    float volume = rate * 0.4f + 0.1f;
    al::holdSeSetPitchVolumeByName(this, "LiftUp", pitch, volume);
    if (mRailRider->isReachEnd()) al::setNerve(this, &NrvLiftMikeBlockRailVibration);
}
void LiftMikeBlockRail::exeVibration() {
    if (al::isFirstStep(this)) {
        float volume = absValue(mSpeed) / 30.0f;
        mVibrationTimer = 90;
        al::startSeSetVolume(this, "LiftStop", volume);
    }
    if (static_cast<u32>(al::getNerveStep(this)) <= 90) {
        mVibrationTimer = 90;
        mSpeed += 4.0f;
        if (mSpeed > 30.0f) mSpeed = 30.0f;
    } else {
        mSpeed += -0.5f;
        --mVibrationTimer;
        if (mSpeed < 0.0f) mSpeed = 0.0f;
    }
    mVibrationStrength = al::lerpValue(0.2f, mVibrationStrength, mSpeed / 30.0f);
    mVibrationAngle = al::wrapValue(mVibrationAngle + 50.0f, 360.0f);
    al::holdSeWithParam(this, "VibrationLv", mVibrationStrength, nullptr);
    if (mVibrationTimer <= 0) al::setNerve(this, &NrvLiftMikeBlockRail.Back);
}
void LiftMikeBlockRail::exeHold() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Hold");
        al::startSeSetVolume(this, "LiftStop", absValue(mSpeed) / 30.0f);
    }
    mSpeed = sead::Mathf::max(0.0f, mSpeed - 0.5f);
    if (al::isMicBreathInputOn(this)) {
        mSpeed = 4.0f;
        al::setNerve(this, &NrvLiftMikeBlockRail.Move);
    } else if (al::isGreaterEqualStep(this, 90)) al::setNerve(this, &NrvLiftMikeBlockRail.Back);
}
void LiftMikeBlockRail::exeBack() {
    if (al::isFirstStep(this)) al::startAction(this, "Back");
    mVibrationStrength = al::lerpValue(0.2f, mVibrationStrength, 0.0f);
    if (mSpeed > 0.0f) mSpeed = 0.0f;
    mSpeed = sead::Mathf::max(-10.0f, mSpeed - 0.5f);
    al::moveBlockRailRiderAndCalcPos(al::getTransPtr(this), mRailRider, mSpeed);
    float rate = absValue(mSpeed / -10.0f);
    float pitch = rate * 0.7f + 0.5f;
    float volume = rate * 0.4f + 0.1f;
    al::holdSeSetPitchVolumeByName(this, "LiftDown", pitch, volume);
    if (al::isMicBreathInputOn(this)) {
        mSpeed = 4.0f;
        al::setNerve(this, &NrvLiftMikeBlockRail.Move);
    } else if (mRailRider->isReachEnd()) {
        mVibrationStrength = 0.8f;
        al::setNerve(this, &NrvLiftMikeBlockRail.Wait);
    }
}
