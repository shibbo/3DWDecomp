#include "MapObj/KouraSurf.hpp"

#include <math/seadQuat.h>

#include "Library/Actor/ComboCounter.hpp"
#include "Library/Actor/ComboCounterWithSe.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Math/MatrixUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "MapObj/ActorStateRouteDokanMove.hpp"
#include "MapObj/ItemStatePlayerHold.hpp"
#include "MapObj/ItemStatePlayerHoldParam.hpp"
#include "MapObj/KouraSurfBindPuppeteer.hpp"
#include "Player/IUsePlayerInput.hpp"
#include "Player/Normal/PlayerActor.hpp"
#include "Player/Normal/PlayerInput.hpp"
#include "Player/Normal/WaterUtil.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/PlayerPuppetUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ScoreUtil.hpp"

namespace rc {
void emitEcho(const al::LiveActor* pActor, const sead::Vector3f& rTrans, f32 radius, s32 step,
              bool isForce);
}  // namespace rc

namespace {
NERVE_DECL(KouraSurf, Wait)
NERVE_DECL(KouraSurf, Hold)
NERVE_DECL(KouraSurf, RouteDokan)
NERVE_DECL(KouraSurf, KouraSlide)
NERVE_DECL(KouraSurf, KouraWait)
NERVE_DECL(KouraSurf, Upper)

/// Bind end after the rider was hit by another player: the rider gets dizzy.
class KouraSurfNrvBindEndDamage : public al::Nerve {
public:
    void execute(al::NerveKeeper* pKeeper) const override {
        pKeeper->getParent<KouraSurf>()->exeBindEnd();
    }
};

NERVE_DECL(KouraSurf, Release)
NERVE_DECL(KouraSurf, KouraJump)
NERVE_DECL(KouraSurf, WaitRelease)
NERVE_DECL(KouraSurf, KouraStop)
NERVE_DECL(KouraSurf, Blow)

/// Blow after the shell was stolen into a goal binder: scores when it disappears.
class KouraSurfNrvBlowGoal : public al::Nerve {
public:
    void execute(al::NerveKeeper* pKeeper) const override {
        pKeeper->getParent<KouraSurf>()->exeBlow();
    }
};

NERVE_DECL(KouraSurf, DemoKill)
NERVE_DECL(KouraSurf, RouteDokanEnd)
NERVE_DECL(KouraSurf, BindStart)

/// Jump started while falling (no extra hop velocity).
class KouraSurfNrvKouraJumpFall : public al::Nerve {
public:
    void execute(al::NerveKeeper* pKeeper) const override {
        pKeeper->getParent<KouraSurf>()->exeKouraJump();
    }
};

NERVE_DECL(KouraSurf, KouraJumpBreak)

/// Bind end after the shell stopped sliding: the rider gets dizzy.
class KouraSurfNrvBindEndStop : public al::Nerve {
public:
    void execute(al::NerveKeeper* pKeeper) const override {
        pKeeper->getParent<KouraSurf>()->exeBindEnd();
    }
};

/// Bind end requested by the rider: it exits the shell.
class KouraSurfNrvBindEndExit : public al::Nerve {
public:
    void execute(al::NerveKeeper* pKeeper) const override {
        pKeeper->getParent<KouraSurf>()->exeBindEnd();
    }
};

/// Bind end requested by the rider during a jump: it exits and stays in the air.
class KouraSurfNrvBindEndExitStay : public al::Nerve {
public:
    void execute(al::NerveKeeper* pKeeper) const override {
        pKeeper->getParent<KouraSurf>()->exeBindEnd();
    }
};

// Non-const nerve objects: the game keeps them in .data, merged into one block.
KouraSurfNrvWait NrvKouraSurfWait;
KouraSurfNrvHold NrvKouraSurfHold;
KouraSurfNrvRouteDokan NrvKouraSurfRouteDokan;
KouraSurfNrvKouraSlide NrvKouraSurfKouraSlide;
KouraSurfNrvKouraWait NrvKouraSurfKouraWait;
KouraSurfNrvUpper NrvKouraSurfUpper;
KouraSurfNrvBindEndDamage NrvKouraSurfBindEndDamage;
KouraSurfNrvRelease NrvKouraSurfRelease;
KouraSurfNrvKouraJump NrvKouraSurfKouraJump;
KouraSurfNrvWaitRelease NrvKouraSurfWaitRelease;
KouraSurfNrvKouraStop NrvKouraSurfKouraStop;
KouraSurfNrvBlow NrvKouraSurfBlow;
KouraSurfNrvBlowGoal NrvKouraSurfBlowGoal;
KouraSurfNrvDemoKill NrvKouraSurfDemoKill;
KouraSurfNrvRouteDokanEnd NrvKouraSurfRouteDokanEnd;
KouraSurfNrvBindStart NrvKouraSurfBindStart;
KouraSurfNrvKouraJumpFall NrvKouraSurfKouraJumpFall;
KouraSurfNrvKouraJumpBreak NrvKouraSurfKouraJumpBreak;
KouraSurfNrvBindEndStop NrvKouraSurfBindEndStop;
KouraSurfNrvBindEndExit NrvKouraSurfBindEndExit;
KouraSurfNrvBindEndExitStay NrvKouraSurfBindEndExitStay;

/// Constant parameters of the shell, constructed at startup.
struct KouraSurfParam {
    sead::Vector3f holdOffset;
    sead::Vector3f routeDokanSensorOffset;
    ItemStatePlayerHoldParam playerHoldParam;
};

KouraSurfParam sParam = {
    sead::Vector3f(0.0f, -25.0f, 50.0f),
    sead::Vector3f(0.0f, 0.0f, 0.0f),
    ItemStatePlayerHoldParam(
        sead::Vector3f(0.0f, -25.0f, 50.0f), sead::Vector3f(0.0f, -25.0f, 50.0f),
        sead::Vector3f(0.0f, -30.0f, 30.0f), sead::Vector3f(0.0f, -25.0f, 50.0f),
        sead::Vector3f(0.0f, -30.0f, 30.0f), sead::Vector3f(0.0f, -25.0f, 50.0f),
        sead::Vector3f(0.0f, -25.0f, 50.0f), sead::Vector3f(0.0f, -30.0f, 45.0f),
        sead::Vector3f(0.0f, -25.0f, 50.0f), sead::Vector3f(0.0f, -30.0f, 45.0f),
        sead::Vector3f(0.0f, 0.0f, 0.0f)),
};

/**
 * @brief Converts a water sink depth into a buoyancy rate.
 * @param depth Depth below the water surface.
 * @return 0 when barely sunk, 1 when deep under water.
 */
inline f32 calcWaterRate(f32 depth) {
    if (depth <= 5.0f) {
        return 0.0f;
    }

    if (depth >= 50.0f) {
        return 1.0f;
    }

    return depth / 45.0f;
}
}  // namespace

/**
 * @brief Creates the shell with its two bind puppeteers.
 * @param pName Actor name.
 */
KouraSurf::KouraSurf(const char* pName)
    : al::LiveActor(pName), mComboCounter(new al::ComboCounterWithSe(this)) {
    mBindPuppeteers = new KouraSurfBindPuppeteer*[2];
    mBindPuppeteers[0] = new KouraSurfBindPuppeteer(this);
    mBindPuppeteers[1] = new KouraSurfBindPuppeteer(this);
    mBindPuppeteer = mBindPuppeteers[0];
    mTimer = new KouraSurfTimer();
}

/**
 * @brief Initializes the model, the nerve states and the sensor/collision controllers.
 * @param rInfo Actor init info.
 */
void KouraSurf::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, getArchiveName(), nullptr);
    al::initNerve(this, &NrvKouraSurfWait, 2);
    mHoldState = new ItemStatePlayerHold(this, &sParam.playerHoldParam, false, true);
    mRouteDokanState = new ActorStateRouteDokanMove(this, rInfo);
    mRouteDokanState->setRouteSelecter(mBindPuppeteer->getRouteSelecter());
    al::initNerveState(this, mHoldState, &NrvKouraSurfHold, "[state]プレイヤーに持たれる");
    al::initNerveState(this, mRouteDokanState, &NrvKouraSurfRouteDokan, "[state]ルート土管移動");
    mHoldState->initColliderControl();
    mSensorController = al::createActorSensorController(this, "Body");
    mCollisionController = al::createActorCollisionController(this);
    al::setEffectFollowMtxPtr(this, "HitCollision", &mEffectMtx);
    makeActorAppeared();
}

