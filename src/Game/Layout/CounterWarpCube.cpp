#include "Layout/CounterWarpCube.hpp"
#include "Layout/LayoutFontUtil.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Se/Project/SePlayParamList.hpp"
#include "MapObj/WarpCube.hpp"
#include "Util/ScoreUtil.hpp"
namespace {
/** @brief Formats a row of collected or empty warp-cube pieces. */
template <const char16_t *(*GetIcon)()>
inline void setPieceCount(al::LayoutActor *pLayout, const char *pPane,
                          int count) {
  sead::WFormatFixedSafeString<10> text(u"");
  for (int i = 0; i < count; ++i)
    text.append(GetIcon());
  al::setPaneString(pLayout, pPane, text.cstr(), 0, -1);
}
NERVE_DECL(CounterWarpCube, Wait);
NERVE_DECL(CounterWarpCube, Appear);
NERVE_DECL(CounterWarpCube, Complete);
NERVE_DECL(CounterWarpCube, Add);
NERVE_DECL(CounterWarpCube, End);
NERVES_MAKE_NOSTRUCT(CounterWarpCube, Wait, Appear, Complete, Add, End)
} // namespace
/** @brief Creates the counter and initializes the row of required pieces. */
CounterWarpCube::CounterWarpCube(const al::LayoutInitInfo &rInfo,
                                 WarpCube *pWarpCube, int requiredCount)
    : al::LayoutActor("ワープキューブカウンタ"), mWarpCube(pWarpCube),
      mRequiredCount(requiredCount) {
  al::initLayoutActor(this, rInfo, "CounterWarpCube", nullptr);
  initNerve(&NrvCounterWarpCubeWait, 0);
  al::setPaneString(this, "TxtCounter", u"", 0, -1);
  setPieceCount<LayoutFontUtil::getIconFontWarpCubeLockedPieceEmpty>(
      this, "TxtCounterBase", requiredCount);
}
/** @brief Activates the counter and starts its appearance animation. */
void CounterWarpCube::appear() {
  al::LayoutActor::appear();
  al::setNerve(this, &NrvCounterWarpCubeAppear);
}
/** @brief Records a collected piece and completes the warp cube when ready. */
void CounterWarpCube::addCount(const al::LiveActor *pCollector,
                               al::HitSensor *pSensor) {
  int count = ++mCount;
  setPieceCount<LayoutFontUtil::getIconFontWarpCubeLockedPiece>(
      this, "TxtCounter", count);
  if (mRequiredCount <= mCount) {
    mWarpCube->endGetPiece(pSensor);
    al::setNerve(this, &NrvCounterWarpCubeComplete);
  } else {
    rc::addScore(pCollector, pSensor, 0.0f, 0);
    if (mCount <= 1)
      appear();
    else
      al::setNerve(this, &NrvCounterWarpCubeAdd);
  }
}
/** @brief Shows the first collected piece with its collection sound. */
void CounterWarpCube::exeAppear() {
  if (al::isFirstStep(this)) {
    al::startAction(this, "Appear", nullptr);
    if (auto *pParam =
            al::startSeByNameWithParam(this, "PgGetPiece", 1.0f, nullptr))
      pParam->setLocalVariable(mCount, 0);
    al::setPaneString(this, "TxtCounter",
                      LayoutFontUtil::getIconFontWarpCubeLockedPiece(), 0, -1);
  }
  if (al::isActionEnd(this, nullptr))
    al::setNerve(this, &NrvCounterWarpCubeWait);
}
/** @brief Starts the waiting animation. */
void CounterWarpCube::exeWait() {
  if (al::isFirstStep(this))
    al::startAction(this, "Wait", nullptr);
}
/** @brief Animates another collected piece. */
void CounterWarpCube::exeAdd() {
  if (al::isFirstStep(this)) {
    al::startAction(this, "Add", nullptr);
    if (auto *pParam =
            al::startSeByNameWithParam(this, "PgGetPiece", 1.0f, nullptr))
      pParam->setLocalVariable(mCount, 0);
  }
  if (al::isActionEnd(this, nullptr))
    al::setNerve(this, &NrvCounterWarpCubeWait);
}
/** @brief Plays the completed-counter animation and sound. */
void CounterWarpCube::exeComplete() {
  if (al::isFirstStep(this)) {
    al::startAction(this, "Complete", nullptr);
    if (auto *pParam = al::startSeByNameWithParam(this, "PgGetPieceComplete",
                                                  1.0f, nullptr))
      pParam->setLocalVariable(mCount, 0);
  }
  if (al::isActionEnd(this, nullptr))
    al::setNerve(this, &NrvCounterWarpCubeEnd);
}
/** @brief Hides the counter after its ending animation. */
void CounterWarpCube::exeEnd() {
  if (al::isFirstStep(this))
    al::startAction(this, "End", nullptr);
  if (al::isActionEnd(this, nullptr))
    kill();
}
