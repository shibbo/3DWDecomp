#include "MapObj/RouteDokanRider.hpp"
#include "Layout/GuideGameWindow.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "MapObj/PuppetStickRouteSelecter.hpp"
#include "MapObj/RouteDokanEntrance.hpp"
#include "MapObj/RouteDokanInOutEffect.hpp"
#include "Player/IUsePlayerPuppet.hpp"
#include "Player/Normal/PlayerActor.hpp"
#include "Player/PlayerBindEndParam.hpp"
#include "Project/Block/BlockRailRider.hpp"
#include "Scene/SceneObjID.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/PlayerPuppetUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
namespace {
NERVE_DECL(RouteDokanRider, Start);
NERVE_DECL(RouteDokanRider, Move);
NERVE_DECL(RouteDokanRider, Invalid);
NERVE_DECL(RouteDokanRider, End);
NERVES_MAKE_NOSTRUCT(RouteDokanRider, Start, Move, Invalid, End)
PlayerBindEndParam sRouteBindEndParam{"Jump", 0, 10,    true, false,
                                      true,   0, -1.0f, 0};
inline void copyQuat(sead::Quatf *dst, const sead::Quatf &src) {
  static_cast<sead::BaseQuat<float> &>(*dst) = src;
}
} // namespace
RouteDokanRider::RouteDokanRider(const char *name, s32 capacity)
    : al::LiveActor(name),
      mRouteSelecter(new PuppetStickRouteSelecter(capacity)) {}
