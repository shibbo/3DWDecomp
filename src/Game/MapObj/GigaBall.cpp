#include "MapObj/GigaBall.hpp"

#include <math/seadQuat.h>

#include "Library/Actor/ComboCounter.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
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
#include "MapObj/ItemStateGigaPlayerHold.hpp"
#include "MapObj/ItemStatePlayerHold.hpp"
#include "MapObj/ItemStatePlayerHoldParam.hpp"
#include "MapObj/ItemStatePopUpFront.hpp"
#include "MapObj/TouchCarryItemState.hpp"
#include "Player/Normal/PlayerActor.hpp"
#include "Player/Normal/PlayerGigaDirector.hpp"
#include "Player/Player.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/DrcUtil.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"

namespace {
NERVE_DECL(GigaBall, Wait)
NERVE_DECL(GigaBall, PopUpFront)
NERVE_DECL(GigaBall, DRCHold)
NERVE_DECL(GigaBall, PlayerHold)
NERVE_DECL(GigaBall, Fall)
NERVE_DECL(GigaBall, Rolling)
NERVE_DECL(GigaBall, Throw)
NERVE_DECL(GigaBall, Kick)
NERVE_DECL(GigaBall, DamageThrow)
NERVE_DECL(GigaBall, RouteDokan)
NERVE_DECL(GigaBall, RouteDokanThrow)
NERVE_DECL(GigaBall, WaterBottom)

// Non-const nerve objects: the game keeps them in .data, merged into one block.
GigaBallNrvWait NrvGigaBallWait;
GigaBallNrvPopUpFront NrvGigaBallPopUpFront;
GigaBallNrvDRCHold NrvGigaBallDRCHold;
GigaBallNrvPlayerHold NrvGigaBallPlayerHold;
GigaBallNrvFall NrvGigaBallFall;
GigaBallNrvRolling NrvGigaBallRolling;
GigaBallNrvThrow NrvGigaBallThrow;
GigaBallNrvKick NrvGigaBallKick;
GigaBallNrvDamageThrow NrvGigaBallDamageThrow;
GigaBallNrvRouteDokan NrvGigaBallRouteDokan;
GigaBallNrvRouteDokanThrow NrvGigaBallRouteDokanThrow;
GigaBallNrvWaterBottom NrvGigaBallWaterBottom;

typedef al::FunctorV0M<GigaBall*, void (GigaBall::*)()> GigaBallFunctor;

ItemStatePlayerHoldParam sPlayerHoldParam(
    sead::Vector3f(10.0f, 0.0f, 40.0f), sead::Vector3f(5.0f, 0.0f, 35.0f),
    sead::Vector3f(5.0f, 0.0f, 25.0f), sead::Vector3f(25.0f, 0.0f, 45.0f),
    sead::Vector3f(5.0f, 0.0f, 25.0f), sead::Vector3f(30.0f, 0.0f, 45.0f),
    sead::Vector3f(30.0f, 0.0f, 45.0f), sead::Vector3f(30.0f, 0.0f, 35.0f),
    sead::Vector3f(35.0f, 0.0f, 45.0f), sead::Vector3f(40.0f, 0.0f, 30.0f),
    sead::Vector3f(0.0f, 0.0f, 30.0f));
BallStateFallParam sFallParam(28.0f, 11.0f, 0.6f, 0.7f, 0.1f, 50.0f, 8.0f, 5.0f, 0.5f, 0.996f,
                              0.65f, 0.7f);
BallStateRollingParam sRollingParam;
BallStateThrowParam sThrowParam(420.0f, 160.0f, 4.5f, 15.0f, 0.5f, 0.996f, 0.65f, 8.0f, 45, false,
                                false);
BallStateThrowParam sKickParam(420.0f, 240.0f, 10.0f, 10.0f, 0.5f, 0.996f, 0.65f, 8.0f, -1, true,
                               false);
BallStateThrowParam sHitParam(200.0f, 160.0f, 10.0f, 10.0f, 0.5f, 0.996f, 0.65f, 8.0f, -1, false,
                              true);
BallStateThrowParam sDamageThrowParam(420.0f, 160.0f, 4.5f, 15.0f, 0.5f, 0.996f, 0.65f, 8.0f, 45,
                                      true, true);
BallStateThrowParam sRouteDokanThrowParam(200.0f, 160.0f, 10.0f, 10.0f, 0.5f, 0.996f, 0.65f, 8.0f,
                                          -1, false, true);
}  // namespace

