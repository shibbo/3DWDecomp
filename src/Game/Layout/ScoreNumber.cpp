#include "Layout/ScoreNumber.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Screen/ScreenFunction.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
namespace {
const char *const sBrigadeColors[] = {
    "KinopioBrigade", "KinopioBrigadeMember_B", "KinopioBrigadeMember_Y",
    "KinopioBrigadeMember_G"};
inline const char *getBrigadeScoreColor(unsigned int playerFlags) {
  if (playerFlags >= 1 && playerFlags <= 4)
    return sBrigadeColors[static_cast<int>(playerFlags) - 1];
  return "KinopioBrigadeMember_Y";
}
NERVE_DECL(ScoreNumber, Appear);
NERVE_DECL(ScoreNumber, Delay);
NERVES_MAKE_NOSTRUCT(ScoreNumber, Appear, Delay)
} // namespace
/** @brief Creates an initially hidden score popup. */
ScoreNumber::ScoreNumber(const al::LayoutInitInfo &rInfo)
    : al::LayoutActor("スコア数字"),
      mSceneCameraInfo(rInfo.getSceneCameraInfo()) {
  al::initLayoutActor(this, rInfo, "PopScoreNumber", nullptr);
  initNerve(&NrvScoreNumberAppear, 0);
  al::setPaneNumberDigit5(this, "TxtScore", 0, 0);
  kill();
}
/** @brief Configures the score, player color, position, and appearance delay.
 */
void ScoreNumber::startScore(int score, unsigned int playerFlags,
                             const sead::Vector2f &rPos, int delay) {
  mPosition = nullptr;
  mOffset.set(sead::Vector3f::zero);
  al::setLocalTrans(this, rPos);
  startScore(score, playerFlags, delay);
}
/** @brief Configures the score, player color, position, and appearance delay.
 */
void ScoreNumber::startScore(int score, unsigned int playerFlags, int delay) {
  if (score < 1000)
    al::startFreezeAction(this, "Level", 0.0f, "Level");
  else if (score < 4000)
    al::startFreezeAction(this, "Level", 1.0f, "Level");
  else
    al::startFreezeAction(this, "Level", 2.0f, "Level");

  bool isBrigade = GameDataFunction::isCurrentStageKinopioBrigade(
      GameDataHolderAccessor(this));
  al::startAction(this,
                  isBrigade
                      ? getBrigadeScoreColor(playerFlags)
                      : GameDataFunction::getPlayerCharacterNameFromBitFlag(
                            GameDataHolderAccessor(this), playerFlags),
                  "PlayerColor");
  al::setPaneNumberDigit5(this, "TxtScore", score, 0);
  start(delay);
}
/** @brief Configures the score, player color, position, and appearance delay.
 */
void ScoreNumber::startScore(int score, unsigned int playerFlags,
                             const sead::Vector3f &rPos, int delay) {
  mFixedPosition.set(rPos);
  mPosition = &mFixedPosition;
  mOffset.set(sead::Vector3f::zero);
  sead::Vector3f pos;
  al::calcLayoutPosFromWorldPos(&pos, this, rPos);
  updateByWorldPosition(pos);
  startScore(score, playerFlags, delay);
}
/** @brief Places the popup at projected coordinates and hides it behind the
 * camera. */
void ScoreNumber::updateByWorldPosition(const sead::Vector3f &rPos) {
  al::setLocalTrans(this, sead::Vector2f(rPos.x, rPos.y));
  float z = rPos.z;
  bool isHidden = al::isHidePaneRoot(this);
  if (!(z >= 0.0f)) {
    if (isHidden)
      al::showPaneRoot(this);
  } else if (!isHidden)
    al::hidePaneRoot(this);
}
/** @brief Configures the score, player color, position, and appearance delay.
 */
void ScoreNumber::startScore(int score, unsigned int playerFlags,
                             const sead::Vector3f *pPos,
                             const sead::Vector3f &rOffset, int delay) {
  mPosition = pPos;
  mOffset.set(rOffset);
  sead::Vector3f pos;
  al::calcLayoutPosFromWorldPos(&pos, this, *mPosition + mOffset);
  updateByWorldPosition(pos);
  startScore(score, playerFlags, delay);
}
/** @brief Configures an extra-life popup and its appearance delay. */
void ScoreNumber::startPlayerUp(int count, const sead::Vector2f &rPos,
                                int delay) {
  mPosition = nullptr;
  mOffset.set(sead::Vector3f::zero);
  al::setLocalTrans(this, rPos);
  startPlayerUp(count, delay);
}
/** @brief Configures an extra-life popup and its appearance delay. */
void ScoreNumber::startPlayerUp(int count, int delay) {
  al::startFreezeAction(this, "Level", 3.0f, "Level");
  al::setPaneString(this, "TxtScore",
                    sead::WFormatFixedSafeString<6>(u"%d", count % 100).cstr(),
                    0, -1);
  al::startAction(this, "OneUp", "PlayerColor");
  start(delay);
  if (delay == 0)
    al::startSe(this, "PgOneUp");
  mIsPlayerUp = true;
}
/** @brief Configures an extra-life popup and its appearance delay. */
void ScoreNumber::startPlayerUp(int count, const sead::Vector3f &rPos,
                                int delay) {
  mFixedPosition.set(rPos);
  mPosition = &mFixedPosition;
  mOffset.set(sead::Vector3f::zero);
  sead::Vector3f pos;
  al::calcLayoutPosFromWorldPos(&pos, this, rPos);
  updateByWorldPosition(pos);
  startPlayerUp(count, delay);
}
/** @brief Configures an extra-life popup and its appearance delay. */
void ScoreNumber::startPlayerUp(int count, const sead::Vector3f *pPos,
                                const sead::Vector3f &rOffset, int delay) {
  mPosition = pPos;
  mOffset.set(rOffset);
  sead::Vector3f pos;
  al::calcLayoutPosFromWorldPos(&pos, this, *mPosition + mOffset);
  updateByWorldPosition(pos);
  startPlayerUp(count, delay);
}
/** @brief Starts the popup immediately or after the requested delay. */
void ScoreNumber::start(int delay) {
  mDelay = delay;
  if (delay > 0) {
    al::startAction(this, "Hide", nullptr);
    al::setNerve(this, &NrvScoreNumberDelay);
  } else
    al::setNerve(this, &NrvScoreNumberAppear);
  appear();
}
/** @brief Tracks the source position while the popup is active. */
void ScoreNumber::control() {
  if (mPosition) {
    sead::Vector3f pos;
    al::calcLayoutPosFromWorldPos(&pos, this, *mPosition + mOffset);
    updateByWorldPosition(pos);
  }
}
/** @brief Waits for the appearance delay and plays the extra-life sound when
 * needed. */
void ScoreNumber::exeDelay() {
  if (al::isGreaterEqualStep(this, mDelay - 1)) {
    if (mIsPlayerUp)
      al::startSe(this, "PgOneUp");
    al::setNerve(this, &NrvScoreNumberAppear);
  }
}
/** @brief Plays the popup animation and kills it when complete. */
void ScoreNumber::exeAppear() {
  if (al::isFirstStep(this))
    al::startAction(this, "Appear", nullptr);
  if (al::isActionEnd(this, nullptr)) {
    mIsPlayerUp = false;
    kill();
  }
}
