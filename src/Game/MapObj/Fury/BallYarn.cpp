#include "MapObj/Fury/BallYarn.hpp"

#include <math/seadQuat.h>

#include "Library/Actor/ComboCounter.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Thread/Functor.hpp"
#include "Project/Base/StringUtil.hpp"
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
#include "NPC/NpcFunction.hpp"
#include "Player/Normal/PlayerActor.hpp"
#include "Player/Normal/PlayerGigaDirector.hpp"
#include "Player/Player.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/DrcUtil.hpp"
#include "Util/InkUtil.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"

namespace {
NERVE_DECL(BallYarn, Wait)
NERVE_DECL(BallYarn, PopUpFront)
NERVE_DECL(BallYarn, DRCHold)
NERVE_DECL(BallYarn, PlayerHold)
NERVE_DECL(BallYarn, Fall)
NERVE_DECL(BallYarn, Rolling)
NERVE_DECL(BallYarn, Throw)
NERVE_DECL(BallYarn, Kick)
NERVE_DECL(BallYarn, DamageThrow)
NERVE_DECL(BallYarn, RouteDokan)
NERVE_DECL(BallYarn, RouteDokanThrow)
NERVE_DECL(BallYarn, Respawn)

// Non-const nerve objects: the game keeps them in .data, merged into one block.
BallYarnNrvWait NrvBallYarnWait;
BallYarnNrvPopUpFront NrvBallYarnPopUpFront;
BallYarnNrvDRCHold NrvBallYarnDRCHold;
BallYarnNrvPlayerHold NrvBallYarnPlayerHold;
BallYarnNrvFall NrvBallYarnFall;
BallYarnNrvRolling NrvBallYarnRolling;
BallYarnNrvThrow NrvBallYarnThrow;
BallYarnNrvKick NrvBallYarnKick;
BallYarnNrvDamageThrow NrvBallYarnDamageThrow;
BallYarnNrvRouteDokan NrvBallYarnRouteDokan;
BallYarnNrvRouteDokanThrow NrvBallYarnRouteDokanThrow;
BallYarnNrvRespawn NrvBallYarnRespawn;

typedef al::FunctorV0M<BallYarn*, void (BallYarn::*)()> BallYarnFunctor;

const sead::Vector3f sRespawnEffectOffset(0.0f, 35.0f, 0.0f);
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
BallStateThrowParam sNekoAttackParam(0.0f, 8.0f, 1.0f, 1.0f, 0.5f, 0.996f, 0.65f, 8.0f, -1, false,
                                     true);
}  // namespace

/**
 * @brief Constructs the ball of yarn. The given name is replaced by "BallNeko".
 * @param pName Unused actor name.
 */
BallYarn::BallYarn(const char* pName) : al::LiveActor("BallNeko") {}

/**
 * @brief Initializes the model, the nerve states and the effect matrix.
 * @param rInfo Placement info of the actor.
 */
void BallYarn::init(const al::ActorInitInfo& rInfo) {
    al::initActorChangeModel(this, rInfo);
    al::initNerve(this, &NrvBallYarnWait, 10);

    mStatePopUpFront = new ItemStatePopUpFront(this);
    mStateTouchCarry = new TouchCarryItemState(this, nullptr);
    mStatePlayerHold = new ItemStatePlayerHold(this, &sPlayerHoldParam, false, false);
    mStateFall = new BallStateFall(this, &sFallParam);
    mStateRolling = new BallStateRolling(this, &sRollingParam);
    mStateThrow = new BallStateThrow(this, &sThrowParam);
    mStateRouteDokan = new ActorStateRouteDokanMove(this, rInfo);

    al::initNerveState(this, mStatePopUpFront, &NrvBallYarnPopUpFront, "[state]跳ね上げ(前方)");
    al::initNerveState(this, mStateTouchCarry, &NrvBallYarnDRCHold, "[state]アイテム持ち運び");
    al::initNerveState(this, mStatePlayerHold, &NrvBallYarnPlayerHold,
                       "[state]プレイヤーに持たれる");
    al::initNerveState(this, mStateFall, &NrvBallYarnFall, "[state]落下");
    al::initNerveState(this, mStateRolling, &NrvBallYarnRolling, "[state]転がる");
    al::initNerveState(this, mStateThrow, &NrvBallYarnThrow, "[state]投げ");
    al::initNerveState(this, mStateThrow, &NrvBallYarnKick, "[state]蹴り");
    al::initNerveState(this, mStateThrow, &NrvBallYarnDamageThrow, "[state]ダメージ時の投げ");
    al::initNerveState(this, mStateRouteDokan, &NrvBallYarnRouteDokan, "[state]ルート土管移動");
    al::initNerveState(this, mStateThrow, &NrvBallYarnRouteDokanThrow,
                       "[state]ルート土管出口時の投げ");
    mStatePlayerHold->initColliderControl();
    mColliderRadius = al::getColliderRadius(this);
    makeActorAppeared();

    al::listenStageSwitchOnKill(this, BallYarnFunctor(this, &BallYarn::kill));
    al::setEffectFollowMtxPtr(this, "HitCollision", &mEffectMtx);
    al::createAndSetColliderSpecialPurpose(this, "BallMoveLimit");
    al::validateCeilWallFloorMaterialCode(this);
    mRespawnPos.set(al::getTrans(this));
}

