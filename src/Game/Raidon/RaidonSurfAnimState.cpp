#include "Raidon/RaidonSurfAnimState.hpp"

#include <hostio/seadHostIOCurve.h>
#include <nerd/nerdMath.h>

#include "Library/Controller/PadRumbleFunction.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorAnimUtil.hpp"
#include "Library/LiveActor/Util/ActorMovementUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Project/OceanWave/OceanWaveUserInfo.hpp"
#include "Raidon/RaidonSurf.hpp"

namespace {
/// A swim/run nerve that also stops the looping swim sound and effects when it is left.
#define RAIDON_SURF_MOVE_NERVE_DECL(Action)                                                        \
    class RaidonSurfAnimStateNrv##Action : public al::Nerve {                                      \
    public:                                                                                        \
        void execute(al::NerveKeeper* pKeeper) const override {                                    \
            pKeeper->getParent<RaidonSurfAnimState>()->exe##Action();                              \
        }                                                                                          \
                                                                                                   \
        void executeOnEnd(al::NerveKeeper* pKeeper) const override {                               \
            pKeeper->getParent<RaidonSurfAnimState>()->endMove();                                  \
        }                                                                                          \
    };

NERVE_DECL(RaidonSurfAnimState, Land)
RAIDON_SURF_MOVE_NERVE_DECL(Swim)
RAIDON_SURF_MOVE_NERVE_DECL(Run)
NERVE_DECL(RaidonSurfAnimState, Fall)
NERVE_DECL(RaidonSurfAnimState, DiveStart)
NERVE_DECL(RaidonSurfAnimState, DiveLoop)
RAIDON_SURF_MOVE_NERVE_DECL(Slide)
NERVE_DECL(RaidonSurfAnimState, JumpStart)
NERVE_DECL(RaidonSurfAnimState, JumpLoop)
NERVE_DECL(RaidonSurfAnimState, SurfaceEnd)
NERVE_DECL(RaidonSurfAnimState, SurfaceStart)
NERVE_DECL(RaidonSurfAnimState, SurfaceLoop)
NERVE_DECL(RaidonSurfAnimState, DiveJump)
NERVE_DECL(RaidonSurfAnimState, Dash)
NERVE_DECL(RaidonSurfAnimState, Bound)
NERVE_DECL(RaidonSurfAnimState, Hit)
NERVE_DECL(RaidonSurfAnimState, Damage)
NERVE_DECL(RaidonSurfAnimState, PlessieChaseBellHit)
NERVES_MAKE_NOSTRUCT(RaidonSurfAnimState, Land, Swim, Run, Fall, DiveStart, DiveLoop, Slide,
                     JumpStart, JumpLoop, SurfaceEnd, SurfaceStart, SurfaceLoop, DiveJump, Dash,
                     Bound, Hit, Damage, PlessieChaseBellHit)

/// A constant linear curve used to map the swim speed to sound parameters.
struct SeCurve {
    const f32* mFloats;
    sead::hostio::CurveDataInfo mInfo;

    /** @brief Evaluates the curve.
     * @param t Curve input.
     * @return Interpolated value.
     */
    f32 interpolate(f32 t) const {
        return sead::hostio::sCurveFunctionTbl_f32[mInfo.curveType](t, &mInfo, mFloats);
    }
};

/// Swim sound volume curves over the speed rate: high, mid, low and decelerating swimming.
const f32 cSwimCurveData[4][8] = {
    {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.95f, 1.0f},
    {0.0f, 0.0f, 0.0f, 0.3f, 0.9f, 0.75f, 1.0f, 0.0f},
    {0.0f, 0.0f, 1.0f, 0.7f, 1.0f, 0.0f, 0.0f, 0.0f},
    {1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
};
const f32 cSwimTurnCurveData[] = {0.0f, 0.0f, 0.0f, 0.25f, 0.05f, 0.7f, 0.5f, 1.0f};
const f32 cStrokeCurveData[] = {0.0f, 0.0f, 0.0f, 0.2f, 1.0f};

const SeCurve cSwimHighCurve = {cSwimCurveData[0], {0, 4, 8, 8}};
const SeCurve cSwimMidCurve = {cSwimCurveData[1], {0, 4, 8, 8}};
const SeCurve cSwimLowCurve = {cSwimCurveData[2], {0, 4, 8, 8}};
const SeCurve cSwimDecelCurve = {cSwimCurveData[3], {0, 4, 8, 8}};
const SeCurve cSwimTurnCurve = {cSwimTurnCurveData, {0, 4, 8, 8}};
const SeCurve cStrokeCurve = {cStrokeCurveData, {0, 4, 5, 5}};

/** @brief Calculates the horizontal length of a velocity.
 * @param rVelocity Velocity to measure.
 * @return Length on the XZ plane.
 */
inline f32 calcSpeedH(const sead::Vector3f& rVelocity) {
    return nerd::sqrt(rVelocity.x * rVelocity.x + rVelocity.z * rVelocity.z);
}
}  // namespace

/** @brief Constructs the animation state and starts it in the landing nerve.
 * @param pName State name.
 * @param pHost Plessie actor whose animations are driven.
 */
RaidonSurfAnimState::RaidonSurfAnimState(const char* pName, RaidonSurf* pHost)
    : al::HostStateBase<RaidonSurf>(pName, pHost) {
    initNerve(&NrvRaidonSurfAnimStateLand, 0);
}

