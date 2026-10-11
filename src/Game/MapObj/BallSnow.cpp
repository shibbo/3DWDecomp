#include "MapObj/BallSnow.hpp"

#include <math/seadQuat.h>

#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Actor/ComboCounter.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorModelUtil.hpp"
#include "Library/LiveActor/Util/ActorMovementUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Enemy/EnemyStateUtil.hpp"
#include "MapObj/BallStateFall.hpp"
#include "MapObj/BallStateFallParam.hpp"
#include "MapObj/BallStateFunction.hpp"
#include "MapObj/BallStateRolling.hpp"
#include "MapObj/BallStateThrow.hpp"
#include "MapObj/BallStateThrowParam.hpp"
#include "MapObj/DrcAssistDirectorUtil.hpp"
#include "MapObj/ItemStatePlayerHold.hpp"
#include "MapObj/ItemStatePlayerHoldParam.hpp"
#include "MapObj/TouchCarryItemState.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/DrcUtil.hpp"
#include "Util/InkUtil.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"

namespace {
NERVE_DECL(BallSnow, Wait)
NERVE_DECL(BallSnow, PlayerHold)
NERVE_DECL(BallSnow, Fall)
NERVE_DECL(BallSnow, Rolling)
NERVE_DECL(BallSnow, Throw)
NERVE_DECL(BallSnow, Kick)
NERVE_DECL(BallSnow, DamageThrow)
NERVE_DECL(BallSnow, DrcHold)
NERVE_DECL(BallSnow, Hide)

// Non-const nerve objects: the game keeps them in .data, merged into one block.
BallSnowNrvWait NrvBallSnowWait;
BallSnowNrvPlayerHold NrvBallSnowPlayerHold;
BallSnowNrvFall NrvBallSnowFall;
BallSnowNrvRolling NrvBallSnowRolling;
BallSnowNrvThrow NrvBallSnowThrow;
BallSnowNrvKick NrvBallSnowKick;
BallSnowNrvDamageThrow NrvBallSnowDamageThrow;
BallSnowNrvDrcHold NrvBallSnowDrcHold;
BallSnowNrvHide NrvBallSnowHide;

ItemStatePlayerHoldParam sPlayerHoldParam(
    sead::Vector3f(10.0f, 0.0f, 55.0f), sead::Vector3f(10.0f, 0.0f, 50.0f),
    sead::Vector3f(5.0f, 0.0f, 35.0f), sead::Vector3f(20.0f, 0.0f, 60.0f),
    sead::Vector3f(5.0f, 0.0f, 40.0f), sead::Vector3f(15.0f, 0.0f, 60.0f),
    sead::Vector3f(15.0f, 0.0f, 60.0f), sead::Vector3f(10.0f, 0.0f, 47.5f),
    sead::Vector3f(15.0f, 0.0f, 60.0f), sead::Vector3f(10.0f, 0.0f, 47.5f),
    sead::Vector3f(0.0f, 0.0f, 30.0f));
BallStateFallParam sFallParam(25.0f, 30.0f, 0.5f, 0.4f, 0.1f, 9.0f, 10.0f, 2.0f, 0.65f, 0.995f,
                              0.8f, 0.85f);
BallStateRollingParam sRollingParam(5.0f, 0.85f, 2.5f, 8);
BallStateThrowParam sThrowParam(30.0f, 6.0f, 0.45f, 2.0f, 0.5f, 0.995f, 0.65f, 10.0f, 45, false,
                                false);
BallStateThrowParam sKickParam(22.0f, 6.0f, 0.45f, 0.45f, 0.5f, 0.995f, 0.65f, 10.0f, -1, true,
                               false);
BallStateThrowParam sDamageThrowParam(18.0f, 4.0f, 0.45f, 0.45f, 0.5f, 0.995f, 0.65f, 10.0f, -1,
                                      true, true);
BallStateThrowParam sHitParam(10.0f, 8.0f, 1.0f, 1.0f, 0.5f, 0.996f, 0.65f, 8.0f, -1, false, true);
TouchCarryItemStateParam sTouchCarryParam(18.0f, 10.0f, 20.0f, 10.0f, 15.0f, 50.0f);
}  // namespace

