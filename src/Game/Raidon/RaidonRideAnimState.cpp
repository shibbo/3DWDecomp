#include "Raidon/RaidonRideAnimState.hpp"

#include <hostio/seadHostIOCurve.h>

#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Raidon/RaidonBase.hpp"
#include "Util/AreaObjUtil.hpp"

namespace {
/// A swim/run nerve that also stops the looping swim sound and effects when it is left.
#define RAIDON_RIDE_MOVE_NERVE_DECL(Action)                                                        \
    class RaidonRideAnimStateNrv##Action : public al::Nerve {                                      \
    public:                                                                                        \
        void execute(al::NerveKeeper* pKeeper) const override {                                    \
            pKeeper->getParent<RaidonRideAnimState>()->exe##Action();                              \
        }                                                                                          \
                                                                                                   \
        void executeOnEnd(al::NerveKeeper* pKeeper) const override {                               \
            pKeeper->getParent<RaidonRideAnimState>()->endMove();                                  \
        }                                                                                          \
    };

NERVE_DECL(RaidonRideAnimState, Land)
NERVE_DECL(RaidonRideAnimState, Fall)
RAIDON_RIDE_MOVE_NERVE_DECL(Run)
RAIDON_RIDE_MOVE_NERVE_DECL(Swim)
NERVE_DECL(RaidonRideAnimState, JumpLoop)
NERVE_DECL(RaidonRideAnimState, Dash)
NERVE_DECL(RaidonRideAnimState, JumpStart)
NERVE_DECL(RaidonRideAnimState, Bound)
NERVE_DECL(RaidonRideAnimState, Hit)
NERVES_MAKE_NOSTRUCT(RaidonRideAnimState, Land, Fall, Run, Swim, JumpLoop, Dash, JumpStart, Bound,
                     Hit)

typedef sead::hostio::Curve<f32> CurveF32;

/** @brief Builds a linear curve over constant control points.
 * @param pFloats Control points of the curve.
 * @param num Number of control points.
 * @return The curve.
 */
CurveF32 createLinearCurve(f32* pFloats, u8 num) {
    CurveF32 curve;
    curve.mInfo.curveType = u8(sead::hostio::CurveType::Linear);
    curve.mInfo.numFloats = num;
    curve.mInfo.numUse = num;
    curve.mFloats = pFloats;
    return curve;
}

/** @brief Evaluates a curve.
 * @param rCurve Curve to evaluate.
 * @param t Curve input.
 * @return Interpolated value.
 */
f32 interpolateCurve(const CurveF32& rCurve, f32 t) {
    return sead::hostio::sCurveFunctionTbl_f32[rCurve.mInfo.curveType](t, &rCurve.mInfo,
                                                                        rCurve.mFloats);
}

f32 sStrokeCurveData[] = {0.0f, 0.0f, 0.0f, 0.2f, 1.0f};
f32 sSwimHighCurveData[] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.95f, 1.0f};
f32 sSwimMidCurveData[] = {0.0f, 0.0f, 0.0f, 0.3f, 0.9f, 1.0f, 0.75f, 0.0f};
f32 sSwimLowCurveData[] = {0.0f, 0.0f, 1.0f, 1.0f, 0.7f, 0.0f, 0.0f, 0.0f};
f32 sSwimDecelCurveData[] = {1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
f32 sSwimTurnCurveData[] = {0.0f, 0.0f, 0.0f, 0.05f, 0.25f, 0.5f, 0.7f, 1.0f};

/// Swim sound volume curves over the speed rate: high, mid, low and decelerating swimming.
const CurveF32 sSwimHighCurve = createLinearCurve(sSwimHighCurveData, 8);
const CurveF32 sSwimMidCurve = createLinearCurve(sSwimMidCurveData, 8);
const CurveF32 sSwimLowCurve = createLinearCurve(sSwimLowCurveData, 8);
const CurveF32 sSwimDecelCurve = createLinearCurve(sSwimDecelCurveData, 8);
/// Swim turn sound volume over the handle strength.
const CurveF32 sSwimTurnCurve = createLinearCurve(sSwimTurnCurveData, 8);
/// Stroke sound volume over the speed rate.
const CurveF32 sStrokeCurve = createLinearCurve(sStrokeCurveData, 5);
}  // namespace

/** @brief Constructs the animation state and starts it in the landing nerve.
 * @param pName State name.
 * @param pHost Plessie actor whose animations are driven.
 */