/**
 * @brief Kills the shell, resetting it to wait.
 */
void KouraSurf::kill() {
    al::setNerve(this, &NrvKouraSurfWait);
    al::LiveActor::kill();
}

/**
 * @brief Handles route pipes, rider binding and attacks against touched sensors.
 * @param pSelf Own sensor.
 * @param pOther Touched sensor.
 */
void KouraSurf::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (isEnableRouteDokan() && mRouteDokanState->tryStart(pSelf, pOther)) {
        al::offCollide(this);
        al::setNerve(this, &NrvKouraSurfRouteDokan);
        return;
    }

    if (al::isSensorEye(pSelf)) {
        if (al::isSensorBindableAll(pOther) && mBindPuppeteer->isBind() &&
            al::sendMsgBindSteal(pOther, pSelf)) {
            mIsBindGoal = al::isSensorBindableGoal(pOther);
            rc::requestPlayerBind(mPlayerSensor, pOther);
        }

        return;
    }

    if (al::isNerve(this, &NrvKouraSurfRouteDokan)) {
        if (al::isSensorRide(pOther)) {
            rc::sendMsgRouteDokanPlayerTouch(pOther, pSelf, mRouteDokanState->getMoveDirection());
        }

        if (!al::isSensorPlayerOrPlayerWeapon(pOther)) {
            rc::sendMsgRouteDokanItemGet(pOther, pSelf);
            rc::sendMsgRouteDokanKouraAttack(pOther, pSelf);
        }
    } else if (isAttack()) {
        if (al::isSensorPlayerOrPlayerWeapon(pOther)) {
            if (mTimer->mInvalidAttackTime <= 0) {
                al::sendMsgEnemyAttack(pOther, pSelf);
            }
        } else {
            if (!isPlayerInside() && mTimer->mInvalidBlowTime <= 0 &&
                al::sendMsgKickKouraBlow(pOther, pSelf)) {
                sead::Vector3f pos =
                    (al::getSensorPos(pOther) + al::getSensorPos(pSelf)) * 0.5f;
                al::startHitReactionHitEffect(this, "命中", pos);
                startBlow(pSelf, pOther);
            }

            if (isPlayerInside() && rc::sendMsgPlayerCheckpointTouch(pOther, pSelf)) {
                return;
            }

            al::sendMsgKickKouraGetItem(pOther, pSelf);
            if (al::sendMsgKickKouraReflect(pOther, pSelf)) {
                sead::Vector3f dir = sead::Vector3f::zero;
                al::calcDirBetweenSensorsH(&dir, pOther, pSelf);
                if (al::isNearZero(dir, 0.001f)) {
                    sead::Vector3f velocity = al::getVelocity(this);
                    velocity.x = -velocity.x;
                    velocity.z = -velocity.z;
                    al::setVelocity(this, velocity);
                } else {
                    f32 dot = dir.dot(al::getVelocity(this));
                    if (dot < 0.0f) {
                        al::addVelocity(this, dir * dot * -2.0f);
                        setMoveDir(al::getVelocity(this));
                        sead::Vector3f pos = al::getTrans(this);
                        al::calcPosBetweenSensors(&pos, pOther, pSelf, 0.0f);
                        sead::Vector3f up = -al::getGravity(this);
                        al::makeMtxFrontUpPos(&mEffectMtx, dir, up, pos);
                        al::startHitReaction(this, "衝突");
                    }
                }
            }

            if (al::sendMsgKickKouraBreak(pOther, pSelf)) {
                startBreak();
                return;
            }

            if (al::isNerve(this, &NrvKouraSurfKouraSlide)) {
                if (al::sendMsgPlayerKouraAttack(pOther, pSelf, mComboCounter)) {
                    resetTimer();
                }
            } else {
                al::sendMsgKickKouraAttack(pOther, pSelf, mComboCounter);
            }
        }
    } else if (al::isSensorMapObj(pOther) && al::sendMsgKickKouraGetItem(pOther, pSelf)) {
        return;
    }

    if (al::isNerve(this, &NrvKouraSurfWait) && !al::isSensorPlayer(pOther)) {
        al::sendMsgPushAndKillVelocityToTarget(this, pSelf, pOther);
        return;
    }

    if (!al::isSensorEnemy(pOther)) {
        return;
    }

    if (al::isNerve(this, &NrvKouraSurfHold) &&
        al::sendMsgKickKouraAttack(pOther, pSelf, mComboCounter)) {
        al::resetActorCollisionController(mCollisionController, 1);
        startBlow(pSelf, pOther);
        return;
    }

    if (al::isNerve(this, &NrvKouraSurfKouraWait) && al::isSensorEnemy(pOther)) {
        sead::Vector3f dir;
        al::calcDirBetweenSensorsH(&dir, pOther, pSelf);
        startKouraSlide(dir);
        al::startHitReactionHitEffect(this, "敵キック", pOther, pSelf);
    }
}

/**
 * @brief Checks whether the shell may enter a route pipe.
 * @return Whether entering a route pipe is allowed.
 */
bool KouraSurf::isEnableRouteDokan() const {
    if (al::isNerve(this, &NrvKouraSurfHold)) {
        return false;
    }

    if (al::isNerve(this, &NrvKouraSurfBlow)) {
        return false;
    }

    if (al::isNerve(this, &NrvKouraSurfBlowGoal)) {
        return false;
    }

    if (al::isNerve(this, &NrvKouraSurfRouteDokan)) {
        return false;
    }

    if (al::isNerve(this, &NrvKouraSurfRouteDokanEnd)) {
        return false;
    }

    return mTimer->mInvalidRouteDokanTime < 1;
}

/**
 * @brief Checks whether the shell is moving fast enough to attack.
 * @return Whether the shell is sliding, released or jumping.
 */
bool KouraSurf::isAttack() const {
    return al::isNerve(this, &NrvKouraSurfRelease) || al::isNerve(this, &NrvKouraSurfKouraSlide) ||
           al::isNerve(this, &NrvKouraSurfKouraJump) ||
           al::isNerve(this, &NrvKouraSurfKouraJumpFall);
}

/**
 * @brief Checks whether a player rides inside the shell.
 * @return Whether a player is inside.
 */
bool KouraSurf::isPlayerInside() const {
    if (al::isNerve(this, &NrvKouraSurfBindStart)) {
        return true;
    }

    if (al::isNerve(this, &NrvKouraSurfKouraSlide)) {
        return true;
    }

    if (al::isNerve(this, &NrvKouraSurfKouraStop)) {
        return true;
    }

    if (al::isNerve(this, &NrvKouraSurfKouraWait)) {
        return true;
    }

    if (al::isNerve(this, &NrvKouraSurfKouraJump)) {
        return true;
    }

    if (al::isNerve(this, &NrvKouraSurfKouraJumpFall)) {
        return true;
    }

    if (al::isNerve(this, &NrvKouraSurfKouraJumpBreak)) {
        return true;
    }

    if (al::isNerve(this, &NrvKouraSurfBindEndStop)) {
        return true;
    }

    if (al::isNerve(this, &NrvKouraSurfBindEndDamage)) {
        return true;
    }

    if (al::isNerve(this, &NrvKouraSurfBindEndExit)) {
        return true;
    }

    if (al::isNerve(this, &NrvKouraSurfBindEndExitStay)) {
        return true;
    }

    if (al::isNerve(this, &NrvKouraSurfRouteDokan)) {
        return mBindPuppeteer->isBind();
    }

    return false;
}

/**
 * @brief Blows the shell away from the attacker.
 * @param pSelf Own sensor.
 * @param pOther Attacking sensor.
 */
void KouraSurf::startBlow(const al::HitSensor* pSelf, const al::HitSensor* pOther) {
    sead::Vector3f velocity;
    al::calcDirBetweenSensorsH(&velocity, pSelf, pOther);
    if (al::isNearZero(velocity, 0.001f)) {
        velocity.x = 0.0f;
        velocity.z = 5.3f;
    } else {
        velocity.x *= 5.3f;
        velocity.z *= 5.3f;
    }

    velocity.y = 27.5f;
    al::setVelocity(this, velocity);
    al::setNerve(this, &NrvKouraSurfBlow);
}

