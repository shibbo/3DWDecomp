#include "MapObj/GigaRock.hpp"

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
NERVE_DECL(GigaRock, Wait)
NERVE_DECL(GigaRock, DRCHold)
NERVE_DECL(GigaRock, PlayerHold)
NERVE_DECL(GigaRock, Fall)
NERVE_DECL(GigaRock, Rolling)
NERVE_DECL(GigaRock, Throw)
NERVE_DECL(GigaRock, Kick)
NERVE_DECL(GigaRock, DamageThrow)
NERVE_DECL(GigaRock, RouteDokan)
NERVE_DECL(GigaRock, RouteDokanThrow)
NERVE_DECL(GigaRock, WaterBottom)

// Non-const nerve objects: the game keeps them in .data, merged into one block.
GigaRockNrvWait NrvGigaRockWait;
GigaRockNrvDRCHold NrvGigaRockDRCHold;
GigaRockNrvPlayerHold NrvGigaRockPlayerHold;
GigaRockNrvFall NrvGigaRockFall;
GigaRockNrvRolling NrvGigaRockRolling;
GigaRockNrvThrow NrvGigaRockThrow;
GigaRockNrvKick NrvGigaRockKick;
GigaRockNrvDamageThrow NrvGigaRockDamageThrow;
GigaRockNrvRouteDokan NrvGigaRockRouteDokan;
GigaRockNrvRouteDokanThrow NrvGigaRockRouteDokanThrow;
GigaRockNrvWaterBottom NrvGigaRockWaterBottom;

typedef al::FunctorV0M<GigaRock*, void (GigaRock::*)()> GigaRockFunctor;

ItemStatePlayerHoldParam sPlayerHoldParam(
    sead::Vector3f(10.0f, 0.0f, 40.0f), sead::Vector3f(5.0f, 0.0f, 35.0f),
    sead::Vector3f(5.0f, 0.0f, 25.0f), sead::Vector3f(25.0f, 0.0f, 45.0f),
    sead::Vector3f(5.0f, 0.0f, 25.0f), sead::Vector3f(30.0f, 0.0f, 45.0f),
    sead::Vector3f(30.0f, 0.0f, 45.0f), sead::Vector3f(30.0f, 0.0f, 35.0f),
    sead::Vector3f(35.0f, 0.0f, 45.0f), sead::Vector3f(40.0f, 0.0f, 30.0f),
    sead::Vector3f(0.0f, 0.0f, 30.0f));
BallStateFallParam sFallParam(28.0f, 11.0f, 0.6f, 0.3f, 0.1f, 50.0f, 2.0f, 15.0f, 0.5f, 0.996f,
                              0.65f, 0.9f);
BallStateRollingParam sRollingParam;
BallStateThrowParam sThrowParam(375.0f, 300.0f, 9.0f, 30.0f, 0.5f, 0.996f, 0.65f, 2.0f, 45, false,
                                false);
BallStateThrowParam sKickParam(300.0f, 300.0f, 20.0f, 20.0f, 0.5f, 0.996f, 0.65f, 2.0f, -1, true,
                               false);
BallStateThrowParam sHitParam(150.0f, 120.0f, 20.0f, 20.0f, 0.5f, 0.996f, 0.65f, 2.0f, -1, false,
                              true);
BallStateThrowParam sDamageThrowParam(300.0f, 120.0f, 9.0f, 30.0f, 0.5f, 0.996f, 0.65f, 2.0f, 45,
                                      true, true);
BallStateThrowParam sRouteDokanThrowParam(150.0f, 120.0f, 20.0f, 20.0f, 0.5f, 0.996f, 0.65f, 2.0f,
                                          -1, false, true);
}  // namespace

/**
 * @brief Constructs the giga rock.
 * @param pName Actor name.
 */
GigaRock::GigaRock(const char* pName) : al::LiveActor(pName) {}

/**
 * @brief Initializes the model, the nerve states and the effect matrix.
 * @param rInfo Placement info of the actor.
 */
