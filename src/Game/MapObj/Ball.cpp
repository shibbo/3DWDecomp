#include "MapObj/Ball.hpp"

#include <math/seadQuat.h>

#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Actor/ComboCounter.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Thread/Functor.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Enemy/EnemyStateUtil.hpp"
#include "MapObj/ActorStateRouteDokanMove.hpp"
#include "MapObj/BallStateFall.hpp"
#include "MapObj/BallStateFallParam.hpp"
#include "MapObj/BallStateFunction.hpp"
#include "MapObj/BallStateRolling.hpp"
#include "MapObj/BallStateThrow.hpp"
#include "MapObj/DrcAssistDirectorUtil.hpp"
#include "MapObj/ItemStatePlayerHold.hpp"
#include "MapObj/ItemStatePlayerHoldParam.hpp"
#include "MapObj/ItemStatePopUpFront.hpp"
#include "MapObj/TouchCarryItemState.hpp"
#include "Player/Normal/PlayerActor.hpp"
#include "Player/Normal/PlayerGigaDirector.hpp"
#include "Player/Player.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/DrcUtil.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"

namespace {
NERVE_DECL(Ball, Wait)
NERVE_DECL(Ball, PopUpFront)
NERVE_DECL(Ball, DRCHold)
NERVE_DECL(Ball, PlayerHold)
NERVE_DECL(Ball, Fall)
NERVE_DECL(Ball, Rolling)
NERVE_DECL(Ball, Throw)
NERVE_DECL(Ball, Kick)
NERVE_DECL(Ball, DamageThrow)
NERVE_DECL(Ball, RouteDokan)
NERVE_DECL(Ball, RouteDokanThrow)
NERVE_DECL(Ball, WaterBottom)

// Non-const nerve objects: the game keeps them in .data, merged into one block.
BallNrvWait NrvBallWait;
BallNrvPopUpFront NrvBallPopUpFront;
BallNrvDRCHold NrvBallDRCHold;
BallNrvPlayerHold NrvBallPlayerHold;
BallNrvFall NrvBallFall;
BallNrvRolling NrvBallRolling;
BallNrvThrow NrvBallThrow;
BallNrvKick NrvBallKick;
BallNrvDamageThrow NrvBallDamageThrow;
BallNrvRouteDokan NrvBallRouteDokan;
BallNrvRouteDokanThrow NrvBallRouteDokanThrow;
BallNrvWaterBottom NrvBallWaterBottom;

typedef al::FunctorV0M<Ball*, void (Ball::*)()> BallFunctor;

ItemStatePlayerHoldParam sPlayerHoldParam(
    sead::Vector3f(10.0f, 0.0f, 40.0f), sead::Vector3f(5.0f, 0.0f, 35.0f),
    sead::Vector3f(5.0f, 0.0f, 25.0f), sead::Vector3f(25.0f, 0.0f, 45.0f),
    sead::Vector3f(5.0f, 0.0f, 25.0f), sead::Vector3f(30.0f, 0.0f, 45.0f),
    sead::Vector3f(30.0f, 0.0f, 45.0f), sead::Vector3f(30.0f, 0.0f, 35.0f),
    sead::Vector3f(35.0f, 0.0f, 45.0f), sead::Vector3f(40.0f, 0.0f, 30.0f),
    sead::Vector3f(0.0f, 0.0f, 30.0f));
BallStateFallParam sFallParam;
BallStateRollingParam sRollingParam;
BallStateThrowParam sThrowParam(30.0f, 8.0f, 0.45f, 1.5f, 0.5f, 0.996f, 0.65f, 8.0f, 45, false,
                                false);
BallStateThrowParam sKickParam(21.0f, 12.0f, 1.0f, 1.0f, 0.5f, 0.996f, 0.65f, 8.0f, -1, true,
                               false);
BallStateThrowParam sHitParam(10.0f, 8.0f, 1.0f, 1.0f, 0.5f, 0.996f, 0.65f, 8.0f, -1, false, true);
BallStateThrowParam sDamageThrowParam(21.0f, 8.0f, 0.45f, 1.5f, 0.5f, 0.996f, 0.65f, 8.0f, 45,
                                      true, true);
BallStateThrowParam sRouteDokanThrowParam(10.0f, 8.0f, 1.0f, 1.0f, 0.5f, 0.996f, 0.65f, 8.0f, -1,
                                          false, true);
}  // namespace