/**
 * @brief Sets the horizontal move direction.
 * @param rDir New direction (flattened and normalized).
 */
void KouraSurf::setMoveDir(const sead::Vector3f& rDir) {
    mMoveDir = rDir;
    mMoveDir.y = 0.0f;
    al::normalizeOrZero(&mMoveDir);
}

/**
 * @brief Breaks the shell, releasing any rider or holder.
 */
void KouraSurf::startBreak() {
    al::setVelocityZero(this);
    al::startHitReactionBreak(this);
    startKill();
}

/**
 * @brief Restarts the slide spin timer.
 */
void KouraSurf::resetTimer() {
    al::startHitReaction(this, "コウラ滑り再スタート");
    mRotateSpeed = 30.0f;
    mSlideTime = 350;
}

/**
 * @brief Starts sliding with a rider inside.
 * @param rDir Slide direction.
 */
void KouraSurf::startKouraSlide(const sead::Vector3f& rDir) {
    mMoveSpeed = 20.0f;
    sead::Vector3f velocity = rDir * 20.0f;
    if (mIsJumpBoost) {
        mIsJumpBoost = false;
        velocity.y = 12.0f;
    }

    al::setVelocity(this, velocity);
    setMoveDir(rDir);
    al::setNerve(this, &NrvKouraSurfKouraSlide);
}

/**
 * @brief Handles the messages received by the shell.
 * @param pMsg Received message.
 * @param pOther Sender sensor.
 * @param pSelf Own sensor.
 * @return Whether the message was handled.
 */
bool KouraSurf::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                           al::HitSensor* pSelf) {
    if (mBindPuppeteers[0]->isTargetSensor(pOther) &&
        mBindPuppeteers[0]->receiveMsg(pMsg, pOther, pSelf)) {
        return true;
    }

    if (mBindPuppeteers[1]->isTargetSensor(pOther) &&
        mBindPuppeteers[1]->receiveMsg(pMsg, pOther, pSelf)) {
        return true;
    }

    if (rc::isMsgAskControlUserId(pMsg, mPlayerSensor)) {
        return true;
    }

    if (al::isMsgPush(pMsg)) {
        if (!al::isSensorPlayer(pOther) && al::isNerve(this, &NrvKouraSurfWait)) {
            return al::tryReceiveMsgPushAndAddVelocityH(this, pMsg, pOther, pSelf, 2.0f);
        }

        return false;
    }

    if ((al::isMsgPlayerFireBallAttack(pMsg) || al::isMsgPlayerTailAttack(pMsg) ||
         al::isMsgPlayerBoomerangAttack(pMsg) || al::isMsgPlayerClimbAttack(pMsg) ||
         al::isMsgPlayerBodyAttack(pMsg) || al::isMsgPlayerGiantHipDrop(pMsg) ||
         al::isMsgPlayerCooperationHipDrop(pMsg) || al::isMsgPlayerSlidingAttack(pMsg) ||
         al::isMsgPlayerSpinAttack(pMsg) || al::isMsgBallAttack(pMsg) ||
         al::isMsgBallTrample(pMsg) || al::isMsgExplosion(pMsg)) &&
        mTimer->mInvalidTrampleTime <= 0) {
        if (isEnableKick() || al::isNerve(this, &NrvKouraSurfRelease) ||
            al::isNerve(this, &NrvKouraSurfKouraSlide) ||
            al::isNerve(this, &NrvKouraSurfKouraWait) ||
            (al::isNerve(this, &NrvKouraSurfUpper) && al::getVelocity(this).y < 0.0f)) {
            if (al::isNerve(this, &NrvKouraSurfKouraWait) ||
                al::isNerve(this, &NrvKouraSurfKouraSlide)) {
                if (mPlayerSensor == nullptr) {
                    return false;
                }

                if (al::getSensorHost(pOther) == al::getSensorHost(mPlayerSensor)) {
                    return false;
                }

                al::setNerve(this, &NrvKouraSurfBindEndDamage);
                return true;
            }

            rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
            if (mLiftingCount >= 7) {
                al::appearItemTiming(this, "リフティング");
                al::startHitReactionBreak(this);
                startKill();
                return true;
            }

            rc::addScoreByFactor(this, pOther, "敵", 100.0f, mLiftingCount);
            if (mLiftingCount >= 1) {
                al::startSeSetSeqLoacalVariableByName(
                    this, "BeatSequentially", 0, mLiftingCount < 7 ? mLiftingCount : 7);
            }

            mLiftingCount++;
            sead::Vector3f dir;
            if (al::isMsgPlayerTailAttack(pMsg) || al::isMsgPlayerClimbAttack(pMsg)) {
                dir = al::getSensorPos(pSelf) - al::getSensorPos(pOther);
            } else {
                bool isBodyAttack = al::isMsgPlayerBodyAttack(pMsg);
                const sead::Vector3f& velocity = al::getVelocity(al::getSensorHost(pOther));
                if (isBodyAttack) {
                    dir = -velocity;
                } else {
                    dir = velocity;
                }
            }

            dir.y = 0.0f;
            al::normalizeOrDirZ(&dir);
            al::setVelocity(this, dir * 4.0f);
            al::setNerve(this, &NrvKouraSurfUpper);
            return true;
        }
    }

    if (al::isMsgPlayerTrampleForCrossoverSensor(pMsg, pOther, pSelf) ||
        al::isMsgPlayerObjHipDropReflectAll(pMsg)) {
        if (isEnableKick()) {
            rc::addScoreComboByFactor(this, pOther, "敵", pMsg, 100.0f);
            if (!isPlayerInside()) {
                mPlayerSensor = pOther;
            }

            rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
            al::startHitReaction(this, "キック");
            startMove(rc::getPlayerFront(pOther), 20.0f, 20.0f);
            mTimer->mInvalidAttackTime = 8;
            mTimer->mInvalidTrampleTime = 8;
            return true;
        }

        if (!isEnableTrampleStop()) {
            return false;
        }

        rc::addScoreComboByFactor(this, pOther, "敵", pMsg, 100.0f);
        mComboCounter->reset();
        if (al::isNerve(this, &NrvKouraSurfRelease)) {
            al::setVelocityZero(this);
            al::setNerve(this, &NrvKouraSurfWait);
        } else if (al::isNerve(this, &NrvKouraSurfKouraWait) ||
                   al::isNerve(this, &NrvKouraSurfKouraSlide)) {
            if (mPlayerSensor == nullptr) {
                return false;
            }

            if (al::getSensorHost(pOther) == al::getSensorHost(mPlayerSensor)) {
                return false;
            }

            al::setNerve(this, &NrvKouraSurfBindEndDamage);
        }

        al::setVelocityZero(this);
        mTimer->mInvalidTrampleTime = 8;
        rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        al::startHitReaction(this, "停止");
        return true;
    }

    if (al::isMsgKickKouraReflect(pMsg)) {
        sead::Vector3f unused;
        al::calcDirBetweenSensorsH(&unused, pOther, pSelf);
        if (al::isNerve(this, &NrvKouraSurfRelease)) {
            sead::Vector3f dir;
            al::calcDirBetweenSensorsH(&dir, pOther, pSelf);
            startMove(dir, 20.0f, 20.0f);
            return true;
        }

        if (al::isNerve(this, &NrvKouraSurfKouraSlide)) {
            sead::Vector3f normal = al::getSensorPos(pSelf) - al::getSensorPos(pOther);
            normal.y = 0.0f;
            if (al::normalizeOrZero(&normal)) {
                normal = sead::Vector3f::ez;
            }

            sead::Vector3f velocity = al::getVelocity(this);
            al::calcReflectionVector(&velocity, normal, 1.0f, 0.0f);
            al::setVelocity(this, velocity);
            return true;
        }

        if (al::isNerve(this, &NrvKouraSurfWait) || al::isNerve(this, &NrvKouraSurfKouraWait)) {
            sead::Vector3f dir;
            al::calcDirBetweenSensorsH(&dir, pOther, pSelf);
            if (al::isNerve(this, &NrvKouraSurfKouraWait)) {
                startKouraSlide(dir);
            } else {
                startMove(dir, 20.0f, 20.0f);
            }

            return true;
        }

        if (al::isNerve(this, &NrvKouraSurfKouraJump)) {
            return true;
        }
    }

    if (al::tryReceiveMsgPushAndAddVelocity(this, pMsg, pOther, pSelf, 2.0f)) {
        return true;
    }

    if (isEnableHold() && mHoldState->tryStartCarryFront(pMsg, pOther, false)) {
        mPlayerSensor = pOther;
        mLiftingCount = 0;
        al::setNerve(this, &NrvKouraSurfHold);
        return true;
    }

    if (isHold()) {
        if (al::isMsgKickKouraBlow(pMsg)) {
            al::resetActorCollisionController(mCollisionController, 1);
            if (al::isNerve(this, &NrvKouraSurfHold)) {
                endHold();
                startBlow(pSelf, pOther);
                return true;
            }

            startBlow(pSelf, pOther);
            mBindPuppeteer->stopBind();
            return true;
        }

        if (al::isMsgPlayerRelease(pMsg)) {
            al::resetActorCollisionController(mCollisionController, 1);
            if (rc::isPlayerBinded(mPlayerSensor, pSelf)) {
                al::setNerve(this, &NrvKouraSurfWaitRelease);
            }

            if (al::isNerve(this, &NrvKouraSurfHold)) {
                mHoldState->receiveMsg(pMsg, pOther, pSelf);
                endHold();
                sead::Vector3f front = rc::getPlayerFront(mPlayerSensor);
                startMove(front, 20.0f, 20.0f);
                mTimer->mInvalidTrampleTime = 13;
                mTimer->mInvalidAttackTime = 20;
                return true;
            }

            startBlow(pSelf, pOther);
            mBindPuppeteer->stopBind();
            return true;
        }

        if (al::isMsgPlayerReleaseDamage(pMsg) || al::isMsgPlayerReleaseDead(pMsg) ||
            al::isMsgHoldCancel(pMsg) || al::isMsgWarpStart(pMsg)) {
            al::resetActorCollisionController(mCollisionController, 1);
            if (al::isNerve(this, &NrvKouraSurfHold)) {
                mHoldState->receiveMsg(pMsg, pOther, pSelf);
                startBlow(pSelf, pOther);
                return true;
            }

            startBlow(pSelf, pOther);
            mBindPuppeteer->stopBind();
            return true;
        }

        return false;
    }

    if (GameDataFunction::isSingleMode(this)) {
        if (al::isMsgKouraDestroy(pMsg)) {
            al::startHitReactionBreak(this);
            startKill();
            return true;
        }

        if (rc::isMsgJumpPanelAction(pMsg)) {
            al::setVelocityY(this, rc::isMsgJumpPanelActionAndSuperJump(pMsg) ? 65.0f : 45.0f);
            if (al::isNerve(this, &NrvKouraSurfKouraSlide) ||
                al::isNerve(this, &NrvKouraSurfRelease)) {
                al::setNerve(this, &NrvKouraSurfKouraStop);
            }

            return true;
        }
    }

    if (al::isNerve(this, &NrvKouraSurfBlow) || al::isNerve(this, &NrvKouraSurfBlowGoal)) {
        return false;
    }

    if (al::isMsgGoalKill(pMsg)) {
        rc::addScoreComboByFactor(this, pOther, "敵", pMsg, 100.0f);
        al::startHitReactionDeath(this);
        startKill();
        return true;
    }

    return false;
}