/**
 * @brief Constructs the giga ball.
 * @param pName Actor name.
 */
GigaBall::GigaBall(const char* pName) : al::LiveActor(pName) {}

/**
 * @brief Initializes the model, the nerve states and the effect matrix.
 * @param rInfo Placement info of the actor.
 */
void GigaBall::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "GigaBall", nullptr);
    al::initNerve(this, &NrvGigaBallWait, 10);

    mStatePopUpFront = new ItemStatePopUpFront(this);
    mStateTouchCarry = new TouchCarryItemState(this, nullptr);
    mStatePlayerHold = new ItemStateGigaPlayerHold(this, &sPlayerHoldParam, false, false);
    mStateFall = new BallStateFall(this, &sFallParam);
    mStateRolling = new BallStateRolling(this, &sRollingParam);
    mStateThrow = new BallStateThrow(this, &sThrowParam);
    mStateRouteDokan = new ActorStateRouteDokanMove(this, rInfo);

    al::initNerveState(this, mStatePopUpFront, &NrvGigaBallPopUpFront, "[state]跳ね上げ(前方)");
    al::initNerveState(this, mStateTouchCarry, &NrvGigaBallDRCHold, "[state]アイテム持ち運び");
    al::initNerveState(this, mStatePlayerHold, &NrvGigaBallPlayerHold,
                       "[state]プレイヤーに持たれる");
    al::initNerveState(this, mStateFall, &NrvGigaBallFall, "[state]落下");
    al::initNerveState(this, mStateRolling, &NrvGigaBallRolling, "[state]転がる");
    al::initNerveState(this, mStateThrow, &NrvGigaBallThrow, "[state]投げ");
    al::initNerveState(this, mStateThrow, &NrvGigaBallKick, "[state]蹴り");
    al::initNerveState(this, mStateThrow, &NrvGigaBallDamageThrow, "[state]ダメージ時の投げ");
    al::initNerveState(this, mStateRouteDokan, &NrvGigaBallRouteDokan, "[state]ルート土管移動");
    al::initNerveState(this, mStateThrow, &NrvGigaBallRouteDokanThrow,
                       "[state]ルート土管出口時の投げ");
    mStatePlayerHold->initColliderControl();
    mColliderRadius = al::getColliderRadius(this);
    makeActorAppeared();

    al::listenStageSwitchOnKill(this, GigaBallFunctor(this, &GigaBall::kill));
    al::setEffectFollowMtxPtr(this, "HitCollision", &mEffectMtx);
    al::createAndSetColliderSpecialPurpose(this, "BallMoveLimit");
    al::setScaleAll(this, 26.0f);
}

/**
 * @brief Dispatches the attack to the handler of the sensor that touched something.
 * @param pSelf Sensor of the ball.
 * @param pOther Sensor that was touched.
 */
void GigaBall::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isNerve(this, &NrvGigaBallDamageThrow) || al::isNerve(this, &NrvGigaBallWaterBottom)) {
        return;
    }

    if (al::isSensorName(pSelf, "Body")) {
        attackSensorBody(pSelf, pOther);
        return;
    }

    if (al::isSensorName(pSelf, "Hold")) {
        attackSensorHold(pSelf, pOther);
        return;
    }

    if (!al::isSensorName(pSelf, "Attack")) {
        return;
    }

    if (!al::isSensorEnemyBody(pOther) && !al::isSensorKickKoura(pOther)) {
        return;
    }

    if (!al::isNerve(this, &NrvGigaBallThrow) && !al::isNerve(this, &NrvGigaBallKick) &&
        !al::isNerve(this, &NrvGigaBallRouteDokanThrow)) {
        return;
    }

    if (mAttackDisableTimer != 0) {
        return;
    }

    if (al::sendMsgBallAttack(pOther, pSelf, mComboCounter)) {
        BallStateFunction::calcReflectSpeed(this, al::getSensorHost(pOther), &sFallParam);
        al::setNerve(this, &NrvGigaBallFall);
        al::startSeWithParam(this, "PgBound", al::getVelocity(this).length());
    }
}

