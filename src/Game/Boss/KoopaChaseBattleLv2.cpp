#include "Boss/KoopaChaseBattleLv2.hpp"

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
#include "Util/PlayerUtil.hpp"

namespace {
NERVE_DECL(KoopaChaseBattleLv2, DemoBattleStart)
NERVE_DECL(KoopaChaseBattleLv2, DemoBattleEnd)
NERVE_DECL(KoopaChaseBattleLv2, Damage)
NERVE_DECL(KoopaChaseBattleLv2, Throw)
NERVE_DECL(KoopaChaseBattleLv2, Fire)
NERVE_DECL(KoopaChaseBattleLv2, Jump)
NERVE_DECL(KoopaChaseBattleLv2, Warp)
NERVE_DECL(KoopaChaseBattleLv2, Provocation)
NERVE_DECL(KoopaChaseBattleLv2, ProvocationFirst)
NERVE_DECL(KoopaChaseBattleLv2, Run)
NERVE_DECL(KoopaChaseBattleLv2, PrepareDemoBattleEnd)
NERVE_DECL(KoopaChaseBattleLv2, DemoBattleWaitEnd)
// Mutable nerve objects; the compiler merges them with the run speed curve data below.
KoopaChaseBattleLv2NrvDemoBattleStart NrvKoopaChaseBattleLv2DemoBattleStart;
KoopaChaseBattleLv2NrvDemoBattleEnd NrvKoopaChaseBattleLv2DemoBattleEnd;
KoopaChaseBattleLv2NrvDamage NrvKoopaChaseBattleLv2Damage;
KoopaChaseBattleLv2NrvThrow NrvKoopaChaseBattleLv2Throw;
KoopaChaseBattleLv2NrvFire NrvKoopaChaseBattleLv2Fire;
KoopaChaseBattleLv2NrvJump NrvKoopaChaseBattleLv2Jump;
KoopaChaseBattleLv2NrvWarp NrvKoopaChaseBattleLv2Warp;
KoopaChaseBattleLv2NrvProvocation NrvKoopaChaseBattleLv2Provocation;
KoopaChaseBattleLv2NrvProvocationFirst NrvKoopaChaseBattleLv2ProvocationFirst;
KoopaChaseBattleLv2NrvRun NrvKoopaChaseBattleLv2Run;
NERVES_MAKE_NOSTRUCT(KoopaChaseBattleLv2, PrepareDemoBattleEnd, DemoBattleWaitEnd)

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
KoopaChaseBattleLv2::KoopaChaseBattleLv2(KoopaChase* pHost, const al::ActorInitInfo& rInfo)
    : KoopaChaseBattle("クッパチェイスLv2"), mHost(pHost) {
    initNerve(&NrvKoopaChaseBattleLv2DemoBattleStart, 16);
    mCameraInfo = al::initAnimCamera(mHost, rInfo);

    mDemoInfoStart = new KoopaChaseDemoInfo(mCameraInfo, mHost, "DemoBattleChaseStart", 0, 0, 60,
                                            nullptr);
    mDemoInfoStart->mName28 = "SwitchStartDemoActiveOn";
    mStateDemoStart =
        new KoopaChaseStateDemo("クッパチェイス開始デモ", mHost, rInfo, mDemoInfoStart);
    mStateDemoStart->entryDemoActor(mHost);
    mStateDemoStart->entryDemoActor(KoopaChaseFunction::getKoopa(mHost));
    al::initNerveState(this, mStateDemoStart, &NrvKoopaChaseBattleLv2DemoBattleStart,
                       "デモ[バトル開始]");

    mDemoInfoEnd = new KoopaChaseDemoInfo(mCameraInfo, mHost, "DemoBattleChaseEndLv2", 0, 0, 60,
                                          nullptr);
    mDemoInfoEnd->mName28 = "SwitchEndDemoActiveOn";
    mDemoInfoEnd->mValue20 = -1;
    mDemoInfoEnd->mFlag30 = true;
    mStateDemoEnd =
        new KoopaChaseStateDemo("クッパチェイスやられデモ", mHost, rInfo, mDemoInfoEnd);
    mStateDemoEnd->entryDemoActor(mHost);
    mStateDemoEnd->entryDemoActor(KoopaChaseFunction::getKoopa(mHost));
    al::initNerveState(this, mStateDemoEnd, &NrvKoopaChaseBattleLv2DemoBattleEnd,
                       "デモ[バトル終了]");

    al::tryGetLinksTrans(&mEndDemoTrans, rInfo, "EndDemoPos");
    al::tryGetLinksQuat(&mEndDemoQuat, rInfo, "EndDemoPos");
    mEndDemoParts = KoopaChaseFunction::tryCreateLinkObj(rInfo, "EndDemoParts", 0);
    if (mEndDemoParts != nullptr) {
        mStateDemoEnd->entryDemoActor(mEndDemoParts);
    }

    mStateDamage = new KoopaChaseStateDamage(mHost);
    al::initNerveState(this, mStateDamage, &NrvKoopaChaseBattleLv2Damage, "State[ダメージ]");

    mStateThrow = new KoopaChaseStateThrow(mHost, rInfo,
                                           calcRunSpeed(KoopaChase::getRunStopDistance() + 500.0f));
    al::initNerveState(this, mStateThrow, &NrvKoopaChaseBattleLv2Throw, "State[投げる]");

    mStateFire = new KoopaChaseStateFire(mHost, rInfo, false);
    mStateFire->setFireNum(1);
    al::initNerveState(this, mStateFire, &NrvKoopaChaseBattleLv2Fire, "State[火を吐く]");

    mStateJump = new KoopaChaseStateJump(mHost, rInfo);
    al::initNerveState(this, mStateJump, &NrvKoopaChaseBattleLv2Jump, "State[ジャンプ]");

    mStateWarp = new KoopaChaseStateWarp(mHost, rInfo);
    al::initNerveState(this, mStateWarp, &NrvKoopaChaseBattleLv2Warp, "State[ワープ]");

    mStateProvocation = new KoopaChaseStateProvocation(mHost, rInfo);
    mStateProvocation->setFireEnabled(true);
    al::initNerveState(this, mStateProvocation, &NrvKoopaChaseBattleLv2Provocation,
                       "State[挑発]");
    al::initNerveState(this, mStateProvocation, &NrvKoopaChaseBattleLv2ProvocationFirst,
                       "State[挑発初回]");
}

/**
 * @brief Forwards attacks to the damage state outside of the demos.
 * @param pMsg Received sensor message.
 * @param pOther Sensor that sent the message.
 * @param pSelf Sensor that received the message.
 * @return Whether the message was handled.
 */
bool KoopaChaseBattleLv2::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                                     al::HitSensor* pSelf) {
    if (al::isNerve(this, &NrvKoopaChaseBattleLv2DemoBattleStart) ||
        al::isNerve(this, &NrvKoopaChaseBattleLv2DemoBattleEnd) ||
        al::isNerve(this, &NrvKoopaChaseBattleLv2Damage)) {
        return false;
    }

    bool isJumpOrWarp = al::isNerve(this, &NrvKoopaChaseBattleLv2Jump) ||
                        al::isNerve(this, &NrvKoopaChaseBattleLv2Warp);
    bool isDamage = false;
    bool isReceived =
        mStateDamage->receiveMsg(&isDamage, pMsg, pOther, pSelf, isJumpOrWarp);
    if (!isDamage) {
        return isReceived;
    }

    if (!tryGoDemoBattleEnd() && !isJumpOrWarp) {
        al::setNerve(this, &NrvKoopaChaseBattleLv2Damage);
    }

    return true;
}