/**
 * @brief Attacks enemies, map objects and players that the ball touches.
 * @param pSelf Sensor of the ball.
 * @param pOther Sensor that was touched.
 */
void BallYarn::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isNerve(this, &NrvBallYarnDamageThrow) || al::isNerve(this, &NrvBallYarnRespawn)) {
        return;
    }

    if (al::isSensorName(pSelf, "Body")) {
        if (al::isSensorEnemyBody(pOther) || al::isSensorKickKoura(pOther) ||
            al::isSensorNpc(pOther) || al::isSensorRide(pOther) || al::isSensorEnemy(pOther) ||
            al::isSensorKoopaJr(pOther)) {
            if (al::isNerve(this, &NrvBallYarnFall) && al::getVelocity(this).y < -10.0f &&
                (!mIsNekoThrow ||
                 (!npc::isSensorNeko(pOther) && !npc::isSensorNekoDisaster(pOther))) &&
                al::sendMsgBallTrample(pOther, pSelf, mComboCounter)) {
                BallStateFunction::calcReflectSpeed(this, nullptr, &sFallParam);
                al::startSeWithParam(this, "PgBound", al::getVelocity(this).length());
                return;
            }

            if (al::isNerve(this, &NrvBallYarnRouteDokan)) {
                if (al::sendMsgBallRouteDokanAttack(pOther, pSelf, mComboCounter)) {
                    kill();
                    return;
                }

                if (al::sendMsgVanish(pOther, pSelf)) {
                    return;
                }
            }

            if (al::isNerve(this, &NrvBallYarnPlayerHold)) {
                if (!al::sendMsgBallAttackHold(pOther, pSelf)) {
                    return;
                }

                rc::requestPlayerRelease(mHolderSensor);
                BallStateFunction::calcReflectSpeed(this, al::getSensorHost(pOther), &sFallParam);
                mStateThrow->setThrowParam(mHolderSensor, &sDamageThrowParam, nullptr);
                al::setNerve(this, &NrvBallYarnDamageThrow);
                if (al::isSensorHostName(pOther, "KoopaJr") && al::isSensorName(pOther, "Attack")) {
                    al::startSe(this, "PgHit");
                    return;
                }

                al::startSeWithParam(this, "PgBound", al::getVelocity(this).length());
                return;
            }

            if (al::isNerve(this, &NrvBallYarnDRCHold)) {
                if (!al::isGreaterEqualStep(this, 25)) {
                    return;
                }

                if (!al::sendMsgBallAttackDRCHold(pOther, pSelf)) {
                    return;
                }

                rc::releaseTouchPointerHoldItem(this, mTouchPointer);
                BallStateFunction::calcReflectSpeed(this, al::getSensorHost(pOther), &sFallParam);
                mStateThrow->setThrowParam(mHolderSensor, &sDamageThrowParam, mTouchPointer);
                al::setNerve(this, &NrvBallYarnDamageThrow);
                al::startSeWithParam(this, "PgBound", al::getVelocity(this).length());
                return;
            }

            if (al::isSensorRide(pOther)) {
                return;
            }

            al::sendMsgPush(pOther, pSelf);
        }

        if (al::isNerve(this, &NrvBallYarnDRCHold)) {
            return;
        }

        if (!al::isNerve(this, &NrvBallYarnRouteDokanThrow) &&
            !al::isNerve(this, &NrvBallYarnRouteDokan) && !al::isNerve(this, &NrvBallYarnWait)) {
            if (al::isNerve(this, &NrvBallYarnFall) && mRouteDokanDisableTimer > 0) {
                return;
            }

            if (mStateRouteDokan->tryStart(pSelf, pOther)) {
                al::offCollide(this);
                al::setNerve(this, &NrvBallYarnRouteDokan);
                return;
            }
        }

        if (al::isNerve(this, &NrvBallYarnPlayerHold)) {
            return;
        }

        if (al::isSensorPlayer(pOther)) {
            if (isEnablePlayerKnockDown(pOther, pSelf) && al::sendMsgKnockDown(pOther, pSelf)) {
                sead::Vector3f dir = al::getVelocity(this);
                al::verticalizeVec(&dir, sead::Vector3f::ey, dir);
                al::normalizeOrZero(&dir);
                al::setVelocity(this, dir.x * -5.0f, 15.0f, dir.z * -5.0f);
                al::startSeWithParam(this, "PgBound", al::getVelocity(this).length());
                al::setNerve(this, &NrvBallYarnFall);
                mHoldDisableTimer = 12;
                mKnockDownDisableTimer = 12;
                return;
            }

            al::sendMsgPush(pOther, pSelf);
            return;
        }
    }

    if (al::isSensorName(pSelf, "Hold")) {
        if (al::isSensorMapObj(pOther) || al::isSensorBindableGoal(pOther) ||
            al::isSensorBindableGoalItem(pOther)) {
            if (mIsThrown && al::sendMsgBallAttack(pOther, pSelf, mComboCounter)) {
                BallStateFunction::calcReflectSpeed(this, al::getSensorHost(pOther), &sFallParam);
                al::setNerve(this, &NrvBallYarnFall);
                al::startSeWithParam(this, "PgBound", al::getVelocity(this).length());
                return;
            }

            if (al::isNerve(this, &NrvBallYarnFall) &&
                al::sendMsgBallTrample(pOther, pSelf, mComboCounter)) {
                BallStateFunction::calcReflectSpeed(this, al::getSensorHost(pOther), &sFallParam);
                al::startSeWithParam(this, "PgBound", al::getVelocity(this).length());
                return;
            }

            if (!al::isNerve(this, &NrvBallYarnWait)) {
                if (al::isNerve(this, &NrvBallYarnRouteDokan)) {
                    rc::sendMsgRouteDokanItemGet(pOther, pSelf);
                    return;
                }

                al::sendMsgBallItemGet(pOther, pSelf);
                return;
            }
        }

        if (!al::isNerve(this, &NrvBallYarnDRCHold) && al::isSensorHoldObj(pOther) &&
            al::sendMsgPush(pOther, pSelf)) {
            return;
        }
    }

    if (al::isNerve(this, &NrvBallYarnWait)) {
        return;
    }

    if (!al::isSensorName(pSelf, "Attack")) {
        return;
    }

    if (!al::isSensorEnemyBody(pOther) && !al::isSensorKickKoura(pOther) &&
        !al::isSensorNpc(pOther) && !al::isSensorEnemy(pOther) && !al::isSensorRide(pOther) &&
        !al::isSensorKoopaJr(pOther)) {
        return;
    }

    if (!mIsThrown || mAttackDisableTimer != 0) {
        return;
    }

    if (mIsNekoThrow && (npc::isSensorNeko(pOther) || npc::isSensorNekoDisaster(pOther))) {
        return;
    }

    if (al::sendMsgBallAttack(pOther, pSelf, mComboCounter)) {
        BallStateFunction::calcReflectSpeed(this, al::getSensorHost(pOther), &sFallParam);
        al::setNerve(this, &NrvBallYarnFall);
        al::startSeWithParam(this, "PgBound", al::getVelocity(this).length());
    }
}