void GigaRock::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "GigaRock", nullptr);
    al::initNerve(this, &NrvGigaRockWait, 10);

    mStateTouchCarry = new TouchCarryItemState(this, nullptr);
    mStatePlayerHold = new ItemStateGigaPlayerHold(this, &sPlayerHoldParam, false, false);
    mStateFall = new BallStateFall(this, &sFallParam);
    mStateRolling = new BallStateRolling(this, &sRollingParam);
    mStateThrow = new BallStateThrow(this, &sThrowParam);
    mStateRouteDokan = new ActorStateRouteDokanMove(this, rInfo);

    al::initNerveState(this, mStateTouchCarry, &NrvGigaRockDRCHold, "[state]アイテム持ち運び");
    al::initNerveState(this, mStatePlayerHold, &NrvGigaRockPlayerHold,
                       "[state]プレイヤーに持たれる");
    al::initNerveState(this, mStateFall, &NrvGigaRockFall, "[state]落下");
    al::initNerveState(this, mStateRolling, &NrvGigaRockRolling, "[state]転がる");
    al::initNerveState(this, mStateThrow, &NrvGigaRockThrow, "[state]投げ");
    al::initNerveState(this, mStateThrow, &NrvGigaRockKick, "[state]蹴り");
    al::initNerveState(this, mStateThrow, &NrvGigaRockDamageThrow, "[state]ダメージ時の投げ");
    al::initNerveState(this, mStateRouteDokan, &NrvGigaRockRouteDokan, "[state]ルート土管移動");
    al::initNerveState(this, mStateThrow, &NrvGigaRockRouteDokanThrow,
                       "[state]ルート土管出口時の投げ");
    mStatePlayerHold->initColliderControl();
    mColliderRadius = al::getColliderRadius(this);
    makeActorAppeared();

    al::listenStageSwitchOnKill(this, GigaRockFunctor(this, &GigaRock::kill));
    al::setEffectFollowMtxPtr(this, "HitCollision", &mEffectMtx);
    al::createAndSetColliderSpecialPurpose(this, "BallMoveLimit");
}

/**
 * @brief Dispatches the attack to the handler of the sensor that touched something.
 * @param pSelf Sensor of the rock.
 * @param pOther Sensor that was touched.
 */
void GigaRock::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isNerve(this, &NrvGigaRockDamageThrow) || al::isNerve(this, &NrvGigaRockWaterBottom)) {
        return;
    }

    if (al::isSensorName(pSelf, "Body")) {
        attackSensorBody(pSelf, pOther);
        return;
    }

    if (al::isSensorName(pSelf, "Hold")) {
        attackSensorHold(pSelf, pOther);
    }
}

/**
 * @brief Tramples enemies, pushes players and enters route pipes with the body sensor.
 * @param pSelf Body sensor of the rock.
 * @param pOther Sensor that was touched.
 */
void GigaRock::attackSensorBody(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isSensorEnemyBody(pOther) || al::isSensorKickKoura(pOther)) {
        if (al::isNerve(this, &NrvGigaRockFall) && al::getVelocity(this).y < -10.0f &&
            al::sendMsgBallTrample(pOther, pSelf, mComboCounter)) {
            BallStateFunction::calcReflectSpeed(this, nullptr, &sFallParam);
            al::startSeWithParam(this, "PgBound", al::getVelocity(this).length());
            return;
        }

        if (al::isNerve(this, &NrvGigaRockRouteDokan)) {
            if (al::sendMsgBallRouteDokanAttack(pOther, pSelf, mComboCounter)) {
                kill();
                return;
            }

            if (al::sendMsgVanish(pOther, pSelf)) {
                return;
            }
        }

        al::sendMsgPush(pOther, pSelf);
    }

    if (al::isNerve(this, &NrvGigaRockDRCHold)) {
        return;
    }

    if (!al::isNerve(this, &NrvGigaRockRouteDokanThrow) &&
        !al::isNerve(this, &NrvGigaRockRouteDokan) && !al::isNerve(this, &NrvGigaRockWait)) {
        if (al::isNerve(this, &NrvGigaRockFall) && mRouteDokanDisableTimer > 0) {
            return;
        }

        if (mStateRouteDokan->tryStart(pSelf, pOther)) {
            al::offCollide(this);
            al::setNerve(this, &NrvGigaRockRouteDokan);
            return;
        }
    }

    if (al::isNerve(this, &NrvGigaRockPlayerHold)) {
        return;
    }

    if (al::isSensorPlayer(pOther)) {
        al::sendMsgPushVeryStrong(pOther, pSelf);
    }
}