/**
 * @brief Starts the battle end demo once the damage state has finished.
 * @return Whether the end demo was started.
 */
bool KoopaChaseBattleLv2::tryGoDemoBattleEnd() {
    if (!mStateDamage->isDamageFinish()) {
        return false;
    }

    al::setVelocityZero(mHost);
    al::setVelocityZero(KoopaChaseFunction::getKoopa(mHost));
    sead::Vector3f front;
    al::calcQuatFront(&front, mEndDemoQuat);
    mStateDemoEnd->setReturnTrans(mEndDemoTrans);
    al::requestCaptureScreenCover(mHost, 11);
    al::setNerve(this, &NrvKoopaChaseBattleLv2PrepareDemoBattleEnd);
    return true;
}

/** @brief Does nothing; the battle is driven by its nerves. */
void KoopaChaseBattleLv2::control() {}

/**
 * @brief Checks whether Bowser should turn towards the player.
 * @return Whether turning is allowed.
 */
bool KoopaChaseBattleLv2::isStateTurnToTarget() const {
    if (!al::isNerve(this, &NrvKoopaChaseBattleLv2Damage) &&
        !al::isNerve(this, &NrvKoopaChaseBattleLv2DemoBattleStart) &&
        !al::isNerve(this, &NrvKoopaChaseBattleLv2DemoBattleEnd)) {
        return true;
    }

    return false;
}

