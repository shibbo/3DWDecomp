#include "Boss/KoopaChaseBattleLv1.hpp"

#include <attributes.h>
#include <hostio/seadHostIOCurve.h>

#include "Boss/KoopaChase.hpp"
#include "Boss/KoopaChaseDemoInfo.hpp"
#include "Boss/KoopaChaseFunction.hpp"
#include "Boss/KoopaChaseKoopa.hpp"
#include "Boss/KoopaChaseStateDamage.hpp"
#include "Boss/KoopaChaseStateDemo.hpp"
#include "Boss/KoopaChaseStateFire.hpp"
#include "Boss/KoopaChaseStateJump.hpp"
#include "Boss/KoopaChaseStateProvocation.hpp"
#include "Boss/KoopaChaseStateThrow.hpp"
#include "Boss/KoopaChaseStateWarp.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"

namespace {
NERVE_DECL(KoopaChaseBattleLv1, DemoBattleStart)
NERVE_DECL(KoopaChaseBattleLv1, DemoBattleEnd)
NERVE_DECL(KoopaChaseBattleLv1, Damage)
NERVE_DECL(KoopaChaseBattleLv1, Throw)
NERVE_DECL(KoopaChaseBattleLv1, Fire)
NERVE_DECL(KoopaChaseBattleLv1, Jump)
NERVE_DECL(KoopaChaseBattleLv1, Warp)
NERVE_DECL(KoopaChaseBattleLv1, Provocation)
NERVE_DECL(KoopaChaseBattleLv1, ProvocationFirst)
NERVE_DECL(KoopaChaseBattleLv1, Run)
// Mutable nerve objects; the compiler merges them with the run speed curve data below.
KoopaChaseBattleLv1NrvDemoBattleStart NrvKoopaChaseBattleLv1DemoBattleStart;
KoopaChaseBattleLv1NrvDemoBattleEnd NrvKoopaChaseBattleLv1DemoBattleEnd;
KoopaChaseBattleLv1NrvDamage NrvKoopaChaseBattleLv1Damage;
KoopaChaseBattleLv1NrvThrow NrvKoopaChaseBattleLv1Throw;
KoopaChaseBattleLv1NrvFire NrvKoopaChaseBattleLv1Fire;
KoopaChaseBattleLv1NrvJump NrvKoopaChaseBattleLv1Jump;
KoopaChaseBattleLv1NrvWarp NrvKoopaChaseBattleLv1Warp;
KoopaChaseBattleLv1NrvProvocation NrvKoopaChaseBattleLv1Provocation;
KoopaChaseBattleLv1NrvProvocationFirst NrvKoopaChaseBattleLv1ProvocationFirst;
KoopaChaseBattleLv1NrvRun NrvKoopaChaseBattleLv1Run;

/// A constant curve evaluated through sead's curve function table.
struct SpeedCurve {
    const f32* mFloats;
    sead::hostio::CurveDataInfo mInfo;

    /**
     * @brief Evaluates the curve.
     * @param t Curve input.
     * @return Interpolated value.
     */
    f32 interpolate(f32 t) const {
        return sead::hostio::sCurveFunctionTbl_f32[mInfo.curveType](t, &mInfo, mFloats);
    }
};

/// Run speed control points over the normalized distance to the player.
f32 sRunSpeedCurveData[8] = {2.0f, 2.8f, 4.65f, 7.15f, 12.7f, 19.6f, 27.35f, 41.0f};

/// Maps the normalized distance to the player to Bowser's run speed.
const SpeedCurve cRunSpeedCurve = {sRunSpeedCurveData, {0, 4, 8, 8}};

/**
 * @brief Calculates the run speed for a distance to the player.
 * @param distance Distance between Bowser and the player.
 * @return Run speed.
 */
inline f32 calcRunSpeed(f32 distance) {
    f32 rate = al::lerpValue(distance, KoopaChase::getRunStartDistance(),
                             KoopaChase::getRunStopDistance(), 0.0f, 1.0f);
    return cRunSpeedCurve.interpolate(rate);
}
}  // namespace

