#include "MapObj/BoxPropeller.hpp"
#include "Layout/GuideGameWindow.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/LiveActor/LiveActorFunc.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Shadow/Common/ShadowUtil.hpp"
#include "MapObj/BoxPropellerPropeller.hpp"
#include "MapObj/HeadgearPoseBuilder.hpp"
#include "MapObj/HeadgearStateBlow.hpp"
#include "Player/IUsePlayerKeyConfig.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Collision/CollisionUtil.hpp"
#include "Project/Controller/PadRumbleKeeper.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/InputUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include <cmath>
#include <prim/seadDelegate.h>
namespace {
NERVE_DECL(BoxPropeller, Wait);
NERVE_DECL(BoxPropeller, Kill);
NERVE_DECL(BoxPropeller, Appear);
NERVE_DECL(BoxPropeller, Carry);
const BoxPropellerNrvCarry NrvBoxPropellerCarry;
const struct {
  BoxPropellerNrvWait Wait;
  BoxPropellerNrvKill Kill;
  BoxPropellerNrvAppear Appear;
} NrvBoxPropeller;
const HeadgearPoseBuilderParam sPropellerPoseParam{false, false, false};
} // namespace
int BoxPropeller::sPropellerJumps = 0;
BoxPropeller::BoxPropeller(const char *name) : al::LiveActor(name) {}
BoxPropeller::~BoxPropeller() {}
void BoxPropeller::init(const al::ActorInitInfo &info) {
  mSingle = al::isSingleMode(info);
  al::initActorWithArchiveName(this, info, "BoxPropeller",
                               mSingle ? "SM" : nullptr);
  if (mSingle)
    alActorSystemFunction::tryCompletelyRemoveFromExecutorDraw(this, &mDrawers);
  al::initNerve(this, &NrvBoxPropeller.Wait, 1);
  mPoseBuilder = new HeadgearPoseBuilder(this, &sPropellerPoseParam);
  mBlowState = new HeadgearStateBlow(this, nullptr);
  al::initNerveState(this, mBlowState, &NrvBoxPropeller.Kill, "吹き飛び状態");
  mPropeller = new BoxPropellerPropeller(this, "Propeller", info, "プロペラ");
  al::invalidateClipping(mPropeller);
  rc::setPlayerColorAnimDefault(this, "BoxPropeller");
  mPoseBuilder->setChildActor(mPropeller);
  mRumble = al::createPadRumbleKeeper(this, -1);
  mPlaced = al::isPlaced(info);
  if (mPlaced)
    makeActorAppeared();
  else {
    makeActorDead();
    mPropeller->makeActorDead();
  }
  al::offCollide(this);
  al::hideSilhouetteModel(this);
  al::hideSilhouetteModel(mPropeller);
  mDelegate =
      new sead::Delegate<BoxPropeller>(this, &BoxPropeller::updateInChangeDemo);
}
void BoxPropeller::validateWaitShadow() {}
void BoxPropeller::validateBodyShadow() {}
void BoxPropeller::updateInChangeDemo() {
  mPoseBuilder->update();
  alLiveActorFunction::calcAnimDirect(this);
  mPropeller->control();
  alLiveActorFunction::calcAnimDirect(mPropeller);
}
void BoxPropeller::appear() {
  if (mSingle)
    al::setShadowDropLength(this, 500.0f);
  al::LiveActor::appear();
  al::validateHitSensors(this);
  al::validateClipping(this);
  al::validateCollisionParts(this);
  al::offCollide(this);
  al::setNerve(this, &NrvBoxPropeller.Appear);
  al::invalidateClipping(mPropeller);
  al::copyPose(mPropeller, this);
  mPropeller->appear();
  mGlobalAlphaLastFrame = 1.0f;
  setGlobalAlphaPtr(&mGlobalAlphaLastFrame);
  mPropeller->setGlobalAlphaPtr(&mGlobalAlphaLastFrame);
}
void BoxPropeller::respawn() {
  if (isCarry())
    return;
  if (mPlaced && al::isDead(this))
    appear();
  if (!mPlaced && !al::isDead(this)) {
    al::validateClipping(mPropeller);
    kill();
  }
}
void BoxPropeller::kill() {
  if (mAlphaLinked)
    mAlphaLinked = false;
  al::LiveActor::kill();
  mPropeller->kill();
}
void BoxPropeller::startClipped() {
  al::LiveActor::startClipped();
  if (!al::isClipped(mPropeller))
    mPropeller->startClipped();
}
void BoxPropeller::endClipped() {
  al::LiveActor::endClipped();
  if (al::isClipped(mPropeller))
    mPropeller->endClipped();
}
bool BoxPropeller::hideActor() {
  if (isCarry())
    return false;
  return al::LiveActor::hideActor();
}
bool BoxPropeller::showActor() { return al::LiveActor::showActor(); }
void BoxPropeller::pause() {
  mPauseFrames = 60;
  alActorSystemFunction::addBackToExecutorDraw(this, &mDrawers);
  mPropeller->pause();
}
void BoxPropeller::resume() {
  mPauseFrames = 0;
  alActorSystemFunction::removeFromExecutorDraw(this, &mDrawers);
  mPropeller->resume();
}
void BoxPropeller::attackSensor(al::HitSensor *self, al::HitSensor *other) {
  if (!al::isSingleMode(this))
    return;
  if (al::isSensorEnemy(other)) {
    if (al::isSensorName(other, "Push"))
      al::sendMsgPush(other, self);
  } else if (al::isSensorKoopaJr(other)) {
    sead::Vector3f diff = al::getSensorPos(other) - al::getSensorPos(self);
    if (diff.squaredLength() > 0.0001f &&
        diff.y / sqrtf(diff.squaredLength()) < -0.707106769f) {
      auto *velocity = al::getVelocityPtr(al::getSensorHost(other));
      if (velocity->y > -3.0f)
        velocity->y = -3.0f;
    }
  }
}
bool BoxPropeller::receiveMsg(const al::SensorMsg *msg, al::HitSensor *other,
                              al::HitSensor *self) {
  if (!al::isSensorPlayerOrPlayerWeapon(other))
    return false;
  if (mPlayerSensor && mBlowState->tryStart(msg, mPlayerSensor)) {
    deletePropellerEffect();
    mPoseBuilder->clearPlayerSensor();
    mPlayerSensor = nullptr;
    if (mSingle) {
      rc::disappearGuideGameWindow(this);
      if (mGuideShown && arePropellerJumpsComplete())
        SingleModeDataFunction::setGuideMessageSeen(
            GameDataHolderWriter(GameDataHolderAccessor(this)), 2, false);
    }
    mPropeller->setAlignUp(false);
    al::setNerve(this, &NrvBoxPropeller.Kill);
    return true;
  }
  if (al::isNerve(this, &NrvBoxPropeller.Wait)) {
    if (al::isMsgPlayerObjUpperPunch(msg))
      return !tryShiftCarry(other, self);
    if (al::isMsgPlayerFloorTouch(msg))
      mFloorTouch = true;
  } else if (al::isNerve(this, &NrvBoxPropeller.Appear) &&
             al::isMsgPlayerObjUpperPunch(msg))
    return !tryShiftCarry(other, self);
  return false;
}
void BoxPropeller::deletePropellerEffect() {
  if (al::isEffectEmitting(mPropeller, "Rise"))
    al::deleteEffect(mPropeller, "Rise");
  if (al::isEffectEmitting(mPropeller, "Glide"))
    al::deleteEffect(mPropeller, "Glide");
}
bool BoxPropeller::arePropellerJumpsComplete() { return sPropellerJumps > 2; }
bool BoxPropeller::tryShiftCarry(al::HitSensor *other, al::HitSensor *self) {
  if (!rc::tryPlayerEquipHeadgear(other, self, mDelegate, 1))
    return false;
  mPlayerSensor = other;
  if (mSingle) {
    setGlobalAlphaPtr(&al::getSensorHost(other)->mGlobalAlphaLastFrame);
    mPropeller->setGlobalAlphaPtr(
        &al::getSensorHost(mPlayerSensor)->mGlobalAlphaLastFrame);
    mAlphaLinked = true;
  }
  mPoseBuilder->start(other);
  al::invalidateClipping(this);
  al::invalidateHitSensors(this);
  al::showSilhouetteModel(this);
  al::showSilhouetteModel(mPropeller);
  al::invalidateCollisionParts(this);
  mRumble->setPort(rc::getPlayerInputPort(other));
  al::setNerve(this, &NrvBoxPropellerCarry);
  return true;
}
void BoxPropeller::control() {
  if (al::isSklAnimPlaying(this, "Wait", 0))
    decideAnimRate();
  else
    setSklAnimFrameRate(1.0f);
  if (mGuideType != 0 && rc::isControllerAssignmentChanged()) {
    if (al::isPadTypeJoySingle(al::getMainControllerPort())) {
      if (rc::appearGuideGameWindowWithPriority(
              this, "SingleMode_GuideMessage", "PropellerGuide_SingleJoycons",
              GuideMessagePriority(1), -1, 0.0f))
        mGuideType = 1;
    } else {
      if (rc::appearGuideGameWindowWithPriority(
              this, "SingleMode_GuideMessage", "PropellerGuide_DualJoycons",
              GuideMessagePriority(1), -1, 0.0f))
        mGuideType = 2;
    }
  }
}
void BoxPropeller::decideAnimRate() {
  bool carry = isCarry();
  if (carry && rc::isPlayerPropellerRising(mPlayerSensor))
    mAnimRate += (12.0f - mAnimRate) * 0.1f;
  else if (carry && rc::isPlayerPropellerJumping(mPlayerSensor)) {
    if (rc::getPlayerKeyConfig(mPlayerSensor)->isPadHoldPlayerJump())
      mAnimRate += (6.0f - mAnimRate) * 0.1f;
    else
      mAnimRate += (3.0f - mAnimRate) * 0.1f;
  } else
    mAnimRate += (1.0f - mAnimRate) * 0.1f;
  setSklAnimFrameRate(mAnimRate);
}
void BoxPropeller::setSklAnimFrameRate(float rate) {
  al::setSklAnimFrameRate(this, rate, 0);
  if (mPropeller->isPaused()) {
    int next = mPauseFrames - 1;
    if (next >= 0) {
      mPauseFrames = next;
      float rate = al::lerpValue(float(next) / 60.0f, 0.0f, 1.0f);
      al::setSklAnimFrameRate(mPropeller, rate, 0);
    } else
      al::setSklAnimFrameRate(mPropeller, 0.0f, 0);
  } else
    al::setSklAnimFrameRate(mPropeller, rate, 0);
}
void BoxPropeller::exeAppear() {
  if (al::isFirstStep(this)) {
    if (mAppearFromHipDrop)
      startAction("AppearHipDrop");
    else
      startAction("Appear");
    al::hideSilhouetteModel(this);
    al::hideSilhouetteModel(mPropeller);
  }
  if (al::isActionEnd(this))
    al::setNerve(this, &NrvBoxPropeller.Wait);
}
void BoxPropeller::startAction(const char *name) {
  al::startAction(this, name);
  if (mHasPropellerAction) {
    if (!al::isEqualString(al::getActionName(mPropeller), name) ||
        (al::isActionOneTime(mPropeller, al::getActionName(mPropeller)) &&
         al::isActionEnd(mPropeller)))
      al::tryStartAction(mPropeller, name);
  } else
    mHasPropellerAction |= al::tryStartAction(mPropeller, name);
}
void BoxPropeller::exeWait() {
  if (al::isFirstStep(this)) {
    al::hideSilhouetteModel(this);
    al::hideSilhouetteModel(mPropeller);
    startAction("WaitItem");
    mFloorTouch = false;
  }
  if (mFloorTouch) {
    if (!al::isActionPlaying(this, "WaitItemStop"))
      startAction("WaitItemStop");
  } else if (!al::isActionPlaying(this, "WaitItem"))
    startAction("WaitItem");
  mFloorTouch = false;
}
void BoxPropeller::exeCarry() {
  if (al::isFirstStep(this)) {
    mPropeller->setAlignUp(true);
    startAction("Attach");
    startCharaMtpAnim();
    mWasRising = false;
    if (mSingle && !SingleModeDataFunction::isGuideMessageAlreadySeen(
                       GameDataHolderAccessor(this), 2, false))
      mGuideShown |= tryUpdateGuideMessage();
  }
  if (mSingle && !SingleModeDataFunction::isGuideMessageAlreadySeen(
                     GameDataHolderAccessor(this), 2, false)) {
    if (!rc::isCurrentGuideGameWindowUser(this))
      mGuideType = 0;
    bool skate = false;
    if (rc::isPlayerBinded(mPlayerSensor)) {
      auto *bound = rc::getPlayerBindedSenser(al::getSensorHost(mPlayerSensor));
      if (bound)
        skate = al::isSensorHostName(bound, "スケート靴");
    }
    if (!rc::isPlayerInRouteDokan(mPlayerSensor) &&
        !rc::isPlayerInRouteDokanBazooka(mPlayerSensor) && !skate)
      mGuideShown |= tryUpdateGuideMessage();
    else if (mGuideType != 0 && rc::isCurrentGuideGameWindowUser(this) &&
             (rc::isPlayerInRouteDokan(mPlayerSensor) ||
              rc::isPlayerInRouteDokanBazooka(mPlayerSensor) || skate)) {
      rc::disappearGuideGameWindow(this);
      mGuideType = 0;
    }
  }
  if (al::isActionPlaying(this, "Attach") && al::isActionEnd(this))
    startAction("Wait");
  if (rc::isPlayerDamageTrigOn(mPlayerSensor)) {
    rc::removePlayerEquipHeadgear(mPlayerSensor, false);
    if (mSingle) {
      rc::disappearGuideGameWindow(this);
      if (mGuideShown)
        SingleModeDataFunction::setGuideMessageSeen(GameDataHolderWriter(this),
                                                    2, false);
    }
    return;
  }
  if (rc::isPlayerPropellerRising(mPlayerSensor)) {
    if (!mWasRising) {
      al::startHitReaction(this, "上昇開始");
      mWasRising = true;
      if (mSingle)
        ++sPropellerJumps;
    }
  } else
    mWasRising = false;
  if (mSingle) {
    sead::Vector3f start;
    sead::Vector3f arrow;
    sead::Vector3f hitPos;
    const auto &trans = al::getTrans(this);
    sead::Vector3f offset = sead::Vector3f::ey * 100.0f;
    sead::Vector3f down = sead::Vector3f::ey * -650.0f;
    start = offset + trans;
    arrow = down;
    if (alCollisionUtil::getFirstPolyOnArrow(this, &hitPos, nullptr, start,
                                             arrow, nullptr, nullptr)) {
      float length = al::getTrans(this).y - hitPos.y + 750.0f;
      if (length < 50.0f)
        length = 50.0f;
      al::setShadowDropLength(this, length);
    } else
      al::setShadowDropLength(this, 18750.0f);
  }
  if (rc::isPlayerPropellerRising(mPlayerSensor)) {
    if (!al::isEffectEmitting(mPropeller, "Rise"))
      al::emitEffect(mPropeller, "Rise", nullptr);
    if (al::isEffectEmitting(mPropeller, "Glide"))
      al::deleteEffect(mPropeller, "Glide");
  } else if (rc::isPlayerPropellerGlide(mPlayerSensor)) {
    if (al::isEffectEmitting(mPropeller, "Rise"))
      al::deleteEffect(mPropeller, "Rise");
    if (!al::isEffectEmitting(mPropeller, "Glide"))
      al::emitEffect(mPropeller, "Glide", nullptr);
  } else
    deletePropellerEffect();
  al::holdSeWithParam(this, "Rotate", mAnimRate, nullptr);
  mPoseBuilder->update();
  if (al::isHideModel(this))
    al::hideModelIfShow(mPropeller);
  else
    al::showModelIfHide(mPropeller);
  if (al::isSilhouetteModelHidden(this))
    al::hideSilhouetteModelIfShow(mPropeller);
  else
    al::showSilhouetteModelIfHide(mPropeller);
}
void BoxPropeller::startCharaMtpAnim() {
  rc::setPlayerColorAnimBySensor(this, mPlayerSensor, "BoxPropeller");
}
bool BoxPropeller::tryUpdateGuideMessage() {
  if (!mSingle)
    return false;
  if (arePropellerJumpsComplete()) {
    if (mGuideType != 0) {
      rc::disappearGuideGameWindow(this);
      mGuideType = 0;
    }
    return false;
  }
  if (rc::isGuideGameWindowActive(this) &&
      !rc::isCurrentGuideGameWindowUser(this))
    return false;
  bool joy = al::isPadTypeJoySingle(al::getMainControllerPort());
  if (mGuideType == 0 || (mGuideType == 1 && !joy) ||
      (mGuideType == 2 && joy)) {
    if (joy) {
      if (rc::appearGuideGameWindowWithPriority(
              this, "SingleMode_GuideMessage", "PropellerGuide_SingleJoycons",
              GuideMessagePriority(1), -1, 0.0f)) {
        mGuideType = 1;
        return true;
      }
    } else if (rc::appearGuideGameWindowWithPriority(
                   this, "SingleMode_GuideMessage",
                   "PropellerGuide_DualJoycons", GuideMessagePriority(1), -1,
                   0.0f)) {
      mGuideType = 2;
      return true;
    }
  }
  return false;
}
void BoxPropeller::exeKill() {
  if (al::isFirstStep(this)) {
    setGlobalAlphaPtr(&mGlobalAlphaLastFrame);
    mPropeller->setGlobalAlphaPtr(&mGlobalAlphaLastFrame);
    if (mSingle)
      al::setShadowDropLength(this, 500.0f);
    showModel();
  }
  if (al::updateNerveState(this))
    kill();
}
void BoxPropeller::showModel() {
  al::showModelIfHide(this);
  al::showModelIfHide(mPropeller);
}
void BoxPropeller::hideModel() {
  al::hideModelIfShow(this);
  al::hideModelIfShow(mPropeller);
}
void BoxPropeller::resetPropellerJumpGuideMessage(GameDataHolderWriter writer) {
  sPropellerJumps = 0;
  SingleModeDataFunction::resetGuideMessageSeen(writer, 2, false);
}
bool BoxPropeller::isCarry() {
  return al::isNerve(this, &NrvBoxPropellerCarry);
}
