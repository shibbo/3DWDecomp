#include "NPC/NpcStateRunAwayParam.hpp"
#include "NPC/NpcStateRunAway.hpp"
#include "NPC/NpcStateFunction.hpp"
#include "NPC/NpcTargetFinder.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorMovementUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"

#include <math/seadQuat.h>

namespace {
NpcStateRunAwayParam sDefaultParam;
NERVE_DECL(NpcStateRunAway, Start)
NERVE_DECL(NpcStateRunAway, End)
NERVES_MAKE_NOSTRUCT(NpcStateRunAway, Start, End)


}

/** @brief Initializes the default run-away acceleration, timing, and actions. */
NpcStateRunAwayParam::NpcStateRunAwayParam()
    : mAcceleration(2.0f), mFastFrames(130), mParam8(1.0f), mMaxFrames(300),
      mTurnDegrees(4.0f), mEndFrames(30), mIsEndless(false), mAvoidFalls(true),
      mAvoidAreas(true), mRunAction("Run"), mEndAction("Wait"), mParam90(-1.0f),
      mRandomTurnInterval(-1) {}

/**
 * @brief Stores the settings for an NPC running away from its target.
 * @param acceleration Acceleration along the facing direction.
 * @param fastFrames Duration of the initial acceleration boost.
 * @param param8 Additional movement parameter.
 * @param maxFrames Maximum run-away duration.
 * @param turnDegrees Turn speed away from the target.
 * @param randomTurnDegrees Range of the random turn offset.
 * @param endFrames Duration of the ending state.
 * @param isEndless Whether to ignore the duration limit.
 * @param avoidFalls Whether to avoid ledges.
 * @param avoidAreas Whether to avoid NPC avoidance areas.
 * @param pRunAction Action played while running away.
 * @param pEndAction Action played while ending the state.
 * @param param90 Additional run-away parameter.
 * @param randomTurnInterval Frames between random turn changes.
 */
NpcStateRunAwayParam::NpcStateRunAwayParam(float acceleration, int fastFrames, float param8,
                                         float maxFrames, float turnDegrees, float randomTurnDegrees,
                                         float endFrames, bool isEndless, bool avoidFalls,
                                         bool avoidAreas, const char* pRunAction,
                                         const char* pEndAction, float param90, int randomTurnInterval)
    : mAcceleration(acceleration), mFastFrames(fastFrames), mParam8(param8),
      mMaxFrames(static_cast<int>(maxFrames)), mTurnDegrees(turnDegrees),
      mEndFrames(static_cast<int>(endFrames)), mIsEndless(isEndless), mAvoidFalls(avoidFalls),
      mAvoidAreas(avoidAreas), mRunAction(pRunAction), mEndAction(pEndAction),
      mParam90(param90), mRandomTurnDegrees(randomTurnDegrees), mRandomTurnInterval(randomTurnInterval) {}

/**
 * @brief Creates a run-away state using the supplied target finder and movement settings.
 * @param pActor Actor controlled by the state.
 * @param pDirection Mutable facing direction.
 * @param pTargetFinder Finder supplying target positions and visibility.
 * @param pStateParam Passive movement settings.
 * @param pRunAwayParam Run-away settings, or nullptr for defaults.
 * @param requireVisibleTarget Whether losing visibility ends the run-away.
 * @param pIsGrounded Optional externally supplied ground-contact flag.
 */
NpcStateRunAway::NpcStateRunAway(al::LiveActor* pActor, sead::Vector3f* pDirection,
                            NpcTargetFinder* pTargetFinder, const NpcStateParam* pStateParam,
                            const NpcStateRunAwayParam* pRunAwayParam, bool requireVisibleTarget,
                            bool* pIsGrounded)
    : al::ActorStateBase("NpcStateRunAway", pActor), mDirection(pDirection),
      mTargetPosition(sead::Vector3f::zero), mStateParam(pStateParam), mRunAwayParam(pRunAwayParam),
      mRequireVisibleTarget(requireVisibleTarget), mIsGrounded(pIsGrounded),
      mLostTargetFrames(0), mTargetFinder(pTargetFinder), mTurnOffset(0.0f),
      mAvoidFrames(0), mAvoidPosition(sead::Vector3f::zero), mIsAvoiding(false) {
    if (!mRunAwayParam) {
        mRunAwayParam = &sDefaultParam;
    }
    initNerve(&NrvNpcStateRunAwayStart, 0);
}

/** @brief Releases the run-away state's nerve executor. */
NpcStateRunAway::~NpcStateRunAway() = default;

/** @brief Restarts the run-away and clears the lost-target counter. */
void NpcStateRunAway::appear() {
    mLostTargetFrames = 0;
    mIsDead = false;
    al::setNerve(this, &NrvNpcStateRunAwayStart);
    mIsAvoiding = false;
}