/**
 * @brief Creates the battle with its demos and every attack state.
 * @param pHost Chase actor controlled by this battle.
 * @param rInfo Actor initialization information.
 */
KoopaChaseBattleLv1::KoopaChaseBattleLv1(KoopaChase* pHost, const al::ActorInitInfo& rInfo)
    : KoopaChaseBattle("クッパチェイスLv1"), mHost(pHost) {
    initNerve(&NrvKoopaChaseBattleLv1DemoBattleStart, 16);
    mCameraInfo = al::initAnimCamera(mHost, rInfo);

    mDemoInfoStart = new KoopaChaseDemoInfo(mCameraInfo, mHost, "DemoBattleChaseStart", 0, 0, 60,
                                            nullptr);
    mDemoInfoStart->mName28 = "SwitchStartDemoActiveOn";
    mStateDemoStart =
        new KoopaChaseStateDemo("クッパチェイス開始デモ", mHost, rInfo, mDemoInfoStart);
    mStateDemoStart->entryDemoActor(mHost);
    mStateDemoStart->entryDemoActor(KoopaChaseFunction::getKoopa(mHost));
    al::initNerveState(this, mStateDemoStart, &NrvKoopaChaseBattleLv1DemoBattleStart,
                       "デモ[バトル開始]");

    mDemoInfoEnd = new KoopaChaseDemoInfo(mCameraInfo, mHost, "DemoBattleChaseEndLv1", 0, 0, 60,
                                          nullptr);
    mDemoInfoEnd->mName28 = "SwitchEndDemoActiveOn";
    mDemoInfoEnd->mValue20 = -1;
    mStateDemoEnd =
        new KoopaChaseStateDemo("クッパチェイスやられデモ", mHost, rInfo, mDemoInfoEnd);
    mStateDemoEnd->entryDemoActor(mHost);
    mStateDemoEnd->entryDemoActor(KoopaChaseFunction::getKoopa(mHost));
    al::initNerveState(this, mStateDemoEnd, &NrvKoopaChaseBattleLv1DemoBattleEnd,
                       "デモ[バトル終了]");

    al::tryGetLinksTrans(&mEndDemoTrans, rInfo, "EndDemoPos");
    al::tryGetLinksQuat(&mEndDemoQuat, rInfo, "EndDemoPos");
    mEndDemoParts = KoopaChaseFunction::tryCreateLinkObj(rInfo, "EndDemoParts", 0);
    if (mEndDemoParts != nullptr) {
        mStateDemoEnd->entryDemoActor(mEndDemoParts);
    }

    mStateDamage = new KoopaChaseStateDamage(mHost);
    al::initNerveState(this, mStateDamage, &NrvKoopaChaseBattleLv1Damage, "State[ダメージ]");

    mStateThrow = new KoopaChaseStateThrow(mHost, rInfo,
                                           calcRunSpeed(KoopaChase::getRunStopDistance() + 500.0f));
    al::initNerveState(this, mStateThrow, &NrvKoopaChaseBattleLv1Throw, "State[投げる]");

    mStateFire = new KoopaChaseStateFire(mHost, rInfo, false);
    mStateFire->setFireNum(1);
    al::initNerveState(this, mStateFire, &NrvKoopaChaseBattleLv1Fire, "State[火を吐く]");

    mStateJump = new KoopaChaseStateJump(mHost, rInfo);
    al::initNerveState(this, mStateJump, &NrvKoopaChaseBattleLv1Jump, "State[ジャンプ]");

    mStateWarp = new KoopaChaseStateWarp(mHost, rInfo);
    al::initNerveState(this, mStateWarp, &NrvKoopaChaseBattleLv1Warp, "State[ワープ]");

    mStateProvocation = new KoopaChaseStateProvocation(mHost, rInfo);
    al::initNerveState(this, mStateProvocation, &NrvKoopaChaseBattleLv1Provocation,
                       "State[挑発]");
    al::initNerveState(this, mStateProvocation, &NrvKoopaChaseBattleLv1ProvocationFirst,
                       "State[挑発初回]");
}