/**
 * @brief Constructs the snow ball.
 * @param pName Actor name.
 */
BallSnow::BallSnow(const char* pName) : al::LiveActor(pName) {}

/**
 * @brief Initializes the model, the nerve states and the effect matrix.
 * @param rInfo Placement info of the actor.
 */
void BallSnow::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "BallSnow", nullptr);
    al::initNerve(this, &NrvBallSnowWait, 7);

    mStatePlayerHold = new ItemStatePlayerHold(this, &sPlayerHoldParam, false, false);
    mStateFall = new BallStateFall(this, &sFallParam);
    mStateRolling = new BallStateRolling(this, &sRollingParam);
    mStateThrow = new BallStateThrow(this, &sThrowParam);
    mStateTouchCarry = new TouchCarryItemState(this, &sTouchCarryParam);

    al::initNerveState(this, mStatePlayerHold, &NrvBallSnowPlayerHold,
                       "[state]プレイヤーに持たれる");
    al::initNerveState(this, mStateFall, &NrvBallSnowFall, "[state]落下");
    al::initNerveState(this, mStateRolling, &NrvBallSnowRolling, "[state]転がる");
    al::initNerveState(this, mStateThrow, &NrvBallSnowThrow, "[state]投げ");
    al::initNerveState(this, mStateThrow, &NrvBallSnowKick, "[state]蹴り");
    al::initNerveState(this, mStateThrow, &NrvBallSnowDamageThrow, "[state]ダメージ時の投げ");
    al::initNerveState(this, mStateTouchCarry, &NrvBallSnowDrcHold, "[state]アイテム持ち運び");
    mStatePlayerHold->initColliderControl();
    mColliderRadius = al::getColliderRadius(this);
    makeActorAppeared();

    al::tryGetArg(&mIsRebirth, rInfo, "IsRebirth");
    mRebirthTrans.set(al::getTrans(this));
    al::setEffectFollowMtxPtr(this, "HitCollision", &mEffectMtx);
}

/**
 * @brief Attacks enemies, map objects, goal items and players that the ball touches.
 * @param pSelf Sensor of the ball.
 * @param pOther Sensor that was touched.
 */