/**
 * @brief Checks whether the shell can be kicked.
 * @return Whether the shell waits and is not protected.
 */
bool KouraSurf::isEnableKick() const {
    return al::isNerve(this, &NrvKouraSurfWait) && mTimer->mInvalidTrampleTime < 1;
}

/**
 * @brief Checks whether the shell slides.
 * @return Whether the shell is released or sliding with a rider.
 */
bool KouraSurf::isSlide() const {
    return al::isNerve(this, &NrvKouraSurfRelease) || al::isNerve(this, &NrvKouraSurfKouraSlide);
}

/**
 * @brief Ends any bind and kills the shell.
 */
void KouraSurf::startKill() {
    if (mBindPuppeteers[0]->isBind()) {
        mBindPuppeteers[0]->endKouraBind();
    }

    if (mBindPuppeteers[1]->isBind()) {
        mBindPuppeteers[1]->endKouraBind();
    }

    if (al::isNerve(this, &NrvKouraSurfHold)) {
        rc::requestPlayerRelease(mPlayerSensor);
    }

    kill();
}

/**
 * @brief Releases the shell in a direction.
 * @param rDir Move direction.
 * @param speed Move speed (negative for the default).
 * @param rotateSpeed Spin speed in degrees per frame (negative for the default).
 */
void KouraSurf::startMove(const sead::Vector3f& rDir, f32 speed, f32 rotateSpeed) {
    f32 moveSpeed = speed < 0.0f ? 20.0f : speed;
    f32 spinSpeed = rotateSpeed < 0.0f ? 20.0f : rotateSpeed;
    sead::Vector3f dirH = rDir;
    dirH.y = 0.0f;
    al::normalizeOrDirZ(&dirH);
    mMoveSpeed = moveSpeed;
    al::setVelocity(this, moveSpeed * rDir);
    setMoveDir(rDir);
    mRotateSpeed = spinSpeed;
    al::setNerve(this, &NrvKouraSurfRelease);
}

/**
 * @brief Checks whether a trample stops the shell.
 * @return Whether the shell moves and is not protected.
 */
bool KouraSurf::isEnableTrampleStop() const {
    if (al::isNerve(this, &NrvKouraSurfRelease) || al::isNerve(this, &NrvKouraSurfKouraWait) ||
        al::isNerve(this, &NrvKouraSurfKouraSlide)) {
        return mTimer->mInvalidTrampleTime < 1;
    }

    return false;
}

/**
 * @brief Checks whether a player can pick the shell up.
 * @return Whether the shell can be held.
 */
bool KouraSurf::isEnableHold() const {
    if (al::isNerve(this, &NrvKouraSurfHold)) {
        return false;
    }

    if (mTimer->mInvalidHoldTime > 0) {
        return false;
    }

    if (al::isNerve(this, &NrvKouraSurfWait)) {
        return true;
    }

    if (al::isNerve(this, &NrvKouraSurfUpper)) {
        return al::getVelocity(this).y < 0.0f;
    }

    return false;
}

/**
 * @brief Checks whether a player holds the shell.
 * @return Whether the shell is held.
 */
bool KouraSurf::isHold() const {
    if ((al::isNerve(this, &NrvKouraSurfHold) || al::isNerve(this, &NrvKouraSurfBindStart)) &&
        mPlayerSensor != nullptr) {
        return rc::isPlayerHolding(mPlayerSensor, this);
    }

    return false;
}

/**
 * @brief Moves the shell back to the holder's position when it is dropped.
 */
void KouraSurf::endHold() {
    sead::Vector3f trans = al::getTrans(this);
    sead::Vector3f pos = al::getActorTrans(mPlayerSensor);
    pos.y = trans.y;
    al::resetPosition(this, pos, false);
    al::setVelocity(this, trans - pos);
    updateCollider();
    al::setVelocityZero(this);
}

/**
 * @brief Stops the shell when it is touched on the touch screen.
 * @param pMsg Received message.
 * @param pPointer Screen pointer.
 * @param pTarget Own screen point target.
 * @return Whether the message was handled.
 */
bool KouraSurf::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                      al::ScreenPointTarget* pTarget) {
    if (!al::isMsgTouchAssist(pMsg)) {
        return false;
    }

    if (al::isNerve(this, &NrvKouraSurfRelease)) {
        al::setVelocityZero(this);
        al::startSe(this, "PgTouchStopped", nullptr);
        al::setNerve(this, &NrvKouraSurfWait);
        return true;
    }

    if (al::isNerve(this, &NrvKouraSurfKouraWait) || al::isNerve(this, &NrvKouraSurfKouraSlide)) {
        al::setVelocityZero(this);
        if (al::isNerve(this, &NrvKouraSurfKouraSlide)) {
            al::startSe(this, "PgTouchStopped", nullptr);
        }

        al::setNerve(this, &NrvKouraSurfKouraWait);
        return true;
    }

    return false;
}