/** @brief Chases the current target while applying ground and avoidance constraints. */
void NpcStateRunAway::exeStart() {
    if (al::isFirstStep(this)) {
        al::startAction(mHostActor, mRunAwayParam->mRunAction.cstr());
        mTurnOffset = al::getRandom(-mRunAwayParam->mRandomTurnDegrees, mRunAwayParam->mRandomTurnDegrees);
        mAvoidFrames = 0;
    } else if (al::isIntervalStep(this, mRunAwayParam->mRandomTurnInterval, 0)) {
        mTurnOffset = al::getRandom(-mRunAwayParam->mRandomTurnDegrees, mRunAwayParam->mRandomTurnDegrees);
    }
    mTargetFinder->update();
    mIsAvoiding = false;
    const float accelerationScale = al::isLessStep(this, mRunAwayParam->mFastFrames) ? 2.0f : 1.0f;
    if (mRequireVisibleTarget) {
        if (mTargetFinder->mHasTargetChanged) {
            al::setNerve(this, &NrvNpcStateRunAwayEnd);
            return;
        }
        if (mTargetFinder->mTarget && mTargetFinder->mIsInChaseRange) {
            updateDirectionPtr();
            if (mIsGrounded ? *mIsGrounded : al::isOnGround(mHostActor, 0, 0.0f)) {
                al::addVelocity(mHostActor, (*mDirection * mRunAwayParam->mAcceleration) * accelerationScale);
            }
            mLostTargetFrames = 0;
        } else if (mLostTargetFrames++ >= 8) {
            al::setNerve(this, &NrvNpcStateRunAwayEnd);
            return;
        }
    } else {
        if (mTargetFinder->mTarget && mTargetFinder->mIsInChaseRange) {
            updateDirectionPtr();
        } else if (mTargetFinder->mTarget || mTargetFinder->mLastTarget) {
            mTargetPosition = mTargetFinder->getLastTargetPos();
            al::turnDirectionFromTargetDegree(mHostActor, mDirection, mTargetPosition,
                                              mRunAwayParam->mTurnDegrees * 0.66f);
        }
        if (mIsGrounded ? *mIsGrounded : al::isOnGround(mHostActor, 0, 0.0f)) {
            al::addVelocity(mHostActor, (*mDirection * mRunAwayParam->mAcceleration) * accelerationScale);
        }
    }
    if (mIsGrounded) {
        NpcStateFunction::calcPassiveMovement(mHostActor, mStateParam, *mIsGrounded);
    } else {
        NpcStateFunction::calcPassiveMovement(mHostActor, mStateParam);
    }
    if ((mRunAwayParam->mAvoidAreas &&
         NpcStateFunction::isNPCAvoidAreaNextMove(mHostActor, 200.0f, 20.0f, 150.0f, false)) ||
        (mRunAwayParam->mAvoidFalls &&
         (NpcStateFunction::isFallNextMove(mHostActor, 200.0f, 150.0f, 70.0f, false) ||
          NpcStateFunction::isFallNextMove(mHostActor, 100.0f, 150.0f, 70.0f, false)))) {
        if (mAvoidFrames < sead::Mathi::max(0, mRunAwayParam->mMaxFrames - 10) &&
            (mTargetFinder->mTarget || mTargetFinder->mLastTarget)) {
            mAvoidPosition = al::getTrans(mHostActor) + *mDirection * 25.0f;
            mAvoidFrames = mRunAwayParam->mMaxFrames;
        }
        al::setVelocityToGravity(mHostActor, 0.1f);
        mTargetPosition = al::getTrans(mHostActor) - *mDirection * 100.0f;
        mIsAvoiding = true;
    }
    if (!mRunAwayParam->mIsEndless && al::isGreaterStep(this, mRunAwayParam->mMaxFrames)) {
        if (mRunAwayParam->mEndFrames > 0) {
            al::setNerve(this, &NrvNpcStateRunAwayEnd);
        } else {
            kill();
        }
    }
}

/** @brief Steers away from a target or obstacle, applying the random turn offset. */
void NpcStateRunAway::updateDirectionPtr() {
    if (mAvoidFrames < sead::Mathi::max(0, mRunAwayParam->mMaxFrames - 10) &&
        al::isCollidedWall(mHostActor)) {
        mAvoidPosition.set(al::getCollidedWallPos(mHostActor));
        mAvoidFrames = mRunAwayParam->mMaxFrames;
        mIsAvoiding = true;
    }
    const int avoidFrames = mAvoidFrames;
    const sead::Vector3f position(al::getTrans(mHostActor));
    sead::Vector3f direction;
    if (avoidFrames > 0) {
        const sead::Vector3f target(mAvoidPosition);
        sead::Quatf rotation;
        rotation.setAxisAngle(sead::Vector3f(0.0f, 1.0f, 0.0f), mTurnOffset);
        direction.setRotated(rotation, target - position);
        mIsAvoiding = true;
    } else {
        const sead::Vector3f target(mTargetFinder->getTargetPos());
        sead::Quatf rotation;
        rotation.setAxisAngle(sead::Vector3f(0.0f, 1.0f, 0.0f), mTurnOffset);
        direction.setRotated(rotation, target - position);
        mIsAvoiding = false;
    }
    mTargetPosition = position + direction;
    al::turnDirectionFromTargetDegree(mHostActor, mDirection, mTargetPosition, mRunAwayParam->mTurnDegrees);
    --mAvoidFrames;
}

/** @brief Plays the ending action and finishes after the configured delay. */
void NpcStateRunAway::exeEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(mHostActor, mRunAwayParam->mEndAction.cstr());
    }
    if (mIsGrounded) {
        NpcStateFunction::calcPassiveMovement(mHostActor, mStateParam, *mIsGrounded);
    } else {
        NpcStateFunction::calcPassiveMovement(mHostActor, mStateParam);
    }
    if (al::isGreaterStep(this, mRunAwayParam->mEndFrames)) {
        kill();
    }
}