RaidonRideAnimState::RaidonRideAnimState(const char* pName, RaidonBase* pHost)
    : al::HostStateBase<RaidonBase>(pName, pHost) {
    initNerve(&NrvRaidonRideAnimStateLand, 0);
}

/** @brief Activates the state and starts with a landing. */
void RaidonRideAnimState::appear() {
    al::NerveStateBase::appear();
    getHost()->startPuppetActionAll("RaidonMove");
    al::setNerve(this, &NrvRaidonRideAnimStateLand);
}

/** @brief Deactivates the state and stops the movement sound and effects. */
void RaidonRideAnimState::kill() {
    al::NerveStateBase::kill();
    endMove();
}

/** @brief Stops the swim turn sound and every looping swim effect. */
void RaidonRideAnimState::endMove() {
    al::stopSeByName(getHost(), "PgSwimTurn");
    al::tryDeleteEffect(getHost(), "SwimFront");
    al::tryDeleteEffect(getHost(), "SwimBack");
    al::tryDeleteEffect(getHost(), "SwimLeft");
    al::tryDeleteEffect(getHost(), "SwimRight");
    al::tryDeleteEffect(getHost(), "SwimNeutral");
    mMoveEffectFlags = 0;
}

/** @brief Ends the ride animation once Plessie stands in the goal area. */
void RaidonRideAnimState::control() {
    if (rc::isInAreaObj(getHost(), rc::AreaObjType::BobsledGoalArea) &&
        getHost()->isOnGroundRaidon()) {
        mIsSwim = al::isNerve(this, &NrvRaidonRideAnimStateLand);
        kill();
    }
}

/** @brief Swims, switching to falling or running when Plessie leaves the water. */
void RaidonRideAnimState::exeSwim() {
    if (al::isFirstStep(this)) {
        getHost()->startPuppetActionAll("RaidonMove");
        al::startAction(getHost(), "SwimMove");
    }

    getHost()->setPuppetInputBlendAnimWeight();
    getHost()->setInputBlendAnimWeight();
    updateSwimSound();
    updateMoveEffect();

    if (getHost()->isOnGroundRaidon()) {
        if (mAirCount >= 10) {
            if (getHost()->isInWater()) {
                al::startSe(getHost(), "AirEnd");
            } else {
                al::startSe(getHost(), "AirEndGround");
            }
        }

        mAirCount = 0;
    } else {
        mAirCount++;
        if (mAirCount >= 30) {
            al::setNerve(this, &NrvRaidonRideAnimStateFall);
            return;
        }
    }

    if (!getHost()->isInWater() && al::isGreaterEqualStep(this, 10)) {
        al::setNerve(this, &NrvRaidonRideAnimStateRun);
    }
}

/** @brief Updates the swim, stroke and turn sounds from Plessie's acceleration and handle. */
void RaidonRideAnimState::updateSwimSound() {
    RaidonBase* pHost = getHost();
    f32 accelRate = (pHost->getAccel() + 1.0f) * 0.5f;
    f32 decelVolume = interpolateCurve(sSwimDecelCurve, accelRate);
    f32 highVolume = interpolateCurve(sSwimHighCurve, accelRate);
    f32 midVolume = interpolateCurve(sSwimMidCurve, accelRate);
    f32 lowVolume = interpolateCurve(sSwimLowCurve, accelRate);

    al::holdSeWithParam(pHost, "PgSwimHigh", highVolume);
    al::holdSeWithParam(pHost, "PgSwimMid", midVolume);
    al::holdSeWithParam(pHost, "PgSwimLow", lowVolume);
    al::holdSeWithParam(pHost, "PgSwimDecel", decelVolume);
    al::holdSeWithParam(pHost, "PgRunOnEachMaterial", accelRate);

    f32 frame = al::getActionFrame(pHost);
    if (frame == 30.0f || frame == 90.0f) {
        if (decelVolume < 0.8) {
            al::startSe(pHost, "PgStroke");
        }
    } else if (frame == 60.0f || frame == 119.0f) {
        if (decelVolume < 0.8) {
            al::startSeWithParam(pHost, "PgStroke", interpolateCurve(sStrokeCurve, accelRate));
        }
    }

    f32 handle = pHost->getHandle();
    f32 handleDiff = handle >= mHandle ? handle - mHandle : mHandle - handle;
    if (handleDiff > 0.1f) {
        f32 turnVolume = interpolateCurve(sSwimTurnCurve, handle > 0.0f ? handle : -handle);
        al::stopSeByName(pHost, "PgSwimTurn");
        al::startSeWithParam(pHost, "PgSwimTurn", turnVolume);
    }

    mHandle = handle;
}

