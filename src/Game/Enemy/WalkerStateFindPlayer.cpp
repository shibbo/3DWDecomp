#include "Enemy/WalkerStateFindPlayer.hpp"
#include "Enemy/TargetFinder.hpp"
#include "Enemy/WalkerStateFunction.hpp"
#include "Enemy/WalkerStateJump.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"

namespace {
NERVE_DECL(WalkerStateFindPlayer, Find)
NERVE_DECL(WalkerStateFindPlayer, Jump)
NERVE_DECL(WalkerStateFindPlayer, FindJump)
const struct {
    WalkerStateFindPlayerNrvFind Find;
    WalkerStateFindPlayerNrvJump Jump;
    WalkerStateFindPlayerNrvFindJump FindJump;
} NrvWalkerStateFindPlayer;
const WalkerStateFindPlayerParam cDefaultParam;
}

/** @brief Initializes a sixty-frame turn with the walking animation and no jump. */
WalkerStateFindPlayerParam::WalkerStateFindPlayerParam()
    : mTurnTime(60), mTurnDegree(2.5f), mIsJump(false), mTurnAction("Walk") {}

/**
 * @brief Initializes the reaction to finding a player.
 * @param turnTime Turning duration in frames.
 * @param turnDegree Maximum turning angle per frame.
 * @param isJump Whether to use the nested jump state after turning.
 * @param pTurnAction Animation played while turning.
 */
WalkerStateFindPlayerParam::WalkerStateFindPlayerParam(s32 turnTime, f32 turnDegree,
        bool isJump, const char* pTurnAction)
    : mTurnTime(turnTime), mTurnDegree(turnDegree), mIsJump(isJump), mTurnAction(pTurnAction) {}

/**
 * @brief Constructs the turn-and-react state for a newly found player.
 * @param pHost Walking enemy.
 * @param pFrontDir Mutable facing direction.
 * @param pTargetFinder Current player target.
 * @param pParam Gravity and friction parameters.
 * @param pFindParam Detection reaction settings, or null for defaults.
 * @param pJumpParam Settings for the nested jump state.
 */
WalkerStateFindPlayer::WalkerStateFindPlayer(al::LiveActor* pHost, sead::Vector3f* pFrontDir,
        const TargetFinder* pTargetFinder, const WalkerStateParam* pParam,
        const WalkerStateFindPlayerParam* pFindParam, const WalkerStateJumpParam* pJumpParam)
    : al::ActorStateBase("歩行型プレイヤー発見挙動", pHost), mFrontDir(pFrontDir),
      mTargetFinder(pTargetFinder), mParam(pParam), mFindParam(pFindParam) {
    initNerve(&NrvWalkerStateFindPlayer.Find, 1);
    mJumpState = new WalkerStateJump(pHost, pParam, pJumpParam);
    al::initNerveState(this, mJumpState, &NrvWalkerStateFindPlayer.Jump, "[state]ジャンプ");
    if (!mFindParam) {
        mFindParam = &cDefaultParam;
    }
}

/** @brief Starts turning toward the detected player. */
void WalkerStateFindPlayer::appear() {
    al::ActorStateBase::appear();
    al::setNerve(this, &NrvWalkerStateFindPlayer.Find);
}

/** @brief Turns toward the target before starting the configured detection reaction. */
void WalkerStateFindPlayer::exeFind() {
    if (al::isFirstStep(this)) {
        if (!al::isSklAnimPlaying(mHostActor, mFindParam->mTurnAction.cstr(), 0)) {
            al::startAction(mHostActor, mFindParam->mTurnAction.cstr());
        }
    }
    WalkerStateFunction::calcPassiveMovement(mHostActor, mParam);
    if (mTargetFinder->isExistTarget()) {
        al::turnDirectionToTargetDegree(mHostActor, mFrontDir, mTargetFinder->getTargetPos(),
                                        mFindParam->mTurnDegree);
    }
    if (al::isGreaterStep(this, mFindParam->mTurnTime)) {
        if (mFindParam->mIsJump) {
            al::setNerve(this, &NrvWalkerStateFindPlayer.Jump);
        } else {
            al::setNerve(this, &NrvWalkerStateFindPlayer.FindJump);
        }
    }
}

/** @brief Plays the detection animation and completes the reaction when it ends. */
void WalkerStateFindPlayer::exeFindJump() {
    if (al::isFirstStep(this)) {
        al::startAction(mHostActor, "Find");
    }
    WalkerStateFunction::calcPassiveMovement(mHostActor, mParam);
    if (al::isActionEnd(mHostActor)) {
        kill();
    }
}

/** @brief Completes the reaction when its nested jump state finishes. */
void WalkerStateFindPlayer::exeJump() {
    if (al::updateNerveState(this)) {
        kill();
    }
}
