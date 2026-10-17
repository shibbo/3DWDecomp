#include "MapObj/BoxLight.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/Collision/PartsConnectorUtil.hpp"
#include "Library/Light/PrePassLightFunction.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/PostProcessing/OccludedEffectDirector.hpp"
#include "Library/PostProcessing/OccludedEffectRequestInfo.hpp"
#include "Library/PostProcessing/OfxFunction.hpp"
#include "MapObj/BoxLightCollisionChecker.hpp"
#include "MapObj/HeadgearPoseBuilder.hpp"
#include "MapObj/HeadgearStateBlow.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Controller/PadRumbleKeeper.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include <prim/seadDelegate.h>
namespace {
NERVE_DECL(BoxLightCollisionChecker, NoHit);
NERVE_DECL(BoxLightCollisionChecker, ToHitCheck);
NERVE_DECL(BoxLightCollisionChecker, Hit);
NERVE_DECL(BoxLightCollisionChecker, ToNoHitCheck);
NERVES_MAKE_NOSTRUCT(BoxLightCollisionChecker, NoHit, ToHitCheck, Hit,
                     ToNoHitCheck)
} // namespace

BoxLightCollisionChecker::BoxLightCollisionChecker()
    : al::NerveExecutor("ライトボックスのコリジョンチェック") {
  initNerve(&NrvBoxLightCollisionCheckerNoHit, 0);
}

void BoxLightCollisionChecker::exeNoHit() {
  if (mIsColliding)
    al::setNerve(this, &NrvBoxLightCollisionCheckerToHitCheck);
}

void BoxLightCollisionChecker::exeToHitCheck() {
  if (mIsColliding) {
    if (al::isGreaterStep(this, 5))
      al::setNerve(this, &NrvBoxLightCollisionCheckerHit);
  } else {
    al::setNerve(this, &NrvBoxLightCollisionCheckerNoHit);
  }
}

void BoxLightCollisionChecker::exeHit() {
  if (!mIsColliding)
    al::setNerve(this, &NrvBoxLightCollisionCheckerToNoHitCheck);
}

void BoxLightCollisionChecker::exeToNoHitCheck() {
  if (mIsColliding) {
    al::setNerve(this, &NrvBoxLightCollisionCheckerHit);
  } else if (al::isGreaterStep(this, 5)) {
    al::setNerve(this, &NrvBoxLightCollisionCheckerNoHit);
  }
}

bool BoxLightCollisionChecker::isHit() const {
  return al::isNerve(this, &NrvBoxLightCollisionCheckerHit) ||
         al::isNerve(this, &NrvBoxLightCollisionCheckerToNoHitCheck);
}

namespace {
NERVE_DECL(BoxLight, Wait);
NERVE_DECL(BoxLight, Kill);
NERVE_DECL(BoxLight, Appear);
NERVE_DECL(BoxLight, Carry);
NERVE_DECL(BoxLight, CarryLighting);
NERVE_DECL(BoxLight, CarryStart);
const struct {
  BoxLightNrvWait Wait;
  BoxLightNrvKill Kill;
  BoxLightNrvAppear Appear;
  BoxLightNrvCarry Carry;
  BoxLightNrvCarryLighting CarryLighting;
  BoxLightNrvCarryStart CarryStart;
} NrvBoxLight;
const sead::Vector3f sLightOffset(0.0f, 60.0f, 100.0f);
const HeadgearPoseBuilderParam sPoseParam{true, false, true};
} // namespace
BoxLight::BoxLight(const char *name)
    : al::LiveActor(name), mCollisionChecker(new BoxLightCollisionChecker) {}