/**
 * @brief Forwards attacks to the damage state outside of the demos.
 * @param pMsg Received sensor message.
 * @param pOther Sensor that sent the message.
 * @param pSelf Sensor that received the message.
 * @return Whether the message was handled.
 */
bool KoopaChaseBattleLv1::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                                     al::HitSensor* pSelf) {
    if (al::isNerve(this, &NrvKoopaChaseBattleLv1DemoBattleStart) ||
        al::isNerve(this, &NrvKoopaChaseBattleLv1DemoBattleEnd) ||
        al::isNerve(this, &NrvKoopaChaseBattleLv1Damage)) {
        return false;
    }

    bool isJumpOrWarp = al::isNerve(this, &NrvKoopaChaseBattleLv1Jump) ||
                        al::isNerve(this, &NrvKoopaChaseBattleLv1Warp);
    bool isDamage = false;
    bool isReceived =
        mStateDamage->receiveMsg(&isDamage, pMsg, pOther, pSelf, isJumpOrWarp);
    if (!isDamage) {
        return isReceived;
    }

    if (!tryGoDemoBattleEnd() && !isJumpOrWarp) {
        al::setNerve(this, &NrvKoopaChaseBattleLv1Damage);
    }

    return true;
}

/**
 * @brief Starts the battle end demo once the damage state has finished.
 * @return Whether the end demo was started.
 */
bool KoopaChaseBattleLv1::tryGoDemoBattleEnd() {
    if (!mStateDamage->isDamageFinish()) {
        return false;
    }

    al::setVelocityZero(mHost);
    al::setVelocityZero(KoopaChaseFunction::getKoopa(mHost));
    sead::Vector3f front;
    al::calcQuatFront(&front, mEndDemoQuat);
    mStateDemoEnd->setReturnTrans(mEndDemoTrans);
    al::requestCaptureScreenCover(mHost, 10);
    al::setNerve(this, &NrvKoopaChaseBattleLv1DemoBattleEnd);
    return true;
}

/** @brief Does nothing; the battle is driven by its nerves. */
void KoopaChaseBattleLv1::control() {}

/**
 * @brief Checks whether Bowser should turn towards the player.
 * @return Whether turning is allowed.
 */
bool KoopaChaseBattleLv1::isStateTurnToTarget() const {
    if (!al::isNerve(this, &NrvKoopaChaseBattleLv1Damage) &&
        !al::isNerve(this, &NrvKoopaChaseBattleLv1DemoBattleStart) &&
        !al::isNerve(this, &NrvKoopaChaseBattleLv1DemoBattleEnd)) {
        return true;
    }

    return false;
}

/**
 * @brief Checks whether Bowser's pose should follow the chase.
 * @return Whether the pose is updated.
 */
bool KoopaChaseBattleLv1::isStateUpdatePose() const {
    if (al::isNerve(this, &NrvKoopaChaseBattleLv1DemoBattleStart)) {
        return false;
    }

    if (al::isNerve(this, &NrvKoopaChaseBattleLv1DemoBattleEnd)) {
        return false;
    }

    if (al::isNerve(this, &NrvKoopaChaseBattleLv1ProvocationFirst)) {
        if (al::isFirstStep(this)) {
            return false;
        }

        if (al::isNewNerve(this)) {
            return false;
        }
    }

    return !al::isNerve(this, &NrvKoopaChaseBattleLv1Jump);
}

/**
 * @brief Checks whether the chase move counter advances.
 * @return Whether Bowser is running.
 */
bool KoopaChaseBattleLv1::isStateUpdateMoveCount() const {
    return al::isNerve(this, &NrvKoopaChaseBattleLv1Run);
}

/**
 * @brief Checks whether the tire move is active.
 * @return Whether no demo is playing.
 */