/** @brief Emits or stops the looping swim effects from Plessie's acceleration and handle. */
void RaidonRideAnimState::updateMoveEffect() {
    tryEmitMoveEffect(getHost()->getAccel() > 0.6f, "SwimFront", MoveEffect_Front);
    tryEmitMoveEffect(getHost()->getAccel() < -0.6f, "SwimBack", MoveEffect_Back);
    tryEmitMoveEffect(getHost()->getHandle() > 0.6f, "SwimRight", MoveEffect_Right);
    tryEmitMoveEffect(getHost()->getHandle() < -0.6f, "SwimLeft", MoveEffect_Left);

    f32 accel = getHost()->getAccel();
    bool isNeutral = false;
    if ((accel > 0.0f ? accel : -accel) < 0.5f) {
        f32 handle = getHost()->getHandle();
        isNeutral = (handle > 0.0f ? handle : -handle) < 0.5f;
    }

    tryEmitMoveEffect(isNeutral, "SwimNeutral", MoveEffect_Neutral);
}

/** @brief Runs on the ground, switching to falling or swimming when needed. */
void RaidonRideAnimState::exeRun() {
    if (al::isFirstStep(this)) {
        getHost()->startPuppetActionAll("RaidonMove");
        al::startAction(getHost(), "RunMove");
    }

    getHost()->setPuppetInputBlendAnimWeight();
    getHost()->setInputBlendAnimWeight();
    updateSwimSound();

    if (getHost()->isOnGroundRaidon()) {
        if (mAirCount >= 10) {
            if (getHost()->isInWater()) {
                al::startSe(getHost(), "AirEnd");
            } else {
                al::startSe(getHost(), "AirEndGround");
            }
        }

        mAirCount = 0;
    } else {
        mAirCount++;
        if (mAirCount == 1) {
            al::startSe(getHost(), "PgJumpShort");
        }
    }

    if (mAirCount >= 30) {
        al::setNerve(this, &NrvRaidonRideAnimStateFall);
    } else if (getHost()->isInWater() && al::isGreaterEqualStep(this, 10)) {
        al::setNerve(this, &NrvRaidonRideAnimStateSwim);
    }
}

/** @brief Starts a jump and lands or keeps flying once it is over. */
void RaidonRideAnimState::exeJumpStart() {
    if (al::isFirstStep(this)) {
        al::startAction(getHost(), getHost()->isInWater() ? "AirStart" : "AirStartGround");
    }

    getHost()->setPuppetInputBlendAnimWeight();

    if (getHost()->isOnGroundRaidon()) {
        al::setNerve(this, &NrvRaidonRideAnimStateLand);
    } else if (al::isActionEnd(getHost())) {
        al::setNerve(this, &NrvRaidonRideAnimStateJumpLoop);
    }
}

/** @brief Stays airborne after a jump until Plessie lands or starts falling. */
void RaidonRideAnimState::exeJumpLoop() {
    if (al::isFirstStep(this)) {
        al::startAction(getHost(), "AirLoop");
    }

    getHost()->setPuppetInputBlendAnimWeight();

    if (getHost()->isOnGroundRaidon()) {
        al::setNerve(this, &NrvRaidonRideAnimStateLand);
    } else if (al::isGreaterEqualStep(this, 45)) {
        al::setNerve(this, &NrvRaidonRideAnimStateFall);
    }
}

/** @brief Falls until Plessie touches the ground. */
void RaidonRideAnimState::exeFall() {
    if (al::isFirstStep(this)) {
        al::startAction(getHost(), "Fall");
    }

    getHost()->setPuppetInputBlendAnimWeight();

    if (getHost()->isOnGroundRaidon()) {
        if (getHost()->isInWater() && al::isGreaterEqualStep(this, 55)) {
            al::startSe(getHost(), "PgFallEnd");
        }

        al::setNerve(this, &NrvRaidonRideAnimStateLand);
    }
}

/** @brief Lands, then continues swimming or running. */
void RaidonRideAnimState::exeLand() {
    if (al::isFirstStep(this)) {
        mAirCount = 0;
        al::startAction(getHost(), getHost()->isInWater() ? "AirEnd" : "AirEndGround");
    }

    getHost()->setPuppetInputBlendAnimWeight();

    if (al::isActionEnd(getHost())) {
        if (getHost()->isInWater()) {
            al::setNerve(this, &NrvRaidonRideAnimStateSwim);
        } else {
            al::setNerve(this, &NrvRaidonRideAnimStateRun);
        }
    }
}

