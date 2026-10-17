#include "Enemy/WalkerStateWander.hpp"
#include "Enemy/TargetFinder.hpp"
#include "Enemy/TerritoryMover.hpp"
#include "Enemy/WalkerStateFunction.hpp"
#include "Enemy/WalkerStateParam.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/Nerve/NerveSetup.hpp"

namespace {
NERVE_DECL(WalkerStateWander, Wait)
NERVE_DECL(WalkerStateWander, Walk)
NERVES_MAKE_NOSTRUCT(WalkerStateWander, Wait, Walk)
const WalkerStateWanderParam cDefaultParam;
}

/** @brief Initializes the default walking and waiting cycle within a five-hundred-unit territory. */
WalkerStateWanderParam::WalkerStateWanderParam()
    : mWaitTime(60), mWalkTime(60), mAccel(0.4f), mTurnRate(6.0f), mRange(20.0f),
      mSearchRange(500.0f), mAvoidLedges(true), mWalkAction("Walk"), mWaitAction("Wait") {}

/**
 * @brief Initializes the wandering movement and animation settings.
 * @param waitTime Waiting duration in frames.
 * @param walkTime Maximum walking duration in frames.
 * @param accel Forward acceleration while facing the destination.
 * @param turnRate Maximum turning angle per frame.
 * @param range Distance considered close enough to the destination.
 * @param searchRange Radius of the wandering territory.
 * @param avoidLedges Whether to stop before crossing a ledge.
 * @param pWalkAction Walking animation name.
 * @param pWaitAction Waiting animation name.
 */
WalkerStateWanderParam::WalkerStateWanderParam(s32 waitTime, s32 walkTime, f32 accel,
        f32 turnRate, f32 range, f32 searchRange, bool avoidLedges,
        const char* pWalkAction, const char* pWaitAction)
    : mWaitTime(waitTime), mWalkTime(walkTime), mAccel(accel), mTurnRate(turnRate),
      mRange(range), mSearchRange(searchRange), mAvoidLedges(avoidLedges),
      mWalkAction(pWalkAction), mWaitAction(pWaitAction) {}

/**
 * @brief Constructs a state that wanders around the enemy's initial position.
 * @param pHost Walking enemy.
 * @param pFrontDir Mutable facing direction.
 * @param pParam Gravity and friction settings.
 * @param pWanderParam Wandering settings, or null for defaults.
 * @param pTargetFinder Optional player target finder.
 */
WalkerStateWander::WalkerStateWander(al::LiveActor* pHost, sead::Vector3f* pFrontDir,
        const WalkerStateParam* pParam, const WalkerStateWanderParam* pWanderParam,
        TargetFinder* pTargetFinder)
    : al::ActorStateBase("クリボー型うろつき状態", pHost), mFrontDir(pFrontDir),
      mParam(pParam), mWanderParam(pWanderParam), mTargetFinder(pTargetFinder) {
    initNerve(&NrvWalkerStateWanderWait, 0);
    if (!mWanderParam) {
        mWanderParam = &cDefaultParam;
    }
    mTerritoryMover = new TerritoryMover(mWanderParam->mSearchRange);
    mTerritoryMover->mCenter.set(al::getTrans(mHostActor));
}

/** @brief Starts waiting and refreshes the optional player target. */
void WalkerStateWander::appear() {
    al::ActorStateBase::appear();
    al::setNerve(this, &NrvWalkerStateWanderWait);
    if (mTargetFinder) {
        mTargetFinder->refindTarget();
    }
}

/**
 * @brief Relocates the center of the wandering territory.
 * @param rCenter New world-space center.
 */
void WalkerStateWander::setWanderCenter(const sead::Vector3f& rCenter) {
    mTerritoryMover->mCenter.set(rCenter);
}

/** @brief Waits before choosing a new destination within the territory. */
void WalkerStateWander::exeWait() {
    if (al::isFirstStep(this)) {
        if (!al::isSklAnimPlaying(mHostActor, mWanderParam->mWaitAction.cstr(), 0)) {
            al::startAction(mHostActor, mWanderParam->mWaitAction.cstr());
        }
    }
    WalkerStateFunction::calcPassiveMovement(mHostActor, mParam);
    if (al::isGreaterStep(this, mWanderParam->mWaitTime)) {
        mTerritoryMover->decideNextTargetPos(mHostActor);
        al::setNerve(this, &NrvWalkerStateWanderWalk);
    }
}

/** @brief Walks toward the destination, stopping for a found player, a ledge, or the end of the walk. */
void WalkerStateWander::exeWalk() {
    if (al::isFirstStep(this)) {
        if (!al::isSklAnimPlaying(mHostActor, mWanderParam->mWalkAction.cstr(), 0)) {
            al::startAction(mHostActor, mWanderParam->mWalkAction.cstr());
        }
    }
    al::turnDirectionToTargetDegree(mHostActor, mFrontDir, mTerritoryMover->mTargetPos,
                                    mWanderParam->mTurnRate);
    if (al::isFaceToTargetDegreeH(mHostActor, mTerritoryMover->mTargetPos, *mFrontDir, 8.0f)) {
        sead::Vector3f* pVelocity = al::getVelocityPtr(mHostActor);
        pVelocity->setScaleAdd(mWanderParam->mAccel, *mFrontDir, *pVelocity);
    }
    WalkerStateFunction::calcPassiveMovement(mHostActor, mParam);
    if (mTargetFinder) {
        mTargetFinder->update();
        if (mTargetFinder->isFoundTarget() &&
                al::isGreaterEqualStep(mHostActor, mWanderParam->mWaitTime) &&
                al::isOnGround(mHostActor, 0, 0.0f)) {
            kill();
            return;
        }
    }
    if (mWanderParam->mAvoidLedges &&
            WalkerStateFunction::isFallNextMove(mHostActor, 150.0f, 150.0f, mParam->mValueC, false)) {
        al::setVelocityZero(mHostActor);
        al::setNerve(this, &NrvWalkerStateWanderWait);
        return;
    }
    if (al::isGreaterStep(this, mWanderParam->mWalkTime) ||
            mTerritoryMover->isReachedTarget(mHostActor, mWanderParam->mRange)) {
        al::setNerve(this, &NrvWalkerStateWanderWait);
    }
}

/** @brief Checks whether the state is waiting between walks. @return Whether the wait nerve is active. */
bool WalkerStateWander::isWait() {
    return al::isNerve(this, &NrvWalkerStateWanderWait);
}