/**
 * @brief Attacks enemies, pushes players and enters route pipes with the body sensor.
 * @param pSelf Body sensor of the ball.
 * @param pOther Sensor that was touched.
 */
void GigaBall::attackSensorBody(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isSensorEnemyBody(pOther) || al::isSensorKickKoura(pOther)) {
        if (al::isNerve(this, &NrvGigaBallFall) && al::getVelocity(this).y < -10.0f &&
            al::sendMsgBallTrample(pOther, pSelf, mComboCounter)) {
            BallStateFunction::calcReflectSpeed(this, nullptr, &sFallParam);
            al::startSeWithParam(this, "PgBound", al::getVelocity(this).length());
            return;
        }

        if (al::isNerve(this, &NrvGigaBallRouteDokan)) {
            if (al::sendMsgBallRouteDokanAttack(pOther, pSelf, mComboCounter)) {
                kill();
                return;
            }

            if (al::sendMsgVanish(pOther, pSelf)) {
                return;
            }
        }

        if (al::isNerve(this, &NrvGigaBallPlayerHold)) {
            if (!al::sendMsgBallAttackHold(pOther, pSelf)) {
                return;
            }

            requestRelease(pOther, &sDamageThrowParam, &NrvGigaBallDamageThrow);
            return;
        }

        if (al::isNerve(this, &NrvGigaBallDRCHold)) {
            if (!al::isGreaterEqualStep(this, 25)) {
                return;
            }

            if (!al::sendMsgBallAttackDRCHold(pOther, pSelf)) {
                return;
            }

            rc::releaseTouchPointerHoldItem(this, mTouchPointer);
            BallStateFunction::calcReflectSpeed(this, al::getSensorHost(pOther), &sFallParam);
            mStateThrow->setThrowParam(mHolderSensor, &sDamageThrowParam, mTouchPointer);
            al::setNerve(this, &NrvGigaBallDamageThrow);
            al::startSeWithParam(this, "PgBound", al::getVelocity(this).length());
            return;
        }

        al::sendMsgPush(pOther, pSelf);
    }

    if (al::isNerve(this, &NrvGigaBallDRCHold)) {
        return;
    }

    if (!al::isNerve(this, &NrvGigaBallRouteDokanThrow) &&
        !al::isNerve(this, &NrvGigaBallRouteDokan) && !al::isNerve(this, &NrvGigaBallWait)) {
        if (al::isNerve(this, &NrvGigaBallFall) && mRouteDokanDisableTimer > 0) {
            return;
        }

        if (mStateRouteDokan->tryStart(pSelf, pOther)) {
            al::offCollide(this);
            al::setNerve(this, &NrvGigaBallRouteDokan);
            return;
        }
    }

    if (al::isNerve(this, &NrvGigaBallPlayerHold)) {
        return;
    }

    if (al::isSensorPlayer(pOther)) {
        al::sendMsgPushVeryStrong(pOther, pSelf);
    }
}

/**
 * @brief Makes the holding player release the ball and throws it away.
 * @param pOther Sensor that hit the ball.
 * @param pParam Throw parameters.
 * @param pNerve Nerve to continue with.
 */
void GigaBall::requestRelease(al::HitSensor* pOther, BallStateThrowParam* pParam,
                              const al::Nerve* pNerve) {
    rc::requestPlayerRelease(mHolderSensor);
    BallStateFunction::calcReflectSpeed(this, al::getSensorHost(pOther), &sFallParam);
    mStateThrow->setThrowParam(mHolderSensor, pParam, nullptr);
    al::setNerve(this, pNerve);
    al::startSeWithParam(this, "PgBound", al::getVelocity(this).length());
    al::offCollide(this);
}