void BallSnow::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isNerve(this, &NrvBallSnowDamageThrow) || al::isNerve(this, &NrvBallSnowHide)) {
        return;
    }

    if (al::isSensorName(pSelf, "Body")) {
        if (al::isNerve(this, &NrvBallSnowPlayerHold)) {
            if (!al::sendMsgBallAttackHold(pOther, pSelf)) {
                return;
            }

            rc::requestPlayerRelease(mHolderSensor);
            BallStateFunction::calcReflectSpeed(this, al::getSensorHost(pSelf), &sFallParam);
            mStateThrow->setThrowParam(mHolderSensor, &sDamageThrowParam, nullptr);
            al::setNerve(this, &NrvBallSnowDamageThrow);
            return;
        }

        if (al::isNerve(this, &NrvBallSnowDrcHold)) {
            if (!al::isGreaterEqualStep(this, 25)) {
                return;
            }

            if (!al::sendMsgBallAttackDRCHold(pOther, pSelf)) {
                return;
            }

            rc::releaseTouchPointerHoldItem(this, mTouchPointer);
            BallStateFunction::calcReflectSpeed(this, al::getSensorHost(pOther), &sFallParam);
            mStateThrow->setThrowParam(mHolderSensor, &sDamageThrowParam, mTouchPointer);
            al::setNerve(this, &NrvBallSnowDamageThrow);
            al::startSeWithParam(this, "PgBound", al::getVelocity(this).length());
            return;
        }

        if (al::isSensorPlayer(pOther)) {
            if (isEnablePlayerKnockDown(pOther, pSelf) && al::sendMsgKnockDown(pOther, pSelf)) {
                sead::Vector3f dir = al::getVelocity(this);
                al::verticalizeVec(&dir, sead::Vector3f::ey, dir);
                al::normalizeOrZero(&dir);
                al::setVelocity(this, dir.x * -5.0f, 15.0f, dir.z * -5.0f);
                al::setNerve(this, &NrvBallSnowFall);
                mHoldDisableTimer = 12;
                mKnockDownDisableTimer = 12;
                return;
            }

            al::sendMsgPush(pOther, pSelf);
            return;
        }
    }

    if (al::isSensorName(pSelf, "Hold") && al::isSensorMapObj(pOther)) {
        if ((al::isNerve(this, &NrvBallSnowThrow) || al::isNerve(this, &NrvBallSnowKick)) &&
            al::sendMsgBallAttack(pOther, pSelf, mComboCounter)) {
            BallStateFunction::calcReflectSpeed(this, al::getSensorHost(pOther), &sFallParam);
            al::setNerve(this, &NrvBallSnowFall);
            mIsBreakOnLand = true;
            return;
        }

        if (!al::isNerve(this, &NrvBallSnowWait) && al::sendMsgBallItemGet(pOther, pSelf)) {
            return;
        }

        if (!al::isNerve(this, &NrvBallSnowPlayerHold) && !al::isNerve(this, &NrvBallSnowDrcHold)) {
            al::sendMsgPush(pOther, pSelf);
        }
    }

    if (al::isSensorEnemy(pOther)) {
        if (al::isSensorName(pSelf, "Attack")) {
            if ((al::isNerve(this, &NrvBallSnowThrow) || al::isNerve(this, &NrvBallSnowKick)) &&
                al::sendMsgBallAttack(pOther, pSelf, mComboCounter)) {
                BallStateFunction::calcReflectSpeed(this, al::getSensorHost(pOther), &sFallParam);
                al::setNerve(this, &NrvBallSnowFall);
                mIsBreakOnLand = true;
                return;
            }
        } else if (al::isSensorName(pSelf, "Body")) {
            if (al::isNerve(this, &NrvBallSnowFall) && al::getVelocity(this).y < -10.0f &&
                al::sendMsgBallTrample(pOther, pSelf, mComboCounter)) {
                BallStateFunction::calcReflectSpeed(this, al::getSensorHost(pOther), &sFallParam);
                mIsBreakOnLand = true;
                return;
            }

            if (!al::isNerve(this, &NrvBallSnowPlayerHold) &&
                !al::isNerve(this, &NrvBallSnowDrcHold)) {
                al::sendMsgPush(pOther, pSelf);
            }
        }
    } else if (al::isSensorGoalItem(pOther) &&
               al::sendMsgBallAttack(pOther, pSelf, mComboCounter)) {
        if (al::isNerve(this, &NrvBallSnowPlayerHold) || al::isNerve(this, &NrvBallSnowDrcHold)) {
            if (mHolderSensor != nullptr) {
                rc::requestPlayerRelease(mHolderSensor);
            }
        }

        BallStateFunction::calcReflectSpeed(this, al::getSensorHost(pOther), &sFallParam);
        al::setNerve(this, &NrvBallSnowFall);
        mIsBreakOnLand = true;
        return;
    }

    if (!GameDataFunction::isSingleMode(GameDataHolderAccessor(this))) {
        return;
    }

    if (al::isSensorNpc(pOther) || al::isSensorRide(pOther) || al::isSensorKoopaJr(pOther)) {
        if (al::isSensorName(pSelf, "Attack")) {
            if ((al::isNerve(this, &NrvBallSnowThrow) || al::isNerve(this, &NrvBallSnowKick)) &&
                al::sendMsgBallAttack(pOther, pSelf, mComboCounter)) {
                BallStateFunction::calcReflectSpeed(this, al::getSensorHost(pOther), &sFallParam);
                al::setNerve(this, &NrvBallSnowFall);
                mIsBreakOnLand = true;
            }

            return;
        }

        if (!al::isSensorName(pSelf, "Body")) {
            return;
        }

        if (al::isNerve(this, &NrvBallSnowFall) && al::getVelocity(this).y < -10.0f &&
            al::sendMsgBallTrample(pOther, pSelf, mComboCounter)) {
            BallStateFunction::calcReflectSpeed(this, al::getSensorHost(pOther), &sFallParam);
            mIsBreakOnLand = true;
            return;
        }

        if (al::isSensorRide(pOther)) {
            return;
        }

        if (!al::isNerve(this, &NrvBallSnowPlayerHold) && !al::isNerve(this, &NrvBallSnowDrcHold)) {
            al::sendMsgPush(pOther, pSelf);
        }

        return;
    }

    if (!al::isSensorKickKoura(pOther) || al::isNerve(this, &NrvBallSnowPlayerHold) ||
        al::isNerve(this, &NrvBallSnowDrcHold) || !al::isSensorName(pSelf, "Body")) {
        return;
    }

    if (al::sendMsgBallAttack(pOther, pSelf, mComboCounter)) {
        BallStateFunction::calcReflectSpeed(this, al::getSensorHost(pOther), &sFallParam);
        al::setNerve(this, &NrvBallSnowFall);
        mIsBreakOnLand = true;
    }
}