BoxLight::~BoxLight() {}
void BoxLight::init(const al::ActorInitInfo &info) {
  al::initActorWithArchiveName(this, info, "BoxLight", nullptr);
  al::initNerve(this, &NrvBoxLight.Wait, 1);
  mPoseBuilder = new HeadgearPoseBuilder(this, &sPoseParam);
  mDelegate = new sead::Delegate<HeadgearPoseBuilder>(
      mPoseBuilder, &HeadgearPoseBuilder::updateAndCalcAnimDirect);
  mBlowState = new HeadgearStateBlow(this, nullptr);
  al::initNerveState(this, mBlowState, &NrvBoxLight.Kill, "吹き飛び状態");
  rc::setPlayerColorAnimDefault(this, "BoxLight");
  if (al::isPlaced(info))
    makeActorAppeared();
  else
    makeActorDead();
  al::tryGetArg(&mEffectOnOff, info, "IsEffectOnOff");
  bool specularOff = false;
  al::tryGetArg(&specularOff, info, "IsForceSpecularOff");
  if (specularOff)
    al::setEnablePrePassLightSpecular(this, "懐中電灯スポットライト", false);
  mRumble = al::createPadRumbleKeeper(this, -1);
  al::offCollide(this);
  al::hideSilhouetteModel(this);
  al::killPrePassLight(this, "懐中電灯スポットライト", -1);
  al::getPrePassSpotLightInfo(this, "懐中電灯スポットライト", nullptr,
                              &mLightLength, nullptr, nullptr);
  mConnector = al::createMtxConnector(this);
  al::attachMtxConnectorToActor(mConnector, this, nullptr);
  mOccludedEffect =
      OfxFunction::getOccludedEffectDirector(this)->createInfoByPresetName(
          "ObjSmall");
  mRotationHistory.allocBuffer(20, nullptr);
}
void BoxLight::appear() {
  al::LiveActor::appear();
  al::validateHitSensors(this);
  al::invalidateHitSensor(this, "FlashRange");
  al::validateClipping(this);
  al::validateCollisionParts(this);
  al::offCollide(this);
  al::setNerve(this, &NrvBoxLight.Appear);
}
void BoxLight::control() {
  if (mRotationHistory.size() >= mRotationHistory.capacity())
    mRotationHistory.popFront();
  sead::Quatf rotation;
  al::calcConnectInfo(mConnector, nullptr, &rotation, nullptr, sLightOffset,
                      sead::Vector3f::zero);
  mRotationHistory.pushBack(rotation);
  float degree = 0.0f;
  sead::Quatf previous = mRotationHistory(0);
  for (auto it = ++mRotationHistory.begin(); it != mRotationHistory.end();
       ++it) {
    sead::Vector3f axis;
    float delta;
    al::calcQuatRotateAxisAndDegree(&axis, &delta, previous, sead::Quatf(*it));
    degree += delta;
    previous = *it;
  }
  mRotationDegree = degree;
  mFlashHit = false;
  bool hitCollision =
      al::isPrePassSpotLightStrikeCollision(this, "懐中電灯スポットライト");
  mCollisionChecker->setCollision(hitCollision);
  mCollisionChecker->updateNerve();
  if (mEffectOnOff && mCollisionChecker->isHit())
    mEffectState = 2;
  if (mPlayerSensor) {
    if (rc::isPlayerHideModel(al::getSensorHost(mPlayerSensor)))
      mEffectState = 2;
    switch (mEffectState) {
    case 1:
      al::tryEmitEffect(
          this,
          al::StringTmp<256>("LightOn%s", rc::getPlayerCharacterName(
                                              al::getSensorHost(mPlayerSensor)))
              .cstr(),
          nullptr);
      break;
    case 2:
      al::tryDeleteEffect(
          this,
          al::StringTmp<256>("LightOn%s", rc::getPlayerCharacterName(
                                              al::getSensorHost(mPlayerSensor)))
              .cstr());
      break;
    }
  }
  al::setPrePassSpotLightLength(this, "懐中電灯スポットライト",
                                sead::Mathf::min(mLightLength, 2000.0f));
}
bool BoxLight::isLighting() const {
  return al::isNerve(this, &NrvBoxLight.Carry) ||
         al::isNerve(this, &NrvBoxLight.CarryLighting);
}
bool BoxLight::isRollingPlayer() const { return mRotationDegree > 360.0f; }
void BoxLight::attackSensor(al::HitSensor *self, al::HitSensor *other) {
  if (!al::isSensorName(self, "FlashRange") || !isLighting() ||
      isRollingPlayer())
    return;
  float angle, length;
  sead::Vector3f dir, pos;
  al::getPrePassSpotLightInfo(this, "懐中電灯スポットライト", &angle, &length,
                              &dir, &pos);
  length = sead::Mathf::min(length, 800.0f);
  if (al::checkHitSphereSpotLight(al::getSensorPos(other),
                                  al::getSensorRadius(other), pos, dir, length,
                                  angle) &&
      al::sendMsgLightFlash(other, self))
    mFlashHit = true;
}
bool BoxLight::receiveMsg(const al::SensorMsg *msg, al::HitSensor *other,
                          al::HitSensor *self) {
  if (rc::isMsgAskControlUserId(msg, mPlayerSensor))
    return true;
  if (!al::isSensorPlayerOrPlayerWeapon(other))
    return false;
  if (mPlayerSensor && mBlowState->tryStart(msg, mPlayerSensor)) {
    mPoseBuilder->clearPlayerSensor();
    al::tryDeleteEffect(
        this,
        al::StringTmp<256>("LightOn%s", rc::getPlayerCharacterName(
                                            al::getSensorHost(mPlayerSensor)))
            .cstr());
    mPlayerSensor = nullptr;
    al::setNerve(this, &NrvBoxLight.Kill);
    return true;
  }
  if (al::isNerve(this, &NrvBoxLight.Wait)) {
    if (al::isMsgPlayerObjUpperPunch(msg)) {
      if (!tryShiftCarry(other, self))
        return true;
    } else if (al::isMsgPlayerFloorTouch(msg))
      mFloorTouch = true;
  } else if (al::isNerve(this, &NrvBoxLight.Appear) &&
             al::isMsgPlayerObjUpperPunch(msg)) {
    if (!tryShiftCarry(other, self))
      return true;
  }
  return false;
}
bool BoxLight::tryShiftCarry(al::HitSensor *other, al::HitSensor *self) {
  if (!rc::tryPlayerEquipHeadgear(other, self, mDelegate, 0))
    return false;
  mPlayerSensor = other;
  mPoseBuilder->start(other);
  al::showSilhouetteModel(this);
  al::invalidateClipping(this);
  al::invalidateHitSensors(this);
  al::invalidateCollisionParts(this);
  al::setNerve(this, &NrvBoxLight.CarryStart);
  return true;
}
void BoxLight::exeAppear() {
  if (al::isFirstStep(this)) {
    if (mHipDrop)
      al::startAction(this, "AppearHipDrop");
    else
      al::startAction(this, "Appear");
    mEffectState = 0;
  }
  if (al::isActionEnd(this))
    al::setNerve(this, &NrvBoxLight.Wait);
}
void BoxLight::exeWait() {
  if (al::isFirstStep(this)) {
    al::startAction(this, "WaitItem");
    mFloorTouch = false;
    mEffectState = 0;
  }
  if (mFloorTouch) {
    if (!al::isActionPlaying(this, "WaitItemStop"))
      al::startAction(this, "WaitItemStop");
  } else {
    if (!al::isActionPlaying(this, "WaitItem"))
      al::startAction(this, "WaitItem");
  }
  mFloorTouch = false;
}
void BoxLight::exeCarryStart() {
  if (al::isFirstStep(this)) {
    mRumble->setPort(rc::getPlayerInputPort(mPlayerSensor));
    mMini = rc::isPlayerMini(mPlayerSensor);
    al::startAction(this, "Attach");
    startCharaMtpAnim();
    mEffectState = 0;
  }
  if (al::isActionEnd(this)) {
    al::appearPrePassLight(this, "懐中電灯スポットライト", -1);
    al::validateHitSensor(this, "FlashRange");
    al::setNerve(this, &NrvBoxLight.Carry);
    return;
  }
  mPoseBuilder->update();
}
void BoxLight::startCharaMtpAnim() {
  rc::setPlayerColorAnimBySensor(this, mPlayerSensor, "BoxLight");
}
bool BoxLight::isFrontNearCamera() const {
  if (rc::getActiveControlUserNum(GameDataHolderAccessor(this)) < 3)
    return false;
  sead::Vector3f camera, front;
  al::calcCameraDir(&camera, this);
  al::calcFrontDir(&front, this);
  return al::calcAngleDegree(camera, front) <= 60.0f;
}