/**
 * @brief Attacks map objects and gives the ball to item collectors with the hold sensor.
 * @param pSelf Hold sensor of the ball.
 * @param pOther Sensor that was touched.
 */
void GigaBall::attackSensorHold(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isSensorMapObj(pOther)) {
        if ((al::isNerve(this, &NrvGigaBallThrow) || al::isNerve(this, &NrvGigaBallKick) ||
             al::isNerve(this, &NrvGigaBallRouteDokanThrow)) &&
            al::sendMsgBallAttack(pOther, pSelf, mComboCounter)) {
            BallStateFunction::calcReflectSpeed(this, al::getSensorHost(pOther), &sFallParam);
            al::setNerve(this, &NrvGigaBallFall);
            al::startSeWithParam(this, "PgBound", al::getVelocity(this).length());
            return;
        }

        if (al::isNerve(this, &NrvGigaBallFall) &&
            al::sendMsgBallTrample(pOther, pSelf, mComboCounter)) {
            BallStateFunction::calcReflectSpeed(this, al::getSensorHost(pOther), &sFallParam);
            al::startSeWithParam(this, "PgBound", al::getVelocity(this).length());
            return;
        }

        if (!al::isNerve(this, &NrvGigaBallWait)) {
            if (al::isNerve(this, &NrvGigaBallRouteDokan)) {
                rc::sendMsgRouteDokanItemGet(pOther, pSelf);
                return;
            }

            al::sendMsgBallItemGet(pOther, pSelf);
            return;
        }
    }

    if (!al::isNerve(this, &NrvGigaBallDRCHold) && al::isSensorHoldObj(pOther)) {
        al::sendMsgPush(pOther, pSelf);
    }
}

/**
 * @brief Reacts to attacks and to the player picking the ball up, throwing or kicking it.
 * @param pMsg Received message.
 * @param pOther Sensor that sent the message.
 * @param pSelf Sensor of the ball.
 * @return True if the message was handled.
 */