/**
 * @brief Whether the ball flying into a player knocks that player down.
 * @param pPlayer Sensor of the player.
 * @param pSelf Sensor of the ball.
 * @return True if the ball moves towards the player.
 */
bool BallSnow::isEnablePlayerKnockDown(al::HitSensor* pPlayer, al::HitSensor* pSelf) {
    if (!al::isNerve(this, &NrvBallSnowThrow) && !al::isNerve(this, &NrvBallSnowKick) &&
        !al::isNerve(this, &NrvBallSnowFall)) {
        return false;
    }

    if (mHolderSensor == nullptr) {
        return false;
    }

    if (al::getSensorHost(pPlayer) == al::getSensorHost(mHolderSensor) &&
        mKnockDownDisableTimer > 0) {
        return false;
    }

    if (al::isLessStep(this, 1) || mWallCollideCount > 1) {
        return false;
    }

    sead::Vector3f velocityDir = al::getVelocity(this);
    al::verticalizeVec(&velocityDir, sead::Vector3f::ey, velocityDir);
    al::normalizeOrZero(&velocityDir);
    if (al::isNearZero(velocityDir, 0.001f)) {
        return false;
    }

    sead::Vector3f dir = rc::getPlayerFront(pPlayer);
    al::calcDirBetweenSensorsH(&dir, pPlayer, pSelf);
    return !(dir.dot(velocityDir) >= 0.0f);
}

/**
 * @brief Reacts to attacks, pushes and the player picking the ball up, kicking or throwing it.
 * @param pMsg Received message.
 * @param pOther Sensor that sent the message.
 * @param pSelf Sensor of the ball.
 * @return True if the message was handled.
 */