void BoxLight::exeCarry() {
  if (al::isFirstStep(this)) {
    mMini = rc::isPlayerMini(mPlayerSensor);
    al::startAction(this, "Wait");
  }

  if (isRollingPlayer() ||
      rc::isPlayerHideModel(al::getSensorHost(mPlayerSensor))) {
    al::requestPrePassLightColor(this, "懐中電灯スポットライト",
                                 sead::Color4f::cBlack);
    mEffectState = 2;
  } else {
    al::requestPrePassLightColor(
        this, "懐中電灯スポットライト",
        rc::getPlayerCharacterName(al::getSensorHost(mPlayerSensor)), 1.0f);
    mEffectState = 1;
    sead::Vector3f pos;
    al::calcConnectInfo(mConnector, &pos, nullptr, nullptr, sLightOffset,
                        sead::Vector3f::zero);
    mOccludedEffect->requestByPos(pos);
  }
  if (isFrontNearCamera())
    mEffectState = 2;

  if (rc::isPlayerDamageTrigOn(mPlayerSensor)) {
    rc::removePlayerEquipHeadgear(mPlayerSensor, false);
    return;
  }
  if (mFlashHit) {
    al::setNerve(this, &NrvBoxLight.CarryLighting);
    return;
  }
  mPoseBuilder->update();
}
void BoxLight::exeCarryLighting() {
  if (al::isFirstStep(this)) {
    mMini = rc::isPlayerMini(mPlayerSensor);
    al::startAction(this, "Lighting");
  }

  if (isRollingPlayer() ||
      rc::isPlayerHideModel(al::getSensorHost(mPlayerSensor))) {
    al::requestPrePassLightColor(this, "懐中電灯スポットライト",
                                 sead::Color4f::cBlack);
    mEffectState = 2;
  } else {
    al::requestPrePassLightColor(
        this, "懐中電灯スポットライト",
        rc::getPlayerCharacterName(al::getSensorHost(mPlayerSensor)), 1.0f);
    mEffectState = 1;
    sead::Vector3f pos;
    al::calcConnectInfo(mConnector, &pos, nullptr, nullptr, sLightOffset,
                        sead::Vector3f::zero);
    mOccludedEffect->requestByPos(pos);
  }
  if (isFrontNearCamera())
    mEffectState = 2;

  if (rc::isPlayerDamageTrigOn(mPlayerSensor)) {
    rc::removePlayerEquipHeadgear(mPlayerSensor, false);
    return;
  }
  if (!mFlashHit) {
    al::setNerve(this, &NrvBoxLight.Carry);
    return;
  }
  mPoseBuilder->update();
}
void BoxLight::exeKill() {
  if (al::updateNerveState(this)) {
    mEffectState = 0;
    al::killPrePassLight(this, "懐中電灯スポットライト", -1);
    kill();
  }
}
