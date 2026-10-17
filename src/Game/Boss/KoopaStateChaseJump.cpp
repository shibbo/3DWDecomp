#include "Boss/KoopaChaseStateJump.hpp"
#include "Boss/KoopaChase.hpp"
#include "Boss/KoopaChaseKoopa.hpp"
#include "Boss/KoopaChaseFunction.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Movement/ParabolicPathMovement.hpp"
#include "Library/Nerve/NerveSetup.hpp"

namespace {
NERVE_DECL(KoopaChaseStateJump, JumpStart);
NERVE_DECL(KoopaChaseStateJump, Jump);
NERVE_DECL(KoopaChaseStateJump, JumpEnd);
NERVES_MAKE_NOSTRUCT(KoopaChaseStateJump, JumpStart, Jump, JumpEnd)
}

/**
 * @brief Creates a chase jump state and its parabolic movement helper.
 * @param pHost Chase actor controlled by this state.
 * @param rInfo Actor initialization information; unused.
 */
KoopaChaseStateJump::KoopaChaseStateJump(KoopaChase* pHost, const al::ActorInitInfo& rInfo)
    : al::HostStateBase<KoopaChase>("State[ジャンプ]", pHost),
      mStartPosition(sead::Vector3f::zero), mTargetPosition(sead::Vector3f::zero),
      mMovement(new al::ParabolicPathMovement(pHost, false)) {
    initNerve(&NrvKoopaChaseStateJumpJumpStart, 0);
}

/**
 * @brief Starts a jump toward the target position.
 * @param rTarget Destination world position.
 */
void KoopaChaseStateJump::startJump(const sead::Vector3f& rTarget) {
    appear();
    mStartPosition.set(al::getTrans(getHost()));
    mTargetPosition.set(rTarget);
    al::setNerve(this, &NrvKoopaChaseStateJumpJumpStart);
}

/**
 * @brief Starts a jump with the warp animations.
 * @param rTarget Destination world position.
 */
void KoopaChaseStateJump::startWarp(const sead::Vector3f& rTarget) {
    mIsWarp = true;
    appear();
    mStartPosition.set(al::getTrans(getHost()));
    mTargetPosition.set(rTarget);
    al::setNerve(this, &NrvKoopaChaseStateJumpJumpStart);
}

/** @brief Plays the selected jump-start animations and follows Koopa's joint. */
void KoopaChaseStateJump::exeJumpStart() {
    if (al::isFirstStep(this)) {
        const char* pAction = mIsWarp ? KoopaChaseFunction::getAnimNameWarpJumpStart(getHost())
                                     : KoopaChaseFunction::getAnimNameJumpStart(getHost());
        al::startAction(getHost(), pAction);
        if (!KoopaChaseFunction::getKoopa(getHost())->isStateDamage()) {
            al::startAction(KoopaChaseFunction::getKoopa(getHost()), mIsWarp ? "WarpJumpStart" : "JumpStart");
        }
    }
    al::calcJointPos(al::getTransPtr(KoopaChaseFunction::getKoopa(getHost())), getHost(), "KoopaPosition");
    if (al::isActionEnd(getHost())) {
        al::setNerve(this, &NrvKoopaChaseStateJumpJump);
    }
}

/** @brief Advances the parabolic jump and starts landing when it reaches the end. */
void KoopaChaseStateJump::exeJump() {
    if (al::isFirstStep(this)) {
        const char* pAction = mIsWarp ? KoopaChaseFunction::getAnimNameWarpJumpLoop(getHost())
                                     : KoopaChaseFunction::getAnimNameJumpLoop(getHost());
        al::startAction(getHost(), pAction);
        if (!KoopaChaseFunction::getKoopa(getHost())->isStateDamage()) {
            al::startAction(KoopaChaseFunction::getKoopa(getHost()), mIsWarp ? "WarpJumpLoop" : "JumpLoop");
        }
        mMovement->start(mTargetPosition, 300.0f, 25.0f);
    }
    al::calcJointPos(al::getTransPtr(KoopaChaseFunction::getKoopa(getHost())), getHost(), "KoopaPosition");
    mMovement->updateNerve();
    if (mMovement->isReachedEnd()) {
        al::setNerve(this, &NrvKoopaChaseStateJumpJumpEnd);
    }
}

/** @brief Plays the landing animations and clears the warp flag when complete. */
void KoopaChaseStateJump::exeJumpEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(getHost(), KoopaChaseFunction::getAnimNameJumpEnd(getHost()));
        if (!KoopaChaseFunction::getKoopa(getHost())->isStateDamage()) {
            al::startAction(KoopaChaseFunction::getKoopa(getHost()), "JumpEnd");
        }
    }
    al::calcJointPos(al::getTransPtr(KoopaChaseFunction::getKoopa(getHost())), getHost(), "KoopaPosition");
    if (al::isActionEnd(getHost())) {
        mIsWarp = false;
        kill();
    }
}