bool BallSnow::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                          al::HitSensor* pSelf) {
    if (rc::isMsgAskControlUserId(pMsg, mHolderSensor)) {
        return true;
    }

    if (al::isNerve(this, &NrvBallSnowDamageThrow) || al::isNerve(this, &NrvBallSnowHide)) {
        return false;
    }

    if (al::isSensorName(pSelf, "Body")) {
        if (!al::isNerve(this, &NrvBallSnowPlayerHold) && !al::isNerve(this, &NrvBallSnowDrcHold) &&
            (al::isMsgPlayerObjHipDropAll(pMsg) || al::isMsgKickKouraAttack(pMsg) ||
             al::isMsgPlayerKouraAttack(pMsg) || al::isMsgPlayerFireBallAttack(pMsg) ||
             al::isMsgPlayerBoomerangAttack(pMsg))) {
            rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
            kill();
            return true;
        }

        if (GameDataFunction::isSingleMode(GameDataHolderAccessor(this)) &&
            !al::isNerve(this, &NrvBallSnowPlayerHold) &&
            !al::isNerve(this, &NrvBallSnowDrcHold)) {
            if (al::isMsgDisasterSpikeAttack(pMsg) || al::isMsgLaserAttack(pMsg) ||
                al::isMsgGigaEnemyAttack(pMsg)) {
                al::startHitReactionDeath(this);
                kill();
                return true;
            }

            if (al::isMsgBowserPush(pMsg) ||
                (al::isMsgEnemyAttack(pMsg) && al::isSensorHostName(pOther, "ブンブン"))) {
                kill();
                return true;
            }
        }

        if ((al::isNerve(this, &NrvBallSnowWait) || al::isNerve(this, &NrvBallSnowRolling)) &&
            (al::isMsgBallAttack(pMsg) || al::isMsgBallTrample(pMsg) ||
             al::isMsgPlayerBodyLanding(pMsg) || al::isMsgPlayerCooperationHipDrop(pMsg) ||
             al::isMsgPlayerSlidingAttack(pMsg) || al::isMsgExplosion(pMsg) ||
             al::isMsgBlockUpperPunch(pMsg) || al::isMsgPlayerGiantHipDrop(pMsg) ||
             rc::isMsgBobsledBodyAttack(pMsg))) {
            if (mHitDisableTimer > 0) {
                return false;
            }

            mHitDisableTimer = 20;
            sead::Vector3f dir = {0.0f, 0.0f, 0.0f};
            al::calcDirBetweenSensorsH(&dir, pOther, pSelf);
            if (al::isNearZero(dir, 0.001f)) {
                if (al::isSensorPlayer(pOther)) {
                    dir.set(rc::getPlayerFront(pOther));
                } else {
                    dir.set(sead::Vector3f::ey);
                }
            }

            al::setVelocity(this, dir.x * 8.0f, 15.0f, dir.z * 8.0f);
            al::startSeWithParam(this, "PgBound", al::getVelocity(this).length());
            mHolderSensor = BallStateFunction::tryGetRelativePlayerSensor(this, pOther);
            mStateThrow->setThrowParam(mHolderSensor, &sHitParam, nullptr);
            al::setNerve(this, &NrvBallSnowThrow);
            mIsRotateOnFall = false;
            rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
            return false;
        }

        if (!al::isNerve(this, &NrvBallSnowPlayerHold) && !al::isNerve(this, &NrvBallSnowDrcHold)) {
            bool isSingleMode = GameDataFunction::isSingleMode(GameDataHolderAccessor(this));
            f32 pushRate = isSingleMode ? 4.0f : 1.0f;
            if (al::tryReceiveMsgPushAndAddVelocity(this, pMsg, pOther, pSelf, pushRate)) {
                return true;
            }
        }
    }

    if (al::isNerve(this, &NrvBallSnowDrcHold)) {
        if (al::isMsgWarpStart(pMsg)) {
            rc::releaseTouchPointerHoldItem(this, mTouchPointer);
            al::setNerve(this, &NrvBallSnowFall);
            mIsRotateOnFall = true;
        }

        return false;
    }

    if (!al::isSensorName(pSelf, "Hold")) {
        return false;
    }

    if (al::isMsgPlayerHideItem(pMsg)) {
        al::hideModelIfShow(this);
    } else if (al::isMsgPlayerShowItem(pMsg)) {
        al::showModelIfHide(this);
    }

    if (al::isNerve(this, &NrvBallSnowPlayerHold)) {
        if (!mStatePlayerHold->receiveMsg(pMsg, pOther, pSelf)) {
            return false;
        }

        if (al::isMsgPlayerRelease(pMsg)) {
            al::setNerve(this, &NrvBallSnowThrow);
            mStateThrow->setThrowParam(mHolderSensor, &sThrowParam, nullptr);
            mIsRotateOnFall = false;
            return true;
        }

        if (al::isMsgPlayerReleaseDamage(pMsg) || al::isMsgPlayerReleaseDead(pMsg)) {
            al::setNerve(this, &NrvBallSnowDamageThrow);
            mStateThrow->setThrowParam(mHolderSensor, &sDamageThrowParam, nullptr);
            return true;
        }

        if (al::isMsgWarpStart(pMsg) || al::isMsgHoldCancel(pMsg)) {
            al::setNerve(this, &NrvBallSnowFall);
            mIsRotateOnFall = true;
            mHoldDisableTimer = 12;
            mKnockDownDisableTimer = 12;
        }

        return true;
    }

    if (isEnableHold(pOther) &&
        (al::isMsgPlayerTailAttack(pMsg) || al::isMsgPlayerClimbAttack(pMsg) ||
         al::isMsgPlayerSpinAttack(pMsg) || al::isMsgPlayerGiantAttack(pMsg) ||
         al::isMsgPlayerObjRollingAttack(pMsg))) {
        mHolderSensor = pOther;
        mIsRotateOnFall = false;
        rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        mStateThrow->setThrowParam(mHolderSensor, &sThrowParam, nullptr);
        al::setNerve(this, &NrvBallSnowThrow);
        return true;
    }

    if (isEnableKick() && (al::isMsgPlayerKick(pMsg) || rc::isMsgSkateShoesAttack(pMsg) ||
                           al::isMsgPlayerBodyAttack(pMsg))) {
        mHolderSensor = pOther;
        mIsRotateOnFall = true;
        rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        mStateThrow->setThrowParam(mHolderSensor, &sKickParam, nullptr);
        al::setNerve(this, &NrvBallSnowKick);
        al::startSe(this, "PgKicked");
        return true;
    }

    if (isEnableHold(pOther) && mStatePlayerHold->tryStartCarryFront(pMsg, pOther, false)) {
        if (al::isNerve(this, &NrvBallSnowThrow) || al::isNerve(this, &NrvBallSnowKick) ||
            al::isNerve(this, &NrvBallSnowFall)) {
            al::startHitReaction(this, "雪玉キャッチ");
        }

        mHolderSensor = pOther;
        al::setNerve(this, &NrvBallSnowPlayerHold);
        al::setColliderRadius(this, 40.0f);
        return true;
    }

    return false;
}