bool KoopaChaseBattleLv1::isStateTireMove() const {
    if (al::isNerve(this, &NrvKoopaChaseBattleLv1DemoBattleStart)) {
        return false;
    }

    return !al::isNerve(this, &NrvKoopaChaseBattleLv1DemoBattleEnd);
}

/**
 * @brief Checks whether Bowser is warping.
 * @return Whether the warp nerve is active.
 */
bool KoopaChaseBattleLv1::isStateWarp() const {
    return al::isNerve(this, &NrvKoopaChaseBattleLv1Warp);
}

/** @brief Plays the battle start demo and starts the battle music. */
void KoopaChaseBattleLv1::exeDemoBattleStart() {
    if (al::isFirstStep(this)) {
        al::invalidateHitSensor(mHost, "ToadBlocker");
    }

    if (al::isStep(this, 720)) {
        al::startBgm(mHost, "Koopa", -1, 0, -1, -1);
    }

    if (al::updateNerveStateAndNextNerve(this, &NrvKoopaChaseBattleLv1ProvocationFirst)) {
        mStateProvocation->setClearInterpole(true);
        if (mStateDemoStart->isSkipped()) {
            al::tryKillEmitterAndParticleAll(mHost);
            if (al::isLessStep(this, 720)) {
                al::startBgm(mHost, "Koopa", -1, 0, -1, -1);
            }
        }
    }
}

/**
 * @brief Sets the run velocity from the distance to the player.
 *
 * Bowser keeps sliding with his current speed when no player is targeted, when the player is
 * out of the run start distance or when the curve gives no speed.
 * @return Whether the run speed is nearly zero.
 */
bool KoopaChaseBattleLv1::updateRunSpeed() {
    f32 speed = 0.0f;
    if (KoopaChaseFunction::getTargetPlayer(mHost) != nullptr) {
        f32 distance = al::calcDistance(mHost, KoopaChaseFunction::getTargetPlayer(mHost));
        if (distance < KoopaChase::getRunStartDistance()) {
            speed = calcRunSpeed(distance);
        }
    }

    if (speed == 0.0f) {
        speed = al::calcSpeedH(mHost) * 0.97f;
    }

    al::setVelocityToDirection(mHost, -al::getFront(mHost), speed);
    return al::isNearZero(speed);
}

namespace sead::hostio {
/**
 * @brief Evaluates the curve through the curve function table.
 * @param t Curve input.
 * @return Interpolated value.
 */
template <>
f32 Curve<f32>::interpolateToF32(f32 t) const {
    return sCurveFunctionTbl_f32[mInfo.curveType](t, &mInfo, mFloats);
}
}  // namespace sead::hostio

/** @brief Runs after the player, starting jumps, fire breath and throws. */
void KoopaChaseBattleLv1::exeRun() {
    bool isStopped = updateRunSpeed();
    if (isStopped && KoopaChaseFunction::getTargetPlayer(mHost) == nullptr) {
        al::setNerve(this, &NrvKoopaChaseBattleLv1Provocation);
        return;
    }

    if (tryStartJumpOrWarp()) {
        return;
    }

    if (mStateDamage->getDamageCount() == 2 &&
        KoopaChaseFunction::getTargetPlayer(mHost) != nullptr &&
        !KoopaChaseFunction::getKoopa(mHost)->isStateDamage()) {
        al::setNerve(this, &NrvKoopaChaseBattleLv1Fire);
        return;
    }

    tryStartThrow(isStopped);
}

/**
 * @brief Starts a jump or a warp when Bowser passes the matching chase point.
 * @return Whether a jump or warp was started.
 */
bool KoopaChaseBattleLv1::tryStartJumpOrWarp() {
    if (KoopaChaseFunction::isOverPointJump(mHost)) {
        sead::Vector3f pos;
        KoopaChaseFunction::calcCurrentPointPos(&pos, mHost);
        if (KoopaChaseFunction::isCurrentPointWarp(mHost)) {
            mStateJump->startWarp(pos);
        } else {
            mStateJump->startJump(pos);
        }

        al::setNerve(this, &NrvKoopaChaseBattleLv1Jump);
        return true;
    }

    if (KoopaChaseFunction::isOverPointWarp(mHost)) {
        sead::Vector3f pos;
        KoopaChaseFunction::calcCurrentPointPos(&pos, mHost);
        mStateWarp->start(pos);
        al::setNerve(this, &NrvKoopaChaseBattleLv1Warp);
        return true;
    }

    return false;
}