/**
 * @brief Whether the ball flying into a player knocks that player down.
 * @param pPlayer Sensor of the player.
 * @param pSelf Sensor of the ball.
 * @return True if the ball moves towards the player fast enough.
 */
bool BallYarn::isEnablePlayerKnockDown(al::HitSensor* pPlayer, al::HitSensor* pSelf) {
    if (!mIsThrown && !al::isNerve(this, &NrvBallYarnFall) &&
        !al::isNerve(this, &NrvBallYarnRolling)) {
        return false;
    }

    if (mHolderSensor == nullptr || !mIsNekoThrow) {
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
bool BallYarn::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                          al::HitSensor* pSelf) {
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

    if (al::isNerve(this, &NrvBallYarnDamageThrow) || al::isNerve(this, &NrvBallYarnRespawn)) {
        return false;
    }

    if (rc::isMsgStartGoalDemoPole(pMsg)) {
        kill();
        return true;
    }

    if (al::isSensorName(pSelf, "Body")) {
        if (!isHold() && rc::isMsgJumpPanelAction(pMsg)) {
            al::setVelocity(this, 0.0f, rc::isMsgJumpPanelActionAndSuperJump(pMsg) ? 70.0f : 45.0f,
                            0.0f);
            al::setNerve(this, &NrvBallYarnFall);
            return true;
        }

        if (!isHold() &&
            (al::isMsgDisasterSpikeAttack(pMsg) || al::isMsgGigaEnemyAttack(pMsg) ||
             rc::isMsgFireRollerAttack(pMsg) || al::isMsgBowserPush(pMsg))) {
            al::setNerve(this, &NrvBallYarnRespawn);
            return true;
        }

        if ((al::isNerve(this, &NrvBallYarnWait) ||
             (al::isNerve(this, &NrvBallYarnRolling) && !mIsThrown)) &&
            (al::isMsgPlayerObjHipDropAll(pMsg) || al::isMsgPlayerFireBallAttack(pMsg) ||
             al::isMsgPlayerBoomerangAttack(pMsg) || al::isMsgKickKouraAttack(pMsg) ||
             al::isMsgPlayerKouraAttack(pMsg) || al::isMsgBallAttack(pMsg) ||
             al::isMsgNekoAttack(pMsg) || al::isMsgBallTrample(pMsg) ||
             al::isMsgPlayerBodyLanding(pMsg) || al::isMsgPlayerGiantHipDrop(pMsg) ||
             al::isMsgPlayerCooperationHipDrop(pMsg) || al::isMsgPlayerSlidingAttack(pMsg) ||
             al::isMsgBlockUpperPunch(pMsg) || al::isMsgExplosion(pMsg) ||
             al::isMsgEnemyAttackBoomerang(pMsg) || al::isMsgEnemyAttackFire(pMsg) ||
             (rc::isMsgBobsledBodyAttack(pMsg) && mAttackDisableTimer == 0))) {
            sead::Vector3f dir = {0.0f, 0.0f, 0.0f};
            al::calcDirBetweenSensorsH(&dir, pOther, pSelf);
            if (al::isNearZero(dir, 0.001f)) {
                if (al::isSensorPlayer(pOther)) {
                    dir.set(rc::getPlayerFront(pOther));
                } else {
                    dir.set(sead::Vector3f::ey);
                }
            }

            mIsNekoThrow = false;
            const BallStateThrowParam* param = &sHitParam;
            if (al::isMsgNekoAttack(pMsg)) {
                mIsNekoThrow = true;
                param = &sNekoAttackParam;
            }

            al::setVelocity(this, dir.x * param->mLaunchSpeed, param->mLaunchSpeedUp,
                            param->mLaunchSpeed * dir.z);
            al::startSeWithParam(this, "PgBound", al::getVelocity(this).length());
            mHolderSensor = BallStateFunction::tryGetRelativePlayerSensor(this, pOther);
            mStateThrow->setThrowParam(mHolderSensor, param, nullptr);
            if (!al::isMsgNekoAttack(pMsg)) {
                al::startSe(this, "PgThrow");
            }

            al::setNerve(this, &NrvBallYarnThrow);
            mIsRotateOnFall = false;
            if (rc::isMsgBobsledBodyAttack(pMsg) && al::isSensorRide(pOther)) {
                rc::requestHitReactionToAttackerNpc(pSelf, pOther);
            } else if (!al::isMsgNekoAttack(pMsg) &&
                       !(al::isMsgEnemyAttackBoomerang(pMsg) &&
                         al::isSensorHostName(pOther, "カメック魔法球"))) {
                rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
            }

            if (al::isMsgPlayerKouraAttack(pMsg) || al::isMsgKickKouraAttack(pMsg) ||
                rc::isMsgBobsledBodyAttack(pMsg)) {
                mAttackDisableTimer = 12;
            }

            return al::isMsgPlayerFireBallAttack(pMsg) || al::isMsgPlayerKouraAttack(pMsg) ||
                   al::isMsgNekoAttack(pMsg) || rc::isMsgBobsledBodyAttack(pMsg);
        }

        if (rc::isMsgDossunPress(pMsg)) {
            kill();
            return true;
        }

        if (!isHold() &&
            (al::isMsgPush(pMsg) || al::isMsgPushStrong(pMsg) || al::isMsgPushVeryStrong(pMsg))) {
            if (!al::isNerve(this, &NrvBallYarnRolling)) {
                al::setNerve(this, &NrvBallYarnRolling);
            }

            al::pushAndAddVelocityH(this, pOther, pSelf, al::isMsgPush(pMsg) ? 1.0f : 3.0f);
            return true;
        }
    }

    if (al::isNerve(this, &NrvBallYarnDRCHold)) {
        if (al::isMsgWarpStart(pMsg)) {
            rc::releaseTouchPointerHoldItem(this, mTouchPointer);
            al::setNerve(this, &NrvBallYarnFall);
            mIsRotateOnFall = true;
        }

        return false;
    }

    if (!al::isSensorName(pSelf, "Hold")) {
        return false;
    }

    if (al::isNerve(this, &NrvBallYarnPlayerHold)) {
        if (!mStatePlayerHold->receiveMsg(pMsg, pOther, pSelf)) {
            return false;
        }

        if (al::isMsgPlayerRelease(pMsg)) {
            al::startSe(this, "PgThrow");
            al::setNerve(this, &NrvBallYarnThrow);
            mStateThrow->setThrowParam(mHolderSensor, &sThrowParam, nullptr);
            mIsRotateOnFall = false;
            return true;
        }

        if (al::isMsgPlayerReleaseDamage(pMsg) || al::isMsgPlayerReleaseDead(pMsg)) {
            al::setNerve(this, &NrvBallYarnDamageThrow);
            mStateThrow->setThrowParam(mHolderSensor, &sDamageThrowParam, nullptr);
            return true;
        }

        if (al::isMsgWarpStart(pMsg) || al::isMsgHoldCancel(pMsg)) {
            al::setNerve(this, &NrvBallYarnFall);
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
        if (al::isMsgPlayerSpinAttack(pMsg) && al::isSensorEnemyAttack(pOther)) {
            al::startSe(this, "PgHit");
        } else if (!al::isNerve(this, &NrvBallYarnThrow)) {
            al::startSeWithParam(this, "PgBound", 500.0f);
            al::startSe(this, "PgThrow");
        }

        al::setNerve(this, &NrvBallYarnThrow);
        return true;
    }

    if (isEnableHold(pOther) && mStatePlayerHold->tryStartCarryFront(pMsg, pOther, false)) {
        if (al::isNerve(this, &NrvBallYarnThrow) || al::isNerve(this, &NrvBallYarnKick) ||
            al::isNerve(this, &NrvBallYarnFall) || al::isNerve(this, &NrvBallYarnPopUpFront) ||
            al::isNerve(this, &NrvBallYarnRouteDokanThrow)) {
            al::startHitReaction(this, "ボールキャッチ");
        }

        al::onCollide(this);
        mHolderSensor = pOther;
        al::setNerve(this, &NrvBallYarnPlayerHold);
        al::setColliderRadius(this, 15.0f);
        return true;
    }

    if ((al::isMsgPlayerKick(pMsg) || al::isMsgPlayerBodyAttack(pMsg)) && isEnableKick()) {
        mHolderSensor = pOther;
        mIsRotateOnFall = true;
        rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        mStateThrow->setThrowParam(mHolderSensor, &sKickParam, nullptr);
        al::startSe(this, "PgKicked");
        al::setNerve(this, &NrvBallYarnKick);
        return true;
    }

    return al::tryReceiveMsgPushAndAddVelocity(this, pMsg, pOther, pSelf, 1.0f);
}

/**
 * @brief Whether the ball is carried by a player or by the touch pointer.
 * @return True while carried.
 */
bool BallYarn::isHold() const {
    return al::isNerve(this, &NrvBallYarnPlayerHold) || al::isNerve(this, &NrvBallYarnDRCHold);
}

/**
 * @brief Whether the owner of a sensor may pick the ball up.
 * @param pSensor Sensor of the actor that wants to hold the ball.
 * @return True if the ball can be held.
 */
bool BallYarn::isEnableHold(al::HitSensor* pSensor) {
    if (al::isNerve(this, &NrvBallYarnRouteDokan)) {
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
bool BallYarn::isEnableKick() {
    return al::isNerve(this, &NrvBallYarnWait) || al::isNerve(this, &NrvBallYarnRolling);
}

/**
 * @brief Lets the touch pointer grab the ball.
 * @param pMsg Received message.
 * @param pPointer Pointer that sent the message.
 * @param pTarget Screen point target of the ball.
 * @return True if the ball is grabbed.
 */
bool BallYarn::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                     al::ScreenPointTarget* pTarget) {
    if (al::isNerve(this, &NrvBallYarnPopUpFront) || al::isNerve(this, &NrvBallYarnPlayerHold) ||
        al::isNerve(this, &NrvBallYarnThrow) || al::isNerve(this, &NrvBallYarnKick) ||
        al::isNerve(this, &NrvBallYarnRouteDokan) ||
        al::isNerve(this, &NrvBallYarnRouteDokanThrow) ||
        // The game checks DamageThrow twice.
        al::isNerve(this, &NrvBallYarnDamageThrow) ||
        al::isNerve(this, &NrvBallYarnDamageThrow) || al::isNerve(this, &NrvBallYarnRespawn)) {
        return false;
    }

    if (!mStateTouchCarry->receiveMsgScreenPoint(pMsg, pPointer, pTarget)) {
        return false;
    }

    if (!al::isNerve(this, &NrvBallYarnDRCHold)) {
        al::setNerve(this, &NrvBallYarnDRCHold);
        mIsNekoThrow = false;
        mHolderSensor = DrcFunction::tryFindDrcPlayerSensor(this, pPointer);
        mTouchPointer = DrcFunction::tryFindDrcTouchActor(this, pPointer);
    }

    return true;
}

/**
 * @brief Counts down the timers and slowly restores the collider radius.
 */
void BallYarn::control() {
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

    if (al::isNerve(this, &NrvBallYarnPlayerHold)) {
        return;
    }

    BallStateFunction::setColliderReturnedSlowly(this, mColliderRadius, 1);
}

/**
 * @brief Releases the ball from its holder and lets it respawn.
 */
void BallYarn::kill() {
    if (al::isNerve(this, &NrvBallYarnRouteDokan)) {
        al::startHitReaction(this, "ルート土管消滅");
    } else if (al::isNerve(this, &NrvBallYarnPlayerHold)) {
        rc::requestPlayerRelease(mHolderSensor);
    } else if (al::isNerve(this, &NrvBallYarnDRCHold)) {
        rc::releaseTouchPointerHoldItem(this, mTouchPointer);
    }

    al::setNerve(this, &NrvBallYarnRespawn);
}

/**
 * @brief Updates the collider, letting the holder state drive it while the ball is carried.
 */
void BallYarn::updateCollider() {
    if (al::isExpandedClippingMode(this)) {
        return;
    }

    ItemStatePlayerHold* state = mStatePlayerHold;
    if (state->isDead()) {
        al::LiveActor::updateCollider();
        return;
    }

    state->updateCollider(al::getHitSensor(this, "Body"));
    if (mHolderSensor != nullptr && al::isOnGround(this, 0, 0.0f) &&
        !rc::isPlayerOnGround(mHolderSensor) &&
        !al::isEqualString(al::getCollidedFloorMaterialCodeName(this), "Cloud")) {
        al::sendMsgPush(mHolderSensor, al::getHitSensor(this, "Hold"));
    }
}

/**
 * @brief Hides the ball unless a player carries it.
 * @return True if the ball was hidden.
 */
bool BallYarn::hideActor() {
    if (al::isNerve(this, &NrvBallYarnPlayerHold)) {
        return false;
    }

    return al::LiveActor::hideActor();
}

/**
 * @brief Whether a player carries the ball.
 * @return True while held by a player.
 */
bool BallYarn::isPlayerHold() const {
    return al::isNerve(this, &NrvBallYarnPlayerHold);
}

/**
 * @brief Appears and pops the ball up to the front.
 */
void BallYarn::appearPopUpFront() {
    al::invalidateHitSensors(this);
    al::LiveActor::appear();
    mStatePopUpFront->setParamBallPopUp();
    al::setNerve(this, &NrvBallYarnPopUpFront);
}

/**
 * @brief Appears and pops the ball straight up.
 */
void BallYarn::appearAbove() {
    al::invalidateHitSensors(this);
    al::LiveActor::appear();
    mStatePopUpFront->setParamBallAbove();
    al::setNerve(this, &NrvBallYarnPopUpFront);
}

/**
 * @brief Puts the ball back to rest.
 */
void BallYarn::reset() {
    al::onCollide(this);
    al::setNerve(this, &NrvBallYarnWait);
    al::setVelocityZero(this);
}

/**
 * @brief Lies on the ground, falling or rolling away once the ground is gone or sloped.
 */
void BallYarn::exeWait() {
    if (al::isFirstStep(this)) {
        al::validateClipping(this);
        mComboCounter->reset();
        mIsThrown = false;
    }

    al::addVelocityToGravity(this, 1.5f);
    if (!al::isOnGround(this, 8, 0.0f) && !al::isLessEqualStep(this, 5)) {
        al::invalidateClipping(this);
        al::setNerve(this, &NrvBallYarnFall);
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
    al::setNerve(this, &NrvBallYarnRolling);
}

/**
 * @brief Carried by a player.
 */
void BallYarn::exePlayerHold() {
    if (al::isFirstStep(this)) {
        al::setVelocityZero(this);
        al::startSe(this, "PgHoldStart");
        mIsNekoThrow = false;
        mIsThrown = false;
    }

    al::updateNerveState(this);
}

/**
 * @brief Carried by the touch pointer, thrown or dropped when released.
 */
void BallYarn::exeDRCHold() {
    if (al::isStep(this, 1)) {
        al::startHitReaction(this, "DRCつかむ");
    }

    mIsThrown = false;
    if (!al::updateNerveState(this)) {
        return;
    }

    if (mStateTouchCarry->isItemThrow()) {
        mStateThrow->setThrowParam(nullptr, &sThrowParam, mTouchPointer);
        al::setVelocity(this, mStateTouchCarry->getThrowVelocity());
        al::startSe(this, "PgThrow");
        al::setNerve(this, &NrvBallYarnThrow);
        return;
    }

    al::setVelocity(this, mStateTouchCarry->getReleaseVelocity());
    al::setNerve(this, &NrvBallYarnFall);
    al::startHitReaction(this, "DRC放す");
}

/**
 * @brief Flies after being thrown.
 */
void BallYarn::exeThrow() {
    if (al::isFirstStep(this)) {
        mWallCollideCount = 0;
        mHoldDisableTimer = 12;
        mKnockDownDisableTimer = 12;
        al::invalidateClipping(this);
        mIsThrown = true;
    } else {
        startEffect();
    }

    countWallCollide();
    if (al::updateNerveState(this)) {
        al::setNerve(this, &NrvBallYarnFall);
        return;
    }

    BallYarnFunction::tryStartRespawn(this);
}

/**
 * @brief Emits the collision effect where the ball hits something.
 */
void BallYarn::startEffect() {
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
}

/**
 * @brief Counts the frames in which the ball hits a wall.
 */
void BallYarn::countWallCollide() {
    if (al::isCollidedWallVelocity(this)) {
        mWallCollideCount++;
    }
}

namespace BallYarnFunction {
/**
 * @brief Lets the ball respawn once it falls into water, a kill area or an ink area.
 * @param pActor The ball.
 * @return True if the ball starts to respawn.
 */
bool tryStartRespawn(al::LiveActor* pActor) {
    bool isInWater = rc::isInWaterArea(pActor, al::getTrans(pActor) + sRespawnEffectOffset);
    bool isKill = EnemyStateUtil::isKillByAreaOrMaterialCode(pActor);
    if (!isInWater && !isKill && !InkUtil::isInInkLimitSphere(pActor)) {
        return false;
    }

    al::startSe(pActor, isInWater ? "HrFallWater" : "HrVanishWater");
    al::setNerve(pActor, &NrvBallYarnRespawn);
    return true;
}
}  // namespace BallYarnFunction

/**
 * @brief Flies after being kicked.
 */
void BallYarn::exeKick() {
    if (al::isFirstStep(this)) {
        mWallCollideCount = 0;
        mHoldDisableTimer = 12;
        mKnockDownDisableTimer = 12;
        al::invalidateClipping(this);
        mIsNekoThrow = false;
        mIsThrown = true;
    } else {
        startEffect();
    }

    countWallCollide();
    if (al::updateNerveState(this)) {
        al::setNerve(this, &NrvBallYarnFall);
        return;
    }

    BallYarnFunction::tryStartRespawn(this);
}

/**
 * @brief Falls and bounces until the ball starts rolling.
 */
void BallYarn::exeFall() {
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
        al::setNerve(this, &NrvBallYarnRolling);
        return;
    }

    BallYarnFunction::tryStartRespawn(this);
}

/**
 * @brief Rolls on the ground until it stops.
 */
void BallYarn::exeRolling() {
    if (al::updateNerveState(this)) {
        if (mStateRolling->isOnAir()) {
            al::setNerve(this, &NrvBallYarnFall);
            return;
        }

        if (BallYarnFunction::tryStartRespawn(this)) {
            return;
        }

        al::setNerve(this, &NrvBallYarnWait);
        return;
    }

    if (al::isVelocitySlow(this, 5.0f)) {
        mIsThrown = false;
    }

    BallYarnFunction::tryStartRespawn(this);
}

/**
 * @brief Flies away after hurting its holder, respawning once it lands or hits something.
 */
void BallYarn::exeDamageThrow() {
    if (al::isFirstStep(this)) {
        al::invalidateClipping(this);
    }

    al::updateNerveState(this);
    if ((al::isCollidedGround(this) && al::getVelocity(this).y < 0.0f) ||
        al::isCollidedWall(this) || al::isCollidedCeiling(this) ||
        al::isGreaterEqualStep(this, 30)) {
        al::validateClipping(this);
        al::setNerve(this, &NrvBallYarnRespawn);
        return;
    }

    BallYarnFunction::tryStartRespawn(this);
}

/**
 * @brief Pops up out of a block.
 */
void BallYarn::exePopUpFront() {
    if (al::updateNerveStateAndNextNerve(this, &NrvBallYarnFall)) {
        al::invalidateClipping(this);
        return;
    }

    rc::startHitReactionIfThroughWater(this);
}

/**
 * @brief Travels through a route pipe.
 */
void BallYarn::exeRouteDokan() {
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
        al::setNerve(this, &NrvBallYarnRouteDokanThrow);
    }
}

/**
 * @brief Flies out of the exit of a route pipe.
 */
void BallYarn::exeRouteDokanThrow() {
    if (al::isFirstStep(this)) {
        al::onCollide(this);
        mWallCollideCount = 0;
        mIsThrown = true;
    } else {
        startEffect();
    }

    countWallCollide();
    if (al::updateNerveState(this)) {
        al::setNerve(this, &NrvBallYarnFall);
        return;
    }

    BallYarnFunction::tryStartRespawn(this);
}

/**
 * @brief Vanishes and reappears at the placement position after a while.
 */
void BallYarn::exeRespawn() {
    if (al::isFirstStep(this)) {
        al::invalidateClipping(this);
        al::hideModelIfShow(this);
        al::invalidateHitSensors(this);
        al::offCollide(this);

        sead::Vector3f pos = al::getTrans(this);
        if (rc::isInWaterArea(this)) {
            pos += sRespawnEffectOffset;
        }

        al::tryEmitEffect(this, "Poof", &pos);
        al::setVelocityZero(this);
    }

    if (al::isGreaterStep(this, 60)) {
        al::resetPosition(this, mRespawnPos, false);
        al::tryEmitEffect(this, "Appear", nullptr);
        al::startSe(this, "PgReappear");
        al::validateClipping(this);
        al::showModelIfHide(this);
        al::validateHitSensors(this);
        al::onCollide(this);
        al::setNerve(this, &NrvBallYarnWait);
    }
}
