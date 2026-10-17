#include "Enemy/FlyerStateWander.hpp"
#include "Enemy/FlyerStateWanderParam.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/LiveActor/ActorParamHolder.hpp"
#include "Library/Nerve/NerveSetup.hpp"

namespace {
NERVE_DECL(FlyerStateWander, Wander)
NERVE_DECL(FlyerStateWander, Wait)
NERVES_MAKE_NOSTRUCT(FlyerStateWander, Wander, Wait)
}

/** @brief Constructs a flying enemy's alternating wander and wait state.
 * @param pHost Flying enemy.
 * @param pCenter Reference flight center.
 * @param pTargetFinder Target-search component.
 * @param pParam General flight settings.
 * @param pWanderParam Wandering motion and duration settings.
 */
FlyerStateWander::FlyerStateWander(al::LiveActor* pHost, sead::Vector3f* pCenter,
        TargetFinder* pTargetFinder, const FlyerStateParam* pParam,
        const FlyerStateWanderParam* pWanderParam)
    : al::ActorStateBase("飛行型うろつき状態", pHost), mCenter(pCenter), mParam(pParam),
      mWanderParam(pWanderParam), mTargetFinder(pTargetFinder) {
    initNerve(&NrvFlyerStateWanderWander, 0);
}

/** @brief Starts a new wandering phase. */
void FlyerStateWander::appear() {
    al::ActorStateBase::appear();
    al::setNerve(this, &NrvFlyerStateWanderWander);
}

/** @brief Flies around the starting position for a randomized duration. */
void FlyerStateWander::exeWander() {
    if (al::isFirstStep(this)) {
        mWanderTarget.set(al::getTrans(mHostActor));
        al::startAction(mHostActor, mWanderParam->mAction.cstr());
        mStepDuration = mWanderParam->mStepWander + mWanderParam->mStepRandomRange * al::getRandom(3);
    }
    const al::ActorParamMove* pMove = mWanderParam->mMoveParam;
    al::flyAndTurnToTarget(mHostActor, mWanderTarget, pMove->moveAccel, pMove->gravity,
                          pMove->moveFriction, pMove->turnSpeedDegree);
    if (al::isGreaterEqualStep(this, mStepDuration)) {
        al::setNerve(this, &NrvFlyerStateWanderWait);
    }
}

/** @brief Pauses in place for a randomized duration before wandering again. */
void FlyerStateWander::exeWait() {
    if (al::isFirstStep(this)) {
        mStepDuration = mWanderParam->mStepWait + mWanderParam->mStepRandomRange * al::getRandom(3);
    }
    al::setVelocityZero(mHostActor);
    if (al::isGreaterEqualStep(this, mStepDuration)) {
        al::setNerve(this, &NrvFlyerStateWanderWander);
    }
}