/**
 * @brief Checks whether Bowser's pose should follow the chase.
 * @return Whether the pose is updated.
 */
bool KoopaChaseBattleLv2::isStateUpdatePose() const {
    if (al::isNerve(this, &NrvKoopaChaseBattleLv2DemoBattleStart)) {
        return false;
    }

    if (al::isNerve(this, &NrvKoopaChaseBattleLv2DemoBattleEnd)) {
        return false;
    }

    if (al::isNerve(this, &NrvKoopaChaseBattleLv2ProvocationFirst)) {
        if (al::isFirstStep(this)) {
            return false;
        }

        if (al::isNewNerve(this)) {
            return false;
        }
    }

    return !al::isNerve(this, &NrvKoopaChaseBattleLv2Jump);
}

/**
 * @brief Checks whether the chase move counter advances.
 * @return Whether Bowser is running.
 */
bool KoopaChaseBattleLv2::isStateUpdateMoveCount() const {
    return al::isNerve(this, &NrvKoopaChaseBattleLv2Run);
}

/**
 * @brief Checks whether the tire move is active.
 * @return Whether no demo is playing.
 */
bool KoopaChaseBattleLv2::isStateTireMove() const {
    if (al::isNerve(this, &NrvKoopaChaseBattleLv2DemoBattleStart)) {
        return false;
    }

    return !al::isNerve(this, &NrvKoopaChaseBattleLv2DemoBattleEnd);
}

/**
 * @brief Checks whether Bowser is warping.
 * @return Whether the warp nerve is active.
 */
bool KoopaChaseBattleLv2::isStateWarp() const {
    return al::isNerve(this, &NrvKoopaChaseBattleLv2Warp);
}