/** @brief Activates the state and picks the initial nerve from Plessie's surroundings. */
void RaidonSurfAnimState::appear() {
    al::NerveStateBase::appear();
    al::setEffectFollowPosPtr(getHost(), "SwimDiveFollow", &mSplashPos);
    endMove();

    if (getHost()->isInWater()) {
        al::setNerve(this, &NrvRaidonSurfAnimStateSwim);
    } else if (getHost()->isOnGroundRaidon()) {
        al::setNerve(this, &NrvRaidonSurfAnimStateRun);
    } else {
        al::setNerve(this, &NrvRaidonSurfAnimStateFall);
    }
}

/** @brief Stops the swim turn sound and every looping swim effect. */
void RaidonSurfAnimState::endMove() {
    al::stopSeByName(getHost(), "PgSwimTurn");
    al::tryDeleteEffect(getHost(), "SwimFrontOcean");
    al::tryDeleteEffect(getHost(), "SwimLeftOcean");
    al::tryDeleteEffect(getHost(), "SwimRightOcean");
    al::tryDeleteEffect(getHost(), "SwimNeutralOcean");
    al::tryDeleteEffect(getHost(), "SwimNeutralRipple");
    al::tryDeleteEffect(getHost(), "SwimNeutralTrail");
    mMoveEffectFlags = 0;
}

/** @brief Drops pending requests, stops the movement effects and deactivates the state. */
void RaidonSurfAnimState::kill() {
    mIsRequestDive = false;
    mIsRequestJump = false;
    endMove();
    al::NerveStateBase::kill();
}

/** @brief Updates per-frame flags and plays the dive splash when Plessie enters the water. */
void RaidonSurfAnimState::control() {
    mIsOnSlideGround = getHost()->isOnSlideGround();

    if (getHost()->isOnGroundRaidon() || !al::isNerve(this, &NrvRaidonSurfAnimStateSwim)) {
        endMove();
    }

    if (isDive()) {
        if (getHost()->isInWater()) {
            if ((al::isNerve(this, &NrvRaidonSurfAnimStateDiveStart) &&
                 (!mIsInWaterPrev || al::isStep(this, 10))) ||
                (al::isNerve(this, &NrvRaidonSurfAnimStateDiveLoop) && !mIsInWaterPrev)) {
                playSplash();
                al::tryEmitEffect(getHost(), "SwimDiveFollow", nullptr);
                sead::Vector3f* pVelocity = al::getVelocityPtr(getHost());
                if (pVelocity->y < -40.0f) {
                    pVelocity->y = -40.0f;
                }

                alPadRumbleFunction::startPadRumble(getHost(), "DarkBowserHit", -1, true);
            }
        }

        if (al::isEffectEmitting(getHost(), "SwimDiveFollow")) {
            mSplashPos = al::getTrans(getHost());
            mSplashPos.y = getHost()->getWaterSurfaceY();
        }
    }

    mIsInWaterPrev = getHost()->isInWater();
}

/** @return Whether Plessie is in one of the dive nerves. */
bool RaidonSurfAnimState::isDive() const {
    return al::isNerve(this, &NrvRaidonSurfAnimStateDiveStart) ||
           al::isNerve(this, &NrvRaidonSurfAnimStateDiveLoop) ||
           al::isNerve(this, &NrvRaidonSurfAnimStateSurfaceStart) ||
           al::isNerve(this, &NrvRaidonSurfAnimStateSurfaceLoop);
}

/** @brief Emits the water splash matching the current dive nerve and plays the splash sound. */
void RaidonSurfAnimState::playSplash() {
    if (al::isNerve(this, &NrvRaidonSurfAnimStateDiveStart) ||
        al::isNerve(this, &NrvRaidonSurfAnimStateDiveLoop)) {
        mSplashPos = al::getTrans(getHost());
        mSplashPos.y = getHost()->getWaterSurfaceY();
        al::tryEmitEffect(getHost(), "DiveInSplash", &mSplashPos);
    }

    if (al::isNerve(this, &NrvRaidonSurfAnimStateSurfaceEnd)) {
        al::tryEmitEffect(getHost(), "DiveOutSplash", nullptr);
        if (mIsRequestJump) {
            al::tryEmitEffect(getHost(), "DiveOutJumpSplash", nullptr);
        }
    }

    if (al::isNerve(this, &NrvRaidonSurfAnimStateSurfaceLoop) && mIsRequestJump) {
        al::tryEmitEffect(getHost(), "DiveOutSplash", nullptr);
        al::tryEmitEffect(getHost(), "DiveOutJumpSplash", nullptr);
    }

    if (al::isNerve(this, &NrvRaidonSurfAnimStateDiveJump)) {
        al::tryEmitEffect(getHost(), "DiveOutSplash", nullptr);
        al::tryEmitEffect(getHost(), "DiveOutJumpSplash", nullptr);
    }

    al::startSe(getHost(), "PgSplash");
}