/**
 * @brief Whether the owner of a sensor may pick the ball up.
 * @param pSensor Sensor of the actor that wants to hold the ball.
 * @return True if the ball can be held.
 */
bool BallSnow::isEnableHold(al::HitSensor* pSensor) {
    if (mHolderSensor == nullptr) {
        return true;
    }

    if (al::getSensorHost(pSensor) != al::getSensorHost(mHolderSensor)) {
        return true;
    }

    return mHoldDisableTimer == 0;
}

/**
 * @brief Whether the ball lies still or rolls, so it can be kicked.
 * @return True if the ball can be kicked.
 */
bool BallSnow::isEnableKick() {
    return al::isNerve(this, &NrvBallSnowWait) || al::isNerve(this, &NrvBallSnowRolling);
}

/**
 * @brief Lets the touch pointer grab the ball, or destroys it on a touch assist tap.
 * @param pMsg Received message.
 * @param pPointer Pointer that sent the message.
 * @param pTarget Screen point target of the ball.
 * @return True if the message was handled.
 */
bool BallSnow::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                     al::ScreenPointTarget* pTarget) {
    if (al::isNerve(this, &NrvBallSnowDamageThrow) || al::isNerve(this, &NrvBallSnowHide) ||
        al::isNerve(this, &NrvBallSnowThrow) || al::isNerve(this, &NrvBallSnowKick) ||
        al::isNerve(this, &NrvBallSnowPlayerHold)) {
        return false;
    }

    if (mStateTouchCarry->receiveMsgScreenPoint(pMsg, pPointer, pTarget)) {
        if (!al::isNerve(this, &NrvBallSnowDrcHold)) {
            al::setNerve(this, &NrvBallSnowDrcHold);
            mHolderSensor = DrcFunction::tryFindDrcPlayerSensor(this, pPointer);
            mTouchPointer = DrcFunction::tryFindDrcTouchActor(this, pPointer);
        }

        return true;
    }

    if (al::isMsgTouchAssistTrig(pMsg)) {
        kill();
        return true;
    }

    return false;
}

/**
 * @brief Releases the ball from its holder and makes it disappear, or hides it until it can
 * reappear at its placement if it is set to be reborn.
 */
void BallSnow::kill() {
    if (al::isNerve(this, &NrvBallSnowPlayerHold)) {
        rc::requestPlayerRelease(mHolderSensor);
    } else if (al::isNerve(this, &NrvBallSnowDrcHold)) {
        rc::releaseTouchPointerHoldItem(this, mTouchPointer);
    }

    al::startHitReactionDisappear(this);
    al::validateClipping(this);
    if (mIsRebirth) {
        al::setNerve(this, &NrvBallSnowHide);
        return;
    }

    al::LiveActor::kill();
}

/**
 * @brief Counts down the timers, melts the ball in water and restores the collider radius.
 */
