#include "Enemy/FlyerStateChase.hpp"
#include "Enemy/FlyerStateChaseParam.hpp"
#include "Enemy/FlyerStateParam.hpp"
#include "Enemy/TargetFinder.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"

namespace {
NERVE_DECL(FlyerStateChase, Chase)
NERVE_DECL(FlyerStateChase, ChaseEnd)
NERVES_MAKE_NOSTRUCT(FlyerStateChase, Chase, ChaseEnd)
const FlyerStateChaseParam cDefaultParam;
}

/** @brief Initializes default flying pursuit settings. */
FlyerStateChaseParam::FlyerStateChaseParam()
    : mAccel(0.45f), mChaseTime(600), mTurnDegree(4.0f), mEndFriction(0.98f),
      mEndTime(70), mAccelTime(100), mChaseAction("Run"), mWaitAction("Wait") {}

/** @brief Initializes custom flying pursuit settings.
 * @param accel Acceleration toward the target.
 * @param chaseTime Minimum duration before testing whether the target is outside the search cone.
 * @param turnDegree Maximum turning angle per frame.
 * @param endFriction Velocity multiplier while stopping.
 * @param endTime Duration of the stopping phase.
 * @param accelTime Duration of the optional acceleration ramp.
 * @param pChaseAction Pursuit animation.
 * @param pWaitAction Stopping animation.
 */
FlyerStateChaseParam::FlyerStateChaseParam(f32 accel, s32 chaseTime, f32 turnDegree,
        f32 endFriction, s32 endTime, s32 accelTime, const char* pChaseAction, const char* pWaitAction)
    : mAccel(accel), mChaseTime(chaseTime), mTurnDegree(turnDegree), mEndFriction(endFriction),
      mEndTime(endTime), mAccelTime(accelTime), mChaseAction(pChaseAction), mWaitAction(pWaitAction) {}

/** @brief Initializes the flying chase state.
 * @param pHost Flying enemy.
 * @param pFront Direction used for turning and acceleration.
 * @param pTargetFinder Player search controller.
 * @param pParam Flight friction settings.
 * @param pChaseParam Chase settings, or nullptr for defaults.
 */
FlyerStateChase::FlyerStateChase(al::LiveActor* pHost, sead::Vector3f* pFront,
        TargetFinder* pTargetFinder, const FlyerStateParam* pParam, const FlyerStateChaseParam* pChaseParam)
    : al::ActorStateBase("飛行敵追いかけ状態", pHost), mFront(pFront), mParam(pParam),
      mChaseParam(pChaseParam), mTargetFinder(pTargetFinder) {
    if (!mChaseParam) { mChaseParam = &cDefaultParam; }
    initNerve(&NrvFlyerStateChaseChase, 0);
}

/** @brief Restarts pursuit. */
void FlyerStateChase::appear() {
    al::ActorStateBase::appear();
    al::setNerve(this, &NrvFlyerStateChaseChase);
}

/** @brief Turns and accelerates toward the current target. */
void FlyerStateChase::exeChase() {
    if (al::isFirstStep(this)) {
        al::startAction(mHostActor, mChaseParam->mChaseAction.cstr());
        if (_21) { al::startAction(al::getSubActor(mHostActor, 0), mChaseParam->mChaseAction.cstr()); }
    }
    mTargetFinder->update();
    if (!mTargetFinder->isFoundTargetNow()) {
        al::setNerve(this, &NrvFlyerStateChaseChaseEnd);
        return;
    }
    al::turnDirectionToTargetDegree(mHostActor, mFront, mTargetFinder->getTargetPos(), mChaseParam->mTurnDegree);
    if (_20) {
        const s32 step = al::getNerveStep(this);
        const s32 accelTime = mChaseParam->mAccelTime;
        f32 accel = mChaseParam->mAccel;
        if (step < accelTime) {
            accel *= al::easeInOut(static_cast<f32>(step) / static_cast<f32>(accelTime));
        }
        al::addVelocityToDirection(mHostActor, *mFront, accel);
    } else {
        al::addVelocityToDirection(mHostActor, *mFront, mChaseParam->mAccel);
    }
    al::scaleVelocity(mHostActor, mParam->mFriction);
    if (al::isGreaterStep(this, mChaseParam->mChaseTime) && !mTargetFinder->isFoundTarget()) {
        al::setNerve(this, &NrvFlyerStateChaseChaseEnd);
    }
}

/** @brief Slows the actor and ends pursuit after the configured delay. */
void FlyerStateChase::exeChaseEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(mHostActor, mChaseParam->mWaitAction.cstr());
        if (_21) { al::startAction(al::getSubActor(mHostActor, 0), mChaseParam->mWaitAction.cstr()); }
    }
    al::scaleVelocity(mHostActor, mChaseParam->mEndFriction);
    if (al::isGreaterStep(this, mChaseParam->mEndTime)) { kill(); }
}
