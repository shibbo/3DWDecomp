#include "MapObj/LiftMike.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/KeyPose/KeyPoseKeeperUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include <cmath>
namespace {
NERVE_ACTION_IMPL(LiftMike, Wait)
NERVE_ACTION_IMPL(LiftMike, Up)
NERVE_ACTION_IMPL(LiftMike, UpReleaseHigh)
NERVE_ACTION_IMPL(LiftMike, UpReleaseLow)
NERVE_ACTION_IMPL(LiftMike, HoldTop)
NERVE_ACTION_IMPL(LiftMike, Hold)
NERVE_ACTION_IMPL(LiftMike, Down)
NERVE_ACTION_IMPL(LiftMike, BottomVibrate)
NERVE_ACTIONS_MAKE_STRUCT(LiftMike, Wait, Up, UpReleaseHigh, UpReleaseLow,
                          HoldTop, Hold, Down, BottomVibrate)
float absValue(float value) { return value > 0.0f ? value : -value; }
} // namespace
LiftMike::LiftMike(const char *name) : al::LiveActor(name) {}
void LiftMike::init(const al::ActorInitInfo &info) {
  al::initNerveAction(this, "Wait", &NrvLiftMike.collector, 0);
  al::initActorWithArchiveName(this, info, "LiftMike", nullptr);
  al::tryGetArg(&mGuideBalloonType, info, "GuideBalloonType");
  mKeyPose = al::createKeyPoseKeeper(info);
  al::getCurrentKeyTrans(mKeyPose);
  al::getNextKeyTrans(mKeyPose);
  float radius;
  al::calcKeyMoveClippingInfo(&mClippingCenter, &radius, mKeyPose, 0.0f);
  radius += al::getClippingRadius(this);
  al::setClippingInfo(this, radius, &mClippingCenter);
  static_cast<sead::BaseVec3<float> &>(mBaseTrans) = al::getTrans(this);
  makeActorAppeared();
}
void LiftMike::makeActorAppeared() {
  al::LiveActor::makeActorAppeared();
  al::startNerveAction(this, "Wait");
}
void LiftMike::control() {
  al::LiveActor::control();
  mInput = 300.0f;
  mPrevInput = 300.0f;
  float rate = mSpeed * 10.0f / 35.0f + 0.2f;
  if (absValue(rate) < absValue(mAnimRate))
    rate = mAnimRate * 0.98f + rate * 0.02f;
  mAnimRate = rate;
  al::setSklAnimFrameRate(this, rate, 0);
}
void LiftMike::exeWait() {
  if (u32(al::getNerveStep(this)) >= 60) {
    mSpeed = 20.0f;
    al::startNerveAction(this, "Up");
  }
}
inline float LiftMike::calcSignedDistanceToTop() const {
  const auto &trans = al::getTrans(this);
  const auto &target = al::getNextKeyTrans(mKeyPose);
  sead::Vector3f diff = trans - target;
  float distance = diff.length();
  return diff.y < 0.0f ? -distance : distance;
}
inline void LiftMike::holdMoveSound(const char *name, float maxSpeed) {
  float rate = absValue(mSpeed / maxSpeed);
  float pitch = rate * 0.7f + 0.5f;
  float volume = rate * 2.0f + 0.1f;
  al::holdSeSetPitchVolumeByName(this, name, pitch, volume);
}
void LiftMike::updateMovement() {
  sead::Vector3f dir(0.0f, 0.0f, 0.0f);
  sead::Vector3CalcCommon<float>::sub(dir, al::getNextKeyTrans(mKeyPose),
                                      al::getCurrentKeyTrans(mKeyPose));
  al::normalize(&dir);
  dir *= mSpeed;
  sead::Vector3f pos = al::getTrans(this);
  *al::getTransPtr(this) = pos + dir;
}
void LiftMike::exeUp() {
  float acceleration = mInput / 300.0f;
  float speed = mSpeed + acceleration;
  if (speed > 35.0f)
    speed = 35.0f;
  if (speed < -25.0f)
    speed = -25.0f;
  mSpeed = speed;
  updateMovement();
  if (calcSignedDistanceToTop() >= 0.0f)
    al::startNerveAction(this, "HoldTop");
  holdMoveSound("LiftUp", 35.0f);
  if (u32(al::getNerveStep(this)) >= 60) {
    if (al::isGreaterEqualStep(this, 25))
      al::startNerveAction(this, "UpReleaseHigh");
    else
      al::startNerveAction(this, "UpReleaseLow");
  }
}
void LiftMike::exeUpReleaseHigh() {
  mSpeed *= 0.999f;
  updateMovement();
  if (calcSignedDistanceToTop() >= 0.0f)
    al::startNerveAction(this, "HoldTop");
  else if (mSpeed < 1.0f) {
    mSpeed = 0.0f;
    al::startNerveAction(this, "Hold");
  }
  holdMoveSound("LiftUp", 35.0f);
  if (mInput > 300.0f && calcSignedDistanceToTop() < 0.0f)
    al::startNerveAction(this, "Up");
}
void LiftMike::exeUpReleaseLow() {
  mSpeed *= 0.92f;
  if (calcSignedDistanceToTop() >= 0.0f)
    al::startNerveAction(this, "HoldTop");
  updateMovement();
  if (mSpeed < 14.0f)
    al::startNerveAction(this, "Hold");
  holdMoveSound("LiftUp", 35.0f);
  if (mInput > 300.0f && calcSignedDistanceToTop() < 0.0f)
    al::startNerveAction(this, "Up");
}
inline void LiftMike::updateVibration(float amplitude) {
  al::getNerveStep(this);
  if (!(mVibrationPhase <= 1.0f))
    mSpeed = 0.0f;
  else {
    float angle = mVibrationPhase * 1.5f * 2.0f * sead::Mathf::pi();
    float offset =
        sinf(al::wrapValue(angle, sead::Mathf::pi() * 2.0f)) * amplitude;
    float decay = 1.0f - mVibrationPhase;
    float height = decay * (offset * decay);
    mSpeed = height - (al::getTrans(this).y - mVibrationY);
  }
}
void LiftMike::exeHoldTop() {
  float distance = calcSignedDistanceToTop();
  if (al::isFirstStep(this)) {
    mVibrationY = al::getNextKeyTrans(mKeyPose).y;
    al::startSeSetVolume(this, "LiftStop", absValue(mSpeed) / 35.0f);
    if (distance > 100.0f)
      mVibrationPhase = sead::Mathf::pi();
    else
      mVibrationPhase = asinf(distance / 100.0f) / (3.0f * sead::Mathf::pi());
  }
  mVibrationPhase += 1.0f / 55.0f;
  updateVibration(100.0f);
  updateMovement();
  if (mInput > 300.0f && mPrevInput <= 300.0f &&
      al::isGreaterEqualStep(this, 30))
    al::startNerveAction(this, "HoldTop");
  else if (al::isGreaterEqualStep(this, 150))
    al::startNerveAction(this, "Down");
}
void LiftMike::exeHold() {
  if (al::isFirstStep(this)) {
    al::startSeSetVolume(this, "LiftStop", absValue(mSpeed) / 35.0f);
    mVibrationPhase = 0.0f;
    mVibrationY = al::getTrans(this).y;
  }
  mVibrationPhase += 0.02f;
  updateVibration(40.0f);
  updateMovement();
  if (al::isGreaterEqualStep(this, 150))
    al::startNerveAction(this, "Down");
  else if (mInput > 300.0f && calcSignedDistanceToTop() < 0.0f)
    al::startNerveAction(this, "Up");
}
void LiftMike::exeDown() {
  float speed = mSpeed + -0.14f;
  if (speed > 35.0f)
    speed = 35.0f;
  if (speed < -25.0f)
    speed = -25.0f;
  mSpeed = speed;
  updateMovement();
  if (al::getTrans(this).y <= mBaseTrans.y)
    al::startNerveAction(this, "BottomVibrate");
  if (mInput > 300.0f)
    al::startNerveAction(this, "Up");
  else
    holdMoveSound("LiftDown", 25.0f);
}
void LiftMike::exeBottomVibrate() {
  if (al::isFirstStep(this)) {
    al::startSe(this, "LiftLand", nullptr);
    mVibrationPhase = 0.0f;
    mVibrationY = al::getCurrentKeyTrans(mKeyPose).y;
  }
  mVibrationPhase += 0.02f;
  updateVibration(-40.0f);
  updateMovement();
  if (mInput > 300.0f) {
    mSpeed = 20.0f;
    al::startNerveAction(this, "Up");
  } else if (al::isGreaterEqualStep(this, 50))
    al::startNerveAction(this, "Wait");
}