/**
 * @brief Appears the shell in its initial waiting state.
 */
void KouraSurf::appear() {
    al::LiveActor::appear();
    al::setNerve(this, &NrvKouraSurfWait);
    al::setVelocityZero(this);
    al::onCollide(this);
    mClippedFrame = 0;
    mRotateSpeed = 0.0f;
    mTimer->reset();
    mSlideTime = 0;
    mMoveSpeed = -1.0f;
    mIsOnGround = false;
    mIsInWater = false;
    mIsJumpBoost = false;
    mPlayerMoveState = KouraSurfPlayerMoveState::None;
}

/**
 * @brief Updates the timers, ground state, puppeteers and clipping every frame.
 */
void KouraSurf::control() {
    mTimer->update();
    mIsOnGround = al::isCollidedGround(this);
    if (mIsOnGround) {
        mGroundNormal = al::getOnGroundNormal(this, 0);
    }

    tryBreak();
    al::updateActorCollisionController(mCollisionController);
    mBindPuppeteers[0]->update();
    mBindPuppeteers[1]->update();
    if (isPlayerInside() || isHold() || mBindPuppeteers[0]->isBind() ||
        mBindPuppeteers[1]->isBind()) {
        al::invalidateClipping(this);
    } else {
        al::validateClipping(this);
    }

    if (!isHold() && GameDataFunction::isSingleMode(this)) {
        rc::emitEcho(this, al::getTrans(this), 170.0f, 120, false);
    }
}

/**
 * @brief Breaks the shell when it touches a deadly area or floor.
 * @return Whether the shell broke.
 */
bool KouraSurf::tryBreak() {
    if (rc::isInDeathArea(this) || rc::isCollidedDamageFire(this) ||
        rc::isCollidedPoison(this) || rc::isCollidedInkSlow(this)) {
        al::setVelocityZero(this);
        al::startHitReactionBreak(this);
        startKill();
        return true;
    }

    return false;
}

/**
 * @brief Updates the collider, following the holder while the shell is carried.
 */
void KouraSurf::updateCollider() {
    if (mHoldState->isDead()) {
        al::LiveActor::updateCollider();
        return;
    }

    mHoldState->updateCollider(al::getHitSensor(this, "Body"));
}

/**
 * @brief Kills the shell when its stage switch turns on.
 */
void KouraSurf::startKillBySwitch() {
    al::startHitReactionDeath(this);
    startKill();
}

/**
 * @brief Called by a puppeteer when its bind gets cancelled.
 * @param pPuppeteer Puppeteer whose bind was cancelled.
 * @param pSensor Sensor of the cancelled player.
 * @param isKill Whether the shell must disappear.
 */
void KouraSurf::receivedBindCancel(KouraSurfBindPuppeteer* pPuppeteer, al::HitSensor* pSensor,
                                   bool isKill) {
    if (isKill) {
        al::setVelocity(this, sead::Vector3f::zero);
        al::setNerve(this, &NrvKouraSurfDemoKill);
        return;
    }

    if (mBindPuppeteer != pPuppeteer) {
        return;
    }

    al::setVelocityZero(this);
    const sead::Vector3f& velocity = al::getVelocity(this);
    sead::Vector3f dir(-velocity.x, 0.0f, -velocity.z);
    al::normalizeOrDirZ(&dir);
    dir.x *= 5.3f;
    dir.z *= 5.3f;
    dir.y = 27.5f;
    al::setVelocity(this, dir);
    if (mIsBindGoal) {
        mIsBindGoal = false;
        al::setNerve(this, &NrvKouraSurfBlowGoal);
    } else {
        al::setNerve(this, &NrvKouraSurfBlow);
    }
}

/**
 * @brief Spins the shell around its up axis by the rotate speed.
 */
void KouraSurf::updateRotatePose() {
    sead::Quatf quat = al::getQuat(this);
    al::rotateQuatRadian(&quat, quat, sead::Vector3f::ey, sead::Mathf::deg2rad(mRotateSpeed));
    al::updatePoseQuat(this, quat);
}

/**
 * @brief Hides the shell unless a player rides inside.
 * @return Whether the shell was hidden.
 */
bool KouraSurf::hideActor() {
    if (mBindPuppeteer->isBind()) {
        return false;
    }

    return al::LiveActor::hideActor();
}

/**
 * @brief Places the hit effect where the shell collided with a wall or the ceiling.
 */
void KouraSurf::startEffectHitCollision() {
    sead::Vector3f normal;
    sead::Vector3f pos;
    if (al::isCollidedWall(this)) {
        normal = al::getCollidedWallNormal(this);
        pos = al::getCollidedWallPos(this);
    } else if (al::isCollidedCeiling(this)) {
        normal = al::getCollidedCeilingNormal(this);
        pos = al::getCollidedCeilingPos(this);
    } else {
        return;
    }

    mEffectMtx.makeQT(sead::Quatf(1.0f, 0.0f, 0.0f, 0.0f), pos);

    sead::Matrix34f rotateMtx;
    rotateMtx.makeIdentity();
    sead::Quatf rotate;
    if (rotate.makeVectorRotation(sead::Vector3f(0.0f, 1.0f, 0.0f), normal)) {
        rotateMtx.makeQT(rotate, sead::Vector3f(0.0f, 0.0f, 0.0f));
    }

    mEffectMtx = mEffectMtx * rotateMtx;
    al::startHitReaction(this, "衝突");
}

/**
 * @brief Applies gravity, water buoyancy and ground bounce while the shell is not sliding.
 * @param bounceRate Ground bounce rate (0 for no bounce).
 * @param scaleH Horizontal velocity scale.
 * @return Whether the shell landed.
 */
bool KouraSurf::doFall(f32 bounceRate, f32 scaleH) {
    f32 scaleV;
    f32 buoyancy;
    if (GameDataFunction::isSingleMode(this)) {
        if (!rc::isInWaterArea(this)) {
            mIsInWater = false;
            scaleV = 0.998f;
            buoyancy = 0.0f;
        } else {
            if (!mIsInWater) {
                mIsInWater = true;
                al::startHitReaction(this, "着水");
            }

            f32 height = WaterUtil::getOceanWaterHeight(this, al::getTrans(this), true);
            f32 depth = height - al::getTrans(this).y;
            if (depth <= 0.0f) {
                f32 rate = calcWaterRate(depth);
                buoyancy = (1.0f - rate) * 1.8f + rate * 2.6f;
                scaleV = 0.7f;
            } else {
                if (mIsInWater) {
                    al::setTransY(this, height);
                    return false;
                }

                scaleV = 0.998f;
                buoyancy = 0.0f;
            }
        }
    } else {
        f32 depth = rc::calcWaterSinkDepth(this);
        if (depth > 0.0f) {
            if (!mIsInWater) {
                mIsInWater = true;
                al::startHitReaction(this, "着水");
            }

            f32 rate = calcWaterRate(depth);
            buoyancy = (1.0f - rate) * 1.8f + rate * 2.6f;
            scaleV = 0.7f;
        } else {
            mIsInWater = false;
            scaleV = 0.998f;
            buoyancy = 0.0f;
        }
    }

    if (al::isCollidedCeilingVelocity(this) && al::getVelocity(this).y > 0.0f) {
        al::scaleVelocityHV(this, scaleH, 0.0f);
    } else if (!al::isCollidedGround(this)) {
        al::scaleVelocityHV(this, scaleH, scaleV);
    } else {
        if (al::getVelocity(this).y <= 0.0f) {
            sead::Vector3f velocity;
            al::verticalizeVec(&velocity, al::getOnGroundNormal(this, 0), al::getVelocity(this));
            al::setVelocity(this, velocity * scaleH);
            if (al::isOnGround(this, 0, 0.0f) &&
                al::getOnGroundNormal(this, 0).dot(al::getVelocity(this)) < -15.0f) {
                al::startHitReactionOnGround(this);
            }

            if (bounceRate <= 0.0f) {
                sead::Vector3f gravity = al::getOnGroundNormal(this, 0) * -1.8f;
                gravity.y += buoyancy;
                al::addVelocity(this, gravity);
            } else {
                al::setVelocityY(this, -(al::getVelocityPtr(this)->y * bounceRate));
            }

            return true;
        }

        return false;
    }

    al::addVelocityToGravity(this, 1.8f - buoyancy);
    return false;
}

