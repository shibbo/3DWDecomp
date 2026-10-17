#include "NPC/NpcStateChaseParam.hpp"
#include "NPC/NpcStateChase.hpp"
#include "NPC/NpcStateFunction.hpp"
#include "NPC/NpcTargetFinder.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorMovementUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"

namespace {
NpcStateChaseParam sDefaultParam;
NERVE_DECL(NpcStateChase, Start)
NERVE_DECL(NpcStateChase, End)
NERVES_MAKE_NOSTRUCT(NpcStateChase, Start, End)
}

/** @brief Initializes the default chase acceleration, timing, and actions. */
NpcStateChaseParam::NpcStateChaseParam()
    : mAcceleration(2.0f), mMinFrames(130), mMaxFrames(300), mTurnDegrees(4.0f),
      mEndFrames(30), mIsEndless(false), mAvoidFalls(true), mAvoidAreas(true),
      mChaseAction("Run"), mEndAction("Wait"), mParam88(-1.0f) {}

/**
 * @brief Stores the settings for an NPC chase.
 * @param acceleration Acceleration toward the facing direction.
 * @param minFrames Minimum chase duration before losing a target ends the chase.
 * @param maxFrames Maximum chase duration.
 * @param turnDegrees Turn speed toward the target.
 * @param endFrames Duration of the ending state.
 * @param isEndless Whether to ignore duration limits.
 * @param avoidFalls Whether to stop movement toward ledges.
 * @param avoidAreas Whether to stop movement into NPC avoidance areas.
 * @param pChaseAction Action played while chasing.
 * @param pEndAction Action played while ending the chase.
 * @param param88 Additional chase parameter.
 */
NpcStateChaseParam::NpcStateChaseParam(float acceleration, float minFrames, float maxFrames,
                                     float turnDegrees, float endFrames, bool isEndless,
                                     bool avoidFalls, bool avoidAreas, const char* pChaseAction,
                                     const char* pEndAction, float param88)
    : mAcceleration(acceleration), mMinFrames(static_cast<int>(minFrames)),
      mMaxFrames(static_cast<int>(maxFrames)), mTurnDegrees(turnDegrees),
      mEndFrames(static_cast<int>(endFrames)), mIsEndless(isEndless),
      mAvoidFalls(avoidFalls), mAvoidAreas(avoidAreas), mChaseAction(pChaseAction),
      mEndAction(pEndAction), mParam88(param88) {}

/**
 * @brief Creates a chase state using the supplied target finder and movement settings.
 * @param pActor Actor controlled by the state.
 * @param pDirection Mutable facing direction.
 * @param pTargetFinder Finder supplying target positions and visibility.
 * @param pStateParam Passive movement settings.
 * @param pChaseParam Chase settings, or nullptr for defaults.
 * @param requireVisibleTarget Whether losing visibility ends the chase.
 * @param pIsGrounded Optional externally supplied ground-contact flag.
 */
NpcStateChase::NpcStateChase(al::LiveActor* pActor, sead::Vector3f* pDirection,
                            NpcTargetFinder* pTargetFinder, const NpcStateParam* pStateParam,
                            const NpcStateChaseParam* pChaseParam, bool requireVisibleTarget,
                            bool* pIsGrounded)
    : al::ActorStateBase("NpcStateChase", pActor), mDirection(pDirection),
      mStateParam(pStateParam), mChaseParam(pChaseParam),
      mRequireVisibleTarget(requireVisibleTarget), mIsGrounded(pIsGrounded),
      mLostTargetFrames(0), mTargetFinder(pTargetFinder) {
    if (!mChaseParam) {
        mChaseParam = &sDefaultParam;
    }
    initNerve(&NrvNpcStateChaseStart, 0);
}

/** @brief Releases the chase state's nerve executor. */
NpcStateChase::~NpcStateChase() = default;

/** @brief Restarts the chase and clears the lost-target counter. */
void NpcStateChase::appear() {
    mLostTargetFrames = 0;
    mIsDead = false;
    al::setNerve(this, &NrvNpcStateChaseStart);
}

/** @brief Chases the current target while applying ground and avoidance constraints. */
void NpcStateChase::exeStart() {
    if (al::isFirstStep(this)) {
        al::startAction(mHostActor, mChaseParam->mChaseAction.cstr());
    }
    mTargetFinder->update();
    if (mRequireVisibleTarget) {
        if (mTargetFinder->mHasTargetChanged) {
            al::setNerve(this, &NrvNpcStateChaseEnd);
            return;
        }
        if (mTargetFinder->mTarget && mTargetFinder->mIsInChaseRange) {
            al::turnDirectionToTargetDegree(mHostActor, mDirection,
                                            mTargetFinder->getTargetPos(), mChaseParam->mTurnDegrees);
            if (mIsGrounded ? *mIsGrounded : al::isOnGround(mHostActor, 0, 0.0f)) {
                al::addVelocity(mHostActor, *mDirection * mChaseParam->mAcceleration);
            }
            mLostTargetFrames = 0;
        } else if (mLostTargetFrames++ >= 8) {
            al::setNerve(this, &NrvNpcStateChaseEnd);
            return;
        }
    } else {
        if (mTargetFinder->mTarget && mTargetFinder->mIsInChaseRange) {
            al::turnDirectionToTargetDegree(mHostActor, mDirection,
                                            mTargetFinder->getTargetPos(), mChaseParam->mTurnDegrees);
        } else if (mTargetFinder->mTarget || mTargetFinder->mLastTarget) {
            al::turnDirectionToTargetDegree(mHostActor, mDirection,
                                            mTargetFinder->getLastTargetPos(), mChaseParam->mTurnDegrees * 0.66f);
        }
        if (mIsGrounded ? *mIsGrounded : al::isOnGround(mHostActor, 0, 0.0f)) {
            al::addVelocity(mHostActor, *mDirection * mChaseParam->mAcceleration);
        }
    }
    if (mIsGrounded) {
        NpcStateFunction::calcPassiveMovement(mHostActor, mStateParam, *mIsGrounded);
    } else {
        NpcStateFunction::calcPassiveMovement(mHostActor, mStateParam);
    }
    if ((mChaseParam->mAvoidAreas &&
         NpcStateFunction::isNPCAvoidAreaNextMove(mHostActor, 200.0f, 20.0f, 150.0f, false)) ||
        (mChaseParam->mAvoidFalls &&
         NpcStateFunction::isFallNextMove(mHostActor, 150.0f, 150.0f, 110.0f, false))) {
        sead::Vector3f* pVelocity = al::getVelocityPtr(mHostActor);
        al::verticalizeVec(pVelocity, *mDirection, *pVelocity);
    }
    if (mChaseParam->mIsEndless) {
        return;
    }
    if (al::isGreaterStep(this, mChaseParam->mMaxFrames) ||
        (al::isGreaterStep(this, mChaseParam->mMinFrames) && !mTargetFinder->mTarget)) {
        if (mChaseParam->mEndFrames > 0) {
            al::setNerve(this, &NrvNpcStateChaseEnd);
        } else {
            kill();
        }
    }
}

/** @brief Plays the ending action and finishes after the configured delay. */
void NpcStateChase::exeEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(mHostActor, mChaseParam->mEndAction.cstr());
    }
    if (mIsGrounded) {
        NpcStateFunction::calcPassiveMovement(mHostActor, mStateParam, *mIsGrounded);
    } else {
        NpcStateFunction::calcPassiveMovement(mHostActor, mStateParam);
    }
    if (al::isGreaterStep(this, mChaseParam->mEndFrames)) {
        kill();
    }
}
