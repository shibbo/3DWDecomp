#include "MapObj/HeadgearPoseBuilder.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/LiveActor/LiveActorFunc.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Math/MatrixUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Shadow/Common/ShadowMaskDrawCategory.hpp"
#include "Player/IUsePlayerKeyConfig.hpp"
#include "Project/AreaObj/AreaObjUtil.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/PlayerUtil.hpp"

namespace {
const sead::Vector3f sClimbOffset(0.0f, 0.0f, -25.0f);
const sead::Vector3f sClimbSquatOffset(0.0f, -20.0f, -25.0f);
const sead::Vector3f sClimbWalkOffset(0.0f, 0.0f, -15.0f);
const sead::Vector3f sKinopioClimbOffset(0.0f, 5.0f, 0.0f);
const HeadgearPoseBuilderParam sHeadgearDefaultParam = {false, false, true};
NERVE_DECL(HeadgearPoseBuilder, Wait);
class HeadgearPoseBuilderNrvSnapRight : public al::Nerve {
public:
  void execute(al::NerveKeeper *keeper) const override {
    keeper->getParent<HeadgearPoseBuilder>()->exeSnap();
  }
};
class HeadgearPoseBuilderNrvSnapLeft : public al::Nerve {
public:
  void execute(al::NerveKeeper *keeper) const override {
    keeper->getParent<HeadgearPoseBuilder>()->exeSnap();
  }
};
class HeadgearPoseBuilderNrvSnapWait : public al::Nerve {
public:
  void execute(al::NerveKeeper *keeper) const override {
    keeper->getParent<HeadgearPoseBuilder>()->exeWait();
  }
};
NERVES_MAKE_STRUCT(HeadgearPoseBuilder, Wait, SnapRight, SnapLeft, SnapWait)

bool isPlayerActionPlaying(const char *name, const al::HitSensor *sensor) {
  const char *action = al::getActionName(al::getSensorHost(sensor));
  if (al::isEqualString(action, name))
    return true;
  const char *character =
      rc::getPlayerCharacterName(rc::getPlayerCharaType(sensor));
  if (al::isEqualString(action,
                        al::StringTmp<128>("%s%s", character, name).cstr()))
    return true;
  const char *prefix;
  if (rc::isPlayerMini(sensor))
    prefix = "Mini";
  else if (rc::isPlayerClimbOrClimbSpecial(sensor))
    prefix = "Climb";
  else if (rc::isPlayerRaccoonDog(sensor))
    prefix = "RaccoonDog";
  else if (rc::isPlayerRaccoonDogWhite(sensor))
    prefix = "RaccoonDogWhite";
  else if (rc::isPlayerClimbWhite(sensor))
    prefix = "Climb";
  else if (rc::isPlayerClimbGiga(sensor))
    prefix = "Climb";
  else if (rc::isPlayerFire(sensor))
    prefix = "Fire";
  else if (rc::isPlayerBoomerang(sensor))
    prefix = "Boomerang";
  else
    return false;
  if (al::isEqualString(action,
                        al::StringTmp<128>("%s%s", prefix, name).cstr()))
    return true;
  return al::isEqualString(
      action, al::StringTmp<128>("%s%s%s", character, prefix, name).cstr());
}
bool isPlayerTreeActionPlaying(const al::HitSensor *sensor) {
  return isPlayerActionPlaying("TreeCatch", sensor) ||
         isPlayerActionPlaying("TreeClimb", sensor) ||
         isPlayerActionPlaying("TreeFall", sensor) ||
         isPlayerActionPlaying("TreeTurn", sensor) ||
         isPlayerActionPlaying("TreeWait", sensor) ||
         isPlayerActionPlaying("TreeHandStandEnd", sensor) ||
         isPlayerActionPlaying("TreeHandStandStart", sensor) ||
         isPlayerActionPlaying("TreeHandStandTurn", sensor) ||
         isPlayerActionPlaying("TreeHandStandWait", sensor);
}
bool isClimbPose(const al::HitSensor *sensor) {
  return isPlayerActionPlaying("ClimbClimbSlidingAttack", sensor) ||
         isPlayerActionPlaying("ClimbJumpBackStart", sensor) ||
         isPlayerActionPlaying("ClimbMove", sensor) ||
         isPlayerActionPlaying("ClimbSquatEnd", sensor) ||
         isPlayerActionPlaying("ClimbSquatStart", sensor) ||
         isPlayerActionPlaying("ClimbSquatWait", sensor) ||
         isPlayerActionPlaying("ClimbSquatWalk", sensor);
}
bool isRollingPose(const al::HitSensor *sensor) {
  return isPlayerActionPlaying("ClimbRollingAttack", sensor) ||
         isPlayerActionPlaying("RollingAir", sensor) ||
         isPlayerActionPlaying("RollingAirShort", sensor);
}
bool isUprightPose(const al::HitSensor *sensor,
                   const HeadgearPoseBuilderParam *param) {
  return isPlayerActionPlaying("Brake", sensor) ||
         isPlayerActionPlaying("CannonFlyLand", sensor) ||
         isPlayerActionPlaying("ClimbClimbSlidingAttack", sensor) ||
         isPlayerActionPlaying("ClimbGrooming", sensor) ||
         isPlayerActionPlaying("ClimbLand", sensor) ||
         isPlayerActionPlaying("ClimbMove", sensor) ||
         isPlayerActionPlaying("ClimbTurn", sensor) ||
         isPlayerActionPlaying("ClimbTurnPoint", sensor) ||
         isPlayerActionPlaying("ClimbWait", sensor) ||
         (param->_0 && (isPlayerActionPlaying("Land", sensor) ||
                        isPlayerActionPlaying("Move", sensor) ||
                        isPlayerActionPlaying("SwimStand", sensor) ||
                        isPlayerActionPlaying("SwimStandMove", sensor) ||
                        isPlayerActionPlaying("SwimStandSurface", sensor) ||
                        isPlayerActionPlaying("SwimStandWait", sensor) ||
                        isPlayerActionPlaying("SwimWait", sensor) ||
                        isPlayerActionPlaying("Turn", sensor) ||
                        isPlayerActionPlaying("Wait", sensor) ||
                        isPlayerActionPlaying("WaitBattle", sensor)));
}
} // namespace