/**
 * @brief Moves the sliding shell: reflects on walls and ceilings and keeps its speed.
 */
void KouraSurf::doMove() {
    bool isRelease = al::isNerve(this, &NrvKouraSurfRelease);
    f32 scaleParallel = isRelease ? 1.0f : 0.997f;
    al::getVelocity(this);
    f32 depth = rc::calcWaterSinkDepth(this);
    f32 buoyancy;
    f32 scaleV;
    if (depth > 0.0f) {
        if (!mIsInWater) {
            mIsInWater = true;
            al::startHitReaction(this, "着水");
        }

        f32 rate = calcWaterRate(depth);
        buoyancy = (1.0f - rate) * 1.8f + rate * 2.6f;
        scaleV = 0.7f;
    } else {
        scaleV = isRelease ? 1.0f : 0.995f;
        buoyancy = 0.0f;
        mIsInWater = false;
    }

    bool isCollided = false;
    if (al::isCollidedWallVelocity(this)) {
        al::HitSensor* wallSensor = al::tryGetCollidedWallSensor(this);
        bool isReflect = true;
        if (wallSensor != nullptr) {
            if (al::sendMsgKickKouraBreak(wallSensor, al::getHitSensor(this, "Body"))) {
                startBreak();
                return;
            }

            isReflect =
                !al::sendMsgKickKouraCollideNoReflect(wallSensor, al::getHitSensor(this, "Body"));
            al::sendMsgKickKouraAttackCollide(wallSensor, al::getHitSensor(this, "Body"),
                                              mComboCounter);
        }

        startEffectHitCollision();
        onHitWall();
        sead::Vector3f normal = al::getCollidedWallNormal(this);
        normal.y = 0.0f;
        bool isNormalZero = al::isNearZero(normal, 0.001f);
        if (isReflect && !isNormalZero) {
            al::normalize(&normal);
            sead::Vector3f velocity = al::getVelocity(this);
            al::calcReflectionVector(&velocity, normal, 1.0f, 0.0f);
            al::setVelocity(this, velocity);
        }

        isCollided = true;
    } else if (al::isCollidedCeilingVelocity(this)) {
        al::HitSensor* ceilingSensor = al::tryGetCollidedCeilingSensor(this);
        if (ceilingSensor != nullptr) {
            if (al::sendMsgKickKouraBreak(ceilingSensor, al::getHitSensor(this, "Body"))) {
                startBreak();
                return;
            }

            if (!al::sendMsgKickKouraCollideNoReflect(ceilingSensor,
                                                      al::getHitSensor(this, "Body"))) {
                al::sendMsgKickKouraAttackCollide(ceilingSensor, al::getHitSensor(this, "Body"),
                                                  mComboCounter);
            }
        }

        startEffectHitCollision();
        sead::Vector3f normal = al::getCollidedCeilingNormal(this);
        if (!al::isNearZero(normal, 0.001f)) {
            al::normalize(&normal);
            sead::Vector3f velocity = al::getVelocity(this);
            al::calcReflectionVector(&velocity, normal, 1.0f, 0.0f);
            al::setVelocity(this, velocity);
        }

        isCollided = true;
    }

    if (al::isCollidedGround(this)) {
        const sead::Vector3f& groundNormal = al::getOnGroundNormal(this, 0);
        sead::Vector3f velocity = al::getVelocity(this);
        if (al::calcReflectionVector(&velocity, groundNormal, 0.0f, 0.0f) &&
            al::isOnGround(this, 0, 0.0f) &&
            al::getOnGroundNormal(this, 0).dot(al::getVelocity(this)) < -15.0f) {
            al::startHitReactionOnGround(this);
        }

        al::setVelocity(this, velocity);
    }

    al::scaleVelocityHV(this, 0.997f, 0.995f);
    if (isCollided) {
        setMoveDir(al::getVelocity(this));
    }

    sead::Vector3f velocity = al::getVelocity(this);
    if (al::isNearZero(velocity, 0.001f)) {
        al::setVelocityZero(this);
    } else {
        sead::Vector3f velocityH = velocity;
        velocityH.y = 0.0f;
        sead::Vector3f parallel;
        al::parallelizeVec(&parallel, mMoveDir, velocityH);
        sead::Vector3f vertical;
        al::verticalizeVec(&vertical, mMoveDir, velocityH);
        f32 velocityY = scaleV * velocity.y;
        velocity = scaleParallel * parallel + vertical * 0.9f;
        velocity *= mMoveSpeed / velocity.length();
        velocity.y = velocityY;
        al::setVelocity(this, velocity);
    }

    if (al::isCollidedGround(this)) {
        sead::Vector3f gravity = al::getOnGroundNormal(this, 0) * -1.8f;
        gravity.y += buoyancy;
        al::addVelocity(this, gravity);
    } else {
        al::addVelocityToGravity(this, 1.8f - buoyancy);
    }
}

/**
 * @brief Checks whether the shell slides without a rider.
 * @return Whether the shell is released.
 */
bool KouraSurf::isSlideWithoutPlayer() const {
    return al::isNerve(this, &NrvKouraSurfRelease);
}

/**
 * @brief Checks whether the shell moves through a route pipe.
 * @return Whether the shell is in a route pipe.
 */
bool KouraSurf::isInRouteDokan() const {
    return al::isNerve(this, &NrvKouraSurfRouteDokan);
}

/**
 * @brief Waits on the ground, slowing down its spin.
 */
void KouraSurf::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "StayWait");
        mPlayerSensor = nullptr;
    }

    doFall(0.0f, 0.9f);
    if (mRotateSpeed > 0.0f) {
        updateRotatePose();
        if (al::isOnGround(this, 0, 0.0f) || mIsInWater) {
            mRotateSpeed *= 0.8f;
        }

        if (mRotateSpeed <= 0.0001f) {
            mRotateSpeed = 0.0f;
        }
    }
}

/**
 * @brief Hops up after being kicked from below or lifted.
 */
void KouraSurf::exeUpper() {
    bool isUpper = al::isNerve(this, &NrvKouraSurfUpper);
    if (al::isFirstStep(this)) {
        if (isUpper) {
            al::startSe(this, "Hop", nullptr);
        }

        al::startAction(this, "StayWait");
        mMoveSpeed = -1.0f;
        mTimer->mInvalidTrampleTime = 8;
        mRotateSpeed = 20.0f;
        if (isUpper) {
            al::addVelocity(this, sead::Vector3f(0.0f, 30.0f, 0.0f));
        } else {
            al::setVelocity(this, sead::Vector3f(0.0f, 25.0f, 0.0f));
        }

        return;
    }

    updateRotatePose();
    f32 velocityY = al::getVelocityPtr(this)->y;
    if (doFall(0.3f, isUpper ? 0.998f : 0.9f) || (velocityY < 0.0f && mIsInWater)) {
        mLiftingCount = 0;
        al::setNerve(this, &NrvKouraSurfWait);
    }
}

/**
 * @brief Is carried by a player; starts the ride when the player squats.
 */
