#include "MapObj/CheckPointStar.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/LiveActorFunc.hpp"
#include "Library/Movement/FlashingCtrl.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Shadow/ShadowKeeper.hpp"
#include "MapObj/CheckPoint.hpp"
#include "MapObj/TimerManager.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include <cmath>
namespace {
NERVE_DECL(CheckPointStar, Wait)
NERVE_DECL(CheckPointStar, SpinSpeedDown)
NERVE_DECL(CheckPointStar, Spin)
NERVE_DECL(CheckPointStar, Appear)
NERVE_DECL(CheckPointStar, MoveArc)
NERVE_DECL(CheckPointStar, MoveLinear)
NERVE_DECL(CheckPointStar, Fail)
class CheckPointStarNrvReappear : public al::Nerve {
public:
  void execute(al::NerveKeeper *keeper) const override {
    keeper->getParent<CheckPointStar>()->exeAppear();
  }
};
NERVES_MAKE_NOSTRUCT(CheckPointStar, Fail)
NERVES_MAKE_STRUCT(CheckPointStar, Wait, SpinSpeedDown, Spin, Appear, MoveArc,
                   MoveLinear, Reappear)
} // namespace
CheckPointStar::CheckPointStar(const char *name, CheckPoint *owner)
    : al::LiveActor(name) {
  _150 = 0;
  _148 = nullptr;
  _154 = true;
  _158 = 120;
  _160 = 0;
  mMoveSource = nullptr;
  mOwner = owner;
  mCollectedModel = nullptr;
  mTarget = nullptr;
  _1a0 = false;
  mSkipAppear = false;
  mUseCollected = false;
  mMoveSpeed = 0.02f;
  mMoveFrames = -1;
}
CheckPointStar::~CheckPointStar() {}
void CheckPointStar::init(const al::ActorInitInfo &info) {
  const char *name = "CheckPointStar";
  alPlacementFunction::tryGetModelName(&name, info);
  al::initActorWithArchiveName(this, info, sead::SafeString(name),
                               rc::getBlockSuffixName(info, false));
  al::offCollide(this);
  makeActorAppeared();
  al::initNerve(this, &NrvCheckPointStar.Wait, 1);
  al::startAction(this, "Wait");
  mStartTrans = al::getTrans(this);
  al::invalidateClipping(this);
  mFlashing = new al::FlashingCtrl(this, true, true);
  mCollectedModel = new al::LiveActor("CheckPointStarEmpty");
  al::initActorWithArchiveName(mCollectedModel, info, "CheckPointStarEmpty",
                               nullptr);
  mCollectedModel->makeActorDead();
  al::invalidateClipping(mCollectedModel);
}
void CheckPointStar::setUseCollectedModel() {
  mUseCollected = true;
  if (al::isDead(mCollectedModel))
    mCollectedModel->appear();
  al::hideModelIfShow(this);
  al::showModelIfHide(mCollectedModel);
  mFlashing->setActor(mCollectedModel);
  if (al::isClipped(this) && !al::isClipped(mCollectedModel))
    mCollectedModel->startClipped();
}
void CheckPointStar::startClipped() {
  al::LiveActor::startClipped();
  if (!al::isDead(mCollectedModel))
    mCollectedModel->startClipped();
}
void CheckPointStar::endClipped() {
  al::LiveActor::endClipped();
  if (!al::isDead(mCollectedModel))
    mCollectedModel->endClipped();
}
void CheckPointStar::startFlashing(s32 time) { mFlashing->start(time); }
void CheckPointStar::updateFlashing() {
  auto *flashing = mFlashing;
  flashing->setPlaySe(!TimerManager::isTimerManagerActive(this));
  mFlashing->movement();
}
void CheckPointStar::overrideFlashingAudio(al::IUseAudioKeeper *keeper) {
  mFlashing->setAudioKeeper(keeper);
}
bool CheckPointStar::isFastFlashing() const {
  return mFlashing->isFastFlashing();
}
void CheckPointStar::startNightAnim() {
  al::tryStartMclAnimIfExist(this, "Night");
  al::tryStartMclAnimIfExist(mCollectedModel, "Night");
}
void CheckPointStar::handleCountDownEffect(bool on) {
  if (mUseCollected) {
    al::tryDeleteEffect(this, "Countdown");
    if (on)
      al::tryEmitEffect(mCollectedModel, "CountdownEmpty", nullptr);
    else
      al::tryDeleteEffect(mCollectedModel, "CountdownEmpty");
  } else if (on)
    al::tryEmitEffect(this, "Countdown", nullptr);
  else
    al::tryDeleteEffect(this, "Countdown");
}
void CheckPointStar::handleWaitEffect(bool on) {
  if (mUseCollected) {
    al::tryDeleteEffect(this, "Wait");
    if (on)
      al::tryEmitEffect(mCollectedModel, "Wait", nullptr);
    else
      al::tryDeleteEffect(mCollectedModel, "Wait");
  } else if (on)
    al::tryEmitEffect(this, "Wait", nullptr);
  else
    al::tryDeleteEffect(this, "Wait");
}
void CheckPointStar::cancel() {
  mOwner->cancel();
  handleCountDownEffect(false);
  handleWaitEffect(false);
  al::tryDeleteEmitterAndParticleAll(this);
}
bool CheckPointStar::receiveMsg(const al::SensorMsg *msg, al::HitSensor *sender,
                                al::HitSensor *receiver) {
  if (al::isMsgPlayerBodyAttack(msg) || al::isMsgPlayerBoomerangAttack(msg) ||
      al::isMsgPlayerClimbAttack(msg) ||
      al::isMsgPlayerClimbRollingAttack(msg) ||
      al::isMsgPlayerClimbSlidingAttack(msg) ||
      al::isMsgPlayerFireBallAttack(msg) || al::isMsgPlayerKouraAttack(msg) ||
      al::isMsgExplosion(msg) || al::isMsgPlayerRollingAttack(msg) ||
      al::isMsgPlayerSpinAttack(msg) || al::isMsgPlayerRollingAttack(msg) ||
      al::isMsgPlayerTailAttack(msg) || al::isMsgLaserAttack(msg) ||
      al::isMsgBallItemGet(msg) || al::isMsgEnemyTouch(msg) ||
      al::isMsgEnemyAttack(msg)) {
    if (!al::isMsgBallItemGet(msg) && !rc::isMsgPackunThrowAttack(msg) &&
        (al::isNerve(this, &NrvCheckPointStar.Wait) ||
         al::isNerve(this, &NrvCheckPointStar.SpinSpeedDown) ||
         al::isMsgPlayerFireBallAttack(msg) || al::isMsgEnemyAttackFire(msg) ||
         (al::isNerve(this, &NrvCheckPointStar.Spin) &&
          al::isGreaterEqualStep(this, 20))))
      rc::requestHitReactionToAttacker(msg, receiver, sender);
    if (al::isNerve(this, &NrvCheckPointStar.Wait) ||
        al::isNerve(this, &NrvCheckPointStar.Spin) ||
        al::isNerve(this, &NrvCheckPointStar.SpinSpeedDown)) {
      al::setNerve(this, &NrvCheckPointStar.Spin);
      return true;
    }
  }
  return false;
}
bool CheckPointStar::receiveMsgScreenPoint(const al::SensorMsg *msg,
                                           al::ScreenPointer *,
                                           al::ScreenPointTarget *) {
  if (al::isMsgTouchAssistTrig(msg)) {
    if (al::isNerve(this, &NrvCheckPointStar.Wait) ||
        al::isNerve(this, &NrvCheckPointStar.Spin) ||
        al::isNerve(this, &NrvCheckPointStar.SpinSpeedDown)) {
      al::setNerve(this, &NrvCheckPointStar.Spin);
      return true;
    }
  }
  return false;
}
inline void CheckPointStar::syncCollectedModel() {
  auto *model = mCollectedModel;
  if (model) {
    al::copyPose(model, this);
    al::copyAction(model, this);
  }
}
inline void CheckPointStar::followTarget() {
  auto *trans = al::getTransPtr(this);
  static_cast<sead::BaseVec3<float> &>(*trans) = al::getTrans(mTarget);
}
void CheckPointStar::exeSpin() {
  followTarget();
  if (al::isFirstStep(this)) {
    if (mUseCollected)
      al::tryStartActionIfNotPlaying(mCollectedModel, "Wait");
    else
      al::tryStartActionIfNotPlaying(this, "Wait");
    al::setActionFrameRate(this, 11.0f);
    al::startSe(this, "Spin", nullptr);
  }
  syncCollectedModel();
  if (al::isGreaterEqualStep(this, 20))
    al::setNerve(this, &NrvCheckPointStar.SpinSpeedDown);
}
void CheckPointStar::exeSpinSpeedDown() {
  followTarget();
  if (al::isFirstStep(this)) {
    if (mUseCollected)
      al::tryStartActionIfNotPlaying(mCollectedModel, "Wait");
    else
      al::tryStartActionIfNotPlaying(this, "Wait");
    al::setActionFrameRate(this, 10.0f);
  }
  al::setActionFrameRate(
      this, (float(al::getNerveStep(this)) / -70.0f + 1.0f) * 10.0f + 1.0f);
  if (al::isGreaterEqualStep(this, 70)) {
    al::setActionFrameRate(this, 1.0f);
    al::setNerve(this, &NrvCheckPointStar.Wait);
    al::stopSeByName(this, "Spin");
  }
  syncCollectedModel();
}
void CheckPointStar::updateShadowLengthToTarget() {
  if (!mTarget)
    return;
  auto *targetKeeper = mTarget->getShadowKeeper();
  if (!targetKeeper || targetKeeper->getShadowMaskNum() < 1)
    return;
  auto *keeper = getShadowKeeper();
  if (!keeper || keeper->getShadowMaskNum() < 1)
    return;
  float length = targetKeeper->getShadowMask(0)->mDropLength;
  if (length < 0.0f)
    length = 0.0f;
  for (int i = 0; i < keeper->getShadowMaskNum(); ++i)
    keeper->getShadowMask(i)->mDropLength = length;
}
void CheckPointStar::exeAppear() {
  if (al::isFirstStep(this)) {
    al::showShadow(this);
    updateShadowLengthToTarget();
    if (!al::isNerve(this, &NrvCheckPointStar.Appear)) {
      if (mUseCollected)
        al::tryEmitEffect(mCollectedModel, "AppearBrightEmpty", nullptr);
      else
        al::tryEmitEffect(this, "AppearBright", nullptr);
    }
  }
  if (mUseCollected)
    al::tryStartAction(mCollectedModel, "Wait");
  else
    al::tryStartAction(this, "Wait");
  al::setNerve(this, &NrvCheckPointStar.Wait);
  followTarget();
  syncCollectedModel();
}
void CheckPointStar::exeWait() {
  if (al::isFirstStep(this))
    updateShadowLengthToTarget();
  followTarget();
  syncCollectedModel();
}
void CheckPointStar::go(CheckPoint *source, al::LiveActor *target, bool arc,
                        float height, bool skip) {
  mMoveSource = source;
  mTarget = target;
  mSkipAppear = skip;
  mArcHeight = height;
  if (arc)
    al::setNerve(this, &NrvCheckPointStar.MoveArc);
  else
    al::setNerve(this, &NrvCheckPointStar.MoveLinear);
}
void CheckPointStar::fail(CheckPoint *target) {
  mTarget = target;
  al::setNerve(this, &NrvCheckPointStarFail);
}
void CheckPointStar::reset(CheckPoint *target) {
  mTarget = target;
  al::setNerve(this, &NrvCheckPointStar.Wait);
}
void CheckPointStar::reappear(bool bright) {
  if (bright)
    al::setNerve(this, &NrvCheckPointStar.Reappear);
  else
    al::setNerve(this, &NrvCheckPointStar.Appear);
  if (mUseCollected) {
    al::startAction(mCollectedModel, "Appear");
    al::showModelIfHide(mCollectedModel);
  } else {
    al::startAction(this, "Appear");
    al::showModelIfHide(this);
  }
  handleWaitEffect(!mUseCollected);
}
void CheckPointStar::startMove() {
  mStartTrans = al::getTrans(mMoveSource);
  handleWaitEffect(false);
  if (mMoveFrames >= 1)
    al::emitEffect(this, "TrailLong", nullptr);
  else if (mUseCollected)
    al::emitEffect(mCollectedModel, "TrailLoopEmpty", nullptr);
  else
    al::emitEffect(this, "TrailLoop", nullptr);
  al::startAction(this, "Move");
  if (mUseCollected)
    al::hideModelIfShow(mCollectedModel);
  else
    al::hideModelIfShow(this);
  al::hideSilhouetteModel(this);
  al::showShadow(this);
  mMoveRate = 0.0f;
  syncCollectedModel();
}
void CheckPointStar::endMove() {
  if (mMoveFrames >= 1)
    al::tryDeleteEffect(this, "TrailLong");
  else if (mUseCollected)
    al::tryDeleteEffect(mCollectedModel, "TrailLoopEmpty");
  else
    al::tryDeleteEffect(this, "TrailLoop");
  if (mSkipAppear) {
    al::setNerve(this, &NrvCheckPointStar.Wait);
    mMoveSource->starDoneMoving();
    syncCollectedModel();
    return;
  }
  if (mUseCollected) {
    al::startAction(mCollectedModel, "Appear");
    al::showModelIfHide(mCollectedModel);
  } else {
    al::startAction(this, "Appear");
    al::showModelIfHide(this);
  }
  handleWaitEffect(true);
  al::setNerve(this, &NrvCheckPointStar.Reappear);
  al::showSilhouetteModel(this);
  mMoveSource->starDoneMoving();
  syncCollectedModel();
}
inline void CheckPointStar::updateMoveEnd() {
  if (mMoveFrames >= 1) {
    if (mMoveRate > 1.0f)
      mMoveRate = 1.0f;
    else if (mMoveRate >= mEndRate) {
      mMoveRate = mEndRate;
      endMove();
    }
    if (al::isGreaterEqualStep(this, mMoveFrames))
      endMove();
  } else if (mMoveRate >= 1.0f)
    endMove();
}
void CheckPointStar::exeMoveLinear() {
  if (al::isFirstStep(this))
    startMove();
  sead::Vector3f target = al::getTrans(mTarget);
  auto *trans = al::getTransPtr(this);
  *trans = mStartTrans + (target - mStartTrans) * mMoveRate;
  mMoveRate = mMoveSpeed + mMoveRate;
  updateShadowLengthToFloor();
  updateMoveEnd();
  syncCollectedModel();
}
void CheckPointStar::updateShadowLengthToFloor() {
  float fromY = 0.0f, toY = 0.0f;
  if (mMoveSource)
    fromY = al::getTrans(mMoveSource).y;
  if (mTarget)
    toY = al::getTrans(mTarget).y;
  float minY = toY < fromY ? toY : fromY;
  float y = al::getTrans(this).y;
  float fromLength = 0.0f, toLength = 0.0f;
  if (mMoveSource && mMoveSource->getShadowKeeper() &&
      mMoveSource->getShadowKeeper()->getShadowMaskNum() >= 1)
    fromLength = mMoveSource->getShadowKeeper()->getShadowMask(0)->mDropLength;
  if (mTarget && mTarget->getShadowKeeper() &&
      mTarget->getShadowKeeper()->getShadowMaskNum() >= 1)
    toLength = mTarget->getShadowKeeper()->getShadowMask(0)->mDropLength;
  float maxLength = fromLength < toLength ? toLength : fromLength;
  auto *keeper = getShadowKeeper();
  if (keeper && keeper->getShadowMaskNum() >= 1) {
    float length = minY - y + maxLength;
    if (length < 0.0f)
      length = 0.0f;
    for (int i = 0; i < keeper->getShadowMaskNum(); ++i)
      keeper->getShadowMask(i)->mDropLength = length;
  }
}
void CheckPointStar::exeMoveArc() {
  if (al::isFirstStep(this))
    startMove();
  sead::Vector3f target = al::getTrans(mTarget);
  float arc = cosf((mMoveRate + -0.5f) * 3.1415927410125732f);
  auto *trans = al::getTransPtr(this);
  *trans = mStartTrans + (target - mStartTrans) * mMoveRate +
           sead::Vector3f(0.0f, arc * mArcHeight, 0.0f);
  mMoveRate = mMoveSpeed + mMoveRate;
  updateShadowLengthToFloor();
  updateMoveEnd();
  syncCollectedModel();
}
void CheckPointStar::exeFail() {
  if (al::isFirstStep(this)) {
    if (mUseCollected)
      al::hideModelIfShow(mCollectedModel);
    else
      al::hideModelIfShow(this);
    al::hideSilhouetteModel(this);
    bool useCollected = mUseCollected;
    auto *actor = mMoveSource->getStar();
    if (useCollected)
      al::tryEmitEffect(actor, "FailEmpty", nullptr);
    else
      al::tryEmitEffect(actor, "Fail", nullptr);
    handleWaitEffect(false);
  }
  if (al::isStep(this, 3))
    followTarget();
  syncCollectedModel();
}
void CheckPointStar::exeNoop() {}