/** @brief Bounces off something until Plessie lands or the bounce animation ends. */
void RaidonRideAnimState::exeBound() {
    if (al::isFirstStep(this)) {
        al::startAction(getHost(), "AirBounce");
    }

    getHost()->setPuppetInputBlendAnimWeight();

    if (getHost()->isOnGroundRaidon()) {
        al::setNerve(this, &NrvRaidonRideAnimStateLand);
    } else if (al::isActionEnd(getHost())) {
        al::setNerve(this, &NrvRaidonRideAnimStateJumpLoop);
    }
}

/** @brief Plays the hit reaction for the current surroundings, then resumes moving. */
void RaidonRideAnimState::exeHit() {
    if (al::isFirstStep(this)) {
        if (getHost()->isOnGroundRaidon()) {
            if (getHost()->isInWater()) {
                al::startAction(getHost(), "HitWater");
            } else {
                al::startAction(getHost(), "HitAir");
            }
        } else {
            al::startAction(getHost(), "HitGround");
        }
    }

    getHost()->setPuppetInputBlendAnimWeight();

    if (al::isActionEnd(getHost())) {
        if (getHost()->isOnGroundRaidon()) {
            mAirCount = 0;
            if (getHost()->isInWater()) {
                al::setNerve(this, &NrvRaidonRideAnimStateSwim);
            } else {
                al::setNerve(this, &NrvRaidonRideAnimStateRun);
            }
        } else {
            al::setNerve(this, &NrvRaidonRideAnimStateFall);
        }
    }
}

/** @brief Dashes forward, then returns to swimming, running or falling. */
void RaidonRideAnimState::exeDash() {
    if (al::isFirstStep(this)) {
        al::startAction(getHost(), getHost()->isInWater() ? "SwimDash" : "RunDash");
    }

    getHost()->setPuppetInputBlendAnimWeight();

    if (al::isActionEnd(getHost())) {
        if (getHost()->isOnGroundRaidon()) {
            mAirCount = 0;
            if (getHost()->isInWater()) {
                al::setNerve(this, &NrvRaidonRideAnimStateSwim);
            } else {
                al::setNerve(this, &NrvRaidonRideAnimStateRun);
            }
        } else {
            al::setNerve(this, &NrvRaidonRideAnimStateFall);
        }
    }
}

/** @brief Starts a jump unless Plessie has only just landed or started a dash.
 * @return Whether the jump was started.
 */
bool RaidonRideAnimState::requestJump() {
    if ((al::isNerve(this, &NrvRaidonRideAnimStateLand) ||
         al::isNerve(this, &NrvRaidonRideAnimStateDash)) &&
        al::isLessEqualStep(this, 1)) {
        return false;
    }

    bool isInWater = getHost()->isInWater();
    bool isDash = al::isNerve(this, &NrvRaidonRideAnimStateDash);
    if (isInWater) {
        if (isDash) {
            al::startSe(getHost(), "PgAirStartHigh");
        } else {
            al::startSe(getHost(), "PgAirStart");
        }
    } else if (isDash) {
        al::startSe(getHost(), "JumpGroundHigh");
    } else {
        al::startSe(getHost(), "PgJumpGround");
    }

    al::setNerve(this, &NrvRaidonRideAnimStateJumpStart);
    return true;
}

/** @brief Makes Plessie bounce. */
void RaidonRideAnimState::requestBound() {
    al::setNerve(this, &NrvRaidonRideAnimStateBound);
}

/** @brief Plays the hit reaction. */
void RaidonRideAnimState::requestHit() {
    al::setNerve(this, &NrvRaidonRideAnimStateHit);
}

/** @brief Starts a dash. */
void RaidonRideAnimState::requestDash() {
    al::setNerve(this, &NrvRaidonRideAnimStateDash);
}

/** @brief Emits a looping swim effect when it should play, or deletes it when it should stop.
 * @param isEmit Whether the effect should currently be playing.
 * @param pName Effect name.
 * @param flag Bit in the move effect flags that tracks this effect.
 */
void RaidonRideAnimState::tryEmitMoveEffect(bool isEmit, const char* pName, s32 flag) {
    if (isEmit) {
        if ((mMoveEffectFlags & flag) == 0) {
            mMoveEffectFlags |= flag;
            al::emitEffect(getHost(), pName, nullptr);
        }
    } else if ((mMoveEffectFlags & flag) != 0) {
        mMoveEffectFlags &= ~flag;
        al::deleteEffect(getHost(), pName);
    }
}

/** @brief Destroys the animation state. */
RaidonRideAnimState::~RaidonRideAnimState() = default;
