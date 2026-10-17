#include "Layout/WipeCurtainRanking.hpp"
#include "Layout/LayoutFontUtil.hpp"
#include "Layout/WipeCurtainRankingParts.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/InputUtil.hpp"
namespace {
NERVE_DECL(WipeCurtainRanking, Appear);
NERVE_DECL(WipeCurtainRanking, End);
NERVE_DECL(WipeCurtainRanking, SkipAppear);
NERVE_DECL(WipeCurtainRanking, EndInit);
NERVE_DECL(WipeCurtainRanking, SkipReady);
NERVE_DECL(WipeCurtainRanking, Rise);
NERVE_DECL(WipeCurtainRanking, SkipWait);
NERVE_DECL(WipeCurtainRanking, SkipEnd);
NERVES_MAKE_NOSTRUCT(WipeCurtainRanking, Appear, End, SkipAppear, EndInit,
                     SkipReady, Rise, SkipWait, SkipEnd)
} // namespace
WipeCurtainRanking::WipeCurtainRanking(const al::LayoutInitInfo &rInfo,
                                       const GameDataHolder *pHolder)
    : al::LayoutActor("WipeCurtainRanking"), mGameDataHolder(pHolder) {
  al::initLayoutActor(this, rInfo, "WipeCurtainRanking", nullptr);
  initNerve(&NrvWipeCurtainRankingAppear, 0);
  mParts = new WipeCurtainRankingParts *[4];
  mParts[0] = new WipeCurtainRankingParts(rInfo, "ParRemoconNum5",
                                          "ParRemoconNum5", 0, this);
  mParts[1] = new WipeCurtainRankingParts(rInfo, "ParRemoconNum1",
                                          "ParRemoconNum1", 1, this);
  mParts[2] = new WipeCurtainRankingParts(rInfo, "ParRemoconNum2",
                                          "ParRemoconNum2", 2, this);
  mParts[3] = new WipeCurtainRankingParts(rInfo, "ParRemoconNum3",
                                          "ParRemoconNum3", 3, this);
  kill();
}
void WipeCurtainRanking::appear() {
  al::LayoutActor::appear();
  mIsEffectEnabled = true;
  int users[4] = {-1};
  int count = rc::findActiveUserIdList(
      users,
      GameDataHolderAccessor(const_cast<GameDataHolder *>(mGameDataHolder)));
  mMaxScore = 0;
  for (int i = 0; i < count; ++i)
    if (users[i] != -1 && mMaxScore < getScore(users[i]))
      mMaxScore = GameDataFunction::getUserScore(
          GameDataHolderAccessor(const_cast<GameDataHolder *>(mGameDataHolder)),
          users[i]);
  int port = rc::tryCalcPadPortByFirstActiveUser(
      GameDataHolderAccessor(const_cast<GameDataHolder *>(mGameDataHolder)));
  if (port < 0)
    port = al::getMainControllerPort();
  al::setPaneString(this, "TxtIconA",
                    LayoutFontUtil::getMessageFontButtonDecide(port), 0, -1);
  if (mMaxScore != 0)
    al::setNerve(this, &NrvWipeCurtainRankingAppear);
  else
    al::setNerve(this, &NrvWipeCurtainRankingEnd);
}
int WipeCurtainRanking::getScore(int userId) const {
  return GameDataFunction::getUserScore(
      GameDataHolderAccessor(const_cast<GameDataHolder *>(mGameDataHolder)),
      userId);
}
int WipeCurtainRanking::calcRank(int userId) const {
  int score = getScore(userId);
  int users[4] = {-1};
  int count = rc::findActiveUserIdList(
      users,
      GameDataHolderAccessor(const_cast<GameDataHolder *>(mGameDataHolder)));
  int rank = 1;
  for (int i = 0; i < count; ++i)
    if (users[i] != -1 && users[i] != userId && score < getScore(users[i]))
      ++rank;
  return rank;
}
bool WipeCurtainRanking::isExistMultiPlayer1st() const {
  if (GameDataFunction::isCurrentStageKinopioBrigade(GameDataHolderAccessor(
          const_cast<GameDataHolder *>(mGameDataHolder))))
    return true;
  int users[4] = {-1};
  int count = rc::findActiveUserIdList(
      users,
      GameDataHolderAccessor(const_cast<GameDataHolder *>(mGameDataHolder)));
  int firstCount = 0, maxScore = -1;
  for (int i = 0; i < count; ++i) {
    int score = getScore(users[i]);
    if (maxScore < score) {
      maxScore = score;
      firstCount = 0;
    }
    if (score == maxScore)
      ++firstCount;
  }
  return firstCount > 1;
}
void WipeCurtainRanking::startEnd() {
  al::setNerve(this, &NrvWipeCurtainRankingEnd);
}
void WipeCurtainRanking::startSkipAppear() {
  al::setNerve(this, &NrvWipeCurtainRankingSkipAppear);
}
bool WipeCurtainRanking::isEnableEnd() const {
  return al::isNerve(this, &NrvWipeCurtainRankingEndInit);
}
bool WipeCurtainRanking::isSkipReady() const {
  return al::isNerve(this, &NrvWipeCurtainRankingSkipReady);
}
bool WipeCurtainRanking::tryEmitCrownEffect(
    const WipeCurtainRankingParts *pParts, const char *pPane) {
  if (!mIsEffectEnabled)
    return false;
  al::calcPaneMtx(&mCrownMtx, pParts, pPane);
  al::setEffectFollowMtxPtr(this, "CrownAppear", &mCrownMtx);
  return al::tryEmitEffect(this, "CrownAppear", nullptr);
}
void WipeCurtainRanking::killEffects() {
  mIsEffectEnabled = false;
  al::tryKillEmitterAndParticleAll(this);
}
void WipeCurtainRanking::exeAppear() {
  if (al::isFirstStep(this)) {
    al::startAction(this, "Appear", nullptr);
    al::startSe(this, "Appear", nullptr);
    if (GameDataFunction::isCurrentStageKinopioBrigade(GameDataHolderAccessor(
            const_cast<GameDataHolder *>(mGameDataHolder)))) {
      for (int i = 0; i < 4; ++i)
        mParts[i]->appearKinopioBrigade();
    } else {
      for (int i = 0; i < 4; ++i)
        mParts[i]->appear();
    }
    al::startSe(this, "PgResultWait", nullptr);
  }
  if (al::isActionEnd(this, nullptr))
    al::setNerve(this, &NrvWipeCurtainRankingRise);
}
void WipeCurtainRanking::exeRise() {
  if (al::isFirstStep(this)) {
    al::startAction(this, "Wait", nullptr);
    for (int i = 0; i < 4; ++i)
      mParts[i]->startRise();
  }
  int finished = 0;
  for (int i = 0; i < 4; ++i)
    finished += mParts[i]->isEndRise();
  for (int i = 0; i < 4; ++i)
    mParts[i]->setFinishedCount(finished);
  bool end0 = mParts[0]->isEndRise(), end1 = mParts[1]->isEndRise(),
       end2 = mParts[2]->isEndRise(), end3 = mParts[3]->isEndRise();
  if (end0 && end1 && end2 && end3)
    al::stopSeByName(this, "PgResultWait");
  if (mParts[0]->isStop() && mParts[1]->isStop() && mParts[2]->isStop() &&
      mParts[3]->isStop())
    al::setNerve(this, &NrvWipeCurtainRankingSkipReady);
}
void WipeCurtainRanking::exeSkipReady() {}
void WipeCurtainRanking::exeSkipAppear() {
  if (al::isFirstStep(this)) {
    al::startAction(this, "SkipButtonAppear", "Skip");
    al::startSe(this, "ButtonAAppear", nullptr);
  }
  if (al::isActionEnd(this, "Skip"))
    al::setNerve(this, &NrvWipeCurtainRankingSkipWait);
}
void WipeCurtainRanking::exeSkipWait() {
  if (al::isFirstStep(this))
    al::startAction(this, "SkipButtonWait", "Skip");
  if (rc::isPadTriggerUiDecide(GameDataHolderAccessor(
          const_cast<GameDataHolder *>(mGameDataHolder))) ||
      (al::isPadConnected(al::getMainControllerPort()) &&
       al::isPadTriggerA(al::getMainControllerPort())))
    al::setNerve(this, &NrvWipeCurtainRankingSkipEnd);
}
void WipeCurtainRanking::exeSkipEnd() {
  if (al::isFirstStep(this)) {
    al::startAction(this, "SkipButtonEnd", "Skip");
    al::startSe(this, "ButtonAPressed", nullptr);
  }
  if (al::isActionEnd(this, "Skip"))
    al::setNerve(this, &NrvWipeCurtainRankingEndInit);
}
void WipeCurtainRanking::exeEndInit() {}
void WipeCurtainRanking::exeEnd() {
  if (al::isFirstStep(this)) {
    al::startAction(this, "End", nullptr);
    al::startSe(this, "End", nullptr);
  }
  if (al::isActionEnd(this, nullptr))
    kill();
}