/**
 * @brief Starts breathing fire once Bowser has been hurt, while a player is targeted.
 * @return Whether the fire breath was started.
 */
bool KoopaChaseBattleLv1::tryStartFire() {
    if (mStateDamage->getDamageCount() < 1) {
        return false;
    }

    if (KoopaChaseFunction::getTargetPlayer(mHost) == nullptr ||
        KoopaChaseFunction::getKoopa(mHost)->isStateDamage()) {
        return false;
    }

    al::setNerve(this, &NrvKoopaChaseBattleLv1Fire);
    return true;
}

/**
 * @brief Starts a throw every twentieth chase move.
 * @param isProvokeOnFail Whether to provoke when the throw cannot start.
 * @return Whether the throw was started.
 */
NOINLINE bool KoopaChaseBattleLv1::tryStartThrow(bool isProvokeOnFail) {
    if (KoopaChaseFunction::getTargetPlayer(mHost) == nullptr ||
        KoopaChaseFunction::getKoopa(mHost)->isStateDamage()) {
        return false;
    }

    if (mHost->mValue168 % 20 != 0) {
        return false;
    }

    if (mStateThrow->tryStartThrow(mStateDamage->getDamageCount() > 0)) {
        al::setNerve(this, &NrvKoopaChaseBattleLv1Throw);
        return true;
    }

    if (isProvokeOnFail) {
        al::setNerve(this, &NrvKoopaChaseBattleLv1Provocation);
    }

    return false;
}

/** @brief Throws while running, then breathes fire or returns to running. */
void KoopaChaseBattleLv1::exeThrow() {
    updateRunSpeed();
    if (tryStartJumpOrWarp()) {
        return;
    }

    if (!al::updateNerveState(this)) {
        return;
    }

    if (tryStartFire()) {
        return;
    }

    KoopaChaseFunction::startActionWithKoopa(mHost, KoopaChaseFunction::getAnimNameRun(mHost),
                                             "Wait");
    al::setNerve(this, &NrvKoopaChaseBattleLv1Run);
}

/** @brief Breathes fire while running, then throws, provokes or returns to running. */
void KoopaChaseBattleLv1::exeFire() {
    bool isStopped = updateRunSpeed();
    if (tryStartJumpOrWarp()) {
        return;
    }

    if (!al::updateNerveState(this)) {
        return;
    }

    if (mStateDamage->getDamageCount() == 2 &&
        KoopaChaseFunction::getTargetPlayer(mHost) != nullptr) {
        if (mStateThrow->tryStartThrow(true)) {
            al::setNerve(this, &NrvKoopaChaseBattleLv1Throw);
            return;
        }

        if (!mStateThrow->canThrow() && isStopped) {
            al::setNerve(this, &NrvKoopaChaseBattleLv1Provocation);
            return;
        }
    }

    al::setNerve(this, &NrvKoopaChaseBattleLv1Run);
    KoopaChaseFunction::startActionWithKoopa(mHost, KoopaChaseFunction::getAnimNameRun(mHost),
                                             "Wait");
}