void BallSnow::control() {
    if (mHoldDisableTimer > 0) {
        mHoldDisableTimer--;
    }

    if (mKnockDownDisableTimer > 0) {
        mKnockDownDisableTimer--;
    }

    if (mHitDisableTimer > 0) {
        mHitDisableTimer--;
    }

    if (mClippingInvalidTimer > 0) {
        mClippingInvalidTimer--;
    }

    if (mClippingInvalidTimer == 0) {
        al::validateClipping(this);
        mClippingInvalidTimer = -1;
    }

    if (EnemyStateUtil::tryKillByAreaOrMaterialCode(this)) {
        return;
    }

    if (GameDataFunction::isSingleMode(GameDataHolderAccessor(this)) &&
        (InkUtil::isInInkLimitSphere(this) || rc::isInWaterArea(this, 75.0f))) {
        kill();
        return;
    }

    if (al::isNerve(this, &NrvBallSnowPlayerHold) || al::isNerve(this, &NrvBallSnowDrcHold)) {
        return;
    }

    BallStateFunction::setColliderReturnedSlowly(this, mColliderRadius, 2);
}

/**
 * @brief Updates the collider, letting the holder state drive it while the ball is carried.
 */
void BallSnow::updateCollider() {
    ItemStatePlayerHold* state = mStatePlayerHold;
    if (state->isDead()) {
        al::LiveActor::updateCollider();
        return;
    }

    state->updateCollider(al::getHitSensor(this, "Body"));
}

/**
 * @brief Puts the ball back to rest.
 */
void BallSnow::reset() {
    mIsBreakOnLand = false;
    al::onCollide(this);
    al::setVelocityZero(this);
    al::setNerve(this, &NrvBallSnowWait);
}

/**
 * @brief Whether a player carries the ball.
 * @return True while held by a player.
 */
bool BallSnow::isPlayerHold() const {
    return al::isNerve(this, &NrvBallSnowPlayerHold);
}

/**
 * @brief Makes the player carrying the ball let go of it.
 */
void BallSnow::requestPlayerRelease() {
    if (al::isNerve(this, &NrvBallSnowPlayerHold) && mHolderSensor != nullptr) {
        rc::requestPlayerRelease(mHolderSensor);
        al::setNerve(this, &NrvBallSnowFall);
    }
}

/**
 * @brief Lies on the ground, sliding to a halt, and falls once the ground is gone.
 */
void BallSnow::exeWait() {
    if (al::isFirstStep(this)) {
        al::validateClipping(this);
        mComboCounter->reset();
    }

    bool isSingleMode = GameDataFunction::isSingleMode(GameDataHolderAccessor(this));
    if (!isSingleMode || !al::isOnGround(this, 0, 0.0f)) {
        al::addVelocityToGravity(this, 2.0f);
    }

    if (!al::isOnGround(this, 8, 0.0f) && !al::isLessEqualStep(this, 5)) {
        al::invalidateClipping(this);
        al::setNerve(this, &NrvBallSnowFall);
        return;
    }

    bool isOnGround = al::isOnGround(this, 0, 0.0f);
    if (isSingleMode) {
        if (isOnGround) {
            *al::getVelocityPtr(this) *= 0.98f;
        }
    } else if (isOnGround) {
        al::setVelocityZero(this);
    }

    BallStateFunction::sendMsgToCollision(this, true);
}

/**
 * @brief Carried by a player.
 */
void BallSnow::exePlayerHold() {
    if (al::isFirstStep(this)) {
        al::setVelocityZero(this);
        al::startSe(this, "PgHoldStart");
    }

    al::updateNerveState(this);
}

/**
 * @brief Carried by the touch pointer, thrown or dropped when released.
 */
void BallSnow::exeDrcHold() {
    if (al::isStep(this, 1)) {
        al::startHitReaction(this, "DRCつかむ");
    }

    if (!al::updateNerveState(this)) {
        return;
    }

    if (mStateTouchCarry->isItemThrow()) {
        mStateThrow->setThrowParam(nullptr, &sThrowParam, mTouchPointer);
        al::setVelocity(this, mStateTouchCarry->getThrowVelocity());
        al::setNerve(this, &NrvBallSnowThrow);
    } else {
        al::setVelocity(this, mStateTouchCarry->getReleaseVelocity());
        al::setNerve(this, &NrvBallSnowFall);
        al::startHitReaction(this, "DRC放す");
    }

    al::invalidateClipping(this);
    mClippingInvalidTimer = 60;
}

