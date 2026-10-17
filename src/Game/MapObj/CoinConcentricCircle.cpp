#include "MapObj/CoinConcentricCircle.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Movement/FlashingCtrl.hpp"
#include "Library/Nerve/NerveExecutor.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Resource/Resource.hpp"
#include "Library/Resource/ResourceFunction.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Yaml/ByamlIter.hpp"
#include "MapObj/Coin.hpp"
#include "MapObj/CoinHolder.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Collision/CollisionUtil.hpp"
#include "Project/Collision/HitDb.hpp"
#include <math/seadMathCalcCommon.h>
namespace {
class CoinCircle : public al::NerveExecutor {
public:
  CoinCircle(unsigned, float, float, unsigned, float, CoinConcentricCircle *,
             const al::ActorInitInfo &);
  void kill();
  void appear();
  void exeWait() {}
  void exeAppear();
  void exeEnd();
  sead::PtrArray<Coin> mCoins;
  float mRadiusBegin;
  float mRadiusEnd;
  unsigned mAppearFrames;
  int mUnused = 0;
  float mJumpHeight;
  CoinConcentricCircle *mHost;
};
static_assert(sizeof(CoinCircle) == 0x40);
} // namespace
namespace {
NERVE_DECL(CoinConcentricCircle, Appear);
NERVE_DECL(CoinConcentricCircle, Wait);
NERVE_DECL(CoinCircle, Wait);
NERVE_DECL(CoinCircle, Appear);
NERVE_DECL(CoinCircle, End);
NERVES_MAKE_NOSTRUCT(CoinConcentricCircle, Appear, Wait)
NERVES_MAKE_NOSTRUCT(CoinCircle, Wait, Appear, End)
const char *sParamTypeNames[] = {"Single", "Double", "Triple"};
struct CircleParam {
  float innerRadiusBegin, innerRadiusEnd, outerRadiusBegin, outerRadiusEnd,
      coinInterval;
  int circleNum, appearInterval, appearFrames;
  float jumpHeightMax, jumpHeightMin;
};
} // namespace
inline CoinCircle::CoinCircle(unsigned count, float begin, float end,
                              unsigned frames, float height,
                              CoinConcentricCircle *host,
                              const al::ActorInitInfo &info)
    : al::NerveExecutor("同心円コイン"), mRadiusBegin(begin), mRadiusEnd(end),
      mAppearFrames(frames), mJumpHeight(height), mHost(host) {
  mCoins.allocBuffer(count, nullptr);
  if (mHost->isShared()) {
    CoinHolder::requestCreateSceneHolder(mHost, info);
  } else {
    for (unsigned i = 0; i < count; ++i) {
      auto *coin = new Coin("コイン");
      al::initCreateActorWithPlacementInfo(coin, info);
      coin->setEnableAppear(false);
      coin->makeActorDead();
      mCoins.pushBack(coin);
    }
  }
  initNerve(&NrvCoinCircleWait, 0);
}
CoinConcentricCircle::CoinConcentricCircle(const char *name)
    : al::LiveActor(name) {}
