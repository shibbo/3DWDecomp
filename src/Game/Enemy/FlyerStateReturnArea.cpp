#include "Enemy/FlyerStateReturnArea.hpp"
#include "Enemy/FlyerStateReturnAreaParam.hpp"
#include "Enemy/FlyerStateParam.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"

namespace {
NERVE_DECL(FlyerStateReturnArea, Brake)
NERVE_DECL(FlyerStateReturnArea, ReturnRotate)
NERVE_DECL(FlyerStateReturnArea, Return)
NERVES_MAKE_NOSTRUCT(FlyerStateReturnArea, ReturnRotate, Return)
FlyerStateReturnAreaNrvBrake NrvFlyerStateReturnAreaBrake;
FlyerStateReturnAreaParam cDefaultParam;
}

/** @brief Initializes default braking and return settings. */
FlyerStateReturnAreaParam::FlyerStateReturnAreaParam()
    : _0(0.95f), _4(60), _8(3.0f), _C(0.35f), _10(30.0f), _14(300) {}

/** @brief Initializes custom return settings.
 * @param brakeFriction Velocity multiplier while braking.
 * @param brakeTime Number of braking frames.
 * @param turnDegree Maximum turning angle per frame.
 * @param accel Return acceleration.
 * @param distance Distance considered within the home area.
 * @param returnTime Maximum return duration.
 */
FlyerStateReturnAreaParam::FlyerStateReturnAreaParam(f32 brakeFriction, s32 brakeTime,
        f32 turnDegree, f32 accel, f32 distance, s32 returnTime)
    : _0(brakeFriction), _4(brakeTime), _8(turnDegree), _C(accel), _10(distance), _14(returnTime) {}

/** @brief Initializes the return state and destination.
 * @param pHost Flying enemy.
 * @param pFront Direction used for steering.
 * @param rReturnPos Home position.
 * @param pParam Flight friction settings.
 * @param pReturnParam Return settings, or nullptr for defaults.
 */
FlyerStateReturnArea::FlyerStateReturnArea(al::LiveActor* pHost, sead::Vector3f* pFront,
        const sead::Vector3f& rReturnPos, const FlyerStateParam* pParam,
        const FlyerStateReturnAreaParam* pReturnParam)
    : al::ActorStateBase("飛行敵エリアに戻る", pHost), mFront(pFront), mReturnPos(rReturnPos),
      mParam(pParam), mReturnParam(pReturnParam ? pReturnParam : &cDefaultParam) {
    initNerve(&NrvFlyerStateReturnAreaBrake, 0);
}

/** @brief Restarts braking before the return flight. */
void FlyerStateReturnArea::appear() {
    al::ActorStateBase::appear();
    al::setNerve(this, &NrvFlyerStateReturnAreaBrake);
}

/** @brief Reduces velocity for the configured braking duration. */
void FlyerStateReturnArea::exeBrake() {
    al::scaleVelocity(mHostActor, mReturnParam->_0);
    if (al::isGreaterEqualStep(this, mReturnParam->_4)) {
        al::setNerve(this, &NrvFlyerStateReturnAreaReturnRotate);
    }
}

/** @brief Turns toward the home position before moving. */
void FlyerStateReturnArea::exeReturnRotate() {
    if (al::turnDirectionToTargetDegree(mHostActor, mFront, mReturnPos, mReturnParam->_8)) {
        al::setNerve(this, &NrvFlyerStateReturnAreaReturn);
    }
}

/** @brief Flies home until near the destination or the time limit expires. */
void FlyerStateReturnArea::exeReturn() {
    al::addVelocityToDirection(mHostActor, *mFront, mReturnParam->_C);
    al::scaleVelocity(mHostActor, mParam->mFriction);
    if (al::isGreaterEqualStep(this, mReturnParam->_14) || al::isNear(mHostActor, mReturnPos, mReturnParam->_10)) {
        kill();
    }
}