/** @brief Jumps to the next chase point, chaining further jumps and warps. */
void KoopaChaseBattleLv1::exeJump() {
    if (tryGoDemoBattleEnd()) {
        return;
    }

    if (!al::updateNerveState(this)) {
        return;
    }

    if (KoopaChaseFunction::isPreviousPointJump(mHost)) {
        sead::Vector3f pos;
        KoopaChaseFunction::calcCurrentPointPos(&pos, mHost);
        mStateJump->startJump(pos);
        al::setNerve(this, &NrvKoopaChaseBattleLv1Jump);
        return;
    }

    if (KoopaChaseFunction::isPreviousPointWarp(mHost)) {
        sead::Vector3f pos;
        KoopaChaseFunction::calcCurrentPointPos(&pos, mHost);
        mStateWarp->start(pos);
        al::setNerve(this, &NrvKoopaChaseBattleLv1Warp);
        return;
    }

    KoopaChaseFunction::startActionWithKoopa(mHost, KoopaChaseFunction::getAnimNameRun(mHost),
                                             "Wait");
    al::setNerve(this, &NrvKoopaChaseBattleLv1Run);
}

/** @brief Warps to the next chase point, then returns to running. */
void KoopaChaseBattleLv1::exeWarp() {
    updateRunSpeed();
    if (tryGoDemoBattleEnd()) {
        return;
    }

    if (al::updateNerveState(this)) {
        KoopaChaseFunction::startActionWithKoopa(mHost, KoopaChaseFunction::getAnimNameRun(mHost),
                                                 "Wait");
        al::setNerve(this, &NrvKoopaChaseBattleLv1Run);
    }
}

/** @brief Provokes the player while stopped, then returns to running. */
void KoopaChaseBattleLv1::exeProvocation() {
    if (tryStartJumpOrWarp()) {
        return;
    }

    bool isStopped = updateRunSpeed();
    mStateProvocation->tryEnd(isStopped);
    al::updateNerveStateAndNextNerve(this, &NrvKoopaChaseBattleLv1Run);
}

/** @brief Provokes the player right after the start demo, then starts running. */
void KoopaChaseBattleLv1::exeProvocationFirst() {
    if (al::isStep(this, 1)) {
        al::validateHitSensor(mHost, "ToadBlocker");
    }

    bool isStopped = updateRunSpeed();
    mStateProvocation->tryEnd(isStopped);
    al::updateNerveStateAndNextNerve(this, &NrvKoopaChaseBattleLv1Run);
}

/** @brief Plays the damage reaction and unlocks new attacks, then returns to running. */
void KoopaChaseBattleLv1::exeDamage() {
    updateRunSpeed();
    if (tryStartJumpOrWarp()) {
        return;
    }

    if (al::isFirstStep(this)) {
        if (mStateDamage->getDamageCount() == 1) {
            mStateProvocation->setFireEnabled(true);
        }

        if (mStateDamage->getDamageCount() >= 2) {
            mStateFire->setFireNum(2);
        }

        if (tryGoDemoBattleEnd()) {
            return;
        }
    }

    al::updateNerveStateAndNextNerve(this, &NrvKoopaChaseBattleLv1Run);
}

/** @brief Plays the defeat demo at the end position and finishes the battle. */
void KoopaChaseBattleLv1::exeDemoBattleEnd() {
    if (al::isFirstStep(this)) {
        al::invalidateHitSensor(mHost, "ToadBlocker");
        al::setTrans(mHost, mEndDemoTrans);
        al::updatePoseQuat(mHost, mEndDemoQuat);
        al::copyPose(KoopaChaseFunction::getKoopa(mHost), mHost);
        al::setTrans(KoopaChaseFunction::getKoopa(mHost), al::getTrans(mHost));
        al::tryOnStageSwitch(mHost, "SwitchEndDemoStartOn");
        al::stopBgm(mHost, "Koopa", -1, -1);
    }

    if (al::isStep(this, 806)) {
        al::startBgm(mHost, "AfterBattle", -1, 0, -1, -1);
    }

    if (!al::updateNerveState(this)) {
        return;
    }

    if (mEndDemoParts != nullptr && !al::isDead(mEndDemoParts)) {
        mEndDemoParts->kill();
    }

    mHost->stopEffectAll();
    if (al::isLessStep(this, 806)) {
        al::startBgm(mHost, "AfterBattle", -1, 0, -1, -1);
    }

    al::tryOnStageSwitch(mHost, "SwitchEndDemoEndOn");
    kill();
}