CoinConcentricCircle::~CoinConcentricCircle() {}
void CoinConcentricCircle::init(const al::ActorInitInfo &info) {
  const char *name = nullptr;
  if (al::tryGetObjectName(&name, info) &&
      al::isEqualString(name, "CoinConcentricCircleShare"))
    mShared = true;
  al::initActorSceneInfo(this, info);
  al::initActorPoseTRSV(this);
  al::initActorSRT(this, info);
  al::initExecutorWatchObj(this, info);
  al::initStageSwitch(this, info);
  al::initActorAudioKeeper(this, info, "CoinConcentricCircle", nullptr);
  al::trySyncStageSwitchAppear(this);
  al::initNerve(this, &NrvCoinConcentricCircleAppear, 0);
  int type = 2;
  al::tryGetArg(&type, info, "Type");
  auto *resource =
      al::findOrCreateResource("ObjectData/CoinConcentricCircle", nullptr);
  al::StringTmp<128> paramName("Param%s", sParamTypeNames[type]);
  al::ByamlIter iter(resource->getByml(paramName));
  CircleParam p;
  iter.tryGetFloatByKey(&p.innerRadiusBegin, "InnerRadiusBegin");
  iter.tryGetFloatByKey(&p.innerRadiusEnd, "InnerRadiusEnd");
  iter.tryGetFloatByKey(&p.outerRadiusBegin, "OuterRadiusBegin");
  iter.tryGetFloatByKey(&p.outerRadiusEnd, "OuterRadiusEnd");
  iter.tryGetFloatByKey(&p.coinInterval, "CoinInterval");
  iter.tryGetIntByKey(&p.circleNum, "CircleNum");
  iter.tryGetIntByKey(&p.appearInterval, "AppearInterval");
  iter.tryGetIntByKey(&p.appearFrames, "AppearFrame");
  iter.tryGetFloatByKey(&p.jumpHeightMax, "CoinJumpHeightMax");
  iter.tryGetFloatByKey(&p.jumpHeightMin, "CoinJumpHeightMin");
  al::tryGetArg(&mFloorCheckType, info, "FloorCheckType");
  al::tryGetArg(&mHoverHeight, info, "HoverHeight");
  mRings.allocBuffer(p.circleNum, nullptr);
  for (int i = 0; i < p.circleNum; ++i) {
    float rate = 1.0f;
    if (p.circleNum >= 2)
      rate = float(i) / (p.circleNum - 1);
    float begin =
        p.innerRadiusBegin + rate * (p.outerRadiusBegin - p.innerRadiusBegin);
    float end = p.innerRadiusEnd + rate * (p.outerRadiusEnd - p.innerRadiusEnd);
    unsigned count = unsigned(end * 6.283185482f / p.coinInterval) + 1;
    float height = p.jumpHeightMax + rate * (p.jumpHeightMin - p.jumpHeightMax);
    mRings.pushBack(
        new CoinCircle(count, begin, end, p.appearFrames, height, this, info));
  }
  mAppearInterval = p.appearInterval;
  mAppearFrames = p.appearFrames;
  mFlashing = new al::FlashingCtrl(this, false, false);
  al::tryGetArg(&mTimer, info, "Timer");
}
void CoinConcentricCircle::calcHeightOffset(float distance) {
  const al::ArrowHitInfo *hit;
  if (alCollisionUtil::getFirstPolyOnArrow(this, &hit, al::getTrans(this),
                                           sead::Vector3f::ey * -distance,
                                           nullptr, nullptr)) {
    float hitDistance = hit->_70;
    mHeightOffset = mHoverHeight - hitDistance;
  }
}
void CoinConcentricCircle::initAfterPlacement() {
  if (mFloorCheckType == 1)
    calcHeightOffset(500.0f);
}
void CoinConcentricCircle::appear() {
  al::LiveActor::appear();
  if (mFloorCheckType == 2)
    calcHeightOffset(500.0f);
  for (auto ring = mRings.begin(); ring != getRingsEnd(); ++ring) {
    auto &coins = ring->mCoins;
    for (auto coin = coins.begin(); coin != coins.end(); ++coin)
      al::showModelIfHide(&*coin);
  }
  mHidden = false;
  al::setNerve(this, &NrvCoinConcentricCircleAppear);
}
void CoinConcentricCircle::kill() {
  for (auto ring = mRings.begin(); ring != getRingsEnd(); ++ring)
    ring->kill();
  al::LiveActor::kill();
}
void CoinCircle::kill() {
  CoinHolder *holder = nullptr;
  if (mHost->isShared())
    holder = CoinHolder::getCoinHolder(mHost);
  for (auto coin = mCoins.begin(); coin != mCoins.end(); ++coin) {
    if (al::isAlive(&*coin))
      coin->kill();
    if (holder)
      holder->sinkCoin(&*coin);
  }
  if (mHost->isShared())
    mCoins.clear();
}
inline void CoinCircle::appear() {
  if (mHost->isShared()) {
    auto *holder = CoinHolder::getCoinHolder(mHost);
    for (int i = 0; i < mCoins.capacity(); ++i)
      mCoins.pushBack(holder->scoopCoin());
  }
  for (auto coin = mCoins.begin(); coin != mCoins.end(); ++coin) {
    coin->appear();
    al::showModelIfHide(&*coin);
  }
  for (auto coin = mCoins.begin(); coin != mCoins.end(); ++coin)
    al::invalidateHitSensors(&*coin);
  al::setNerve(this, &NrvCoinCircleAppear);
}
void CoinConcentricCircle::exeAppear() {
  if (al::getNerveStep(this) % mAppearInterval == 0) {
    unsigned index = al::getNerveStep(this) / mAppearInterval;
    if (index < unsigned(mRings.size())) {
      mRings.unsafeAt(mRings.size() - index - 1)->appear();
      al::setSeSeqLocalVariableDefault(this, 0, index * 2);
      al::startSe(this, "Appear", nullptr);
    }
  }
  for (auto ring = mRings.begin(); ring != getRingsEnd(); ++ring)
    ring->updateNerve();
  if (al::isGreaterEqualStep(this,
                             mAppearInterval * mRings.size() + mAppearFrames))
    al::setNerve(this, &NrvCoinConcentricCircleWait);
}
void CoinConcentricCircle::exeWait() {
  if (mTimer > 0) {
    if (al::isFirstStep(this))
      mFlashing->start(mTimer);
    if (mFlashing->isNowJustFlashed()) {
      if (mHidden) {
        for (auto ring = mRings.begin(); ring != getRingsEnd(); ++ring) {
          auto &coins = ring->mCoins;
          for (auto coin = coins.begin(); coin != coins.end(); ++coin)
            al::showModelIfHide(&*coin);
        }
        mHidden = false;
      } else {
        for (auto ring = mRings.begin(); ring != getRingsEnd(); ++ring) {
          auto &coins = ring->mCoins;
          for (auto coin = coins.begin(); coin != coins.end(); ++coin)
            al::hideModelIfShow(&*coin);
        }
        mHidden = true;
      }
    }
    if (mFlashing->isEnded())
      for (auto ring = mRings.begin(); ring != getRingsEnd(); ++ring)
        ring->kill();
    mFlashing->movement();
  }
  for (auto ring = mRings.begin(); ring != getRingsEnd(); ++ring)
    ring->updateNerve();
  for (auto ring = mRings.begin(); ring != getRingsEnd(); ++ring) {
    auto &coins = ring->mCoins;
    for (auto coin = coins.begin(); coin != coins.end(); ++coin)
      if (al::isAlive(&*coin))
        return;
  }
  kill();
}
void CoinConcentricCircle::control() {}
inline void CoinCircle::exeAppear() {
  {
    float rate = float(al::getNerveStep(this)) / mAppearFrames;
    float bounce =
        sead::Mathf::clamp(sead::Mathf::abs(rate + rate - 1.0f), 0.0f, 1.0f);
    float jumpHeight = mJumpHeight;
    float heightOffset = mHost->getHeightOffset();
    float begin = mRadiusBegin;
    float end = mRadiusEnd;
    float height = jumpHeight * (1.0f - bounce * bounce) +
                   heightOffset * sead::Mathf::clamp(rate, 0.0f, 1.0f);
    float radius = begin + rate * (end - begin);
    sead::Vector3f up;
    sead::Vector3f front;
    al::calcUpDir(&up, mHost);
    al::calcFrontDir(&front, mHost);
    sead::Quatf rotation;
    rotation.setAxisRadian(up, 6.283185482f / mCoins.size());
    for (auto coin = mCoins.begin(); coin != mCoins.end(); ++coin) {
      auto *pos = al::getTransPtr(&*coin);
      const auto &hostPos = al::getTrans(mHost);
      *pos = radius * front + hostPos + height * up;
      front.rotate(rotation);
    }
  }
  if (al::isGreaterEqualStep(this, mAppearFrames)) {
    for (auto coin = mCoins.begin(); coin != mCoins.end(); ++coin)
      al::startSeWithParam(&*coin, "PgBound", 25.0f, nullptr);
    al::setNerve(this, &NrvCoinCircleEnd);
  }
}
inline void CoinCircle::exeEnd() {
  if (al::isStep(this, 0))
    for (auto coin = mCoins.begin(); coin != mCoins.end(); ++coin)
      al::validateHitSensors(&*coin);
}
