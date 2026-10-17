#include "Enemy/WalkerStateChase.hpp"
#include "Enemy/TargetFinder.hpp"
#include "Enemy/WalkerStateFunction.hpp"
#include "Enemy/WalkerStateParam.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"

namespace {
NERVE_DECL(WalkerStateChase, Start)
NERVE_DECL(WalkerStateChase, End)
NERVES_MAKE_NOSTRUCT(WalkerStateChase, Start, End)
const WalkerStateChaseParam cDefaultParam;
}

/** @brief Initializes a timed chase with running, waiting, and ledge avoidance. */
WalkerStateChaseParam::WalkerStateChaseParam()
    : mAccel(2.0f), mMinTime(130), mMaxTime(300), mRunAnimRate(4.0f), mEndTime(30),
      mIsKeepChase(false), mAvoidLedges(true), mRunAction("Run"), mWaitAction("Wait"),
      mValue88(-1.0f) {}

/**
 * @brief Initializes chase timing, movement, and animation settings.
 * @param accel Acceleration along the facing direction.
 * @param minTime Minimum chase duration before loss of the target ends it.
 * @param maxTime Maximum chase duration.
 * @param turnDegree Maximum turning angle per frame.
 * @param endTime Duration of the ending phase.
 * @param isKeepChase Whether to ignore duration-based termination.
 * @param avoidLedges Whether to remove forward velocity at a ledge.
 * @param pRunAction Chase animation name.
 * @param pWaitAction Ending animation name.
 * @param value88 Additional value retained by the original interface; unused by this state.
 */
WalkerStateChaseParam::WalkerStateChaseParam(f32 accel, f32 minTime, f32 maxTime,
        f32 turnDegree, f32 endTime, bool isKeepChase, bool avoidLedges,
        const char* pRunAction, const char* pWaitAction, f32 value88)
    : mAccel(accel), mMinTime(static_cast<s32>(minTime)), mMaxTime(static_cast<s32>(maxTime)),
      mRunAnimRate(turnDegree), mEndTime(static_cast<s32>(endTime)), mIsKeepChase(isKeepChase),
      mAvoidLedges(avoidLedges), mRunAction(pRunAction), mWaitAction(pWaitAction), mValue88(value88) {}

/**
 * @brief Constructs a target-chasing state.
 * @param pHost Walking enemy.
 * @param pFrontDir Mutable facing direction.
 * @param pTargetFinder Target finder updated by this state.
 * @param pParam Gravity and friction settings.
 * @param pChaseParam Chase settings, or null for defaults.
 * @param endOnLostTarget Whether a lost target immediately starts the ending phase.
 * @param pIsOnGround Optional externally supplied ground-contact flag.
 */
WalkerStateChase::WalkerStateChase(al::LiveActor* pHost, sead::Vector3f* pFrontDir,
        TargetFinder* pTargetFinder, const WalkerStateParam* pParam,
        const WalkerStateChaseParam* pChaseParam, bool endOnLostTarget, bool* pIsOnGround)
    : al::ActorStateBase("クリボー追いかけ状態", pHost), mFrontDir(pFrontDir), mParam(pParam),
      mChaseParam(pChaseParam), mEndOnLostTarget(endOnLostTarget), mIsOnGround(pIsOnGround),
      mTargetFinder(pTargetFinder) {
    if (!mChaseParam) {
        mChaseParam = &cDefaultParam;
    }
    initNerve(&NrvWalkerStateChaseStart, 0);
}

/** @brief Resets target-loss timing and begins chasing. */
void WalkerStateChase::appear() {
    mLostTargetStep = 0;
    al::ActorStateBase::appear();
    al::setNerve(this, &NrvWalkerStateChaseStart);
}

/** @brief Steers toward the target and checks for target loss, ledges, and timeout. */
void WalkerStateChase::exeStart() {
    if (al::isFirstStep(this)) {
        al::startAction(mHostActor, mChaseParam->mRunAction.cstr());
    }
    mTargetFinder->update();
    if (mEndOnLostTarget) {
        if (mTargetFinder->_12[0]) {
            al::setNerve(this, &NrvWalkerStateChaseEnd);
            return;
        }
        if (mTargetFinder->isFoundTargetNow()) {
            al::turnDirectionToTargetDegree(mHostActor, mFrontDir,
                    mTargetFinder->getTargetPos(), mChaseParam->mRunAnimRate);
            if (mIsOnGround ? *mIsOnGround : al::isOnGround(mHostActor, 0, 0.0f)) {
                al::addVelocity(mHostActor, *mFrontDir * mChaseParam->mAccel);
            }
            mLostTargetStep = 0;
        } else if (mLostTargetStep++ >= 8) {
            al::setNerve(this, &NrvWalkerStateChaseEnd);
            return;
        }
    } else {
        if (mTargetFinder->isFoundTargetNow()) {
            al::turnDirectionToTargetDegree(mHostActor, mFrontDir,
                    mTargetFinder->getTargetPos(), mChaseParam->mRunAnimRate);
        } else {
            al::turnDirectionToTargetDegree(mHostActor, mFrontDir,
                    mTargetFinder->getLastTargetPos(), mChaseParam->mRunAnimRate * 0.66f);
        }
        if (mIsOnGround ? *mIsOnGround : al::isOnGround(mHostActor, 0, 0.0f)) {
            al::addVelocity(mHostActor, *mFrontDir * mChaseParam->mAccel);
        }
    }
    if (mIsOnGround) {
        WalkerStateFunction::calcPassiveMovement(mHostActor, mParam, *mIsOnGround);
    } else {
        WalkerStateFunction::calcPassiveMovement(mHostActor, mParam);
    }
    if (mChaseParam->mAvoidLedges &&
            WalkerStateFunction::isFallNextMove(mHostActor, 50.0f, 150.0f, mParam->mValueC, false)) {
        sead::Vector3f* pVelocity = al::getVelocityPtr(mHostActor);
        al::verticalizeVec(pVelocity, *mFrontDir, *pVelocity);
    }
    if (mChaseParam->mIsKeepChase) {
        return;
    }
    if (al::isGreaterStep(this, mChaseParam->mMaxTime) ||
            (al::isGreaterStep(this, mChaseParam->mMinTime) && !mTargetFinder->isExistTarget())) {
        if (mChaseParam->mEndTime > 0) {
            al::setNerve(this, &NrvWalkerStateChaseEnd);
        } else {
            kill();
        }
    }
}

/** @brief Slows the actor during its ending animation and completes the state. */
void WalkerStateChase::exeEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(mHostActor, mChaseParam->mWaitAction.cstr());
    }
    if (mIsOnGround) {
        WalkerStateFunction::calcPassiveMovement(mHostActor, mParam, *mIsOnGround);
    } else {
        WalkerStateFunction::calcPassiveMovement(mHostActor, mParam);
    }
    if (al::isGreaterStep(this, mChaseParam->mEndTime)) {
        kill();
    }
}