bool GigaBall::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                          al::HitSensor* pSelf) {
    if (pOther != nullptr && al::isSensorPlayer(pOther)) {
        auto* player = static_cast<PlayerActor*>(al::getSensorHost(pOther));
        if (player != nullptr && player->getPlayer() != nullptr &&
            player->getPlayer()->getGigaDirector() != nullptr &&
            !player->getPlayerGigaDirector()->isFullScale()) {
            return false;
        }
    }

    if (rc::isMsgAskControlUserId(pMsg, mHolderSensor)) {
        return true;
    }

    if (al::isNerve(this, &NrvGigaBallDamageThrow) || al::isNerve(this, &NrvGigaBallWaterBottom)) {
        return false;
    }

    if (rc::isMsgStartGoalDemoPole(pMsg)) {
        kill();
        return true;
    }

    if (al::isSensorName(pSelf, "Body")) {
        if (!al::isNerve(this, &NrvGigaBallPlayerHold) && !al::isNerve(this, &NrvGigaBallDRCHold) &&
            rc::isMsgJumpPanelAction(pMsg)) {
            al::setVelocity(this, 0.0f, 45.0f, 0.0f);
            al::setNerve(this, &NrvGigaBallFall);
            return true;
        }

        if ((al::isNerve(this, &NrvGigaBallWait) || al::isNerve(this, &NrvGigaBallRolling)) &&
            (al::isMsgPlayerObjHipDropAll(pMsg) || al::isMsgPlayerFireBallAttack(pMsg) ||
             al::isMsgPlayerBoomerangAttack(pMsg) || al::isMsgKickKouraAttack(pMsg) ||
             al::isMsgPlayerKouraAttack(pMsg) || al::isMsgBallAttack(pMsg) ||
             al::isMsgBallTrample(pMsg) || al::isMsgPlayerBodyLanding(pMsg) ||
             al::isMsgPlayerGiantHipDrop(pMsg) || al::isMsgPlayerCooperationHipDrop(pMsg) ||
             al::isMsgPlayerSlidingAttack(pMsg) || al::isMsgBlockUpperPunch(pMsg) ||
             al::isMsgExplosion(pMsg))) {
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
            al::setNerve(this, &NrvGigaBallThrow);
            al::offCollide(this);
            mIsRotateOnFall = false;
            rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
            if (al::isMsgPlayerKouraAttack(pMsg) || al::isMsgKickKouraAttack(pMsg)) {
                mAttackDisableTimer = 12;
            }

            return al::isMsgPlayerFireBallAttack(pMsg) || al::isMsgPlayerKouraAttack(pMsg);
        }

        if (rc::isMsgDossunPress(pMsg)) {
            kill();
            return true;
        }
    }

    if (al::isNerve(this, &NrvGigaBallDRCHold)) {
        if (al::isMsgWarpStart(pMsg)) {
            rc::releaseTouchPointerHoldItem(this, mTouchPointer);
            al::setNerve(this, &NrvGigaBallFall);
            mIsRotateOnFall = true;
        }

        return false;
    }

    if (!al::isSensorName(pSelf, "Hold")) {
        return false;
    }

    if (al::isNerve(this, &NrvGigaBallPlayerHold)) {
        if (!mStatePlayerHold->receiveMsg(pMsg, pOther, pSelf)) {
            return false;
        }

        if (al::isMsgPlayerRelease(pMsg)) {
            al::setNerve(this, &NrvGigaBallThrow);
            al::offCollide(this);
            mStateThrow->setThrowParam(mHolderSensor, &sThrowParam, nullptr);
            mIsRotateOnFall = false;
            return true;
        }

        if (al::isMsgPlayerReleaseDamage(pMsg) || al::isMsgPlayerReleaseDead(pMsg)) {
            al::setNerve(this, &NrvGigaBallDamageThrow);
            mStateThrow->setThrowParam(mHolderSensor, &sDamageThrowParam, nullptr);
            return true;
        }

        if (al::isMsgWarpStart(pMsg) || al::isMsgHoldCancel(pMsg)) {
            al::setNerve(this, &NrvGigaBallFall);
            mHoldDisableTimer = 12;
            mKnockDownDisableTimer = 12;
            mIsRotateOnFall = true;
            mRouteDokanDisableTimer = 75;
        }

        return true;
    }

    if ((isEnableHold(pOther) && (al::isMsgPlayerTailAttack(pMsg) ||
                                  al::isMsgPlayerClimbAttack(pMsg) ||
                                  al::isMsgPlayerSpinAttack(pMsg))) ||
        al::isMsgPlayerObjRollingAttack(pMsg) || al::isMsgPlayerGiantAttack(pMsg)) {
        mHolderSensor = pOther;
        mIsRotateOnFall = false;
        rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        mStateThrow->setThrowParam(mHolderSensor, &sThrowParam, nullptr);
        al::startSeWithParam(this, "PgBound", 500.0f);
        al::setNerve(this, &NrvGigaBallThrow);
        al::offCollide(this);
        return true;
    }

    if (isEnableHold(pOther) && mStatePlayerHold->tryStartCarryFront(pMsg, pOther, false)) {
        if (al::isNerve(this, &NrvGigaBallThrow) || al::isNerve(this, &NrvGigaBallKick) ||
            al::isNerve(this, &NrvGigaBallFall) || al::isNerve(this, &NrvGigaBallPopUpFront) ||
            al::isNerve(this, &NrvGigaBallRouteDokanThrow)) {
            al::startHitReaction(this, "ボールキャッチ");
        }

        al::onCollide(this);
        mHolderSensor = pOther;
        al::setNerve(this, &NrvGigaBallPlayerHold);
        al::setColliderRadius(this, 15.0f);
        al::offCollide(this);
        return true;
    }

    if ((al::isMsgPlayerKick(pMsg) || al::isMsgPlayerBodyAttack(pMsg)) && isEnableKick()) {
        al::offCollide(this);
        mHolderSensor = pOther;
        mIsRotateOnFall = true;
        rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        mStateThrow->setThrowParam(mHolderSensor, &sKickParam, nullptr);
        al::startSe(this, "PgKicked");
        al::setNerve(this, &NrvGigaBallKick);
        return true;
    }

    if (al::tryReceiveMsgPushAndAddVelocity(this, pMsg, pOther, pSelf, 50.0f)) {
        return true;
    }

    return false;
}

