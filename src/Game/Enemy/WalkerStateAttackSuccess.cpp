#include "Enemy/WalkerStateAttackSuccess.hpp"
#include "Enemy/WalkerStateFunction.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Project/Base/StringUtil.hpp"

namespace {
NERVE_DECL(WalkerStateAttackSuccess, AttackSuccessStart)
NERVE_DECL(WalkerStateAttackSuccess, AttackSuccess)
NERVE_DECL(WalkerStateAttackSuccess, AttackSuccessLoop)
NERVE_DECL(WalkerStateAttackSuccess, AttackSuccessEnd)
NERVES_MAKE_NOSTRUCT(WalkerStateAttackSuccess, AttackSuccessStart, AttackSuccess,
                     AttackSuccessLoop, AttackSuccessEnd)
}

/** @brief Initializes the default attack-success animations without a jump. */
WalkerStateAttackSuccessParam::WalkerStateAttackSuccessParam()
    : mActionStart("AttackStart"), mActionLoop("AttackLoop"), mActionEnd("AttackEnd"),
      mIsJump(false), mJumpSpeed(0.0f) {}

/**
 * @brief Builds the animation names and jump settings for an attack-success reaction.
 * @param pAction Base animation name.
 * @param isJump Whether the reaction uses separate start, loop, and end phases.
 * @param jumpSpeed Initial upward speed for the jumping reaction.
 */
WalkerStateAttackSuccessParam::WalkerStateAttackSuccessParam(const char* pAction,
        bool isJump, float jumpSpeed) : mIsJump(isJump), mJumpSpeed(jumpSpeed) {
    mActionStart = al::StringTmp<32>("%sStart", pAction);
    if (mIsJump) {
        mActionLoop = al::StringTmp<32>("%sLoop", pAction);
    } else {
        mActionLoop = pAction;
    }
    mActionEnd = al::StringTmp<32>("%sEnd", pAction);
}

/**
 * @brief Constructs the enemy's successful-attack reaction state.
 * @param pHost Walking enemy.
 * @param pParam Gravity and friction parameters.
 * @param pAttackParam Reaction animations and jump settings.
 */
WalkerStateAttackSuccess::WalkerStateAttackSuccess(al::LiveActor* pHost,
        const WalkerStateParam* pParam, const WalkerStateAttackSuccessParam* pAttackParam)
    : al::ActorStateBase("攻撃成功挙動", pHost), mParam(pParam), mAttackParam(pAttackParam) {
    if (mAttackParam->mIsJump) {
        initNerve(&NrvWalkerStateAttackSuccessAttackSuccessStart, 0);
    } else {
        initNerve(&NrvWalkerStateAttackSuccessAttackSuccess, 0);
    }
}

/** @brief Stops the enemy and starts its configured reaction sequence. */
void WalkerStateAttackSuccess::appear() {
    al::ActorStateBase::appear();
    al::setVelocityZero(mHostActor);
    if (mAttackParam->mIsJump) {
        al::setNerve(this, &NrvWalkerStateAttackSuccessAttackSuccessStart);
    } else {
        al::setNerve(this, &NrvWalkerStateAttackSuccessAttackSuccess);
    }
}

/** @brief Plays a single reaction animation and completes the state when it ends. */
void WalkerStateAttackSuccess::exeAttackSuccess() {
    if (al::isFirstStep(this)) {
        al::startAction(mHostActor, mAttackParam->mActionLoop.cstr());
    }
    WalkerStateFunction::calcPassiveMovement(mHostActor, mParam);
    if (al::isActionEnd(mHostActor)) {
        kill();
    }
}

/** @brief Starts the jump and transitions to the airborne reaction animation. */
void WalkerStateAttackSuccess::exeAttackSuccessStart() {
    if (al::isFirstStep(this)) {
        al::startAction(mHostActor, mAttackParam->mActionStart.cstr());
        al::setVelocityJump(mHostActor, mAttackParam->mJumpSpeed);
    }
    WalkerStateFunction::calcPassiveMovement(mHostActor, mParam);
    if (al::isActionEnd(mHostActor)) {
        al::setNerve(this, &NrvWalkerStateAttackSuccessAttackSuccessLoop);
    }
}

/** @brief Plays the airborne reaction until the enemy lands. */
void WalkerStateAttackSuccess::exeAttackSuccessLoop() {
    if (al::isFirstStep(this)) {
        al::startAction(mHostActor, mAttackParam->mActionLoop.cstr());
    }
    WalkerStateFunction::calcPassiveMovement(mHostActor, mParam);
    if (al::isOnGround(mHostActor, 0, 0.0f)) {
        al::setNerve(this, &NrvWalkerStateAttackSuccessAttackSuccessEnd);
    }
}

/** @brief Plays the landing reaction and completes the state when it ends. */
void WalkerStateAttackSuccess::exeAttackSuccessEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(mHostActor, mAttackParam->mActionEnd.cstr());
    }
    WalkerStateFunction::calcPassiveMovement(mHostActor, mParam);
    if (al::isActionEnd(mHostActor)) {
        kill();
    }
}