RouteDokanRider::~RouteDokanRider() {}
void RouteDokanRider::init(const al::ActorInitInfo &info) {
  al::initActorWithArchiveName(this, info, "RouteDokanRider", nullptr);
  al::initNerve(this, &NrvRouteDokanRiderStart, 0);
  mRailRider = new al::BlockRailRider;
  mRailRider->setRouteSelecter(mRouteSelecter);
  mInOutEffect = new RouteDokanInOutEffect("ルート土管出入りエフェクト");
  al::initCreateActorNoPlacementInfoNoViewId(mInOutEffect, info);
  makeActorDead();
}
void RouteDokanRider::attackSensor(al::HitSensor *self, al::HitSensor *other) {
  if (!al::isNerve(this, &NrvRouteDokanRiderMove))
    return;
  sead::Vector3f front;
  al::calcFrontDir(&front, this);
  rc::sendMsgRouteDokanPlayerTouch(other, self, front);
  if (al::isSensorRide(self)) {
    rc::sendMsgRouteDokanItemGet(other, rc::getPuppetSensor(mPuppet));
    if (rc::sendMsgRouteDokanPlayerAttack(other,
                                          rc::getPuppetSensor(mPuppet))) {
      mRollTimer = 10;
      rc::startPuppetSe(mPuppet, "PgRouteDokanBlowEnemy");
    }
  }
  if (mReflectTimer > 0)
    return;
  if (al::isSensorName(self, "Touch") && isEnablePlayerRefrect() &&
      rc::sendMsgRouteDokanPlayerReflectNoDamage(
          other, rc::getPuppetSensor(mPuppet))) {
    sead::Vector3f front;
    al::calcFrontDir(&front, this);
    if ((al::getActorTrans(other) - al::getTrans(this)).dot(front) >= 0.0f) {
      al::startHitReaction(this, "ダメージなし反転");
      al::rotateQuatYDirDegree(&mQuat, mQuat, 180.0f);
      mRailRider->reverse();
      mReflectTimer = 5;
    }
  }
  if (al::isSensorName(self, "Reflect") && isEnablePlayerRefrect() &&
      rc::sendMsgRouteDokanPlayerReflect(other, rc::getPuppetSensor(mPuppet))) {
    if (!rc::isPlayerDamageInvalid(rc::getPuppetSensor(mPuppet)))
      rc::damagePuppet(mPuppet);
    al::rotateQuatYDirDegree(&mQuat, mQuat, 180.0f);
    mRailRider->reverse();
    mReflectTimer = 5;
  }
}
bool RouteDokanRider::isEnablePlayerRefrect() const {
  if (al::isNerve(this, &NrvRouteDokanRiderStart))
    return false;
  if (rc::isPlayerInvincible(this, rc::getPuppetSensor(mPuppet)))
    return false;
  return !rc::isPlayerRaccoonDogWhite(rc::getPuppetSensor(mPuppet)) &&
         !rc::isPlayerClimbWhite(rc::getPuppetSensor(mPuppet));
}
bool RouteDokanRider::receiveMsg(const al::SensorMsg *msg, al::HitSensor *other,
                                 al::HitSensor *self) {
  sead::Vector3f otherFront;
  if (rc::tryGetRouteDokanPlayerTouchFront(&otherFront, msg)) {
    sead::Vector3f front;
    al::calcFrontDir(&front, this);
    float dot = front.dot(otherFront);
    if (dot <= -0.5f)
      mRollTimer = 10;
    else if (dot >= 0.5f) {
      float along =
          (al::getActorTrans(other) - al::getActorTrans(self)).dot(front);
      if (al::isNearZero(along, 0.001f))
        mSpeedOffset = other > self ? 4.0f : -4.0f;
      else if (along > 0.0f)
        mSpeedOffset = -3.0f;
      else
        mSpeedOffset = 3.0f;
    }
  }
  return false;
}
void RouteDokanRider::control() {
  if (mRollTimer > 0) {
    mRollAngle = al::modf(mRollAngle + 20.0f + 360.0f, 360.0f) + 0.0f;
    --mRollTimer;
  } else
    mRollAngle = al::convergeDegree(mRollAngle, 0.0f, 20.0f);
  if (mReflectTimer - 1 >= 0)
    --mReflectTimer;
  mSpeedOffset = al::converge(mSpeedOffset, 0.0f, 1.0f);
  al::makeMtxSRT(&mBaseMtx, this);
}
bool RouteDokanRider::damage(al::HitSensor *other) {
  if (alPlayerFunction::findPlayerHolderIndex(other) != mPlayerIndex)
    return false;
  rc::damagePuppet(mPuppet);
  return true;
}
void RouteDokanRider::setMoveSpeed(float speed) { mMoveSpeed = speed; }
void RouteDokanRider::setOutSpeed(float h, float v) {
  mOutSpeedH = h;
  mOutSpeedV = v;
}
void RouteDokanRider::setMoveActionName(const char *name) {
  mMoveActionName = name;
}
void RouteDokanRider::setBaseUpDir(const sead::Vector3f &up) {
  static_cast<sead::BaseVec3<float> &>(mBaseUpDir) = up;
}
bool RouteDokanRider::isActive(int index) const {
  return !al::isDead(this) && mPlayerIndex == index;
}
bool RouteDokanRider::isEndBind() const { return !mPuppet; }
void RouteDokanRider::startBind(RouteDokanEntrance *entrance,
                                al::HitSensor *other, al::HitSensor *self) {
  int index = alPlayerFunction::findPlayerHolderIndex(other);
  startBind(index, entrance, rc::startPuppet(self, other));
}
void RouteDokanRider::startBind(int index, RouteDokanEntrance *entrance,
                                IUsePlayerPuppet *puppet) {
  mPlayerIndex = index;
  mPuppet = puppet;
  mRouteSelecter->addPuppet(puppet);
  rc::calcPuppetQuat(&mStartQuat, mPuppet);
  mStartTrans = rc::getPuppetTrans(mPuppet);
  mQuat = mStartQuat;
  mTrans = mStartTrans;
  al::setTrans(this, mTrans);
  copyQuat(al::getQuatPtr(this), mQuat);
  al::makeMtxSRT(&mBaseMtx, this);
  entrance->startRouteDokanRider(mRailRider);
  rc::forceEndSubActionPuppet(mPuppet);
  rc::killPuppetPrePassLight(mPuppet, "ヘッドライト");
  al::setNerve(this, &NrvRouteDokanRiderStart);
  makeActorAppeared();
}
void RouteDokanRider::addRouteSelectPuppet(IUsePlayerPuppet *puppet) {
  mRouteSelecter->addPuppet(puppet);
}
void RouteDokanRider::tryCancelBind(al::HitSensor *other) {
  if (mPuppet && rc::isPuppetSensor(mPuppet, other)) {
    rc::invalidateMaterialRouteDokan(mPuppet);
    rc::appearPuppetPrePassLight(mPuppet, "ヘッドライト");
    mPuppet = nullptr;
    mRouteSelecter->clearPuppetAll();
    makeActorDead();
  }
}
void RouteDokanRider::forceCancelBind() {
  if (mPuppet) {
    rc::invalidateMaterialRouteDokan(mPuppet);
    rc::appearPuppetPrePassLight(mPuppet, "ヘッドライト");
    mPuppet = nullptr;
    mRouteSelecter->clearPuppetAll();
    makeActorDead();
  }
}
bool RouteDokanRider::canBindSingleMode() {
  return GameDataFunction::isSingleMode(GameDataHolderAccessor(this)) &&
         al::isNerve(this, &NrvRouteDokanRiderInvalid) &&
         al::isGreaterEqualStep(this, 15);
}
void RouteDokanRider::exeStart() {
  if (al::isFirstStep(this)) {
    rc::startPuppetAction(mPuppet, mMoveActionName);
    rc::startPuppetSe(mPuppet, "RouteDokanIn");
    mRollAngle = 0.0f;
    if (GameDataFunction::isSingleMode(GameDataHolderAccessor(this))) {
      auto *window =
          al::tryGetSceneObj<GuideGameWindow>(this, SceneObjID_GuideGameWindow);
      if (window)
        window->setPriorityLimit(GuideMessagePriority(5));
    }
  }
  sead::Vector3f pos = sead::Vector3f::zero;
  sead::Vector3f dir = sead::Vector3f::ez;
  mRailRider->calcPosAndDir(&pos, &dir);
  float rate = al::calcNerveEaseInOutRate(this, 0);
  sead::Vector3f front;
  al::calcQuatFront(&front, mStartQuat);
  if (al::isReverseDirection(dir, front, 0.01f))
    al::rotateQuatYDirDegree(&mQuat, mStartQuat, rate * 180.0f);
  else {
    sead::Quatf quat;
    al::turnQuatZDirRate(&quat, mStartQuat, dir, 1.0f);
    al::slerpQuat(&mQuat, mStartQuat, quat, rate);
  }
  al::lerpVec(&mTrans, mStartTrans, pos, rate);
  sead::Quatf rotated;
  al::rotateQuatZDirDegree(&rotated, mQuat, mRollAngle);
  rc::setPuppetQuat(mPuppet, rotated);
  rc::setPuppetTrans(mPuppet, mTrans);
  al::setTrans(this, mTrans);
  copyQuat(al::getQuatPtr(this), mQuat);
  if (al::isGreaterEqualStep(this, 0)) {
    mInOutEffect->startIn(pos, dir);
    mReflectTimer = 5;
    al::setNerve(this, &NrvRouteDokanRiderMove);
  }
}
void RouteDokanRider::exeMove() {
  if (al::isFirstStep(this))
    rc::validateMaterialRouteDokan(mPuppet);
  sead::Vector3f dir = sead::Vector3f::ez;
  float speed = al::calcNerveValue(this, 0, 1.0f, mMoveSpeed + mSpeedOffset);
  mRailRider->move(speed, &mTrans, &dir);
  if (!al::isParallelDirection(dir, mBaseUpDir, 0.01f)) {
    sead::Vector3f up;
    al::calcQuatUp(&up, mQuat);
    if (al::isReverseDirection(up, mBaseUpDir, 0.01f))
      al::rotateQuatRadian(&mQuat, mQuat, dir, 0.17453292f);
    else
      al::turnQuatYDirRadian(&mQuat, mQuat, mBaseUpDir, 0.17453292f);
  }
  al::turnQuatZDirRate(&mQuat, mQuat, dir, 1.0f);
  sead::Quatf rotated;
  al::rotateQuatZDirDegree(&rotated, mQuat, mRollAngle);
  rc::setPuppetQuat(mPuppet, rotated);
  rc::setPuppetTrans(mPuppet, mTrans);
  al::setTrans(this, mTrans);
  copyQuat(al::getQuatPtr(this), mQuat);
  if (mRailRider->isReachEnd()) {
    rc::invalidateMaterialRouteDokan(mPuppet);
    mOutDir = dir;
    mInOutEffect->startOut(mTrans, dir);
    al::setNerve(this, &NrvRouteDokanRiderEnd);
  }
}
void RouteDokanRider::exeEnd() {
  mTrans = mMoveSpeed * mOutDir + mTrans;
  rc::setPuppetTrans(mPuppet, mTrans);
  if (!al::isParallelDirection(mOutDir, mBaseUpDir, 0.01f)) {
    sead::Vector3f up;
    al::calcQuatUp(&up, mQuat);
    if (al::isReverseDirection(up, mBaseUpDir, 0.01f))
      al::rotateQuatRadian(&mQuat, mQuat, mOutDir, sead::Mathf::pi());
    else
      al::turnQuatYDirRadian(&mQuat, mQuat, mBaseUpDir, sead::Mathf::pi());
  }
  al::turnQuatZDirRate(&mQuat, mQuat, mOutDir, 1.0f);
  mRollAngle = al::convergeDegree(mRollAngle, 0.0f, 180.0f);
  sead::Quatf rotated;
  al::rotateQuatZDirDegree(&rotated, mQuat, mRollAngle);
  rc::setPuppetQuat(mPuppet, rotated);
  al::setTrans(this, mTrans);
  copyQuat(al::getQuatPtr(this), mQuat);
  if (al::isGreaterEqualStep(this, 1)) {
    emitAndEndPuppet(&mPuppet, mOutSpeedH, mOutSpeedV);
    al::setNerve(this, &NrvRouteDokanRiderInvalid);
  }
}
void RouteDokanRider::emitAndEndPuppet(IUsePlayerPuppet **puppet, float h,
                                       float v) {
  if (mPuppet) {
    auto *player = static_cast<PlayerActor *>(
        al::getSensorHost(mPuppet->getMsgTargetSensor()));
    if (player) {
      rc::invalidatePlayerFlash(player);
      rc::invalidatePlayerDamage(player, 20);
      rc::invalidatePlayerDamagePipe(player);
      player->notifyReaction("RouteDokanEnd");
    }
  }
  emitPuppet(puppet, h, v);
  mRouteSelecter->clearPuppet(*puppet);
  rc::appearPuppetPrePassLight(*puppet, "ヘッドライト");
  rc::endBindAndPuppetNull(puppet, &sRouteBindEndParam);
}
void RouteDokanRider::emitPuppet(IUsePlayerPuppet **puppet, float h, float v) {
  rc::startPuppetSe(*puppet, "RouteDokanOut");
  sead::Vector3f velocity(mOutDir.x * h, mOutDir.y * v, mOutDir.z * h);
  rc::setPuppetVelocity(*puppet, velocity);
}
const PlayerBindEndParam *RouteDokanRider::getPlayerBindEndParam() const {
  return &sRouteBindEndParam;
}
void RouteDokanRider::exeInvalid() {
  if (al::isGreaterEqualStep(this, 15)) {
    mPlayerIndex = -1;
    if (!GameDataFunction::isSingleMode(GameDataHolderAccessor(this))) {
      makeActorDead();
      return;
    }
  }
  if (GameDataFunction::isSingleMode(GameDataHolderAccessor(this))) {
    bool end = al::isGreaterEqualStep(this, 48);
    auto *player =
        rc::tryFindActivePlayerActorFirstByUserId(this, mPlayerIndex);
    if (player)
      end |= rc::isPlayerOnGround(player);
    if (end) {
      auto *window =
          al::tryGetSceneObj<GuideGameWindow>(this, SceneObjID_GuideGameWindow);
      if (window)
        window->setPriorityLimit(GuideMessagePriority(0));
      mPlayerIndex = -1;
      makeActorDead();
    }
  }
}
const sead::Matrix34f *RouteDokanRider::getBaseMtx() const { return &mBaseMtx; }