/**
 * @brief Whether the owner of a sensor may pick the ball up.
 * @param pSensor Sensor of the actor that wants to hold the ball.
 * @return True if the ball can be held.
 */
bool GigaBall::isEnableHold(al::HitSensor* pSensor) {
    if (al::isNerve(this, &NrvGigaBallRouteDokan)) {
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
 * @brief Whether the ball lies still, rolls or falls, so it can be kicked.
 * @return True if the ball can be kicked.
 */
bool GigaBall::isEnableKick() {
    return al::isNerve(this, &NrvGigaBallWait) || al::isNerve(this, &NrvGigaBallRolling) ||
           al::isNerve(this, &NrvGigaBallFall);
}

/**
 * @brief Lets the touch pointer grab the ball.
 * @param pMsg Received message.
 * @param pPointer Pointer that sent the message.
 * @param pTarget Screen point target of the ball.
 * @return True if the ball is grabbed.
 */
bool GigaBall::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                     al::ScreenPointTarget* pTarget) {
    if (al::isNerve(this, &NrvGigaBallPopUpFront) || al::isNerve(this, &NrvGigaBallPlayerHold) ||
        al::isNerve(this, &NrvGigaBallThrow) || al::isNerve(this, &NrvGigaBallKick) ||
        al::isNerve(this, &NrvGigaBallRouteDokan) ||
        al::isNerve(this, &NrvGigaBallRouteDokanThrow) ||
        al::isNerve(this, &NrvGigaBallDamageThrow) || al::isNerve(this, &NrvGigaBallWaterBottom)) {
        return false;
    }

    if (!mStateTouchCarry->receiveMsgScreenPoint(pMsg, pPointer, pTarget)) {
        return false;
    }

    if (!al::isNerve(this, &NrvGigaBallDRCHold)) {
        al::setNerve(this, &NrvGigaBallDRCHold);
        mHolderSensor = DrcFunction::tryFindDrcPlayerSensor(this, pPointer);
        mTouchPointer = DrcFunction::tryFindDrcTouchActor(this, pPointer);
    }

    return true;
}

/**
 * @brief Counts down the timers, kills the ball in kill areas and restores the collider radius.
 */
void GigaBall::control() {
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

    if (EnemyStateUtil::tryKillByAreaOrMaterialCode(this)) {
        return;
    }

    if (al::isNerve(this, &NrvGigaBallPlayerHold)) {
        return;
    }

    BallStateFunction::setColliderReturnedSlowly(this, mColliderRadius, 20);
}

/**
 * @brief Releases the ball from its holder and kills it.
 */
void GigaBall::kill() {
    if (al::isNerve(this, &NrvGigaBallRouteDokan)) {
        al::startHitReaction(this, "ルート土管消滅");
    } else {
        if (al::isNerve(this, &NrvGigaBallPlayerHold)) {
            rc::requestPlayerRelease(mHolderSensor);
        } else if (al::isNerve(this, &NrvGigaBallDRCHold)) {
            rc::releaseTouchPointerHoldItem(this, mTouchPointer);
        }

        al::startHitReactionDisappear(this);
    }

    al::LiveActor::kill();
}

/**
 * @brief Updates the collider, letting the holder state drive it while the ball is carried.
 */
void GigaBall::updateCollider() {
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
void GigaBall::appearPopUpFront() {
    al::invalidateHitSensors(this);
    al::LiveActor::appear();
    mStatePopUpFront->setParamBallPopUp();
    al::setNerve(this, &NrvGigaBallPopUpFront);
}

/**
 * @brief Appears and pops the ball straight up.
 */
void GigaBall::appearAbove() {
    al::invalidateHitSensors(this);
    al::LiveActor::appear();
    mStatePopUpFront->setParamBallAbove();
    al::setNerve(this, &NrvGigaBallPopUpFront);
}

/**
 * @brief Puts the ball back to rest.
 */
void GigaBall::reset() {
    al::onCollide(this);
    al::setNerve(this, &NrvGigaBallWait);
    al::setVelocityZero(this);
}

/**
 * @brief Whether a player carries the ball.
 * @return True while held by a player.
 */
bool GigaBall::isPlayerHold() const {
    return al::isNerve(this, &NrvGigaBallPlayerHold);
}

/**
 * @brief Lies on the ground, falling or rolling away once the ground is gone or sloped.
 */
void GigaBall::exeWait() {
    if (al::isFirstStep(this)) {
        al::validateClipping(this);
        mComboCounter->reset();
    }

    al::addVelocityToGravity(this, 5.0f);
    if (!al::isOnGround(this, 8, 0.0f) && !al::isLessEqualStep(this, 5)) {
        al::invalidateClipping(this);
        al::setNerve(this, &NrvGigaBallFall);
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
    al::setNerve(this, &NrvGigaBallRolling);
}

/**
 * @brief Carried by a player, thrown away once the player is no longer fully giant.
 */
void GigaBall::exePlayerHold() {
    if (al::isFirstStep(this)) {
        al::setVelocityZero(this);
        al::startSe(this, "PgHoldStart");
    } else if (al::isStep(this, 3)) {
        al::onCollide(this);
    }

    al::updateNerveState(this);
    if (mHolderSensor != nullptr && al::isSensorPlayer(mHolderSensor)) {
        auto* player = static_cast<PlayerActor*>(al::getSensorHost(mHolderSensor));
        if (player != nullptr && player->getPlayer() != nullptr &&
            player->getPlayer()->getGigaDirector() != nullptr &&
            !player->getPlayerGigaDirector()->isFullScale()) {
            requestRelease(mHolderSensor, &sThrowParam, &NrvGigaBallThrow);
        }
    }
}

/**
 * @brief Carried by the touch pointer, thrown or dropped when released.
 */
void GigaBall::exeDRCHold() {
    if (al::isStep(this, 1)) {
        al::startHitReaction(this, "DRCつかむ");
    }

    if (!al::updateNerveState(this)) {
        return;
    }

    if (mStateTouchCarry->isItemThrow()) {
        mStateThrow->setThrowParam(nullptr, &sThrowParam, mTouchPointer);
        al::setVelocity(this, mStateTouchCarry->getThrowVelocity());
        al::setNerve(this, &NrvGigaBallThrow);
        al::offCollide(this);
        return;
    }

    al::setVelocity(this, mStateTouchCarry->getReleaseVelocity());
    al::setNerve(this, &NrvGigaBallFall);
    al::startHitReaction(this, "DRC放す");
}

/**
 * @brief Flies after being thrown.
 */
void GigaBall::exeThrow() {
    if (al::isFirstStep(this)) {
        mWallCollideCount = 0;
        mHoldDisableTimer = 12;
        mKnockDownDisableTimer = 12;
        al::invalidateClipping(this);
        al::startSe(this, "PgThrow");
    } else {
        startEffect();
        if (al::isStep(this, 1)) {
            al::onCollide(this);
        }
    }

    countWallCollide();
    if (al::updateNerveState(this)) {
        al::setNerve(this, &NrvGigaBallFall);
    }
}

/**
 * @brief Emits the collision effect where the ball hits something.
 */
void GigaBall::startEffect() {
    rc::startHitReactionIfThroughWater(this);

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
    al::startSeWithParam(this, "PgBound", al::getVelocity(this).length());
}

/**
 * @brief Counts the frames in which the ball hits a wall.
 */
void GigaBall::countWallCollide() {
    if (al::isCollidedWallVelocity(this)) {
        mWallCollideCount++;
    }
}

/**
 * @brief Flies after being kicked.
 */
void GigaBall::exeKick() {
    if (al::isFirstStep(this)) {
        mWallCollideCount = 0;
        mHoldDisableTimer = 12;
        mKnockDownDisableTimer = 12;
        al::invalidateClipping(this);
    } else {
        startEffect();
        if (al::isStep(this, 1)) {
            al::onCollide(this);
        }
    }

    countWallCollide();
    if (al::updateNerveState(this)) {
        al::setNerve(this, &NrvGigaBallFall);
    }
}

/**
 * @brief Falls and bounces until the ball starts rolling.
 */
void GigaBall::exeFall() {
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
        al::setNerve(this, &NrvGigaBallRolling);
    }
}

/**
 * @brief Rolls on the ground until it stops, sinking if it stops in water.
 */
void GigaBall::exeRolling() {
    if (!al::updateNerveState(this)) {
        return;
    }

    if (mStateRolling->isOnAir()) {
        al::setNerve(this, &NrvGigaBallFall);
        return;
    }

    if (rc::isInWaterArea(this)) {
        al::setNerve(this, &NrvGigaBallWaterBottom);
        return;
    }

    al::setNerve(this, &NrvGigaBallWait);
}

/**
 * @brief Flies away after hurting its holder, vanishing once it lands or hits something.
 */
void GigaBall::exeDamageThrow() {
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
void GigaBall::exeWaterBottom() {
    if (al::isGreaterEqualStep(this, 30)) {
        al::startHitReaction(this, "水底消滅");
        al::LiveActor::kill();
    }
}

/**
 * @brief Pops up out of a block.
 */
void GigaBall::exePopUpFront() {
    if (al::updateNerveStateAndNextNerve(this, &NrvGigaBallFall)) {
        al::invalidateClipping(this);
        return;
    }

    rc::startHitReactionIfThroughWater(this);
}

/**
 * @brief Travels through a route pipe.
 */
void GigaBall::exeRouteDokan() {
    if (al::isFirstStep(this)) {
        al::setVelocityZero(this);
        mStateRouteDokan->setMoveSpeed(18.0f);
    }

    sead::Quatf* quat = al::getQuatPtr(this);
    al::turnQuatZDirRadian(quat, *quat, mStateRouteDokan->getMoveDirection(), sead::Mathf::pi());
    if (al::updateNerveState(this)) {
        BallStateFunction::calcLaunchSpeed(this, mStateRouteDokan->getMoveDirection(),
                                           &sRouteDokanThrowParam);
        mStateThrow->setThrowParam(mHolderSensor, &sRouteDokanThrowParam, nullptr);
        al::setNerve(this, &NrvGigaBallRouteDokanThrow);
    }
}

/**
 * @brief Flies out of the exit of a route pipe.
 */
void GigaBall::exeRouteDokanThrow() {
    if (al::isFirstStep(this)) {
        al::onCollide(this);
        mWallCollideCount = 0;
    } else {
        startEffect();
    }

    countWallCollide();
    if (al::updateNerveState(this)) {
        al::setNerve(this, &NrvGigaBallFall);
    }
}

/**
 * @brief Whether the ball flying into a player knocks that player down.
 * @param pPlayer Sensor of the player.
 * @param pSelf Sensor of the ball.
 * @return True if the ball moves towards the player.
 */
bool GigaBall::isEnablePlayerKnockDown(al::HitSensor* pPlayer, al::HitSensor* pSelf) {
    if (!al::isNerve(this, &NrvGigaBallThrow) && !al::isNerve(this, &NrvGigaBallKick) &&
        !al::isNerve(this, &NrvGigaBallRouteDokanThrow) && !al::isNerve(this, &NrvGigaBallFall)) {
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