/** @brief Swims on the water surface, switching to landing, falling or running when needed. */
void RaidonSurfAnimState::exeSwim() {
    if (al::isFirstStep(this)) {
        getHost()->startPuppetActionAll("RaidonMove");
        mIsEnableDive = true;
        _2f = true;
        mIsWaitDoDive = true;
        mIsRequestJump = false;
    }

    if (calcSpeedH(*al::getVelocityPtr(getHost())) < 2.0f) {
        if (!al::isActionPlaying(getHost(), "SwimWait") &&
            (!al::isActionOneTime(getHost(), al::getActionName(getHost())) || al::isActionEnd(getHost()))) {
            al::startAction(getHost(), "SwimWait");
        }
    } else if (!al::isActionPlaying(getHost(), "SwimMove") &&
               (!al::isActionOneTime(getHost(), al::getActionName(getHost())) || al::isActionEnd(getHost()))) {
        al::startAction(getHost(), "SwimMove");
    }

    getHost()->setPuppetInputBlendAnimWeight();
    getHost()->setInputBlendAnimWeight();
    updateSwimSound();
    updateMoveEffect();

    if (getHost()->isOnGroundRaidon() || getHost()->isInWater()) {
        if (mAirCount >= 10) {
            changeToNerve(this, &NrvRaidonSurfAnimStateLand);
        }

        mAirCount = 0;
    } else {
        mAirCount++;
        if (mAirCount >= 30) {
            changeToNerve(this, &NrvRaidonSurfAnimStateFall);
            return;
        }
    }

    if (!getHost()->isInWater() && al::isGreaterEqualStep(this, 10)) {
        al::startSe(getHost(), "PgGoOutOfWater");
        if (mIsOnSlideGround) {
            changeToNerve(this, &NrvRaidonSurfAnimStateSlide);
        } else if (getHost()->isOnGroundRaidon()) {
            changeToNerve(this, &NrvRaidonSurfAnimStateRun);
        }
    }
}

/** @brief Updates the swim, stroke and turn sounds from Plessie's speed and animation frame. */
void RaidonSurfAnimState::updateSwimSound() {
    RaidonSurf* pHost = getHost();
    f32 speed = calcSpeedH(al::getVelocity(pHost));
    f32 speedRate = speed / 66.0f;
    f32 decelVolume = cSwimDecelCurve.interpolate(speedRate);
    f32 highVolume = cSwimHighCurve.interpolate(speedRate);
    f32 midVolume = cSwimMidCurve.interpolate(speedRate);
    f32 lowVolume = cSwimLowCurve.interpolate(speedRate);

    if (speedRate > 0.9f) {
        al::holdSeWithParam(pHost, "PgSwimHigh", highVolume);
    }

    if (speedRate < 0.9f && speedRate > 0.6f) {
        al::holdSeWithParam(pHost, "PgSwimMid", midVolume);
    }

    if (speedRate < 0.6f && speedRate > 0.2f) {
        al::holdSeWithParam(pHost, "PgSwimLow", lowVolume);
    }

    if (speedRate < 0.2f && speedRate > 0.1f) {
        al::holdSeWithParam(pHost, "PgSwimDecel", decelVolume);
    }

    al::holdSeWithParam(pHost, "PgRunOnEachMaterial", speedRate);

    if (al::isNerve(this, &NrvRaidonSurfAnimStateSurfaceEnd) ||
        al::isNerve(this, &NrvRaidonSurfAnimStateRun) ||
        al::isNerve(this, &NrvRaidonSurfAnimStateSlide)) {
        if (al::isNerve(this, &NrvRaidonSurfAnimStateRun)) {
            if (al::isActionPlaying(getHost(), "RunMoveGround")) {
                if (al::isSklAnimPlaying(getHost(), "Run", 0) &&
                    al::getSklAnimBlendWeight(getHost(), 0) > 0.9f) {
                    f32 frame = al::getSklAnimFrame(getHost(), 0);
                    if (frame == 20.0f || frame == 40.0f) {
                        al::startSe(pHost, "PgStrokeRun");
                    }
                } else if (al::getActionFrame(pHost) == 5.0f) {
                    al::startSe(pHost, "PgStrokeGround");
                }
            }
        } else if (al::isNerve(this, &NrvRaidonSurfAnimStateSlide) &&
                   al::isActionPlaying(getHost(), "RunMove")) {
            if (al::isSklAnimPlaying(getHost(), "SlideFront", 0) &&
                al::getSklAnimBlendWeight(getHost(), 3) > 0.6f) {
                f32 frame = al::getSklAnimFrame(getHost(), 3);
                if (frame == 45.0f || frame == 80.0f) {
                    al::startSe(pHost, "PgStrokeGround");
                }
            } else if (al::getActionFrame(pHost) == 80.0f) {
                al::startSe(pHost, "PgStrokeGround");
            }
        }
    } else {
        f32 frame = al::getActionFrame(pHost);
        f32 stickY = pHost->getPuppetInputStickY();
        f32 strokeHard = stickY > 0.0f ? stickY : -stickY;
        if (frame == 30.0f || frame == 90.0f) {
            al::startSe(pHost, "PgStroke");
            al::startSeWithParam(pHost, "PgStrokeHard", strokeHard);
        } else if ((frame == 60.0f || frame == 119.0f) && decelVolume < 0.8) {
            f32 strokeVolume = cStrokeCurve.interpolate(speedRate);
            al::startSeWithParam(pHost, "PgStroke", strokeVolume);
            al::startSeWithParam(pHost, "PgStrokeHard", strokeHard);
        }
    }

    f32 handle = pHost->getHandle();
    f32 turnVolume = cSwimTurnCurve.interpolate(handle > 0.0f ? handle : -handle);
    if (speed < 10.0f) {
        turnVolume = speed / 10.0 * turnVolume;
    }

    al::holdSeWithParam(pHost, "PgSwimTurn", turnVolume);
    mHandle = handle;
}