/**
 * @brief Tramples map objects and gives the rock to item collectors with the hold sensor.
 * @param pSelf Hold sensor of the rock.
 * @param pOther Sensor that was touched.
 */
void GigaRock::attackSensorHold(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isSensorMapObj(pOther)) {
        if (al::isNerve(this, &NrvGigaRockFall) &&
            al::sendMsgBallTrample(pOther, pSelf, mComboCounter)) {
            BallStateFunction::calcReflectSpeed(this, al::getSensorHost(pOther), &sFallParam);
            al::startSeWithParam(this, "PgBound", al::getVelocity(this).length());
            return;
        }

        if (!al::isNerve(this, &NrvGigaRockWait)) {
            if (al::isNerve(this, &NrvGigaRockRouteDokan)) {
                rc::sendMsgRouteDokanItemGet(pOther, pSelf);
                return;
            }

            al::sendMsgBallItemGet(pOther, pSelf);
            return;
        }
    }

    if (!al::isNerve(this, &NrvGigaRockDRCHold) && al::isSensorHoldObj(pOther)) {
        al::sendMsgPush(pOther, pSelf);
    }
}

/**
 * @brief Reacts to attacks and to the player picking the rock up, throwing or kicking it.
 * @param pMsg Received message.
 * @param pOther Sensor that sent the message.
 * @param pSelf Sensor of the rock.
 * @return True if the message was handled.
 */
