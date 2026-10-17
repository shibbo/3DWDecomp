#include "Layout/PlayerEntryPlayer.hpp"
#include "Layout/PlayerEntry.hpp"
#include "Layout/PlayerEntryPlayerHolder.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/SubActorKeeper.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Player/Normal/PlayerAnimFunc.hpp"
#include "Player/Normal/PlayerKeyConfig.hpp"
#include "Player/Normal/PlayerModelHolder.hpp"
#include "Player/Normal/PlayerRetargettingSelector.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Util/ControlUserUtil.hpp"
namespace {
const sead::Vector3f sEntryBasePosition(0, 0, 0);
NERVE_DECL(PlayerEntryPlayer, Fall);
NERVE_DECL(PlayerEntryPlayer, Decision);
NERVE_DECL(PlayerEntryPlayer, JumpOut);
NERVE_DECL(PlayerEntryPlayer, Land);
NERVE_DECL(PlayerEntryPlayer, Wait);
NERVE_DECL(PlayerEntryPlayer, Turn);
NERVE_DECL(PlayerEntryPlayer, Jump);
NERVES_MAKE_NOSTRUCT(PlayerEntryPlayer, Fall, Decision, JumpOut, Land, Wait,
                     Turn, Jump)
/** @brief Chooses the model yaw from horizontal stick input. */
inline float calcTargetRotation(const PlayerKeyConfig &rKeyConfig) {
  sead::Vector2f stick(0, 0);
  rKeyConfig.calcLeftStick(&stick);
  if (stick.x < -0.5f || stick.x > 0.5f)
    return stick.x > 0 ? 75.0f : -75.0f;
  return 0.0f;
}
} // namespace
/** @brief Creates a player entry actor linked to its model holder. */
PlayerEntryPlayer::PlayerEntryPlayer(const char *pName,
                                     PlayerEntryPlayerHolder *pHolder)
    : al::LiveActor(pName), mHolder(pHolder) {}