/** @brief Emits or stops the looping swim effects from Plessie's horizontal speed and turning. */
void RaidonSurfAnimState::updateMoveEffect() {
    if (getHost()->isOnGroundRaidon()) {
        return;
    }

    sead::Vector3f velocity = al::getVelocity(getHost());
    sead::Vector3f front;
    al::calcFrontDir(&front, getHost());
    f32 turn = velocity.x * front.z - velocity.z * front.x;
    f32 speed = calcSpeedH(velocity);

    tryEmitMoveEffect(speed > 35.0f, "SwimFrontOcean", MoveEffect_Front);
    tryEmitMoveEffect(speed > 12.0f, "SwimNeutralTrail", MoveEffect_Trail);
    tryEmitMoveEffect(speed > 12.0f && turn > 30.0f, "SwimRightOcean", MoveEffect_Right);
    tryEmitMoveEffect(speed > 12.0f && turn < -30.0f, "SwimLeftOcean", MoveEffect_Left);
    tryEmitMoveEffect(speed > 12.0f && speed < 35.0f, "SwimNeutralOcean", MoveEffect_Neutral);
    tryEmitMoveEffect(speed < 12.0f, "SwimNeutralRipple", MoveEffect_Ripple);
}

/** @brief Switches nerves, cleaning up the swim or dive effects of the nerve being left.
 * @param pUser Nerve user to switch.
 * @param pNerve Nerve to switch to.
 */
void RaidonSurfAnimState::changeToNerve(al::IUseNerve* pUser, const al::Nerve* pNerve) {
    if (al::isNerve(pUser, &NrvRaidonSurfAnimStateSwim)) {
        mIsWaitDoDive = false;
        endMove();
    } else if (isDive()) {
        if (al::isEffectEmitting(getHost(), "SwimDiveFollow")) {
            al::tryDeleteEffect(getHost(), "SwimDiveFollow");
        }
    }

    al::setNerve(pUser, pNerve);
}

/** @brief Runs on the ground, switching to landing, falling or swimming when needed. */
void RaidonSurfAnimState::exeRun() {
    if (al::isFirstStep(this)) {
        getHost()->startPuppetActionAll("RaidonMove");
        al::startAction(getHost(), "RunMoveGround");
        mIsEnableDive = false;
        _2f = true;
        mIsWaitDoDive = true;
    }

    getHost()->setPuppetInputBlendAnimWeight();
    getHost()->setInputBlendAnimWeight();
    updateSwimSound();

    if (getHost()->isOnGroundRaidon() || getHost()->isInWater()) {
        if (mAirCount >= 10) {
            changeToNerve(this, &NrvRaidonSurfAnimStateLand);
        }

        mAirCount = 0;
    } else {
        mAirCount++;
    }

    if (mIsRequestDive) {
        al::tryStartActionIfNotPlaying(getHost(), "AirEndGround");
        mIsRequestDive = false;
    }

    if (!al::isActionPlaying(getHost(), "AirEndGround") ||
        (al::isActionPlaying(getHost(), "AirEndGround") && al::isActionEnd(getHost()))) {
        if (calcSpeedH(al::getVelocity(getHost())) < 5.0f) {
            al::tryStartActionIfNotPlaying(getHost(), "RunIdle");
        } else {
            al::tryStartActionIfNotPlaying(getHost(), "RunMoveGround");
        }
    }

    if (mAirCount >= 30) {
        changeToNerve(this, &NrvRaidonSurfAnimStateFall);
        return;
    }

    if (getHost()->isInWater() && al::isGreaterEqualStep(this, 10)) {
        al::startSe(getHost(), "PgGoIntoWater");
        changeToNerve(this, &NrvRaidonSurfAnimStateSwim);
    }
}

/** @brief Slides down sloped ground, switching to jumping, landing, falling or swimming. */
void RaidonSurfAnimState::exeSlide() {
    if (al::isFirstStep(this)) {
        getHost()->startPuppetActionAll("RaidonMove");
        al::startAction(getHost(), "SlideFront");
        mIsEnableDive = false;
        _2f = true;
        mIsWaitEnterWaterSe = !getHost()->isInWater();
    }

    getHost()->setPuppetInputBlendAnimWeight();
    getHost()->setInputBlendAnimWeight();
    updateSwimSound();

    if (mIsWaitEnterWaterSe && getHost()->isInWater()) {
        al::startSe(getHost(), "PgGoIntoWater");
        mIsWaitEnterWaterSe = false;
    }

    if (mIsRequestJump) {
        changeToNerve(this, &NrvRaidonSurfAnimStateJumpStart);
        return;
    }

    mIsRequestJump = false;

    if (getHost()->isOnGroundOrWaterRaidon()) {
        if (mAirCount >= 10) {
            changeToNerve(this, &NrvRaidonSurfAnimStateLand);
        }

        mAirCount = 0;
    } else {
        mAirCount++;
    }

    if (calcSpeedH(al::getVelocity(getHost())) < 5.0f) {
        al::tryStartActionIfNotPlaying(getHost(), "SlideIdle");
    } else {
        al::tryStartActionIfNotPlaying(getHost(), "SlideFront");
    }

    if (mAirCount >= 30) {
        changeToNerve(this, &NrvRaidonSurfAnimStateFall);
    } else if (getHost()->isInWater() && al::isGreaterEqualStep(this, 10)) {
        changeToNerve(this, &NrvRaidonSurfAnimStateSwim);
    }
}

/** @brief Starts a jump, allowing a dive once the take-off is far enough along. */
void RaidonSurfAnimState::exeJumpStart() {
    if (al::isFirstStep(this)) {
        mIsWaitDoDive = false;
        bool isInWater = getHost()->isInWater();
        al::startAction(getHost(), isInWater ? "AirStart" : "AirStartGround");
        if (isInWater) {
            al::startOceanWave(getHost(), "JumpStart");
        }

        mIsEnableDive = false;
        mIsRequestJump = false;
        getHost()->doJump(false);
    }

    getHost()->setPuppetInputBlendAnimWeight();

    if (al::isGreaterEqualStep(this, 14)) {
        mIsEnableDive = true;
    }

    if (mIsEnableDive && mIsRequestDive) {
        mIsRequestDive = false;
        changeToNerve(this, &NrvRaidonSurfAnimStateDiveStart);
        return;
    }

    if (al::isActionEnd(getHost())) {
        changeToNerve(this, &NrvRaidonSurfAnimStateJumpLoop);
    }
}