/**
 * @brief Constructs the ball.
 * @param pName Actor name.
 */
Ball::Ball(const char* pName) : al::LiveActor(pName) {}

/**
 * @brief Initializes the model, the nerve states and the effect matrix.
 * @param rInfo Placement info of the actor.
 */
void Ball::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "Ball", nullptr);
    al::initNerve(this, &NrvBallWait, 10);
    mIsSingleMode = rInfo.getActorSceneInfo().isSingleMode;

    mStatePopUpFront = new ItemStatePopUpFront(this);
    mStateTouchCarry = new TouchCarryItemState(this, nullptr);
    mStatePlayerHold = new ItemStatePlayerHold(this, &sPlayerHoldParam, false, false);
    mStateFall = new BallStateFall(this, &sFallParam);
    mStateRolling = new BallStateRolling(this, &sRollingParam);
    sThrowParam.mLaunchSpeed = rc::isUsingOldPlayerParams() ? 21.0f : 30.0f;
    mStateThrow = new BallStateThrow(this, &sThrowParam);
    mStateRouteDokan = new ActorStateRouteDokanMove(this, rInfo);

    al::initNerveState(this, mStatePopUpFront, &NrvBallPopUpFront, "[state]跳ね上げ(前方)");
    al::initNerveState(this, mStateTouchCarry, &NrvBallDRCHold, "[state]アイテム持ち運び");
    al::initNerveState(this, mStatePlayerHold, &NrvBallPlayerHold, "[state]プレイヤーに持たれる");
    al::initNerveState(this, mStateFall, &NrvBallFall, "[state]落下");
    al::initNerveState(this, mStateRolling, &NrvBallRolling, "[state]転がる");
    al::initNerveState(this, mStateThrow, &NrvBallThrow, "[state]投げ");
    al::initNerveState(this, mStateThrow, &NrvBallKick, "[state]蹴り");
    al::initNerveState(this, mStateThrow, &NrvBallDamageThrow, "[state]ダメージ時の投げ");
    al::initNerveState(this, mStateRouteDokan, &NrvBallRouteDokan, "[state]ルート土管移動");
    al::initNerveState(this, mStateThrow, &NrvBallRouteDokanThrow,
                       "[state]ルート土管出口時の投げ");
    mStatePlayerHold->initColliderControl();
    mColliderRadius = al::getColliderRadius(this);
    makeActorAppeared();

    al::listenStageSwitchOnKill(this, BallFunctor(this, &Ball::kill));
    al::setEffectFollowMtxPtr(this, "HitCollision", &mEffectMtx);
    al::createAndSetColliderSpecialPurpose(this, "BallMoveLimit");
}

/**
 * @brief Attacks enemies, map objects and players that the ball touches.
 * @param pSelf Sensor of the ball.
 * @param pOther Sensor that was touched.
 */