bool GigaRock::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
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

    if (al::isNerve(this, &NrvGigaRockDamageThrow) || al::isNerve(this, &NrvGigaRockWaterBottom)) {
        return false;
    }

    if (rc::isMsgStartGoalDemoPole(pMsg)) {
        kill();
        return true;
    }

    if (al::isSensorName(pSelf, "Body")) {
        if (!al::isNerve(this, &NrvGigaRockPlayerHold) && !al::isNerve(this, &NrvGigaRockDRCHold) &&
            rc::isMsgJumpPanelAction(pMsg)) {
            al::setVelocity(this, 0.0f, 45.0f, 0.0f);
            al::setNerve(this, &NrvGigaRockFall);
            return true;
        }

        if ((al::isNerve(this, &NrvGigaRockWait) || al::isNerve(this, &NrvGigaRockRolling)) &&
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
            al::setNerve(this, &NrvGigaRockThrow);
            al::offCollide(this);
            mIsRotateOnFall = false;
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

    if (al::isNerve(this, &NrvGigaRockDRCHold)) {
        if (al::isMsgWarpStart(pMsg)) {
            rc::releaseTouchPointerHoldItem(this, mTouchPointer);
            al::setNerve(this, &NrvGigaRockFall);
            mIsRotateOnFall = true;
        }

        return false;
    }

    if (!al::isSensorName(pSelf, "Hold")) {
        return false;
    }

    if (al::isNerve(this, &NrvGigaRockPlayerHold)) {
        if (!mStatePlayerHold->receiveMsg(pMsg, pOther, pSelf)) {
            return false;
        }

        if (al::isMsgPlayerRelease(pMsg)) {
            al::setNerve(this, &NrvGigaRockThrow);
            al::offCollide(this);
            mStateThrow->setThrowParam(mHolderSensor, &sThrowParam, nullptr);
            mIsRotateOnFall = false;
            return true;
        }

        if (al::isMsgPlayerReleaseDamage(pMsg) || al::isMsgPlayerReleaseDead(pMsg)) {
            al::setNerve(this, &NrvGigaRockDamageThrow);
            mStateThrow->setThrowParam(mHolderSensor, &sDamageThrowParam, nullptr);
            return true;
        }

        if (al::isMsgWarpStart(pMsg) || al::isMsgHoldCancel(pMsg)) {
            al::setNerve(this, &NrvGigaRockFall);
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
        mStateThrow->setThrowParam(mHolderSensor, &sThrowParam, nullptr);
        al::startSeWithParam(this, "PgBound", 500.0f);
        al::setNerve(this, &NrvGigaRockThrow);
        al::offCollide(this);
        return true;
    }

    if (isEnableHold(pOther) && mStatePlayerHold->tryStartCarryFront(pMsg, pOther, false)) {
        // The nerve checks survive, but the rock has no catch reaction to start.
        if (al::isNerve(this, &NrvGigaRockThrow) || al::isNerve(this, &NrvGigaRockKick) ||
            al::isNerve(this, &NrvGigaRockFall) || al::isNerve(this, &NrvGigaRockRouteDokanThrow)) {
        }

        al::onCollide(this);
        mHolderSensor = pOther;
        al::setNerve(this, &NrvGigaRockPlayerHold);
        al::setColliderRadius(this, 15.0f);
        al::offCollide(this);
        return true;
    }

    if ((al::isMsgPlayerKick(pMsg) || al::isMsgPlayerBodyAttack(pMsg)) && isEnableKick()) {
        al::offCollide(this);
        mHolderSensor = pOther;
        mIsRotateOnFall = true;
        mStateThrow->setThrowParam(mHolderSensor, &sKickParam, nullptr);
        al::startSe(this, "PgKicked");
        al::setNerve(this, &NrvGigaRockKick);
        return true;
    }

    if (al::tryReceiveMsgPushAndAddVelocity(this, pMsg, pOther, pSelf, 50.0f)) {
        return true;
    }

    return false;
}

/**
 * @brief Whether the owner of a sensor may pick the rock up.
 * @param pSensor Sensor of the actor that wants to hold the rock.
 * @return True if the rock can be held.
 */
bool GigaRock::isEnableHold(al::HitSensor* pSensor) {
    if (al::isNerve(this, &NrvGigaRockRouteDokan)) {
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
 * @brief Whether the rock lies still, rolls or falls, so it can be kicked.
 * @return True if the rock can be kicked.
 */
bool GigaRock::isEnableKick() {
    return al::isNerve(this, &NrvGigaRockWait) || al::isNerve(this, &NrvGigaRockRolling) ||
           al::isNerve(this, &NrvGigaRockFall);
}

/**
 * @brief Lets the touch pointer grab the rock.
 * @param pMsg Received message.
 * @param pPointer Pointer that sent the message.
 * @param pTarget Screen point target of the rock.
 * @return True if the rock is grabbed.
 */
bool GigaRock::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                     al::ScreenPointTarget* pTarget) {
    if (al::isNerve(this, &NrvGigaRockPlayerHold) || al::isNerve(this, &NrvGigaRockThrow) ||
        al::isNerve(this, &NrvGigaRockKick) || al::isNerve(this, &NrvGigaRockRouteDokan) ||
        al::isNerve(this, &NrvGigaRockRouteDokanThrow) ||
        al::isNerve(this, &NrvGigaRockDamageThrow) || al::isNerve(this, &NrvGigaRockWaterBottom)) {
        return false;
    }

    if (!mStateTouchCarry->receiveMsgScreenPoint(pMsg, pPointer, pTarget)) {
        return false;
    }

    if (!al::isNerve(this, &NrvGigaRockDRCHold)) {
        al::setNerve(this, &NrvGigaRockDRCHold);
        mHolderSensor = DrcFunction::tryFindDrcPlayerSensor(this, pPointer);
        mTouchPointer = DrcFunction::tryFindDrcTouchActor(this, pPointer);
    }

    return true;
}

/**
 * @brief Counts down the timers, kills the rock in kill areas and restores the collider radius.
 */
void GigaRock::control() {
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

    if (al::isNerve(this, &NrvGigaRockPlayerHold)) {
        return;
    }

    BallStateFunction::setColliderReturnedSlowly(this, mColliderRadius, 20);
}

/**
 * @brief Releases the rock from its holder and kills it.
 */
void GigaRock::kill() {
    if (!al::isNerve(this, &NrvGigaRockRouteDokan)) {
        if (al::isNerve(this, &NrvGigaRockPlayerHold)) {
            rc::requestPlayerRelease(mHolderSensor);
        } else if (al::isNerve(this, &NrvGigaRockDRCHold)) {
            rc::releaseTouchPointerHoldItem(this, mTouchPointer);
        }
    }

    al::LiveActor::kill();
}

/**
 * @brief Updates the collider, letting the holder state drive it while the rock is carried.
 */
void GigaRock::updateCollider() {
    ItemStatePlayerHold* state = mStatePlayerHold;
    if (state->isDead()) {
        al::LiveActor::updateCollider();
        return;
    }

    state->updateCollider(al::getHitSensor(this, "Body"));
}

/**
 * @brief Appears with the hit sensors disabled.
 */
void GigaRock::appearPopUpFront() {
    al::invalidateHitSensors(this);
    al::LiveActor::appear();
}

/**
 * @brief Appears with the hit sensors disabled.
 */
void GigaRock::appearAbove() {
    al::invalidateHitSensors(this);
    al::LiveActor::appear();
}

/**
 * @brief Puts the rock back to rest.
 */
void GigaRock::reset() {
    al::onCollide(this);
    al::setNerve(this, &NrvGigaRockWait);
    al::setVelocityZero(this);
}

/**
 * @brief Whether a player carries the rock.
 * @return True while held by a player.
 */
bool GigaRock::isPlayerHold() const {
    return al::isNerve(this, &NrvGigaRockPlayerHold);
}

/**
 * @brief Lies on the ground, falling or rolling away once the ground is gone or sloped.
 */
void GigaRock::exeWait() {
    if (al::isFirstStep(this)) {
        al::validateClipping(this);
        mComboCounter->reset();
    }

    al::addVelocityToGravity(this, 20.0f);
    if (!al::isOnGround(this, 8, 0.0f) && !al::isLessEqualStep(this, 5)) {
        al::invalidateClipping(this);
        al::setNerve(this, &NrvGigaRockFall);
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
    al::setNerve(this, &NrvGigaRockRolling);
}

/**
 * @brief Carried by a player, thrown away once the player is no longer fully giant.
 */
void GigaRock::exePlayerHold() {
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
            requestRelease(mHolderSensor, &sThrowParam, &NrvGigaRockThrow);
        }
    }
}

/**
 * @brief Makes the holding player release the rock and throws it away.
 * @param pOther Sensor that caused the release.
 * @param pParam Throw parameters.
 * @param pNerve Nerve to continue with.
 */
void GigaRock::requestRelease(al::HitSensor* pOther, BallStateThrowParam* pParam,
                              const al::Nerve* pNerve) {
    rc::requestPlayerRelease(mHolderSensor);
    BallStateFunction::calcReflectSpeed(this, al::getSensorHost(pOther), &sFallParam);
    mStateThrow->setThrowParam(mHolderSensor, pParam, nullptr);
    al::setNerve(this, pNerve);
    al::startSeWithParam(this, "PgBound", al::getVelocity(this).length());
    al::offCollide(this);
}

/**
 * @brief Carried by the touch pointer, thrown or dropped when released.
 */
void GigaRock::exeDRCHold() {
    // The step check is still evaluated, but the rock has no grab reaction to start.
    if (al::isStep(this, 1)) {
    }

    if (!al::updateNerveState(this)) {
        return;
    }

    if (mStateTouchCarry->isItemThrow()) {
        mStateThrow->setThrowParam(nullptr, &sThrowParam, mTouchPointer);
        al::setVelocity(this, mStateTouchCarry->getThrowVelocity());
        al::setNerve(this, &NrvGigaRockThrow);
        al::offCollide(this);
        return;
    }

    al::setVelocity(this, mStateTouchCarry->getReleaseVelocity());
    al::setNerve(this, &NrvGigaRockFall);
}

/**
 * @brief Flies after being thrown.
 */
void GigaRock::exeThrow() {
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
        al::setNerve(this, &NrvGigaRockFall);
    }
}

/**
 * @brief Emits the collision effect where the rock hits something.
 */
void GigaRock::startEffect() {
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
    al::startHitReaction(this, "HitCollision");
    al::startSeWithParam(this, "PgBound", al::getVelocity(this).length());
}

/**
 * @brief Counts the frames in which the rock hits a wall.
 */
void GigaRock::countWallCollide() {
    if (al::isCollidedWallVelocity(this)) {
        mWallCollideCount++;
    }
}

/**
 * @brief Flies after being kicked.
 */
void GigaRock::exeKick() {
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
        al::setNerve(this, &NrvGigaRockFall);
    }
}