/** @brief Stays airborne after a jump until Plessie dives, lands or starts falling. */
void RaidonSurfAnimState::exeJumpLoop() {
    if (al::isFirstStep(this)) {
        al::startAction(getHost(), "AirLoop");
        mIsEnableDive = true;
        mIsRequestJump = false;
    }

    getHost()->setPuppetInputBlendAnimWeight();

    if (mIsRequestDive) {
        mIsRequestDive = false;
        changeToNerve(this, &NrvRaidonSurfAnimStateDiveStart);
        return;
    }

    if (getHost()->isOnGroundRaidon() || getHost()->isInWater()) {
        changeToNerve(this, &NrvRaidonSurfAnimStateLand);
        return;
    }

    if (al::isGreaterEqualStep(this, 60)) {
        changeToNerve(this, &NrvRaidonSurfAnimStateFall);
    }
}

/** @brief Falls until Plessie touches ground or water. */
void RaidonSurfAnimState::exeFall() {
    if (al::isFirstStep(this)) {
        al::startAction(getHost(), "Fall");
        mIsEnableDive = true;
    }

    getHost()->setPuppetInputBlendAnimWeight();

    if (getHost()->isOnGroundOrWaterRaidon()) {
        if (getHost()->isInWater() && al::isGreaterEqualStep(this, 55)) {
            al::startSe(getHost(), "PgFallEnd");
        }

        changeToNerve(this, &NrvRaidonSurfAnimStateLand);
    }
}

/** @brief Lands on ground or water and continues swimming, sliding, running, jumping or diving. */
void RaidonSurfAnimState::exeLand() {
    if (al::isFirstStep(this)) {
        mAirCount = 0;
        al::startAction(getHost(), getHost()->isInWater() ? "AirEnd" : "AirEndGround");
        if (getHost()->isInWater()) {
            al::startOceanWave(getHost(), "JumpEnd");
            f32 velocityY = al::getVelocity(getHost()).y;
            if (getHost()->isInWater() && velocityY < -40.0f) {
                al::startHitReaction(getHost(), "BigLand");
            }
        }

        _2f = true;
        mIsLanding = true;
        mIsEnableDive = true;
        mIsWaitDoDive = true;
        if (getHost()->isInWater()) {
            mIsWaitEnterWaterSe = false;
        } else {
            mIsWaitEnterWaterSe = true;
        }
    }

    if (mIsWaitEnterWaterSe && getHost()->isInWater()) {
        al::startSe(getHost(), "PgGoIntoWater");
        mIsWaitEnterWaterSe = false;
    }

    getHost()->setPuppetInputBlendAnimWeight();

    if (mIsRequestJump) {
        playJumpSound();
        changeToNerve(this, &NrvRaidonSurfAnimStateJumpStart);
        return;
    }

    if (mIsRequestDive) {
        if (al::isActionPlaying(getHost(), "AirEndGround")) {
            mIsRequestDive = false;
            return;
        }

        changeToNerve(this, &NrvRaidonSurfAnimStateDiveStart);
        return;
    }

    if (al::isActionEnd(getHost())) {
        mIsLanding = false;
        if (getHost()->isInWater()) {
            changeToNerve(this, &NrvRaidonSurfAnimStateSwim);
        } else if (mIsOnSlideGround) {
            changeToNerve(this, &NrvRaidonSurfAnimStateSlide);
        } else {
            changeToNerve(this, &NrvRaidonSurfAnimStateRun);
        }
    }
}

/** @brief Plays the jump sound matching whether Plessie jumps from water or ground. */
void RaidonSurfAnimState::playJumpSound() {
    if (getHost()->isInWater()) {
        al::startSe(getHost(), "PgAirStart");
    } else {
        al::startSe(getHost(), "PgJumpGround");
    }
}

/** @brief Bounces off something until Plessie lands or the bounce animation ends. */
void RaidonSurfAnimState::exeBound() {
    if (al::isFirstStep(this)) {
        al::startAction(getHost(), "AirBounce");
        mIsEnableDive = false;
    }

    getHost()->setPuppetInputBlendAnimWeight();

    if (getHost()->isOnGroundRaidon()) {
        changeToNerve(this, &NrvRaidonSurfAnimStateLand);
    } else if (al::isActionEnd(getHost())) {
        changeToNerve(this, &NrvRaidonSurfAnimStateJumpLoop);
    }
}

/** @brief Plays the hit reaction for the current surroundings, then resumes moving. */
void RaidonSurfAnimState::exeHit() {
    if (al::isFirstStep(this)) {
        mIsRequestJump = false;
        mIsEnableDive = false;
        mIsWaitDoDive = true;
        if (getHost()->isInWater()) {
            al::tryStartActionIfNotPlaying(getHost(), "HitWater");
        } else if (getHost()->isOnGroundRaidon()) {
            al::tryStartActionIfNotPlaying(getHost(), "HitGround");
        } else {
            al::tryStartActionIfNotPlaying(getHost(), "HitAir");
        }

        if (getHost()->isUnderwater()) {
            getHost()->doJump(false);
        }
    }

    getHost()->setPuppetInputBlendAnimWeight();

    if (al::isActionEnd(getHost())) {
        if (getHost()->isOnGroundRaidon() || getHost()->isInWater()) {
            mAirCount = 0;
            if (getHost()->isInWater()) {
                changeToNerve(this, &NrvRaidonSurfAnimStateSwim);
            } else if (mIsOnSlideGround) {
                changeToNerve(this, &NrvRaidonSurfAnimStateSlide);
            } else if (getHost()->isUnderwater()) {
                changeToNerve(this, &NrvRaidonSurfAnimStateSurfaceEnd);
            } else {
                changeToNerve(this, &NrvRaidonSurfAnimStateRun);
            }
        } else {
            changeToNerve(this, &NrvRaidonSurfAnimStateFall);
        }
    }
}