void KouraSurf::exeHold() {
    if (al::isFirstStep(this)) {
        al::setColliderRadius(mCollisionController, 15.0f);
        al::setColliderOffsetY(mCollisionController, 80.0f);
        mComboCounter->reset();
        al::startAction(this, "HoldWait");
        mIsInWater = false;
        al::startSe(this, "Hold", nullptr);
        al::setVelocityZero(this);
    }

    al::updateNerveState(this);
    auto* player = static_cast<PlayerActor*>(al::getSensorHost(mPlayerSensor));
    if (!rc::isPlayerBinded(mPlayerSensor) && player->getInput()->isSquatTrigOn()) {
        if (rc::isPlayerOnGround(mPlayerSensor)) {
            mIsJumpBoost = true;
        } else {
            mIsJumpBoost = rc::isPlayerInWater(mPlayerSensor);
        }

        endHold();
        al::resetActorCollisionController(mCollisionController, 1);
        al::setNerve(this, &NrvKouraSurfBindStart);
        return;
    }

    mPlayerMoveState = KouraSurfPlayerMoveState::None;
    if (rc::isPlayerWait(al::getSensorHost(mPlayerSensor))) {
        mPlayerMoveState = KouraSurfPlayerMoveState::Wait;
    } else if (rc::isPlayerDash(al::getSensorHost(mPlayerSensor))) {
        mPlayerMoveState = KouraSurfPlayerMoveState::Dash;
    } else if (rc::isPlayerDashFast(al::getSensorHost(mPlayerSensor))) {
        mPlayerMoveState = KouraSurfPlayerMoveState::DashFast;
    }
}

/**
 * @brief Called when the shell gets held (nothing to do).
 */
void KouraSurf::startHold() {}

/**
 * @brief Follows the holder while the release is pending, then is released forward.
 */
void KouraSurf::exeWaitRelease() {
    if (al::isFirstStep(this)) {
        al::resetActorSensorController(mSensorController);
    }

    sead::Vector3f holdPos;
    rc::calcPlayerHoldPos(&holdPos, mPlayerSensor);
    sead::Vector3f front = rc::getPlayerFront(mPlayerSensor);
    front.y = 0.0f;
    al::normalizeOrDirZ(&front);
    sead::Quatf quat;
    al::makeQuatFrontUp(&quat, front, sead::Vector3f::ey);
    sead::Vector3f pos = sParam.holdOffset;
    pos.rotate(quat);
    pos = holdPos + pos;
    al::updatePoseQuat(this, quat);
    al::setTrans(this, pos);
    if (!rc::isPlayerBinded(mPlayerSensor)) {
        endHold();
        startMove(rc::getPlayerFront(mPlayerSensor), 20.0f, 20.0f);
    }
}

/**
 * @brief Slides without a rider until it stops or leaves the screen for too long.
 */
void KouraSurf::exeRelease() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "StayWait");
    }

    doMove();
    const char* actionName =
        mIsInWater ? "WaterMove" : (al::isCollidedGround(this) ? "GroundMove" : "AirMove");
    if (!al::isActionPlaying(this, actionName)) {
        al::startAction(this, actionName);
    }

    updateRotatePose();
    if (rc::tryFindAreaObj(this, rc::AreaObjType::FrameOutCtrlArea, al::getTrans(this)) ==
            nullptr &&
        al::isJudgedToClipFrustum(this, 100.0f, 300.0f)) {
        if (mClippedFrame++ >= 200) {
            makeActorDead();
            return;
        }
    } else {
        mClippedFrame = 0;
    }

    if (al::isOnGround(this, 0, 0.0f) && al::isVelocitySlow(this, 5.0f)) {
        al::setVelocityZero(this);
        al::setNerve(this, &NrvKouraSurfWait);
    }

    al::HitSensor* groundSensor = al::tryGetCollidedGroundSensor(this);
    if (groundSensor != nullptr) {
        if (GameDataFunction::isSingleMode(this)) {
            al::sendMsgKouraThrow(groundSensor, al::getHitSensor(this, "Body"));
        }

        if (mPlayerSensor != nullptr) {
            al::sendMsgPlayerFloorTouch(groundSensor, mPlayerSensor);
        } else {
            al::sendMsgEnemyFloorTouch(groundSensor, al::getHitSensor(this, "Body"));
        }
    }
}

/**
 * @brief Lets the player enter the shell, then starts sliding or jumping.
 */
void KouraSurf::exeBindStart() {
    if (al::isFirstStep(this)) {
        if (!mBindPuppeteer->startEnter(mPlayerSensor)) {
            al::setNerve(this, &NrvKouraSurfBlow);
            return;
        }

        rc::tryRequestClearFlingPoleDashFlag(al::getSensorHost(mPlayerSensor));
    }

    if (mBindPuppeteer->isBind()) {
        if (mPlayerMoveState == KouraSurfPlayerMoveState::Wait) {
            al::setNerve(this, &NrvKouraSurfKouraJump);
            return;
        }

        startKouraSlide(rc::getPlayerFront(mPlayerSensor));
    }
}

/**
 * @brief Slides with a rider inside, steered by the rider's stick.
 */
void KouraSurf::exeKouraSlide() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "StayWait");
        mRotateSpeed = 30.0f;
        mSlideTime = 350;
        al::startHitReaction(this, "コウラ滑り開始");
    }

    f32 rate = mSlideTime / -350.0f + 1.0f;
    if (rate < 0.0f) {
        rate = 0.0f;
    } else if (rate > 1.0f) {
        rate = 1.0f;
    }

    mRotateSpeed = ((1.0f - rate * (rate * rate)) * 0.5f + 0.5f) * 30.0f;
    doMove();
    const char* actionName =
        mIsInWater ? "WaterMove" : (al::isCollidedGround(this) ? "GroundMove" : "AirMove");
    if (!al::isActionPlaying(this, actionName)) {
        al::startAction(this, actionName);
    }

    updateRotatePose();
    sead::Vector3f dir = al::getVelocity(this);
    dir.y = 0.0f;
    if (!al::isNearZero(dir, 0.001f)) {
        al::normalize(&dir);
        sead::Vector3f stickH =
            rc::getPuppetStickWorldWithoutSnap(mBindPuppeteer->getPlayerPuppet());
        stickH.y = 0.0f;
        if (!al::isNearZero(stickH, 0.001f)) {
            al::normalize(&stickH);
            f32 cross = dir.z * stickH.x - stickH.z * dir.x;
            sead::Vector3f side(dir.z, 0.0f, -dir.x);
            if (cross < -0.5f) {
                al::addVelocity(this, side * -0.4f);
            } else if (cross > 0.5f) {
                al::addVelocity(this, side * 0.4f);
            }

            sead::Vector3f moveDir = al::getVelocity(this);
            al::normalizeOrDirZ(&moveDir);
            setMoveDir(moveDir);
        }
    }

    al::HitSensor* groundSensor = al::tryGetCollidedGroundSensor(this);
    if (groundSensor != nullptr) {
        al::sendMsgPlayerFloorTouch(groundSensor, mPlayerSensor);
    }

    const sead::Vector3f& newVelocity = al::getVelocity(this);
    if (al::isCollidedGround(this) &&
        newVelocity.x * newVelocity.x + newVelocity.z * newVelocity.z < 100.0f) {
        al::setNerve(this, &NrvKouraSurfBindEndStop);
        return;
    }

    doKouraInputProc();
}

/**
 * @brief Handles the rider's input: start sliding, jump or exit.
 */
void KouraSurf::doKouraInputProc() {
    if (!mBindPuppeteer->isBind()) {
        return;
    }

    if (al::isNerve(this, &NrvKouraSurfKouraWait) && al::isCollidedGround(this) &&
        rc::isPuppetStickOn(mBindPuppeteer->getPlayerPuppet())) {
        startKouraSlide(rc::getPuppetStickWorldWithoutSnap(mBindPuppeteer->getPlayerPuppet()));
        return;
    }

    if (rc::isPuppetTrigJumpButtonWithoutPrecedeInput(mBindPuppeteer->getPlayerPuppet())) {
        bool isEnableJump = al::isCollidedGround(this);
        if (!isEnableJump) {
            f32 depth = rc::calcWaterSinkDepth(this);
            isEnableJump = depth < 100.0f && depth >= 0.0f;
        }

        if (isEnableJump && !al::isNerve(this, &NrvKouraSurfKouraJump) &&
            !al::isNerve(this, &NrvKouraSurfKouraJumpBreak)) {
            if (al::isNerve(this, &NrvKouraSurfKouraWait)) {
                al::setNerve(this, &NrvKouraSurfKouraJump);
            } else {
                sead::Vector3f velocity = al::getVelocity(this);
                velocity.y = 25.0f;
                al::setVelocity(this, velocity);
                al::startSe(this, "Jump", nullptr);
            }

            if (mIsInWater) {
                al::startHitReaction(this, "水中ジャンプ");
            }
        }
    }

    if (rc::isPuppetTrigSquatButton(mBindPuppeteer->getPlayerPuppet()) && isPlayerInside()) {
        if (al::isNerve(this, &NrvKouraSurfKouraJump)) {
            al::setNerve(this, &NrvKouraSurfBindEndExitStay);
        } else {
            al::setNerve(this, &NrvKouraSurfBindEndExit);
        }
    }
}

