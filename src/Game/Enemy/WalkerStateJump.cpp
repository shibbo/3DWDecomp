#include "Enemy/WalkerStateJump.hpp"
#include "Enemy/WalkerStateFunction.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Project/Base/StringUtil.hpp"

namespace {
NERVE_DECL(WalkerStateJump, JumpStart)
NERVE_DECL(WalkerStateJump, JumpLoop)
NERVE_DECL(WalkerStateJump, JumpEnd)
NERVES_MAKE_NOSTRUCT(WalkerStateJump, JumpStart, JumpLoop, JumpEnd)
const WalkerStateJumpParam cDefaultParam;
}

/** @brief Initializes the default animated jump with an upward speed of twenty. */
WalkerStateJumpParam::WalkerStateJumpParam()
    : mJumpSpeed(20.0f), mReplaceVelocity(true), mUseAnimation(true),
      mActionStart("JumpStart"), mActionLoop("JumpLoop"), mActionEnd("JumpEnd") {}

/**
 * @brief Initializes a jump with the standard animation names.
 * @param speed Initial upward speed.
 * @param replaceVelocity Whether to replace upward velocity instead of adding to it.
 */
WalkerStateJumpParam::WalkerStateJumpParam(float speed, bool replaceVelocity)
    : mJumpSpeed(speed), mReplaceVelocity(replaceVelocity), mUseAnimation(true),
      mActionStart("JumpStart"), mActionLoop("JumpLoop"), mActionEnd("JumpEnd") {}

/**
 * @brief Builds jump animation names from a common prefix.
 * @param speed Initial upward speed.
 * @param pAction Animation prefix, or null to disable animations.
 * @param replaceVelocity Whether to replace upward velocity instead of adding to it.
 */
WalkerStateJumpParam::WalkerStateJumpParam(float speed, const char* pAction, bool replaceVelocity)
    : mJumpSpeed(speed), mReplaceVelocity(replaceVelocity), mUseAnimation(true) {
    if (pAction) {
        mActionStart = al::StringTmp<32>("%sStart", pAction);
        mActionLoop = al::StringTmp<32>("%sLoop", pAction);
        mActionEnd = al::StringTmp<32>("%sEnd", pAction);
    } else {
        mUseAnimation = false;
    }
}

/**
 * @brief Initializes a jump with separately specified animations.
 * @param speed Initial upward speed.
 * @param pStart Launch animation name.
 * @param pLoop Airborne animation name.
 * @param pEnd Landing animation name.
 * @param replaceVelocity Whether to replace upward velocity instead of adding to it.
 */
WalkerStateJumpParam::WalkerStateJumpParam(float speed, const char* pStart, const char* pLoop,
        const char* pEnd, bool replaceVelocity)
    : mJumpSpeed(speed), mReplaceVelocity(replaceVelocity), mUseAnimation(true),
      mActionStart(pStart), mActionLoop(pLoop), mActionEnd(pEnd) {}

/**
 * @brief Constructs the walking enemy's jump state.
 * @param pHost Walking enemy.
 * @param pParam Gravity and friction parameters.
 * @param pJumpParam Jump settings, or null to use the defaults.
 */
WalkerStateJump::WalkerStateJump(al::LiveActor* pHost, const WalkerStateParam* pParam,
        const WalkerStateJumpParam* pJumpParam)
    : al::ActorStateBase("歩行型ジャンプ挙動", pHost), mParam(pParam),
      mJumpParam(pJumpParam ? pJumpParam : &cDefaultParam) {
    initNerve(&NrvWalkerStateJumpJumpStart, 0);
}

/** @brief Activates the state and starts a fresh jump. */
void WalkerStateJump::appear() {
    al::ActorStateBase::appear();
    al::setNerve(this, &NrvWalkerStateJumpJumpStart);
}

/** @brief Launches the actor and waits for the optional launch animation. */
void WalkerStateJump::exeJumpStart() {
    if (al::isFirstStep(this)) {
        if (mJumpParam->mReplaceVelocity) {
            al::setVelocityJump(mHostActor, mJumpParam->mJumpSpeed);
        } else {
            al::addVelocityJump(mHostActor, mJumpParam->mJumpSpeed);
        }
        if (mJumpParam->mUseAnimation) {
            al::startAction(mHostActor, mJumpParam->mActionStart.cstr());
        }
    }
    WalkerStateFunction::calcPassiveMovement(mHostActor, mParam);
    if (!mJumpParam->mUseAnimation || al::isActionEnd(mHostActor)) {
        al::setNerve(this, &NrvWalkerStateJumpJumpLoop);
    }
}

/** @brief Applies airborne motion until the actor lands. */
void WalkerStateJump::exeJumpLoop() {
    if (al::isFirstStep(this) && mJumpParam->mUseAnimation) {
        al::startAction(mHostActor, mJumpParam->mActionLoop.cstr());
    }
    WalkerStateFunction::calcPassiveMovement(mHostActor, mParam);
    if (al::isOnGround(mHostActor, 0, 0.0f)) {
        al::setNerve(this, &NrvWalkerStateJumpJumpEnd);
    }
}

/** @brief Completes the state after the optional landing animation. */
void WalkerStateJump::exeJumpEnd() {
    if (al::isFirstStep(this) && mJumpParam->mUseAnimation) {
        al::startAction(mHostActor, mJumpParam->mActionEnd.cstr());
    }
    WalkerStateFunction::calcPassiveMovement(mHostActor, mParam);
    if (!mJumpParam->mUseAnimation || al::isActionEnd(mHostActor)) {
        kill();
    }
}