/** @brief Dashes forward, then returns to swimming, sliding, running or falling. */
void RaidonSurfAnimState::exeDash() {
    if (al::isFirstStep(this)) {
        _2f = true;
        mIsWaitDoDive = true;
        mIsEnableDive = false;
        al::tryDeleteEffect(getHost(), "SwimDash");
        al::startAction(getHost(), getHost()->isInWater() ? "SwimDash" : "RunDash");
        if (getHost()->isInWater()) {
            mIsWaitEnterWaterSe = false;
        } else {
            mIsWaitEnterWaterSe = true;
        }
    }

    if (mIsWaitEnterWaterSe && getHost()->isInWater()) {
        al::startSe(getHost(), "PgGoIntoWater");
        mIsWaitEnterWaterSe = false;
    }

    getHost()->setPuppetInputBlendAnimWeight();

    if (mIsRequestJump && getHost()->isOnGroundOrWaterRaidon()) {
        changeToNerve(this, &NrvRaidonSurfAnimStateJumpStart);
        return;
    }

    if (getHost()->isOnGroundRaidon() || getHost()->isInWater()) {
        mAirCount = 0;
    } else {
        mAirCount++;
    }

    if (al::isActionEnd(getHost())) {
        if (mAirCount >= 30) {
            changeToNerve(this, &NrvRaidonSurfAnimStateFall);
        } else {
            mAirCount = 0;
            if (getHost()->isInWater()) {
                changeToNerve(this, &NrvRaidonSurfAnimStateSwim);
            } else if (mIsOnSlideGround) {
                changeToNerve(this, &NrvRaidonSurfAnimStateSlide);
            } else {
                changeToNerve(this, &NrvRaidonSurfAnimStateRun);
            }
        }
    }
}

/** @brief Starts diving under the water surface. */
void RaidonSurfAnimState::exeDiveStart() {
    if (al::isFirstStep(this)) {
        _2f = false;
        mIsRequestDive = false;
        mIsEnableDive = false;
        mIsWaitDoDive = true;
        al::startAction(getHost(), "DiveStart");
        if (!getHost()->isOnGroundRaidon() || al::getVelocityPtr(getHost())->y < 0.0f) {
            al::getVelocityPtr(getHost())->y = 0.0f;
        }
    }

    getHost()->setPuppetInputBlendAnimWeight();

    if (al::isGreaterEqualStep(this, 10) && mIsWaitDoDive) {
        mIsWaitDoDive = false;
        getHost()->doDive(false);
    }

    if (al::isActionEnd(getHost())) {
        al::setNerve(this, &NrvRaidonSurfAnimStateDiveLoop);
    }
}

/** @brief Swims underwater until Plessie is deep enough or starts rising. */
void RaidonSurfAnimState::exeDiveLoop() {
    if (al::isFirstStep(this)) {
        _2f = true;
        mIsRequestDive = false;
        mIsEnableDive = false;
        mIsWaitDoDive = false;
        al::startAction(getHost(), "DiveLoop");
        if (!al::isEffectEmitting(getHost(), "SwimDiveFollow")) {
            al::tryEmitEffect(getHost(), "SwimDiveFollow", nullptr);
        }
    }

    updateSwimSound();
    getHost()->setPuppetInputBlendAnimWeight();

    if (getHost()->getDiveDepth() > getHost()->getDiveDepthLimit() ||
        al::getVelocityPtr(getHost())->y > 0.0f) {
        al::setNerve(this, &NrvRaidonSurfAnimStateSurfaceStart);
    }
}

/** @brief Starts rising back towards the water surface. */
void RaidonSurfAnimState::exeSurfaceStart() {
    if (al::isFirstStep(this)) {
        _2f = true;
        mIsEnableDive = true;
        al::startAction(getHost(), "SurfaceStart");
        if (!al::isEffectEmitting(getHost(), "SwimDiveFollow")) {
            al::tryEmitEffect(getHost(), "SwimDiveFollow", nullptr);
        }

        if (al::getVelocityPtr(getHost())->y < 0.0f) {
            al::getVelocityPtr(getHost())->y *= 0.8f;
        }
    }

    getHost()->setPuppetInputBlendAnimWeight();

    if (al::isActionEnd(getHost())) {
        al::setNerve(this, &NrvRaidonSurfAnimStateSurfaceLoop);
    }
}

/** @brief Rises towards the surface, jumping out of the water if a jump was requested. */
void RaidonSurfAnimState::exeSurfaceLoop() {
    if (al::isFirstStep(this)) {
        mIsEnableDive = true;
        _2f = true;
        al::startAction(getHost(), "SurfaceLoop");
    }

    getHost()->setPuppetInputBlendAnimWeight();

    if (getHost()->getDiveDepth() < 100.0f) {
        if (mIsRequestJump) {
            al::getVelocityPtr(getHost())->y = 6.5f;
            playJumpSound();
            if (mIsRequestDiveJump) {
                mIsRequestDiveJump = false;
                changeToNerve(this, &NrvRaidonSurfAnimStateDiveJump);
            } else {
                playSplash();
                changeToNerve(this, &NrvRaidonSurfAnimStateJumpStart);
            }
        } else {
            changeToNerve(this, &NrvRaidonSurfAnimStateSurfaceEnd);
        }
    }
}