/** @brief Initializes the scene, transform, executor, and entry states. */
void PlayerEntryPlayer::init(const al::ActorInitInfo &rInfo) {
  al::initActorSceneInfo(this, rInfo);
  al::initActorSRT(this, rInfo);
  al::initExecutorWatchObj(this, rInfo);
  al::initNerve(this, &NrvPlayerEntryPlayerFall, 0);
  makeActorDead();
}
/** @brief Activates the model and prepares the initial fall. */
void PlayerEntryPlayer::appear() {
  al::LiveActor::appear();
  mPlayerModel->appear();
  mPlayerModel->getCurrentModel()->appear();
  al::hideModelIfShow(mPlayerModel->getCurrentModel());
  mIsWaitStartDecision = true;
  mRotateY = 0.0f;
  al::setNerve(this, &NrvPlayerEntryPlayerFall);
}
/** @brief Positions this user in the player-entry row. */
void PlayerEntryPlayer::appearPlayer(s32 userId) {
  appear();
  mUserId = userId;
  al::setTrans(this, sEntryBasePosition);
  int entryCount = PlayerEntry::getEntryPartsNumMax();
  float offset = (userId + (entryCount - 1) * -0.5f) * 160.0f;
  al::getTransPtr(this)->x += offset;
}
/** @brief Hides the actor and its player model. */
void PlayerEntryPlayer::kill() {
  al::LiveActor::kill();
  if (mPlayerModel) {
    mPlayerModel->kill();
    mPlayerModel->getCurrentModel()->kill();
  }
}
/** @brief Synchronizes the visible model with the entry actor. */
void PlayerEntryPlayer::control() {
  if (!mPlayerModel)
    return;
  auto *pModel = mPlayerModel->getCurrentModel();
  al::setTrans(pModel, al::getTrans(this));
  al::setRotateY(pModel, mRotateY);
}
/** @brief Stops movement and starts the decision animation. */
void PlayerEntryPlayer::startDecision() {
  al::getTransPtr(this)->y = 0;
  al::setVelocityZero(this);
  al::setNerve(this, &NrvPlayerEntryPlayerDecision);
}
/** @brief Starts leaving the player-entry screen. */
void PlayerEntryPlayer::startJumpOut() {
  al::setNerve(this, &NrvPlayerEntryPlayerJumpOut);
}
/** @brief Assigns the selected character model. */
void PlayerEntryPlayer::setPlayerModel(PlayerModelHolder *pModel) {
  mPlayerModel = pModel;
}
/** @brief Drops the model into the entry row. */
void PlayerEntryPlayer::exeFall() {
  if (al::isFirstStep(this)) {
    al::getTransPtr(this)->y = 500;
    startPlayerAction("PlayerEntryFall");
  }
  if (al::isStep(this, 8))
    al::startSe(mPlayerModel->getCurrentModel(), "PlayerEntryFallVoice");
  if (al::isStep(this, 1))
    al::showModelIfHide(mPlayerModel->getCurrentModel());
  al::addVelocityToGravity(this, 1.0f);
  al::scaleVelocityY(this, 0.99f);
  if (al::getTrans(this).y < 0)
    al::setNerve(this, &NrvPlayerEntryPlayerLand);
}
/** @brief Starts a retargeted animation on the model and its subactors. */
void PlayerEntryPlayer::startPlayerAction(const char *pActionName) {
  auto *pModel = mPlayerModel->getCurrentModel();
  al::StringTmp<128> name;
  volatile PlayerAnimFunc::RetargettingType type;
  type = PlayerAnimFunc::convertToRegularName(&name, pModel, pActionName);
  PlayerAnimFunc::controlRetargetting(pModel, mHolder->getSelector(), type);
  al::startAction(pModel, name.cstr());
  auto *pKeeper = pModel->getSubActorKeeper();
  if (!pKeeper)
    return;
  for (int i = 0; i < pKeeper->getSubActorNum(); ++i) {
    auto *pInfo = pKeeper->getSubActorInfo(i);
    if (!al::tryStartAction(pInfo->mSubActor, name.cstr()))
      al::tryStartAction(pInfo->mSubActor, "Wait");
  }
}
/** @brief Plays landing and accepts another jump. */
void PlayerEntryPlayer::exeLand() {
  if (al::isFirstStep(this)) {
    al::setVelocityZero(this);
    al::getTransPtr(this)->y = 0;
    startPlayerAction("PlayerEntryLand");
  }
  PlayerKeyConfig key(rc::getPadPortByUserId(mUserId));
  if (tryStartJump(key))
    return;
  if (isPlayerActionEnd())
    al::setNerve(this, &NrvPlayerEntryPlayerWait);
}
/** @brief Starts a jump when permitted by the entry state and input. */
bool PlayerEntryPlayer::tryStartJump(const PlayerKeyConfig &rKeyConfig) {
  if (!mIsWaitStartDecision)
    return false;
  if (!rKeyConfig.isPadTriggerPlayerJump())
    return false;
  if (rKeyConfig.isPadTriggerCancel())
    return false;
  al::setNerve(this, &NrvPlayerEntryPlayerJump);
  return true;
}
/** @brief Checks whether the current model animation has ended. */
bool PlayerEntryPlayer::isPlayerActionEnd() const {
  return al::isActionEnd(mPlayerModel->getCurrentModel());
}
/** @brief Waits for jump input and turns toward horizontal stick input. */
void PlayerEntryPlayer::exeWait() {
  if (al::isFirstStep(this))
    startPlayerAction("PlayerEntryWait");
  PlayerKeyConfig key(rc::getPadPortByUserId(mUserId));
  if (tryStartJump(key))
    return;
  mRotateY = al::converge(mRotateY, calcTargetRotation(key), 10.0f);
  if (!al::isNearZero(mRotateY, 0.001f))
    al::setNerve(this, &NrvPlayerEntryPlayerTurn);
}
/** @brief Updates yaw and returns to waiting when facing forward. */
void PlayerEntryPlayer::exeTurn() {
  if (al::isFirstStep(this))
    startPlayerAction("PlayerEntryTurn");
  PlayerKeyConfig key(rc::getPadPortByUserId(mUserId));
  if (tryStartJump(key))
    return;
  mRotateY = al::converge(mRotateY, calcTargetRotation(key), 10.0f);
  if (al::isNearZero(mRotateY, 0.001f))
    al::setNerve(this, &NrvPlayerEntryPlayerWait);
}
/** @brief Plays a jump variation and returns to landing at floor height. */
void PlayerEntryPlayer::exeJump() {
  if (al::isFirstStep(this)) {
    if (al::isHalfProbability())
      startPlayerAction("PlayerEntryJump");
    else
      startPlayerAction("PlayerEntryJumpReverse");
    al::setVelocityJump(this, 18.0f);
  }
  al::addVelocityToGravity(this, 1.0f);
  al::scaleVelocityY(this, 0.99f);
  if (al::getTrans(this).y < 0)
    al::setNerve(this, &NrvPlayerEntryPlayerLand);
}
/** @brief Jumps away and hides the model when it starts descending. */
void PlayerEntryPlayer::exeJumpOut() {
  if (al::isFirstStep(this)) {
    startPlayerAction("PlayerEntryJumpOut");
    al::setVelocityJump(this, 30.0f);
  }
  al::addVelocityToGravity(this, 1.0f);
  al::scaleVelocityY(this, 0.99f);
  if (al::getVelocity(this).y < 0)
    kill();
}
/** @brief Plays the decision animation while facing forward. */
void PlayerEntryPlayer::exeDecision() {
  if (al::isFirstStep(this))
    startPlayerAction("PlayerEntryDecision");
  mRotateY = al::converge(mRotateY, 0.0f, 10.0f);
}
/** @brief Reports whether the entry actor is inactive. */
bool PlayerEntryPlayer::isDead() const { return !al::isAlive(this); }
/** @brief Checks grounded entry states and the first few jump frames. */
bool PlayerEntryPlayer::isLand() const {
  if (al::isNerve(this, &NrvPlayerEntryPlayerLand))
    return true;
  if (al::isNerve(this, &NrvPlayerEntryPlayerWait))
    return true;
  if (al::isNerve(this, &NrvPlayerEntryPlayerTurn))
    return true;
  if (al::isNerve(this, &NrvPlayerEntryPlayerDecision))
    return true;
  if (al::isNerve(this, &NrvPlayerEntryPlayerJump) &&
      al::isLessEqualStep(this, 3))
    return true;
  return false;
}
/** @brief Checks whether the decision animation has ended. */
bool PlayerEntryPlayer::isEndDecision() const { return isPlayerActionEnd(); }