/**
 * @brief Falls and bounces until the rock starts rolling.
 */
void GigaRock::exeFall() {
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
        al::setNerve(this, &NrvGigaRockRolling);
    }
}

/**
 * @brief Rolls on the ground until it stops, sinking if it stops in water.
 */
void GigaRock::exeRolling() {
    if (!al::updateNerveState(this)) {
        return;
    }

    if (mStateRolling->isOnAir()) {
        al::setNerve(this, &NrvGigaRockFall);
        return;
    }

    if (rc::isInWaterArea(this)) {
        al::setNerve(this, &NrvGigaRockWaterBottom);
        return;
    }

    al::setNerve(this, &NrvGigaRockWait);
}

/**
 * @brief Flies away after hurting its holder, vanishing once it lands or hits something.
 */
void GigaRock::exeDamageThrow() {
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
void GigaRock::exeWaterBottom() {
    if (al::isGreaterEqualStep(this, 30)) {
        al::LiveActor::kill();
    }
}

/**
 * @brief Pops up out of a block (unused: the rock has no pop-up nerve).
 */
void GigaRock::exePopUpFront() {
    if (al::updateNerveStateAndNextNerve(this, &NrvGigaRockFall)) {
        al::invalidateClipping(this);
    }
}

/**
 * @brief Travels through a route pipe.
 */
void GigaRock::exeRouteDokan() {
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
        al::setNerve(this, &NrvGigaRockRouteDokanThrow);
    }
}

/**
 * @brief Flies out of the exit of a route pipe.
 */
void GigaRock::exeRouteDokanThrow() {
    if (al::isFirstStep(this)) {
        al::onCollide(this);
        mWallCollideCount = 0;
    } else {
        startEffect();
    }

    countWallCollide();
    if (al::updateNerveState(this)) {
        al::setNerve(this, &NrvGigaRockFall);
    }
}

/**
 * @brief Whether the rock flying into a player knocks that player down.
 * @param pPlayer Sensor of the player.
 * @param pSelf Sensor of the rock.
 * @return True if the rock moves towards the player.
 */
bool GigaRock::isEnablePlayerKnockDown(al::HitSensor* pPlayer, al::HitSensor* pSelf) {
    if (!al::isNerve(this, &NrvGigaRockThrow) && !al::isNerve(this, &NrvGigaRockKick) &&
        !al::isNerve(this, &NrvGigaRockRouteDokanThrow) && !al::isNerve(this, &NrvGigaRockFall)) {
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