/** @brief Performs the perfectly timed jump out of a dive. */
void RaidonSurfAnimState::exeDiveJump() {
    if (al::isFirstStep(this)) {
        _2f = false;
        mIsEnableDive = true;
        mIsRequestJump = false;
        al::startAction(getHost(), "DiveToJumpPerfect");
        al::tryEmitEffect(getHost(), "SpecialJump", nullptr);
        if (mIsInWaterPrev) {
            al::startOceanWave(getHost(), "JumpStart");
            playSplash();
        }

        getHost()->doJump(true);
    }

    if (mIsRequestDive) {
        mIsRequestDive = false;
        changeToNerve(this, &NrvRaidonSurfAnimStateDiveStart);
        return;
    }

    if (al::isActionEnd(getHost())) {
        changeToNerve(this, &NrvRaidonSurfAnimStateJumpLoop);
    }
}

/** @brief Breaks through the water surface and returns to swimming. */
void RaidonSurfAnimState::exeSurfaceEnd() {
    if (al::isFirstStep(this)) {
        mIsEnableDive = true;
        _2f = true;
        mIsWaitDoDive = true;
        if (calcSpeedH(al::getVelocity(getHost())) < 5.0f) {
            al::startAction(getHost(), "SurfaceEndSwim");
        } else {
            al::startAction(getHost(), "SurfaceEndSwimFront");
        }
    }

    if (mIsRequestJump) {
        mIsWaitDoDive = false;
        al::getVelocityPtr(getHost())->y = 6.5f;
        playJumpSound();
        if (mIsRequestDiveJump) {
            mIsRequestDiveJump = false;
            changeToNerve(this, &NrvRaidonSurfAnimStateDiveJump);
        } else {
            playSplash();
            changeToNerve(this, &NrvRaidonSurfAnimStateJumpStart);
        }

        return;
    }

    if (mIsRequestDive) {
        changeToNerve(this, &NrvRaidonSurfAnimStateDiveStart);
        return;
    }

    updateSwimSound();
    getHost()->setPuppetInputBlendAnimWeight();

    if (al::isActionEnd(getHost())) {
        changeToNerve(this, &NrvRaidonSurfAnimStateSwim);
    }
}

/** @brief Plays the damage reaction, then resumes moving or dives if requested. */
void RaidonSurfAnimState::exeDamage() {
    if (al::isFirstStep(this)) {
        mIsEnableDive = true;
        _2f = true;
        mIsWaitDoDive = true;
        al::tryStartAction(getHost(), "Damage");
    }

    getHost()->setPuppetInputBlendAnimWeight();

    if (mIsRequestDive && !getHost()->isOnGroundRaidon() && getHost()->isInWater() &&
        !getHost()->isUnderwater()) {
        changeToNerve(this, &NrvRaidonSurfAnimStateDiveStart);
        return;
    }

    if (al::isActionEnd(getHost())) {
        if (getHost()->isOnGroundRaidon() || getHost()->isInWater()) {
            mAirCount = 0;
            if (getHost()->isInWater()) {
                changeToNerve(this, &NrvRaidonSurfAnimStateSwim);
            } else if (mIsOnSlideGround) {
                changeToNerve(this, &NrvRaidonSurfAnimStateSlide);
            } else if (getHost()->isUnderwater()) {
                changeToNerve(this, &NrvRaidonSurfAnimStateSurfaceEnd);
            } else {
                changeToNerve(this, &NrvRaidonSurfAnimStateRun);
            }
        } else {
            changeToNerve(this, &NrvRaidonSurfAnimStateFall);
        }
    }
}

/** @brief Plays the reaction to hitting a giant bell during the Plessie chase. */
void RaidonSurfAnimState::exePlessieChaseBellHit() {
    if (al::isFirstStep(this)) {
        mIsEnableDive = true;
        al::startAction(getHost(), "ChaseGigaBellHit");
    }

    if (al::isGreaterEqualStep(this, 32) && mIsRequestDive) {
        changeToNerve(this, &NrvRaidonSurfAnimStateDiveStart);
    } else if (al::isStep(this, 75)) {
        changeToNerve(this, &NrvRaidonSurfAnimStateFall);
    }
}

/** @brief Requests a jump, starting it immediately when the current nerve allows it.
 * @return Whether the jump was started right away.
 */