HeadgearPoseBuilder::HeadgearPoseBuilder(al::LiveActor *host,
                                         const HeadgearPoseBuilderParam *param)
    : al::NerveExecutor("被り物姿勢制御"), mHost(host), mChildActor(nullptr),
      mPlayerSensor(nullptr), mParam(param) {
  if (!mParam)
    mParam = &sHeadgearDefaultParam;
  initNerve(&NrvHeadgearPoseBuilder.Wait, 0);
  if (al::isExistShadow(mHost))
    al::invalidateShadow(mHost, al::ShadowMaskDrawCategory(4));
}
void HeadgearPoseBuilder::start(al::HitSensor *sensor) {
  mPlayerSensor = sensor;
  if (al::isExistShadow(mHost)) {
    al::validateShadow(mHost);
    al::invalidateShadow(mHost, al::ShadowMaskDrawCategory(0));
  }
}
void HeadgearPoseBuilder::update() {
  al::LiveActor *player = al::getSensorHost(mPlayerSensor);
  updateNerve();
  bool hide = rc::isPlayerHideModel(mPlayerSensor);
  bool hidden = al::isHideModel(mHost);
  if (hide) {
    if (!hidden) {
      al::hideModel(mHost);
      if (mHost->getEffectKeeper()) {
        mEffectEnabled = false;
        al::offCalcAndDrawEffect(mHost);
      }
    }
  } else {
    if (hidden)
      al::showModel(mHost);
    if (mHost->getEffectKeeper()) {
      if (mEffectEnabled) {
        if (isPlayerActionPlaying("DokanIn") &&
            s32(al::getActionFrame(player)) >= 23) {
          mEffectEnabled = false;
          al::offCalcAndDrawEffect(mHost);
        }
      } else if (!(isPlayerActionPlaying("DokanOutUpsideDown") &&
                   al::getActionFrame(player) < 10.0f) &&
                 !(isPlayerActionPlaying("DokanOut") &&
                   al::getActionFrame(player) < 3.0f) &&
                 !(isPlayerActionPlaying("DokanIn") &&
                   s32(al::getActionFrame(player)) >= 23)) {
        mEffectEnabled = true;
        al::onCalcAndDrawEffect(mHost);
      }
    }
  }
  bool hideSilhouette = rc::isPlayerHideSilhouette(mPlayerSensor);
  bool silhouetteHidden = al::isSilhouetteModelHidden(mHost);
  if (hideSilhouette) {
    if (!silhouetteHidden)
      al::hideSilhouetteModel(mHost);
  } else if (silhouetteHidden)
    al::showSilhouetteModel(mHost);
  if (al::isExistShadow(mHost)) {
    if (!isPlayerTreeActionPlaying(mPlayerSensor) &&
        !isPlayerRaidonActionPlaying() && !al::isHideModel(mHost))
      al::showShadow(mHost, al::ShadowMaskDrawCategory(4));
    else if (isPlayerTreeActionPlaying(mPlayerSensor) ||
             isPlayerRaidonActionPlaying() || al::isHideModel(mHost))
      al::hideShadow(mHost, al::ShadowMaskDrawCategory(4));
  }
  if (mParam->_2)
    al::setShadowDropLengthWithDrawCategory(
        mHost, rc::getPlayerShadowLength(mPlayerSensor),
        al::ShadowMaskDrawCategory(4));
  sead::Vector3f rootPos(0.0f, 0.0f, 0.0f);
  rc::calcPlayerModelJointPos(&rootPos, mPlayerSensor, "JointRoot");
  if (rootPos.isNan())
    return;
  if (mParam->_1 && !rc::isPlayerBinded(mPlayerSensor) &&
      rc::isInAreaObj(mHost, rc::AreaObjType::HeadgearWidenStickXSnapArea)) {
    al::setTrans(mHost, rootPos);
    mClimbPose = 0;
  } else if (isClimbPose(mPlayerSensor)) {
    sead::Vector3f offset = sClimbOffset;
    if (isPlayerActionPlaying("ClimbSquatStart") ||
        isPlayerActionPlaying("ClimbSquatWait") ||
        isPlayerActionPlaying("ClimbSquatEnd")) {
      static_cast<sead::BaseVec3<f32> &>(offset) = sClimbSquatOffset;
      mClimbPose = 2;
    } else {
      if (mClimbPose == 2)
        mClimbPoseBlendFrames = rc::isPlayerCharaKinopio(mPlayerSensor) ? 3 : 7;
      if (mClimbPoseBlendFrames >= 1) {
        f32 rate = al::normalize(f32(7 - mClimbPoseBlendFrames), 0.0f, 7.0f);
        al::lerpVec(&offset, sClimbSquatOffset, sClimbOffset, rate);
        --mClimbPoseBlendFrames;
      }
      mClimbPose = 1;
    }
    sead::Vector3f headPos(0.0f, 0.0f, 0.0f);
    rc::calcPlayerModelJointPos(&headPos, mPlayerSensor, "Head");
    sead::Vector3f front(0.0f, 0.0f, 0.0f);
    sead::Matrix34f pose = sead::Matrix34f::ident;
    al::calcFrontDir(&front, player);
    front.y = 0.0f;
    al::makeMtxFrontUpPos(&pose, front, sead::Vector3f::ey, headPos);
    al::updatePoseMtx(mHost, &pose);
    al::calcTransLocalOffset(al::getTransPtr(mHost), mHost, offset);
  } else if (isRollingPose(mPlayerSensor)) {
    sead::Matrix34f pose = sead::Matrix34f::ident;
    const sead::Matrix34f *joint =
        rc::getPlayerModelJointMtxPtr(mPlayerSensor, "Head");
    sead::Vector3f front(0.0f, 0.0f, 0.0f), pos(0.0f, 0.0f, 0.0f);
    front.x = joint->m[0][0];
    front.y = joint->m[1][0];
    front.z = joint->m[2][0];
    sead::Vector3f up(joint->m[0][1], joint->m[1][1], joint->m[2][1]);
    rc::calcPlayerModelJointPos(&pos, mPlayerSensor, "Head");
    sead::Vector3f down = -up;
    al::makeMtxFrontUpPos(&pose, front, down, pos);
    al::updatePoseMtx(mHost, &pose);
    mClimbPose = 0;
  } else {
    if (isUprightPose(mPlayerSensor, mParam)) {
      sead::Vector3f front(0.0f, 0.0f, 0.0f);
      sead::Matrix34f pose = sead::Matrix34f::ident;
      al::calcFrontDir(&front, player);
      front.y = 0.0f;
      al::makeMtxFrontUpPos(&pose, front, sead::Vector3f::ey, rootPos);
      al::updatePoseMtx(mHost, &pose);
    } else
      al::updatePoseMtx(
          mHost, rc::getPlayerModelJointMtxPtr(mPlayerSensor, "JointRoot"));
    mClimbPose = 0;
  }
  if (isPlayerActionPlaying("DokanSideIn")) {
    ++mDokanInFrames;
    f32 rate = al::normalize(f32(mDokanInFrames), 0.0f, 20.0f);
    f32 height = rc::isPlayerMini(mPlayerSensor) ? 0.0f : -35.0f;
    al::calcTransLocalOffset(
        al::getTransPtr(mHost), mHost,
        sead::Vector3f(0.0f, al::lerpValue(rate, 0.0f, height), 0.0f));
    f32 shrink = mChildActor ? 0.5f : 0.7f;
    al::setScaleAll(mHost, al::lerpValue(rate, 1.0f, shrink));
    if (mChildActor)
      al::setScaleAll(mChildActor, al::lerpValue(rate, 1.0f, shrink));
  } else if (mDokanInFrames > 0) {
    mDokanInFrames = 0;
    al::setScaleAll(mHost, 1.0f);
    if (mChildActor)
      al::setScaleAll(mChildActor, 1.0f);
  }
  if (isPlayerActionPlaying("ClimbMove")) {
    if (mClimbMoveFrames <= 9) {
      f32 offset = al::lerpValue(
          al::normalize(f32(mClimbMoveFrames), 0.0f, 10.0f), 0.0f, -10.0f);
      auto *trans = al::getTransPtr(mHost);
      trans->y = offset + trans->y;
      ++mClimbMoveFrames;
    } else
      al::getTransPtr(mHost)->y += -10.0f;
  } else if (mClimbMoveFrames >= 1) {
    f32 offset = al::lerpValue(
        al::normalize(f32(mClimbMoveFrames), 0.0f, 10.0f), 0.0f, -10.0f);
    auto *trans = al::getTransPtr(mHost);
    trans->y = offset + trans->y;
    --mClimbMoveFrames;
  }
  if (rc::isPlayerClimbOrClimbSpecial(mPlayerSensor)) {
    if (isPlayerActionPlaying("ClimbSquatWalk"))
      al::calcTransLocalOffset(al::getTransPtr(mHost), mHost, sClimbWalkOffset);
    else if (rc::isPlayerCharaKinopio(mPlayerSensor))
      al::calcTransLocalOffset(al::getTransPtr(mHost), mHost,
                               sKinopioClimbOffset);
  }
  if (mParam->_1 &&
      rc::isInAreaObj(mHost, rc::AreaObjType::HeadgearWidenStickXSnapArea)) {
    sead::Vector2f stick(0.0f, 0.0f);
    rc::getPlayerKeyConfig(mPlayerSensor)->calcLeftStick(&stick);
    if (!al::isNearZero(stick.x, 0.001f) && mFacingRight != (stick.x > 0.0f)) {
      mFacingRight = !mFacingRight;
      al::setNerve(this, mFacingRight ? static_cast<const al::Nerve *>(
                                            &NrvHeadgearPoseBuilder.SnapRight)
                                      : &NrvHeadgearPoseBuilder.SnapLeft);
    }
  }
}
bool HeadgearPoseBuilder::isPlayerActionPlaying(const char *name) const {
  return ::isPlayerActionPlaying(name, mPlayerSensor);
}
bool HeadgearPoseBuilder::isPlayerRaidonActionPlaying() const {
  return isPlayerActionPlaying("RaidonMove") ||
         isPlayerActionPlaying("RaidonGetOn");
}
const char *HeadgearPoseBuilder::getPlayerActionName() const {
  return al::getActionName(al::getSensorHost(mPlayerSensor));
}
bool HeadgearPoseBuilder::isSnapping() const {
  return al::isNerve(this, &NrvHeadgearPoseBuilder.SnapRight) ||
         al::isNerve(this, &NrvHeadgearPoseBuilder.SnapLeft);
}
void HeadgearPoseBuilder::exeWait() {
  if (!mParam->_1)
    return;
  if (al::isNerve(this, &NrvHeadgearPoseBuilder.Wait) &&
      !rc::isPlayerBinded(mPlayerSensor) &&
      rc::isInAreaObj(mHost, rc::AreaObjType::HeadgearWidenStickXSnapArea)) {
    al::setNerve(this, &NrvHeadgearPoseBuilder.SnapRight);
    return;
  }
  bool inArea =
      rc::isInAreaObj(mHost, rc::AreaObjType::HeadgearWidenStickXSnapArea);
  if (al::isNerve(this, &NrvHeadgearPoseBuilder.SnapWait) && mParam->_1 &&
      !rc::isPlayerBinded(mPlayerSensor) &&
      rc::isInAreaObj(mHost, rc::AreaObjType::HeadgearWidenStickXSnapArea))
    updateSnapPose();
  if (al::isNerve(this, &NrvHeadgearPoseBuilder.SnapWait))
    al::setNerve(this, &NrvHeadgearPoseBuilder.Wait);
  else if (al::isNerve(this, &NrvHeadgearPoseBuilder.Wait) && inArea)
    al::setNerve(this, &NrvHeadgearPoseBuilder.SnapWait);
}
void HeadgearPoseBuilder::updateSnapPose() {
  auto *area = rc::tryFindAreaObj(
      mHost, rc::AreaObjType::HeadgearWidenStickXSnapArea, al::getTrans(mHost));
  sead::Vector3f front(0.0f, 0.0f, 0.0f);
  const auto &base = al::getAreaObjBaseMtx(area);
  front.x = base.m[0][0];
  front.y = base.m[1][0];
  front.z = base.m[2][0];
  al::rotateVectorDegreeY(&front, 360.0f - mSnapAngle);
  sead::Matrix34f pose = sead::Matrix34f::ident;
  al::makeMtxFrontUpPos(&pose, front, sead::Vector3f::ey, al::getTrans(mHost));
  al::updatePoseMtx(mHost, &pose);
}
void HeadgearPoseBuilder::exeSnap() {
  if (!(mParam->_1 && !rc::isPlayerBinded(mPlayerSensor) &&
        rc::isInAreaObj(mHost, rc::AreaObjType::HeadgearWidenStickXSnapArea))) {
    al::setNerve(
        this,
        rc::isInAreaObj(mHost, rc::AreaObjType::HeadgearWidenStickXSnapArea)
            ? static_cast<const al::Nerve *>(&NrvHeadgearPoseBuilder.SnapWait)
            : &NrvHeadgearPoseBuilder.Wait);
    return;
  }
  f32 step = mFacingRight ? -12.5f : 12.5f;
  f32 angle = mSnapAngle + step;
  if (angle < 0.0f)
    angle = 0.0f;
  else if (angle > 180.0f)
    angle = 180.0f;
  mSnapAngle = angle;
  updateSnapPose();
  if (mSnapAngle < 0.1f || mSnapAngle >= 179.9f)
    al::setNerve(this, &NrvHeadgearPoseBuilder.SnapWait);
}
void HeadgearPoseBuilder::updateAndCalcAnimDirect() {
  update();
  alLiveActorFunction::calcAnimDirect(mHost);
}