void Ball::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isNerve(this, &NrvBallDamageThrow) || al::isNerve(this, &NrvBallWaterBottom)) {
        return;
    }

    if (al::isSensorName(pSelf, "Body")) {
        if (al::isSensorEnemyBody(pOther) || al::isSensorKickKoura(pOther) ||
            al::isSensorRide(pOther) || (mIsSingleMode && al::isSensorEnemy(pOther))) {
            if (al::isNerve(this, &NrvBallFall) && al::getVelocity(this).y < -10.0f &&
                al::sendMsgBallTrample(pOther, pSelf, mComboCounter)) {
                BallStateFunction::calcReflectSpeed(this, nullptr, &sFallParam);
                al::startSeWithParam(this, "PgBound", al::getVelocity(this).length());
                return;
            }

            if (al::isNerve(this, &NrvBallRouteDokan)) {
                if (al::sendMsgBallRouteDokanAttack(pOther, pSelf, mComboCounter)) {
                    kill();
                    return;
                }

                if (al::sendMsgVanish(pOther, pSelf)) {
                    return;
                }
            }

            if (al::isNerve(this, &NrvBallPlayerHold)) {
                if (!al::sendMsgBallAttackHold(pOther, pSelf)) {
                    return;
                }

                rc::requestPlayerRelease(mHolderSensor);
                BallStateFunction::calcReflectSpeed(this, al::getSensorHost(pOther), &sFallParam);
                mStateThrow->setThrowParam(mHolderSensor, &sDamageThrowParam, nullptr);
                al::setNerve(this, &NrvBallDamageThrow);
                al::startSeWithParam(this, "PgBound", al::getVelocity(this).length());
                return;
            }

            if (al::isNerve(this, &NrvBallDRCHold)) {
                if (!al::isGreaterEqualStep(this, 25)) {
                    return;
                }

                if (!al::sendMsgBallAttackDRCHold(pOther, pSelf)) {
                    return;
                }

                rc::releaseTouchPointerHoldItem(this, mTouchPointer);
                BallStateFunction::calcReflectSpeed(this, al::getSensorHost(pOther), &sFallParam);
                mStateThrow->setThrowParam(mHolderSensor, &sDamageThrowParam, mTouchPointer);
                al::setNerve(this, &NrvBallDamageThrow);
                al::startSeWithParam(this, "PgBound", al::getVelocity(this).length());
                return;
            }

            if (mIsSingleMode && al::isSensorRide(pOther)) {
                return;
            }

            al::sendMsgPush(pOther, pSelf);
        }

        if (al::isNerve(this, &NrvBallDRCHold)) {
            return;
        }

        if (!al::isNerve(this, &NrvBallRouteDokanThrow) && !al::isNerve(this, &NrvBallRouteDokan) &&
            !al::isNerve(this, &NrvBallWait)) {
            if (al::isNerve(this, &NrvBallFall) && mRouteDokanDisableTimer > 0) {
                return;
            }

            if (mStateRouteDokan->tryStart(pSelf, pOther)) {
                al::offCollide(this);
                al::setNerve(this, &NrvBallRouteDokan);
                return;
            }
        }

        if (al::isNerve(this, &NrvBallPlayerHold)) {
            return;
        }

        if (al::isSensorPlayer(pOther)) {
            if (isEnablePlayerKnockDown(pOther, pSelf) && al::sendMsgKnockDown(pOther, pSelf)) {
                sead::Vector3f dir = al::getVelocity(this);
                al::verticalizeVec(&dir, sead::Vector3f::ey, dir);
                al::normalizeOrZero(&dir);
                al::setVelocity(this, dir.x * -5.0f, 15.0f, dir.z * -5.0f);
                al::startSeWithParam(this, "PgBound", al::getVelocity(this).length());
                al::setNerve(this, &NrvBallFall);
                mHoldDisableTimer = 12;
                mKnockDownDisableTimer = 12;
                return;
            }

            al::sendMsgPush(pOther, pSelf);
            return;
        }
    }

    if (al::isSensorName(pSelf, "Hold")) {
        if (al::isSensorMapObj(pOther)) {
            if ((al::isNerve(this, &NrvBallThrow) || al::isNerve(this, &NrvBallKick) ||
                 al::isNerve(this, &NrvBallRouteDokanThrow)) &&
                al::sendMsgBallAttack(pOther, pSelf, mComboCounter)) {
                BallStateFunction::calcReflectSpeed(this, al::getSensorHost(pOther), &sFallParam);
                al::setNerve(this, &NrvBallFall);
                al::startSeWithParam(this, "PgBound", al::getVelocity(this).length());
                return;
            }

            if (al::isNerve(this, &NrvBallFall) &&
                al::sendMsgBallTrample(pOther, pSelf, mComboCounter)) {
                BallStateFunction::calcReflectSpeed(this, al::getSensorHost(pOther), &sFallParam);
                al::startSeWithParam(this, "PgBound", al::getVelocity(this).length());
                return;
            }

            if (!al::isNerve(this, &NrvBallWait)) {
                if (al::isNerve(this, &NrvBallRouteDokan)) {
                    rc::sendMsgRouteDokanItemGet(pOther, pSelf);
                    return;
                }

                al::sendMsgBallItemGet(pOther, pSelf);
                return;
            }
        }

        if (!al::isNerve(this, &NrvBallDRCHold) && al::isSensorHoldObj(pOther) &&
            al::sendMsgPush(pOther, pSelf)) {
            return;
        }
    }

    if (!al::isSensorName(pSelf, "Attack")) {
        return;
    }

    if (!al::isSensorEnemyBody(pOther) && !al::isSensorKickKoura(pOther) &&
        !(mIsSingleMode && al::isSensorEnemy(pOther))) {
        return;
    }

    if (!al::isNerve(this, &NrvBallThrow) && !al::isNerve(this, &NrvBallKick) &&
        !al::isNerve(this, &NrvBallRouteDokanThrow)) {
        return;
    }

    if (mAttackDisableTimer != 0) {
        return;
    }

    if (al::sendMsgBallAttack(pOther, pSelf, mComboCounter)) {
        BallStateFunction::calcReflectSpeed(this, al::getSensorHost(pOther), &sFallParam);
        al::setNerve(this, &NrvBallFall);
        al::startSeWithParam(this, "PgBound", al::getVelocity(this).length());
    }
}