bool RaidonSurfAnimState::requestJump() {
    if (mIsRequestJump) {
        return false;
    }

    if (al::isNerve(this, &NrvRaidonSurfAnimStateFall)) {
        return false;
    }

    if (al::isNerve(this, &NrvRaidonSurfAnimStateDash) && al::isLessEqualStep(this, 1)) {
        return false;
    }

    if (al::isNerve(this, &NrvRaidonSurfAnimStateSurfaceLoop)) {
        if (getHost()->getDiveDepth() < 200.0f) {
            mIsRequestDiveJump = true;
        }
    } else if (al::isNerve(this, &NrvRaidonSurfAnimStateSurfaceEnd)) {
        if (al::isLessEqualStep(this, 10)) {
            mIsRequestDiveJump = true;
        }
    } else if (al::isNerve(this, &NrvRaidonSurfAnimStateDiveStart) ||
               al::isNerve(this, &NrvRaidonSurfAnimStateDiveLoop) ||
               al::isNerve(this, &NrvRaidonSurfAnimStateSurfaceStart) ||
               al::isNerve(this, &NrvRaidonSurfAnimStateLand)) {
        // The jump starts once the dive or landing nerve finishes.
    } else if (al::isNerve(this, &NrvRaidonSurfAnimStateSlide) ||
               al::isNerve(this, &NrvRaidonSurfAnimStateDash)) {
        bool isInWater = getHost()->isInWater();
        bool isDash = al::isNerve(this, &NrvRaidonSurfAnimStateDash);
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
    } else {
        if (!isDead()) {
            playJumpSound();
        }

        changeToNerve(this, &NrvRaidonSurfAnimStateJumpStart);
        return true;
    }

    mIsRequestJump = true;
    return false;
}

/** @brief Makes Plessie bounce. */
void RaidonSurfAnimState::requestBound() {
    changeToNerve(this, &NrvRaidonSurfAnimStateBound);
}

/** @brief Plays the hit reaction. */
void RaidonSurfAnimState::requestHit() {
    changeToNerve(this, &NrvRaidonSurfAnimStateHit);
}

/** @brief Starts a dash. */
void RaidonSurfAnimState::requestDash() {
    changeToNerve(this, &NrvRaidonSurfAnimStateDash);
}

/** @return Whether the hit reaction is playing. */
bool RaidonSurfAnimState::isHit() const {
    return al::isNerve(this, &NrvRaidonSurfAnimStateHit);
}

/** @brief Starts falling unless Plessie is already falling or landing. */
void RaidonSurfAnimState::requestFall() {
    if (al::isNerve(this, &NrvRaidonSurfAnimStateFall) ||
        al::isNerve(this, &NrvRaidonSurfAnimStateLand)) {
        return;
    }

    changeToNerve(this, &NrvRaidonSurfAnimStateFall);
}

/** @brief Returns to the idle movement nerve matching Plessie's surroundings. */
void RaidonSurfAnimState::requestIdle() {
    getHost()->startPuppetActionAll("RaidonMove");

    if (getHost()->isInWater()) {
        if (getHost()->getDiveDepth() > 100.0f) {
            changeToNerve(this, &NrvRaidonSurfAnimStateSurfaceLoop);
        } else {
            changeToNerve(this, &NrvRaidonSurfAnimStateSwim);
        }
    } else if (al::isActionPlaying(getHost(), "Fall") || al::isActionPlaying(getHost(), "AirLoop") ||
               al::isActionPlaying(getHost(), "DiveStart") ||
               al::isActionPlaying(getHost(), "DiveLoop")) {
        changeToNerve(this, &NrvRaidonSurfAnimStateLand);
    }
}

/** @brief Dives right away when swimming, otherwise remembers the dive request. */
void RaidonSurfAnimState::requestDive() {
    if (al::isNerve(this, &NrvRaidonSurfAnimStateSwim)) {
        changeToNerve(this, &NrvRaidonSurfAnimStateDiveStart);
        return;
    }

    if (mIsEnableDive || al::isNerve(this, &NrvRaidonSurfAnimStateRun)) {
        mIsRequestDive = true;
    }
}

/** @brief Plays the damage reaction unless it is already playing. */
void RaidonSurfAnimState::requestDamage() {
    if (al::isNerve(this, &NrvRaidonSurfAnimStateDamage)) {
        return;
    }

    changeToNerve(this, &NrvRaidonSurfAnimStateDamage);
}

/** @brief Plays the giant bell hit reaction unless it is already playing. */
void RaidonSurfAnimState::requestPlessieChaseBellHit() {
    if (al::isNerve(this, &NrvRaidonSurfAnimStatePlessieChaseBellHit)) {
        return;
    }

    changeToNerve(this, &NrvRaidonSurfAnimStatePlessieChaseBellHit);
}

/** @brief Aborts a dive and starts rising to the surface. */
void RaidonSurfAnimState::forceDiveEnd() {
    al::tryStartActionIfNotPlaying(getHost(), "HitGround");
    changeToNerve(this, &NrvRaidonSurfAnimStateSurfaceStart);
}

/** @return Whether Plessie is jumping out of a dive. */
bool RaidonSurfAnimState::isDiveJump() const {
    return al::isNerve(this, &NrvRaidonSurfAnimStateDiveJump);
}

/** @return Whether Plessie is diving below the water surface. */
bool RaidonSurfAnimState::isUnderwater() const {
    if (!isDive()) {
        return false;
    }

    if (!getHost()->isInWater()) {
        return false;
    }

    if (al::isNerve(this, &NrvRaidonSurfAnimStateDiveStart) && al::isLessEqualStep(this, 5)) {
        return false;
    }

    return true;
}

/** @brief Emits a looping swim effect when it should play, or deletes it when it should stop.
 * @param isEmit Whether the effect should currently be playing.
 * @param pName Effect name.
 * @param flag Bit in the move effect flags that tracks this effect.
 */
void RaidonSurfAnimState::tryEmitMoveEffect(bool isEmit, const char* pName, s32 flag) {
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

/** @return Whether Plessie is in a jump. */
bool RaidonSurfAnimState::isJumping() {
    return al::isNerve(this, &NrvRaidonSurfAnimStateJumpStart) ||
           al::isNerve(this, &NrvRaidonSurfAnimStateJumpLoop);
}

/** @brief Destroys the animation state. */
RaidonSurfAnimState::~RaidonSurfAnimState() = default;
