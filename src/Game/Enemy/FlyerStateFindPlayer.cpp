#include "Enemy/FlyerStateFindPlayer.hpp"
#include "Enemy/FlyerStateFindPlayerParam.hpp"
#include "Enemy/TargetFinder.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"

namespace {
NERVE_DECL(FlyerStateFindPlayer, TurnPlayer)
NERVE_DECL(FlyerStateFindPlayer, Find)
NERVES_MAKE_NOSTRUCT(FlyerStateFindPlayer, TurnPlayer, Find)
const FlyerStateFindPlayerParam cDefaultParam;
}

/** @brief Initializes default turning speed and the discovery action. */
FlyerStateFindPlayerParam::FlyerStateFindPlayerParam() : _0(2.5f), mAction("Find") {}

/** @brief Initializes custom turning and animation settings.
 * @param turnDegree Maximum turning angle per frame.
 * @param pAction Discovery animation name.
 */
FlyerStateFindPlayerParam::FlyerStateFindPlayerParam(f32 turnDegree, char* pAction)
    : _0(turnDegree), mAction(pAction) {}

/** @brief Initializes the player discovery state.
 * @param pHost Flying enemy.
 * @param pFront Direction to turn toward the player.
 * @param pTargetFinder Player search controller.
 * @param pParam Flight settings.
 * @param pFindParam Discovery settings, or nullptr for defaults.
 */
FlyerStateFindPlayer::FlyerStateFindPlayer(al::LiveActor* pHost, sead::Vector3f* pFront,
        TargetFinder* pTargetFinder, const FlyerStateParam* pParam, const FlyerStateFindPlayerParam* pFindParam)
    : al::ActorStateBase("飛行型プレイヤー発見挙動", pHost), mFront(pFront), mTargetFinder(pTargetFinder),
      mParam(pParam), mFindParam(pFindParam) {
    initNerve(&NrvFlyerStateFindPlayerTurnPlayer, 1);
    if (!mFindParam) { mFindParam = &cDefaultParam; }
}

/** @brief Restarts turning toward the player. */
void FlyerStateFindPlayer::appear() {
    al::ActorStateBase::appear();
    al::setNerve(this, &NrvFlyerStateFindPlayerTurnPlayer);
}

/** @brief Ends discovery and clears the animation request. */
void FlyerStateFindPlayer::kill() {
    al::ActorStateBase::kill();
    _20 = false;
}

/** @brief Stops the actor and turns toward the current player. */
void FlyerStateFindPlayer::exeTurnPlayer() {
    if (al::isFirstStep(this)) { al::setVelocityZero(mHostActor); }
    mTargetFinder->update();
    if (!mTargetFinder->isExistTarget()) { kill(); return; }
    if (al::turnDirectionToTargetDegree(mHostActor, mFront, mTargetFinder->getTargetPos(), mFindParam->_0)) {
        if (_20) { al::setNerve(this, &NrvFlyerStateFindPlayerFind); }
        else { kill(); }
    }
}

/** @brief Plays the discovery action and ends when it finishes. */
void FlyerStateFindPlayer::exeFind() {
    if (al::isFirstStep(this)) {
        al::startAction(mHostActor, mFindParam->mAction.cstr());
        if (_21) { al::startAction(al::getSubActor(mHostActor, 0), mFindParam->mAction.cstr()); }
    }
    if (al::isActionEnd(mHostActor)) { kill(); }
}
