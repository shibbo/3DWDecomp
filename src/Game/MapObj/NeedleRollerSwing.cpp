#include "MapObj/NeedleRollerSwing.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Audio/System/AudioKeeper.hpp"
#include "Library/Joint/JointControllerKeeper.hpp"
#include "Library/MapObj/SupportFreezeSyncGroupHolder.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Movement/SwingMovement.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Obj/MessageSyncParts.hpp"
#include "Library/Resource/ResourceFunction.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Thread/Functor.hpp"
#include "Library/Yaml/ByamlIter.hpp"
#include "MapObj/ChainModel.hpp"
#include "Project/Collision/CollisionPartsFilterBase.hpp"
#include "Project/Collision/CollisionUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ScoreUtil.hpp"
namespace {
NERVE_DECL(NeedleRollerSwing, Move);
NERVE_DECL(NeedleRollerSwing, StandBy);
NERVE_DECL(NeedleRollerSwing, SupportFreeze);
NERVE_DECL(NeedleRollerSwing, SupportFreezeSync);
NERVES_MAKE_STRUCT(NeedleRollerSwing, Move, StandBy, SupportFreeze,
                   SupportFreezeSync)
} // namespace
NeedleRollerSwing::NeedleRollerSwing(const char *name) : al::LiveActor(name) {}
NeedleRollerSwing::~NeedleRollerSwing() {}
void NeedleRollerSwing::init(const al::ActorInitInfo &info) {
  al::tryGetArg(&mLength, info, "Length");
  int count = sead::Mathi::max(int((mLength + 30.0f) / 60.0f) - 1, 1);
  al::initSubActorKeeperNoFile(this, info, count * 2 + 1);
  al::initActorWithArchiveName(this, info, "NeedleRollerSwing", nullptr);
  al::initNerve(this, &NrvNeedleRollerSwing.Move, 0);
  al::registSupportFreezeSyncGroup(this, info);
  mBaseQuat = al::getQuat(this);
  mSwing = new al::SwingMovement(info);
  mGroundHeight = al::getTrans(this).y - mLength;
  mChains = new al::DeriveActorGroup<ChainModel>("鎖リスト", count * 2);
  for (int i = 0; i < count * 2; ++i) {
    auto *chain = new ChainModel("鎖", "Chain", nullptr);
    al::initCreateActorNoPlacementInfo(chain, info);
    al::setQuat(chain, mBaseQuat);
    mChains->registerActor(chain);
    al::registerSubActorSyncClipping(this, chain, false);
  }
  float groundDistance = -1.0f;
  al::tryGetArg(&groundDistance, info, "DistanceToGround");
  if (groundDistance >= 0.0f) {
    mGroundHeight = al::getTrans(this).y - groundDistance;
    mFindGround = false;
  }
  al::tryGetArg(&mChainWidthOffset, info, "ChainWidthOffset");
  al::calcQuatFront(&mFront, mBaseQuat);
  al::calcQuatSide(&mSide, mBaseQuat);
  const char *modelName = "";
  alPlacementFunction::tryGetModelName(&modelName, info);
  const char *archive = "NeedleRoller4M";
  mWidth = 400.0f;
  al::ByamlIter params(
      al::getBymlFromObjectResource("NeedleRollerSwing", "RollerParam"));
  al::ByamlIter param;
  if (params.tryGetIterByKey(&param, modelName)) {
    param.tryGetStringByKey(&archive, "ArcName");
    param.tryGetFloatByKey(&mWidth, "Length");
  }
  float halfWidth = mWidth * 0.5f - 20.0f;
  float chainWidth = mChainWidthOffset + halfWidth;
  sead::Vector3f offset(mSide.x * chainWidth, mSide.y * chainWidth,
                        chainWidth * mSide.z);
  mLeftHead = new al::LiveActor("根本モデル[左]");
  al::initActorWithArchiveName(mLeftHead, info, "ChainHead", nullptr);
  mLeftHead->appear();
  mLeftHead->getName();
  al::setTrans(mLeftHead, offset + al::getTrans(this));
  mRightHead = new al::LiveActor("根本モデル[右]");
  al::initActorWithArchiveName(mRightHead, info, "ChainHead", nullptr);
  mRightHead->appear();
  mRightHead->getName();
  al::setTrans(mRightHead, al::getTrans(this) - offset);
  mRoller = new al::MessageSyncParts("トゲロール部分", this);
  al::initActorWithArchiveNameNoPlacementInfo(mRoller, info, archive, nullptr);
  al::initJointControllerKeeper(mRoller, 1);
  al::initJointLocalXRotator(mRoller, &mRollAngle, "NeedleRoller");
  al::setQuat(mRoller, mBaseQuat);
  al::invalidateClipping(mRoller);
  al::registerSubActorSyncClipping(this, mRoller, false);
  if (al::listenStageSwitchOnStart(
          this, al::Functor(this, &NeedleRollerSwing::start)))
    al::setNerve(this, &NrvNeedleRollerSwing.StandBy);
  updateParts();
  mRoller->appear();
  float widthSquared = (mWidth * (mWidth * 0.5f)) * 0.5f;
  al::setClippingInfo(this, sead::Mathf::sqrt(mLength * mLength + widthSquared),
                      nullptr);
  al::trySyncStageSwitchAppear(this);
}
void NeedleRollerSwing::start() {
  if (al::isNerve(this, &NrvNeedleRollerSwing.StandBy))
    al::setNerve(this, &NrvNeedleRollerSwing.Move);
}
void NeedleRollerSwing::updateParts() {
  al::rotateQuatXDirDegree(this, mBaseQuat, mSwing->getCurrentAngle());
  al::getQuat(this);
  sead::Vector3f down;
  al::calcQuatUp(&down, this);
  down.negate();
  sead::Vector3f oldPos = al::getTrans(mRoller);
  auto *roller = mRoller;
  const auto &origin = al::getTrans(this);
  al::setTrans(roller, mLength * down + origin);
  if (al::getTrans(mRoller).y < mGroundHeight) {
    mRollSpeed = (al::getTrans(mRoller) - oldPos).dot(mFront) /
                 78.5398178100586f * sead::Mathf::rad2deg(1.0f);
    mRollAngle += mRollSpeed;
    al::getTransPtr(mRoller)->y = mGroundHeight;
    mOnGround = true;
  } else {
    mRollAngle += mRollSpeed;
    mRollSpeed *= 0.96f;
    mOnGround = false;
  }
  float halfWidth = mWidth * 0.5f - 20.0f;
  float chainWidth = mChainWidthOffset + halfWidth;
  sead::Vector3f offset(mSide.x * chainWidth, mSide.y * chainWidth,
                        chainWidth * mSide.z);
  sead::Vector3f delta = al::getTrans(mRoller) - al::getTrans(this);
  int count = mChains->getActorCount() / 2;
  for (int i = 0; i < count; ++i) {
    auto *left = mChains->getDeriveActor(i);
    auto *right = mChains->getDeriveActor(count + i);
    const auto &origin = al::getTrans(this);
    sead::Vector3f pos = delta * (float(i + 1) / float(count + 1)) + origin;
    al::setTrans(left, offset + pos);
    al::setTrans(right, pos - offset);
  }
}
void NeedleRollerSwing::attackSensor(al::HitSensor *self,
                                     al::HitSensor *other) {
  if (al::isHitCylinderSensor(other, self, mSide, 30.0f)) {
    rc::sendMsgNeedleRollerAttack(other, self);
    al::sendMsgEnemyAttack(other, self);
  }
}
bool NeedleRollerSwing::receiveMsg(const al::SensorMsg *msg,
                                   al::HitSensor *other, al::HitSensor *self) {
  if ((al::isMsgPlayerInvincibleAttack(msg) ||
       al::isMsgPlayerInvincibleTouch(msg) ||
       al::isMsgPlayerObjStatueDrop(msg) || al::isMsgPlayerStatueTouch(msg) ||
       al::isMsgPlayerGiantAttack(msg) || al::isMsgLaserAttack(msg) ||
       al::isMsgBlockUpperPunch(msg)) &&
      al::isHitCylinderSensor(other, self, mSide, 30.0f)) {
    rc::requestHitReactionToAttacker(msg, self, other);
    rc::addScoreCombo(mRoller, other, msg, 0.0f);
    al::setAppearItemAttackerSensor(mRoller, other);
    al::appearItem(mRoller);
    doBreak();
    return true;
  }
  if ((al::isMsgPlayerFireBallAttack(msg) &&
       al::isHitCylinderSensor(other, self, mSide, 30.0f)) ||
      ((al::isMsgKickKouraBreak(msg) || al::isMsgPlayerBoomerangBreak(msg)) &&
       (al::isSensorCollision(self) ||
        al::isHitCylinderSensor(other, self, mSide, 30.0f)))) {
    rc::requestHitReactionToAttacker(msg, self, other);
    return true;
  }
  if (al::isMsgIsNerveSupportFreeze(msg))
    return al::isNerve(this, &NrvNeedleRollerSwing.SupportFreeze);
  if (al::isMsgOnSyncSupportFreeze(msg) &&
      al::isNerve(this, &NrvNeedleRollerSwing.Move)) {
    al::setNerve(this, &NrvNeedleRollerSwing.SupportFreezeSync);
    return true;
  }
  if (al::isMsgOffSyncSupportFreeze(msg) &&
      al::isNerve(this, &NrvNeedleRollerSwing.SupportFreezeSync)) {
    al::setNerve(this, &NrvNeedleRollerSwing.Move);
    return true;
  }
  return false;
}
void NeedleRollerSwing::doBreak() {
  int count = mChains->getActorCount();
  for (int i = 0; i < count; ++i) {
    auto *chain = mChains->getDeriveActor(i);
    float angle =
        sead::Mathf::deg2rad(float(i) * 72.0f + al::getRandom(-20.0f, 20.0f));
    float x = sead::Mathf::cos(angle) * 5.0f;
    float z = sead::Mathf::sin(angle) * -5.0f;
    sead::Vector3f velocity;
    velocity.x = x;
    velocity.y = 35.0f;
    velocity.z = z;
    chain->startFall(velocity);
  }
  al::startHitReactionBreak(mRoller);
  al::startHitReactionBreak(mLeftHead);
  mLeftHead->kill();
  al::startHitReactionBreak(mRightHead);
  mRightHead->kill();
  mRoller->getAudioKeeper()->update();
  mRoller->kill();
  kill();
}
bool NeedleRollerSwing::receiveMsgScreenPoint(const al::SensorMsg *msg,
                                              al::ScreenPointer *,
                                              al::ScreenPointTarget *) {
  if (!al::isMsgTouchAssist(msg))
    return false;
  mFreezeTimer = 60;
  if (al::isNerve(this, &NrvNeedleRollerSwing.Move))
    al::setNerve(this, &NrvNeedleRollerSwing.SupportFreeze);
  return true;
}
void NeedleRollerSwing::initAfterPlacement() {
  if (!mFindGround)
    return;
  al::CollisionPartsFilterActor filter(mRoller);
  sead::Vector3f hit;
  if (alCollisionUtil::getFirstPolyOnArrow(
          this, &hit, nullptr, al::getTrans(this),
          sead::Vector3f::ey * (-50.0f - mLength), &filter, nullptr))
    mGroundHeight = hit.y + 50.0f;
}
void NeedleRollerSwing::exeStandBy() { updateParts(); }
void NeedleRollerSwing::exeMove() {
  if (al::isFirstStep(this)) {
    al::startAction(mRoller, "Wait");
    mOnGround = false;
  }
  mSwing->updateNerve();
  bool wasOnGround = mOnGround;
  updateParts();
  if (wasOnGround != mOnGround) {
    if (mOnGround) {
      al::startHitReaction(mRoller, "接地");
      al::startAction(mRoller, "Ground");
    } else
      al::startAction(mRoller, "Wait");
  }
  if (mOnGround && al::isExistSeKeeper(mRoller)) {
    float speed = mRollSpeed > 0.0f ? mRollSpeed : -mRollSpeed;
    float volume = speed / 15.0f;
    if (volume > 0.3f) {
      float tempo = speed / 17.0f + 0.2f;
      al::holdSeSetVolumeTempoByName(mRoller, "プログラム呼び出し", volume,
                                     tempo);
    }
  }
}
void NeedleRollerSwing::exeSupportFreeze() {
  if (al::isFirstStep(this))
    al::startAction(mRoller, "SupportFreeze");
  if (--mFreezeTimer <= 0)
    al::setNerve(this, &NrvNeedleRollerSwing.Move);
}
void NeedleRollerSwing::exeSupportFreezeSync() {
  if (al::isFirstStep(this))
    al::startAction(mRoller, "SupportFreeze");
}