/**
 * @brief Whether the ball flying into a player knocks that player down.
 * @param pPlayer Sensor of the player.
 * @param pSelf Sensor of the ball.
 * @return True if the ball moves towards the player fast enough.
 */
bool Ball::isEnablePlayerKnockDown(al::HitSensor* pPlayer, al::HitSensor* pSelf) {
    if (!al::isNerve(this, &NrvBallThrow) && !al::isNerve(this, &NrvBallKick) &&
        !al::isNerve(this, &NrvBallRouteDokanThrow) && !al::isNerve(this, &NrvBallFall)) {
        return false;
    }

    if (mHolderSensor == nullptr) {
        return false;
    }

    if (al::getSensorHost(pPlayer) == al::getSensorHost(mHolderSensor) &&
        mKnockDownDisableTimer > 0) {
        return false;
    }

    if (al::isLessStep(this, 1) || rc::isInWaterArea(this) || mWallCollideCount > 1) {
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
 * @brief Reacts to attacks, pushes and the player picking the ball up or throwing it.
 * @param pMsg Received message.
 * @param pOther Sensor that sent the message.
 * @param pSelf Sensor of the ball.
 * @return True if the message was handled.
 */
bool Ball::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) {
    if (pOther != nullptr && al::isSensorPlayer(pOther)) {
        auto* player = static_cast<PlayerActor*>(al::getSensorHost(pOther));
        if (player != nullptr && player->getPlayer() != nullptr &&
            player->getPlayer()->getGigaDirector() != nullptr &&
            player->getPlayerGigaDirector()->isGiga()) {
            return false;
        }
    }

    if (rc::isMsgAskControlUserId(pMsg, mHolderSensor)) {
        return true;
    }

    if (al::isNerve(this, &NrvBallDamageThrow) || al::isNerve(this, &NrvBallWaterBottom)) {
        return false;
    }

    if (rc::isMsgStartGoalDemoPole(pMsg)) {
        kill();
        return true;
    }

    if (al::isSensorName(pSelf, "Body")) {
        if (!al::isNerve(this, &NrvBallPlayerHold) && !al::isNerve(this, &NrvBallDRCHold) &&
            rc::isMsgJumpPanelAction(pMsg)) {
            al::setVelocity(this, 0.0f, 45.0f, 0.0f);
            al::setNerve(this, &NrvBallFall);
            return true;
        }

        if ((al::isNerve(this, &NrvBallWait) || al::isNerve(this, &NrvBallRolling)) &&
            (al::isMsgPlayerObjHipDropAll(pMsg) || al::isMsgPlayerFireBallAttack(pMsg) ||
             al::isMsgPlayerBoomerangAttack(pMsg) || al::isMsgKickKouraAttack(pMsg) ||
             al::isMsgPlayerKouraAttack(pMsg) || al::isMsgBallAttack(pMsg) ||
             al::isMsgBallTrample(pMsg) || al::isMsgPlayerBodyLanding(pMsg) ||
             al::isMsgPlayerGiantHipDrop(pMsg) || al::isMsgPlayerCooperationHipDrop(pMsg) ||
             al::isMsgPlayerSlidingAttack(pMsg) || al::isMsgBlockUpperPunch(pMsg) ||
             al::isMsgExplosion(pMsg) || rc::isMsgBobsledBodyAttack(pMsg))) {
            sead::Vector3f dir = {0.0f, 0.0f, 0.0f};
            al::calcDirBetweenSensorsH(&dir, pOther, pSelf);
            if (al::isNearZero(dir, 0.001f)) {
                if (al::isSensorPlayer(pOther)) {
                    dir.set(rc::getPlayerFront(pOther));
                } else {
                    dir.set(sead::Vector3f::ey);
                }
            }

            al::setVelocity(this, dir.x * 8.0f, 22.0f, dir.z * 8.0f);
            al::startSeWithParam(this, "PgBound", al::getVelocity(this).length());
            mHolderSensor = BallStateFunction::tryGetRelativePlayerSensor(this, pOther);
            mStateThrow->setThrowParam(mHolderSensor, &sHitParam, nullptr);
            al::setNerve(this, &NrvBallThrow);
            mIsRotateOnFall = false;
            if (!rc::isMsgBobsledBodyAttack(pMsg)) {
                rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
            }

            if (al::isMsgPlayerKouraAttack(pMsg) || al::isMsgKickKouraAttack(pMsg)) {
                mAttackDisableTimer = 12;
            }

            return al::isMsgPlayerFireBallAttack(pMsg) || al::isMsgPlayerKouraAttack(pMsg);
        }

        if (GameDataFunction::isSingleMode(GameDataHolderAccessor(this)) &&
            al::tryReceiveMsgPushAndAddVelocity(this, pMsg, pOther, pSelf, 1.0f)) {
            return true;
        }

        if (rc::isMsgDossunPress(pMsg)) {
            kill();
            return true;
        }
    }

    if (al::isNerve(this, &NrvBallDRCHold)) {
        if (al::isMsgWarpStart(pMsg)) {
            rc::releaseTouchPointerHoldItem(this, mTouchPointer);
            al::setNerve(this, &NrvBallFall);
            mIsRotateOnFall = true;
        }

        return false;
    }

    if (!al::isSensorName(pSelf, "Hold")) {
        return false;
    }

    if (al::isNerve(this, &NrvBallPlayerHold)) {
        if (!mStatePlayerHold->receiveMsg(pMsg, pOther, pSelf)) {
            return false;
        }

        if (al::isMsgPlayerRelease(pMsg)) {
            al::setNerve(this, &NrvBallThrow);
            mStateThrow->setThrowParam(mHolderSensor, &sThrowParam, nullptr);
            mIsRotateOnFall = false;
            return true;
        }

        if (al::isMsgPlayerReleaseDamage(pMsg) || al::isMsgPlayerReleaseDead(pMsg)) {
            al::setNerve(this, &NrvBallDamageThrow);
            mStateThrow->setThrowParam(mHolderSensor, &sDamageThrowParam, nullptr);
            return true;
        }

        if (al::isMsgWarpStart(pMsg) || al::isMsgHoldCancel(pMsg)) {
            al::setNerve(this, &NrvBallFall);
            mHoldDisableTimer = 12;
            mKnockDownDisableTimer = 12;
            mIsRotateOnFall = true;
            mRouteDokanDisableTimer = 75;
        }

        return true;
    }

    if ((isEnableHold(pOther) &&
         (al::isMsgPlayerTailAttack(pMsg) || al::isMsgPlayerClimbAttack(pMsg) ||
          al::isMsgPlayerSpinAttack(pMsg))) ||
        al::isMsgPlayerObjRollingAttack(pMsg) || al::isMsgPlayerGiantAttack(pMsg)) {
        mHolderSensor = pOther;
        mIsRotateOnFall = false;
        rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        mStateThrow->setThrowParam(mHolderSensor, &sThrowParam, nullptr);
        al::startSeWithParam(this, "PgBound", 500.0f);
        al::setNerve(this, &NrvBallThrow);
        return true;
    }

    if (isEnableHold(pOther) && mStatePlayerHold->tryStartCarryFront(pMsg, pOther, false)) {
        if (al::isNerve(this, &NrvBallThrow) || al::isNerve(this, &NrvBallKick) ||
            al::isNerve(this, &NrvBallFall) || al::isNerve(this, &NrvBallPopUpFront) ||
            al::isNerve(this, &NrvBallRouteDokanThrow)) {
            al::startHitReaction(this, "ボールキャッチ");
        }

        al::onCollide(this);
        mHolderSensor = pOther;
        al::setNerve(this, &NrvBallPlayerHold);
        al::setColliderRadius(this, 15.0f);
        return true;
    }

    if ((al::isMsgPlayerKick(pMsg) || al::isMsgPlayerBodyAttack(pMsg)) && isEnableKick()) {
        mHolderSensor = pOther;
        mIsRotateOnFall = true;
        rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        mStateThrow->setThrowParam(mHolderSensor, &sKickParam, nullptr);
        al::startSe(this, "PgKicked");
        al::setNerve(this, &NrvBallKick);
        return true;
    }

    return al::tryReceiveMsgPushAndAddVelocity(this, pMsg, pOther, pSelf, 1.0f);
}

/**
 * @brief Whether the owner of a sensor may pick the ball up.
 * @param pSensor Sensor of the actor that wants to hold the ball.
 * @return True if the ball can be held.
 */
bool Ball::isEnableHold(al::HitSensor* pSensor) {
    if (al::isNerve(this, &NrvBallRouteDokan)) {
        return false;
    }

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
bool Ball::isEnableKick() {
    return al::isNerve(this, &NrvBallWait) || al::isNerve(this, &NrvBallRolling);
}

/**
 * @brief Lets the touch pointer grab the ball.
 * @param pMsg Received message.
 * @param pPointer Pointer that sent the message.
 * @param pTarget Screen point target of the ball.
 * @return True if the ball is grabbed.
 */
bool Ball::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                 al::ScreenPointTarget* pTarget) {
    if (al::isNerve(this, &NrvBallPopUpFront) || al::isNerve(this, &NrvBallPlayerHold) ||
        al::isNerve(this, &NrvBallThrow) || al::isNerve(this, &NrvBallKick) ||
        al::isNerve(this, &NrvBallRouteDokan) || al::isNerve(this, &NrvBallRouteDokanThrow) ||
        al::isNerve(this, &NrvBallDamageThrow) || al::isNerve(this, &NrvBallWaterBottom)) {
        return false;
    }

    if (!mStateTouchCarry->receiveMsgScreenPoint(pMsg, pPointer, pTarget)) {
        return false;
    }

    if (!al::isNerve(this, &NrvBallDRCHold)) {
        al::setNerve(this, &NrvBallDRCHold);
        mHolderSensor = DrcFunction::tryFindDrcPlayerSensor(this, pPointer);
        mTouchPointer = DrcFunction::tryFindDrcTouchActor(this, pPointer);
    }

    return true;
}

/**
 * @brief Counts down the timers, kills the ball in kill areas and restores the collider radius.
 */
void Ball::control() {
    if (mHoldDisableTimer > 0) {
        mHoldDisableTimer--;
    }

    if (mKnockDownDisableTimer > 0) {
        mKnockDownDisableTimer--;
    }

    if (mAttackDisableTimer > 0) {
        mAttackDisableTimer--;
    }

    if (mRouteDokanDisableTimer > 0) {
        mRouteDokanDisableTimer--;
    }

    if (EnemyStateUtil::tryKillByAreaOrMaterialCode(this) ||
        al::isNerve(this, &NrvBallPlayerHold)) {
        return;
    }

    BallStateFunction::setColliderReturnedSlowly(this, mColliderRadius, 1);
}

/**
 * @brief Releases the ball from its holder and makes it disappear.
 */
void Ball::kill() {
    if (al::isNerve(this, &NrvBallRouteDokan)) {
        al::startHitReaction(this, "ルート土管消滅");
        al::LiveActor::kill();
        return;
    }

    if (al::isNerve(this, &NrvBallPlayerHold)) {
        rc::requestPlayerRelease(mHolderSensor);
    } else if (al::isNerve(this, &NrvBallDRCHold)) {
        rc::releaseTouchPointerHoldItem(this, mTouchPointer);
    }

    al::startHitReactionDisappear(this);
    al::LiveActor::kill();
}

/**
 * @brief Updates the collider, letting the holder state drive it while the ball is carried.
 */
void Ball::updateCollider() {
    ItemStatePlayerHold* state = mStatePlayerHold;
    if (state->isDead()) {
        al::LiveActor::updateCollider();
        return;
    }

    state->updateCollider(al::getHitSensor(this, "Body"));
}

/**
 * @brief Appears and pops the ball up to the front.
 */
void Ball::appearPopUpFront() {
    al::invalidateHitSensors(this);
    al::LiveActor::appear();
    mStatePopUpFront->setParamBallPopUp();
    al::setNerve(this, &NrvBallPopUpFront);
}

/**
 * @brief Appears and pops the ball straight up.
 */
void Ball::appearAbove() {
    al::invalidateHitSensors(this);
    al::LiveActor::appear();
    mStatePopUpFront->setParamBallAbove();
    al::setNerve(this, &NrvBallPopUpFront);
}

/**
 * @brief Puts the ball back to rest.
 */
void Ball::reset() {
    al::onCollide(this);
    al::setNerve(this, &NrvBallWait);
    al::setVelocityZero(this);
}

/**
 * @brief Whether a player carries the ball.
 * @return True while held by a player.
 */
bool Ball::isPlayerHold() const {
    return al::isNerve(this, &NrvBallPlayerHold);
}

/**
 * @brief Hides the ball unless a player carries it.
 * @return True if the ball was hidden.
 */
bool Ball::hideActor() {
    if (al::isNerve(this, &NrvBallPlayerHold)) {
        return false;
    }

    return al::LiveActor::hideActor();
}

/**
 * @brief Lies on the ground, falling or rolling away once the ground is gone or sloped.
 */
void Ball::exeWait() {
    if (al::isFirstStep(this)) {
        al::validateClipping(this);
        mComboCounter->reset();
    }

    al::addVelocityToGravity(this, 1.5f);
    if (!al::isOnGround(this, 8, 0.0f) && !al::isLessEqualStep(this, 5)) {
        al::invalidateClipping(this);
        al::setNerve(this, &NrvBallFall);
        return;
    }

    if (!al::isCollidedGround(this)) {
        return;
    }

    sead::Vector3f normal = al::getOnGroundNormal(this, 0);
    if (al::isNearDirection(normal, sead::Vector3f::ey, 0.01f)) {
        al::setVelocityZero(this);
        return;
    }

    al::invalidateClipping(this);
    al::setNerve(this, &NrvBallRolling);
}

/**
 * @brief Carried by a player.
 */
void Ball::exePlayerHold() {
    if (al::isFirstStep(this)) {
        al::setVelocityZero(this);
        al::startSe(this, "PgHoldStart");
    }

    al::updateNerveState(this);
}

/**
 * @brief Carried by the touch pointer, thrown or dropped when released.
 */
void Ball::exeDRCHold() {
    if (al::isStep(this, 1)) {
        al::startHitReaction(this, "DRCつかむ");
    }

    if (!al::updateNerveState(this)) {
        return;
    }

    if (mStateTouchCarry->isItemThrow()) {
        mStateThrow->setThrowParam(nullptr, &sThrowParam, mTouchPointer);
        al::setVelocity(this, mStateTouchCarry->getThrowVelocity());
        al::setNerve(this, &NrvBallThrow);
        return;
    }

    al::setVelocity(this, mStateTouchCarry->getReleaseVelocity());
    al::setNerve(this, &NrvBallFall);
    al::startHitReaction(this, "DRC放す");
}

/**
 * @brief Flies after being thrown.
 */
void Ball::exeThrow() {
    if (al::isFirstStep(this)) {
        mWallCollideCount = 0;
        mHoldDisableTimer = 12;
        mKnockDownDisableTimer = 12;
        al::invalidateClipping(this);
        al::startSe(this, "PgThrow");
    } else {
        startEffect();
    }

    countWallCollide();
    if (al::updateNerveState(this)) {
        al::setNerve(this, &NrvBallFall);
    }
}

/**
 * @brief Emits the collision effect where the ball hits a wall or a ceiling.
 */
void Ball::startEffect() {
    rc::startHitReactionIfThroughWater(this);

    sead::Vector3f normal = {0.0f, 0.0f, 0.0f};
    sead::Vector3f pos = {0.0f, 0.0f, 0.0f};
    if (!al::isCollidedWallVelocity(this) && !al::isCollidedCeilingVelocity(this)) {
        return;
    }

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
void Ball::countWallCollide() {
    if (al::isCollidedWallVelocity(this)) {
        mWallCollideCount++;
    }
}

/**
 * @brief Flies after being kicked.
 */
void Ball::exeKick() {
    if (al::isFirstStep(this)) {
        mWallCollideCount = 0;
        mHoldDisableTimer = 12;
        mKnockDownDisableTimer = 12;
        al::invalidateClipping(this);
    } else {
        startEffect();
    }

    countWallCollide();
    if (al::updateNerveState(this)) {
        al::setNerve(this, &NrvBallFall);
    }
}

/**
 * @brief Falls and bounces until the ball starts rolling.
 */
void Ball::exeFall() {
    if (al::isFirstStep(this)) {
        al::invalidateClipping(this);
        mStateFall->setIsRotate(mIsRotateOnFall);
    }

    if (al::isCollidedGround(this)) {
        mComboCounter->reset();
    }

    startEffect();
    countWallCollide();
    if (al::updateNerveState(this)) {
        al::setNerve(this, &NrvBallRolling);
    }
}

/**
 * @brief Rolls on the ground until it stops, sinking once it comes to rest in water.
 */
void Ball::exeRolling() {
    if (!al::updateNerveState(this)) {
        return;
    }

    if (mStateRolling->isOnAir()) {
        al::setNerve(this, &NrvBallFall);
        return;
    }

    if (rc::isInWaterArea(this)) {
        al::setNerve(this, &NrvBallWaterBottom);
        return;
    }

    al::setNerve(this, &NrvBallWait);
}

/**
 * @brief Flies away after hurting its holder, vanishing once it lands or hits something.
 */
void Ball::exeDamageThrow() {
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
 * @brief Lies at the bottom of the water for a while, then vanishes.
 */
void Ball::exeWaterBottom() {
    if (al::isGreaterEqualStep(this, 30)) {
        al::startHitReaction(this, "水底消滅");
        al::LiveActor::kill();
    }
}

/**
 * @brief Pops up out of a block.
 */
void Ball::exePopUpFront() {
    if (al::updateNerveStateAndNextNerve(this, &NrvBallFall)) {
        al::invalidateClipping(this);
        return;
    }

    rc::startHitReactionIfThroughWater(this);
}

/**
 * @brief Travels through a route pipe.
 */
void Ball::exeRouteDokan() {
    if (al::isFirstStep(this)) {
        al::setVelocityZero(this);
        mStateRouteDokan->setMoveSpeed(20.0f);
    }

    sead::Quatf* quat = al::getQuatPtr(this);
    al::turnQuatZDirRadian(quat, *quat, mStateRouteDokan->getMoveDirection(), sead::Mathf::pi());
    if (al::updateNerveState(this)) {
        BallStateFunction::calcLaunchSpeed(this, mStateRouteDokan->getMoveDirection(),
                                           &sRouteDokanThrowParam);
        mStateThrow->setThrowParam(mHolderSensor, &sRouteDokanThrowParam, nullptr);
        al::setNerve(this, &NrvBallRouteDokanThrow);
    }
}

/**
 * @brief Flies out of the exit of a route pipe.
 */
void Ball::exeRouteDokanThrow() {
    if (al::isFirstStep(this)) {
        al::onCollide(this);
        mWallCollideCount = 0;
    } else {
        startEffect();
    }

    countWallCollide();
    if (al::updateNerveState(this)) {
        al::setNerve(this, &NrvBallFall);
    }
}