/**
 * @brief Flies after being thrown.
 */
void BallSnow::exeThrow() {
    if (al::isFirstStep(this)) {
        mIsBreakOnLand = false;
        mHoldDisableTimer = 12;
        mKnockDownDisableTimer = 12;
        mWallCollideCount = 0;
        al::invalidateClipping(this);
        al::startSe(this, "PgThrow");
    } else {
        startEffect();
    }

    countWallCollide();
    if (al::updateNerveState(this)) {
        al::setNerve(this, &NrvBallSnowFall);
    }
}

/**
 * @brief Emits the collision effect where the ball hits something.
 */
void BallSnow::startEffect() {
    sead::Vector3f normal = {0.0f, 0.0f, 0.0f};
    sead::Vector3f pos = {0.0f, 0.0f, 0.0f};
    if (!BallStateFunction::getCollidedNormalAndPos(this, &normal, &pos)) {
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
    al::startHitReaction(this, "コリジョンヒット");
}

/**
 * @brief Counts the frames in which the ball hits a wall.
 */
void BallSnow::countWallCollide() {
    if (al::isCollidedWallVelocity(this)) {
        mWallCollideCount++;
    }
}

/**
 * @brief Flies after being kicked.
 */
void BallSnow::exeKick() {
    if (al::isFirstStep(this)) {
        mIsBreakOnLand = false;
        mHoldDisableTimer = 12;
        mKnockDownDisableTimer = 12;
        mWallCollideCount = 0;
        al::invalidateClipping(this);
    } else {
        startEffect();
    }

    countWallCollide();
    if (al::updateNerveState(this)) {
        al::setNerve(this, &NrvBallSnowFall);
    }
}

/**
 * @brief Falls and bounces until the ball starts rolling, breaking on landing after a hit.
 */
void BallSnow::exeFall() {
    if (al::isFirstStep(this)) {
        mStateFall->setIsRotate(mIsRotateOnFall);
    }

    if (al::isCollidedGround(this)) {
        mComboCounter->reset();
        if (mIsBreakOnLand) {
            kill();
            return;
        }
    }

    startEffect();
    countWallCollide();
    if (al::updateNerveState(this)) {
        al::setNerve(this, &NrvBallSnowRolling);
    }
}

/**
 * @brief Rolls on the ground until it stops.
 */
void BallSnow::exeRolling() {
    if (!al::updateNerveState(this)) {
        return;
    }

    if (al::isOnGround(this, 0, 0.0f)) {
        al::setNerve(this, &NrvBallSnowWait);
    } else {
        al::setNerve(this, &NrvBallSnowFall);
    }
}

/**
 * @brief Flies away after hurting its holder, vanishing once it lands or hits something.
 */
void BallSnow::exeDamageThrow() {
    if (al::isFirstStep(this)) {
        al::invalidateClipping(this);
    }

    al::updateNerveState(this);
    if ((al::isCollidedGround(this) && al::getVelocity(this).y < 0.0f) ||
        al::isCollidedWall(this) || al::isCollidedCeiling(this) ||
        al::isGreaterEqualStep(this, 30)) {
        al::validateClipping(this);
        kill();
    }
}

/**
 * @brief Stays hidden, moves back to its placement and reappears once out of view.
 */
void BallSnow::exeHide() {
    if (al::isFirstStep(this)) {
        al::setVelocityZero(this);
        al::offCollide(this);
        al::hideModelIfShow(this);
        al::invalidateHitSensors(this);
        al::invalidateClipping(this);
    }

    if (al::isStep(this, 120)) {
        al::resetPosition(this, mRebirthTrans, false);
    }

    if (al::isGreaterStep(this, 120) && al::isJudgedToClipFrustum(this, 200.0f, 300.0f)) {
        al::showModelIfHide(this);
        al::validateHitSensors(this);
        al::validateClipping(this);
        reset();
    }
}

/**
 * @brief Hides the ball unless a player carries it.
 * @return True if the ball was hidden.
 */
bool BallSnow::hideActor() {
    if (al::isNerve(this, &NrvBallSnowPlayerHold)) {
        return false;
    }

    return al::LiveActor::hideActor();
}