/**
 * @brief Stops after a jump panel, then ends the bind.
 */
void KouraSurf::exeKouraStop() {
    doMove();
    if (al::isGreaterEqualStep(this, 0)) {
        al::setNerve(this, &NrvKouraSurfBindEndStop);
        return;
    }

    doKouraInputProc();
}

/**
 * @brief Waits with a rider inside.
 */
void KouraSurf::exeKouraWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "StayWait");
        mMoveSpeed = -1.0f;
        mTimer->mInvalidTrampleTime = 8;
        al::setVelocityZero(this);
    }

    doFall(0.0f, 0.0f);
    doKouraInputProc();
}

/**
 * @brief Jumps with a rider inside.
 */
void KouraSurf::exeKouraJump() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "StayWait");
        al::startSe(this, "Jump", nullptr);
        mMoveSpeed = -1.0f;
        mTimer->mInvalidTrampleTime = 8;
        mRotateSpeed = 20.0f;
        if (al::isNerve(this, &NrvKouraSurfKouraJump)) {
            sead::Vector3f velocity = al::getVelocity(this);
            velocity.y = 25.0f;
            al::setVelocity(this, velocity);
        }

        return;
    }

    updateRotatePose();
    f32 velocityY = al::getVelocityPtr(this)->y;
    bool isLanded = doFall(0.3f, 0.0f);
    doKouraInputProc();
    if (isLanded || (velocityY < 0.0f && mIsInWater)) {
        al::setNerve(this, &NrvKouraSurfKouraJumpBreak);
    }
}

/**
 * @brief Lands after a jump with a rider inside, slowing down the spin.
 */
void KouraSurf::exeKouraJumpBreak() {
    doFall(0.3f, 0.0f);
    doKouraInputProc();
    if (al::getNerveStep(this) <= 10) {
        f32 rate = al::getNerveStep(this) / 10.0f;
        f32 rotateSpeed = mRotateSpeed * (1.0f - rate) + rate * 5.0f;
        sead::Quatf quat = al::getQuat(this);
        al::rotateQuatRadian(&quat, quat, sead::Vector3f::ey, sead::Mathf::deg2rad(rotateSpeed));
        al::updatePoseQuat(this, quat);
    }

    if (al::isGreaterEqualStep(this, 15)) {
        al::setNerve(this, &NrvKouraSurfKouraWait);
    }
}

/**
 * @brief Ends the rider's bind and goes back to waiting.
 */
void KouraSurf::exeBindEnd() {
    if (!al::isFirstStep(this)) {
        return;
    }

    mComboCounter->reset();
    if (al::isNerve(this, &NrvKouraSurfBindEndExit)) {
        mBindPuppeteer->startExit();
    } else if (al::isNerve(this, &NrvKouraSurfBindEndExitStay)) {
        mBindPuppeteer->startExitStay();
    } else {
        mBindPuppeteer->startDizzy();
        if (mBindPuppeteer == mBindPuppeteers[1]) {
            mBindPuppeteer = mBindPuppeteers[0];
        } else {
            mBindPuppeteer = mBindPuppeteers[1];
        }

        mRouteDokanState->setRouteSelecter(mBindPuppeteer->getRouteSelecter());
        if (mBindPuppeteer->isBind()) {
            mBindPuppeteer->endBindForce();
        }
    }

    f32 rotateSpeed;
    if ((al::isNerve(this, &NrvKouraSurfBindEndExit) && al::isCollidedGround(this)) ||
        mIsInWater) {
        mTimer->mInvalidTrampleTime = 40;
        al::setVelocity(this, sead::Vector3f(0.0f, 15.0f, 0.0f));
        rotateSpeed = 20.0f;
    } else {
        mTimer->mInvalidTrampleTime = 8;
        al::setVelocityZero(this);
        rotateSpeed = 0.0f;
    }

    mRotateSpeed = rotateSpeed;
    mTimer->mInvalidHoldTime = 10;
    al::setNerve(this, &NrvKouraSurfWait);
}

/**
 * @brief Flies away after being blown, then disappears.
 */
void KouraSurf::exeBlow() {
    if (al::isFirstStep(this)) {
        mIsInWater = false;
        al::startAction(this, "Blow");
        if (mPlayerSensor != nullptr) {
            rc::requestPlayerRelease(mPlayerSensor);
        }
    }

    al::calcSpeed(this);
    sead::Quatf quat = al::getQuat(this);
    al::rotateQuatRadian(&quat, quat, sead::Vector3f::ez, 0.0f);
    al::setQuat(this, quat);
    al::scaleVelocityHV(this, 0.9f, 0.998f);
    if (!al::isCollidedGround(this)) {
        al::addVelocityToGravity(this, 1.1f);
    }

    if (al::isGreaterEqualStep(this, 30) && al::isAlive(this)) {
        al::startHitReactionDisappear(this);
        if (al::isNerve(this, &NrvKouraSurfBlowGoal)) {
            rc::addScoreByFactor(this, mPlayerSensor, "成功", 100.0f, 0);
        }

        startKill();
    }
}

/**
 * @brief Waits for the bind to end, then kills the shell.
 */
void KouraSurf::exeWaitBindEndForKill() {
    if (mBindPuppeteer->isBind()) {
        return;
    }

    startKill();
}

/**
 * @brief Moves through a route pipe, then leaves it sliding, jumping or waiting.
 */
void KouraSurf::exeRouteDokan() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "RouteDokanMove");
        mRouteDokanState->setMoveSpeed(20.0f);
        mRouteDokanState->setEndSpeed(20.0f);
        mRotateSpeed = 30.0f;
        mSlideTime = 350;
        mUp = sead::Vector3f::ey;
        al::setSensorRadius(mSensorController, 40.0f);
        al::setSensorFollowPosOffset(mSensorController, sParam.routeDokanSensorOffset);
    }

    if (!al::updateNerveState(this)) {
        updateRotatePose();
        return;
    }

    al::resetActorSensorController(mSensorController);
    al::onCollide(this);
    al::setNerve(this, &NrvKouraSurfRouteDokanEnd);
    sead::Vector3f front = mRouteDokanState->getMoveDirection();
    front.y = 0.0f;
    al::normalizeOrDirZ(&front);
    sead::Quatf quat;
    al::makeQuatFrontUp(&quat, front, sead::Vector3f::ey);
    al::rotateQuatRadian(&quat, quat, sead::Vector3f::ey, sead::Mathf::deg2rad(mRotateDegree));
    al::updatePoseQuat(this, quat);
    al::startSe(this, "RouteDokanOut", nullptr);
    mTimer->mInvalidRouteDokanTime = 8;
    if (al::isNearZero(al::getVelocity(this).y, 0.001f)) {
        if (mBindPuppeteer->isBind()) {
            startKouraSlide(mRouteDokanState->getMoveDirection());
        } else {
            startMove(mRouteDokanState->getMoveDirection(), 20.0f, 20.0f);
        }
    } else if (mBindPuppeteer->isBind()) {
        if (al::getVelocity(this).y < 0.0f) {
            al::setNerve(this, &NrvKouraSurfKouraJumpFall);
        } else {
            al::setNerve(this, &NrvKouraSurfKouraJump);
        }
    } else {
        al::setNerve(this, &NrvKouraSurfWait);
    }
}

/**
 * @brief Leaves the route pipe.
 */
void KouraSurf::exeRouteDokanEnd() {
    al::isFirstStep(this);
    doMove();
    al::setNerve(this, &NrvKouraSurfWait);
}

/**
 * @brief Disappears during a demo.
 */
void KouraSurf::exeDemoKill() {
    al::tryKillEmitterAndParticleAll(this);
    startKill();
}

/**
 * @brief Gets the name of the shell's model archive.
 * @return The archive name.
 */
const char* KouraSurf::getArchiveName() const {
    return "Koura";
}