/** @brief Plays the battle start demo and starts the battle music. */
void KoopaChaseBattleLv2::exeDemoBattleStart() {
    if (al::isFirstStep(this)) {
        al::invalidateHitSensor(mHost, "ToadBlocker");
    }

    if (al::isStep(this, 720)) {
        al::startBgm(mHost, "Koopa", -1, 0, -1, -1);
    }

    if (al::updateNerveStateAndNextNerve(this, &NrvKoopaChaseBattleLv2ProvocationFirst)) {
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
 * @return Whether the run speed is nearly zero.
 */
bool KoopaChaseBattleLv2::updateRunSpeed() {
    f32 speed;
    if (KoopaChaseFunction::getTargetPlayer(mHost) != nullptr) {
        speed = calcRunSpeed(
            al::calcDistance(mHost, KoopaChaseFunction::getTargetPlayer(mHost)));
    } else {
        speed = al::calcSpeedH(mHost) * 0.97f;
    }

    al::setVelocityToDirection(mHost, -al::getFront(mHost), speed);
    return al::isNearZero(speed);
}

/** @brief Runs after the player, starting jumps, fire breath and throws. */
void KoopaChaseBattleLv2::exeRun() {
    if (updateRunSpeed()) {
        al::setNerve(this, &NrvKoopaChaseBattleLv2Provocation);
        return;
    }

    if (tryStartJumpOrWarp()) {
        return;
    }

    if (tryStartFire()) {
        return;
    }

    tryStartThrow(false);
}

/**
 * @brief Starts a jump or a warp when Bowser passes the matching chase point.
 * @return Whether a jump or warp was started.
 */
bool KoopaChaseBattleLv2::tryStartJumpOrWarp() {
    if (KoopaChaseFunction::isOverPointJump(mHost)) {
        sead::Vector3f pos;
        KoopaChaseFunction::calcCurrentPointPos(&pos, mHost);
        if (KoopaChaseFunction::isCurrentPointWarp(mHost)) {
            mStateJump->startWarp(pos);
        } else {
            mStateJump->startJump(pos);
        }

        al::setNerve(this, &NrvKoopaChaseBattleLv2Jump);
        return true;
    }

    if (KoopaChaseFunction::isOverPointWarp(mHost)) {
        sead::Vector3f pos;
        KoopaChaseFunction::calcCurrentPointPos(&pos, mHost);
        mStateWarp->start(pos);
        al::setNerve(this, &NrvKoopaChaseBattleLv2Warp);
        return true;
    }

    return false;
}

/**
 * @brief Starts breathing fire while a player is targeted and Bowser is not hurt.
 * @return Whether the fire breath was started.
 */
bool KoopaChaseBattleLv2::tryStartFire() {
    if (KoopaChaseFunction::getTargetPlayer(mHost) == nullptr ||
        KoopaChaseFunction::getKoopa(mHost)->isStateDamage()) {
        return false;
    }

    al::setNerve(this, &NrvKoopaChaseBattleLv2Fire);
    return true;
}

/**
 * @brief Starts a throw every twentieth chase move.
 * @param isProvokeOnFail Whether to provoke when the throw cannot start.
 * @return Whether the throw was started.
 */
NOINLINE bool KoopaChaseBattleLv2::tryStartThrow(bool isProvokeOnFail) {
    if (KoopaChaseFunction::getTargetPlayer(mHost) == nullptr ||
        KoopaChaseFunction::getKoopa(mHost)->isStateDamage()) {
        return false;
    }

    if (mHost->mValue168 % 20 != 0) {
        return false;
    }

    if (mStateThrow->tryStartThrow(true)) {
        al::setNerve(this, &NrvKoopaChaseBattleLv2Throw);
        return true;
    }

    if (isProvokeOnFail) {
        al::setNerve(this, &NrvKoopaChaseBattleLv2Provocation);
    }

    return false;
}

/** @brief Throws while running, then breathes fire or returns to running. */
void KoopaChaseBattleLv2::exeThrow() {
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
    al::setNerve(this, &NrvKoopaChaseBattleLv2Run);
}

/** @brief Breathes fire while running, then throws or returns to running. */
void KoopaChaseBattleLv2::exeFire() {
    updateRunSpeed();
    if (tryStartJumpOrWarp()) {
        return;
    }

    if (!al::updateNerveState(this)) {
        return;
    }

    if (KoopaChaseFunction::getTargetPlayer(mHost) != nullptr &&
        mStateThrow->tryStartThrow(true)) {
        al::setNerve(this, &NrvKoopaChaseBattleLv2Throw);
        return;
    }

    al::setNerve(this, &NrvKoopaChaseBattleLv2Run);
    KoopaChaseFunction::startActionWithKoopa(mHost, KoopaChaseFunction::getAnimNameRun(mHost),
                                             "Wait");
}

/** @brief Jumps to the next chase point, chaining further jumps and warps. */
void KoopaChaseBattleLv2::exeJump() {
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
        al::setNerve(this, &NrvKoopaChaseBattleLv2Jump);
        return;
    }

    if (KoopaChaseFunction::isPreviousPointWarp(mHost)) {
        sead::Vector3f pos;
        KoopaChaseFunction::calcCurrentPointPos(&pos, mHost);
        mStateWarp->start(pos);
        al::setNerve(this, &NrvKoopaChaseBattleLv2Warp);
        return;
    }

    KoopaChaseFunction::startActionWithKoopa(mHost, KoopaChaseFunction::getAnimNameRun(mHost),
                                             "Wait");
    al::setNerve(this, &NrvKoopaChaseBattleLv2Run);
}

/** @brief Warps to the next chase point, then returns to running. */
void KoopaChaseBattleLv2::exeWarp() {
    updateRunSpeed();
    if (tryGoDemoBattleEnd()) {
        return;
    }

    if (al::updateNerveState(this)) {
        KoopaChaseFunction::startActionWithKoopa(mHost, KoopaChaseFunction::getAnimNameRun(mHost),
                                                 "Wait");
        al::setNerve(this, &NrvKoopaChaseBattleLv2Run);
    }
}

/** @brief Provokes the player while stopped, then returns to running. */
void KoopaChaseBattleLv2::exeProvocation() {
    if (tryStartJumpOrWarp()) {
        return;
    }

    bool isStopped = updateRunSpeed();
    mStateProvocation->tryEnd(isStopped);
    al::updateNerveStateAndNextNerve(this, &NrvKoopaChaseBattleLv2Run);
}

/** @brief Provokes the player right after the start demo, then starts running. */
void KoopaChaseBattleLv2::exeProvocationFirst() {
    if (al::isStep(this, 1)) {
        al::validateHitSensor(mHost, "ToadBlocker");
    }

    bool isStopped = updateRunSpeed();
    mStateProvocation->tryEnd(isStopped);
    al::updateNerveStateAndNextNerve(this, &NrvKoopaChaseBattleLv2Run);
}

/** @brief Plays the damage reaction and raises the fire count, then returns to running. */
void KoopaChaseBattleLv2::exeDamage() {
    updateRunSpeed();
    if (tryStartJumpOrWarp()) {
        return;
    }

    if (al::isFirstStep(this)) {
        mStateFire->setFireNum(mStateDamage->getDamageCount() + 1);
        if (tryGoDemoBattleEnd()) {
            return;
        }
    }

    al::updateNerveStateAndNextNerve(this, &NrvKoopaChaseBattleLv2Run);
}

/** @brief Moves on to the battle end demo. */
void KoopaChaseBattleLv2::exePrepareDemoBattleEnd() {
    al::setNerve(this, &NrvKoopaChaseBattleLv2DemoBattleEnd);
}

/** @brief Plays the defeat demo at the end position and finishes the battle. */
void KoopaChaseBattleLv2::exeDemoBattleEnd() {
    if (al::isFirstStep(this)) {
        al::invalidateHitSensor(mHost, "ToadBlocker");
        al::setTrans(mHost, mEndDemoTrans);
        al::updatePoseQuat(mHost, mEndDemoQuat);
        al::copyPose(KoopaChaseFunction::getKoopa(mHost), mHost);
        al::setTrans(KoopaChaseFunction::getKoopa(mHost), al::getTrans(mHost));
        al::tryOnStageSwitch(mHost, "SwitchEndDemoStartOn");
        al::stopBgm(mHost, "Koopa", -1, -1);
    }

    if (al::isStep(this, 1044)) {
        al::startBgm(mHost, "AfterBattle", -1, 0, -1, -1);
    }

    if (!al::updateNerveState(this)) {
        return;
    }

    if (mEndDemoParts != nullptr && !al::isDead(mEndDemoParts)) {
        mEndDemoParts->kill();
    }

    mHost->stopEffectAll();
    if (al::isLessStep(this, 1044)) {
        al::startBgm(mHost, "AfterBattle", -1, 0, -1, -1);
    }

    al::tryOnStageSwitch(mHost, "SwitchEndDemoEndOn");
    al::startBgm(mHost, "AfterBattle", -1, 0, -1, -1);
    al::requestCancelInterpole(mHost);
    if (mStateDemoEnd->isSkipped()) {
        al::setNerve(this, &NrvKoopaChaseBattleLv2DemoBattleWaitEnd);
        return;
    }

    kill();
}

/** @brief Keeps the screen covered after a skipped end demo, then finishes the battle. */
void KoopaChaseBattleLv2::exeDemoBattleWaitEnd() {
    al::requestCancelInterpole(mHost);
    if (al::isStep(this, 60)) {
        kill();
        return;
    }

    rc::invalidatePlayerInput(mHost, 2);
    al::requestCaptureScreenCover(mHost, 2);
}
