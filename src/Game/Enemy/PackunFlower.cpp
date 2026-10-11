#include "Enemy/PackunFlower.hpp"

#include <math.h>
#include <math/seadVector.h>

#include "Enemy/ActorMicRumbler.hpp"
#include "Enemy/ActorStateSupportFreeze.hpp"
#include "Enemy/EnemyStateBlowDown.hpp"
#include "Enemy/EnemyStateUtil.hpp"
#include "Enemy/PackunTrace.hpp"
#include "Enemy/PackunTraceBig.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Collision/PartsConnectorUtil.hpp"
#include "Library/Connector/MtxConnector.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/StageSwitch/StageSwitchFunc.hpp"
#include "Library/Thread/Functor.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Collision/CollisionPartsTriangle.hpp"
#include "Project/Collision/CollisionUtil.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ScoreUtil.hpp"

namespace {
NERVE_DECL(PackunFlower, Sleep)
NERVE_DECL(PackunFlower, BlowDown)
NERVE_DECL(PackunFlower, SupportFreeze)
NERVE_DECL(PackunFlower, SupportFreezeSwoon)
NERVE_DECL(PackunFlower, Wait)
NERVE_DECL(PackunFlower, PressDown)
NERVE_DECL(PackunFlower, SwoonEnd)
NERVE_DECL(PackunFlower, Trampled)
NERVE_DECL(PackunFlower, Damage)
NERVE_DECL(PackunFlower, Swoon)
NERVE_DECL(PackunFlower, Find)
NERVE_DECL(PackunFlower, TurnFast)
NERVE_DECL(PackunFlower, Turn)
NERVE_DECL(PackunFlower, Attack)
NERVE_DECL(PackunFlower, AfterAttack)
// Non-const nerve objects: the game keeps them together in .data in this order.
PackunFlowerNrvSleep NrvPackunFlowerSleep;
PackunFlowerNrvBlowDown NrvPackunFlowerBlowDown;
PackunFlowerNrvSupportFreeze NrvPackunFlowerSupportFreeze;
PackunFlowerNrvSupportFreezeSwoon NrvPackunFlowerSupportFreezeSwoon;
PackunFlowerNrvWait NrvPackunFlowerWait;
PackunFlowerNrvPressDown NrvPackunFlowerPressDown;
PackunFlowerNrvSwoonEnd NrvPackunFlowerSwoonEnd;
PackunFlowerNrvTrampled NrvPackunFlowerTrampled;
PackunFlowerNrvDamage NrvPackunFlowerDamage;
PackunFlowerNrvSwoon NrvPackunFlowerSwoon;
PackunFlowerNrvFind NrvPackunFlowerFind;
PackunFlowerNrvTurnFast NrvPackunFlowerTurnFast;
PackunFlowerNrvTurn NrvPackunFlowerTurn;
PackunFlowerNrvAttack NrvPackunFlowerAttack;
PackunFlowerNrvAfterAttack NrvPackunFlowerAfterAttack;

EnemyStateBlowDownParam sBlowDownParam(false);
ActorStateSupportFreezeParam sSupportFreezeParam(true, 15, false, true, 120,
                                                 sead::Vector3f(0.0f, 150.0f, 0.0f));
ActorStateSupportFreezeParam sSupportFreezeParamBig(true, 15, false, true, 120,
                                                    sead::Vector3f(0.0f, 300.0f, 0.0f));

typedef al::FunctorV0M<PackunFlower*, void (PackunFlower::*)()> PackunFlowerFunctor;
}  // namespace

/**
 * @brief Constructs the Piranha Plant.
 * @param pName Actor name.
 */
PackunFlower::PackunFlower(const char* pName) : al::LiveActor(pName) {}

/**
 * @brief Initializes the model (big or normal), states, remains and stage switch listener.
 * @param rInfo Actor placement and scene information.
 */
void PackunFlower::init(const al::ActorInitInfo& rInfo) {
    const char* objectName;
    al::tryGetObjectName(&objectName, rInfo);

    if (al::isEqualString(objectName, "PackunFlowerBig") ||
        al::isEqualString(objectName, "PackunFlowerBigFur")) {
        mIsBig = true;
    }

    if (rInfo.getActorSceneInfo().isSingleMode) {
        al::initActorWithArchiveName(this, rInfo,
                                     mIsBig ? "PackunFlowerBigFur" : "PackunFlowerFur", nullptr);
    } else {
        al::initActor(this, rInfo);
    }

    mStateBlowDown = new EnemyStateBlowDown(this, &sBlowDownParam);
    mStateSupportFreeze = new ActorStateSupportFreeze(
        this, mIsBig ? &sSupportFreezeParamBig : &sSupportFreezeParam);
    al::initNerve(this, &NrvPackunFlowerSleep, 3);
    al::initNerveState(this, mStateBlowDown, &NrvPackunFlowerBlowDown, "[state]吹き飛び");
    al::initNerveState(this, mStateSupportFreeze, &NrvPackunFlowerSupportFreeze, "[state]フリーズ");
    al::initNerveState(this, mStateSupportFreeze, &NrvPackunFlowerSupportFreezeSwoon,
                       "[state]フリーズ(気絶中)");

    if (mIsBig) {
        mStateSupportFreeze->setScaleAnimTypeHard();
        mSpinMtx = al::getJointMtxPtr(this, "Spin3");
        al::setNerve(this, &NrvPackunFlowerWait);
        al::tryGetArg(&mIsBlowBackDir, rInfo, "IsBlowBackDir");
    }

    al::tryGetArg(&mIsRemainTrace, rInfo, "IsRemainTrace");

    if (mIsRemainTrace) {
        if (mIsBig) {
            mTrace = new PackunTraceBig(this);
        } else {
            mTrace = new PackunTrace(this);
        }

        al::initCreateActorWithPlacementInfo(mTrace, rInfo);
    }

    al::trySetShadowLength(this, rInfo, nullptr);
    mMtxConnector = al::createMtxConnector(this);
    al::offCollide(this);
    mMicRumbler = new ActorMicRumbler(this, nullptr);
    al::listenStageSwitchOnKill(this, PackunFlowerFunctor(this, &PackunFlower::killBySwitch));
    makeActorAppeared();
    mInitQuat = al::getQuat(this);
}

/** @brief Dies with a death reaction when the kill switch turns on. */
void PackunFlower::killBySwitch() {
    al::startHitReactionDeath(this);
    kill();
}

/** @brief Attaches to the collision below and aligns the up direction with the ground normal. */
void PackunFlower::initAfterPlacement() {
    al::attachMtxConnectorToCollision(mMtxConnector, this, false);
    sead::Quatf placedQuat = al::getQuat(this);
    sead::Vector3f up;
    al::calcQuatUp(&up, this);
    sead::Vector3f hitPos = sead::Vector3f::zero;
    al::Triangle triangle;

    if (alCollisionUtil::getFirstPolyOnArrow(this, &hitPos, &triangle,
                                             al::getTrans(this) + up * 50.0f, up * -150.0f,
                                             static_cast<const al::CollisionPartsFilterBase*>(nullptr),
                                             nullptr)) {
        mUpDir = *triangle.getNormal(0);
        sead::Vector3f front;
        al::calcQuatFront(&front, this);
        sead::Quatf quat;
        al::makeQuatUpFront(&quat, mUpDir, front);
        al::setQuat(this, quat);
    }

    sead::Quatf groundQuat = al::getQuat(this);
    al::makeQuatFromToQuat(&mPoseOffsetQuat, placedQuat, groundQuat);
}

/** @brief Reappears waiting at the initial rotation (single mode only). */
void PackunFlower::reappear() {
    if (GameDataFunction::isSingleMode(GameDataHolderAccessor(this))) {
        mTrace->makeActorDead();
        al::setNerve(this, &NrvPackunFlowerWait);
        al::startAction(this, "Wait");
        al::setQuat(this, mInitQuat);
        al::LiveActor::appear();
    }
}

/**
 * @brief Kills the plant together with its remains.
 * @param isNoReaction Whether to skip the death reaction.
 */
void PackunFlower::killComplete(bool isNoReaction) {
    if (!al::isDead(this) && !isNoReaction) {
        al::startHitReactionDeath(this);
    }

    mTrace->kill();
    kill();
}

/** @brief Follows the collision pose, updates the timers, stem direction and mic rumble. */
void PackunFlower::control() {
    if (al::isNerve(this, &NrvPackunFlowerBlowDown)) {
        return;
    }

    if (mInvincibleTimer > 0) {
        mInvincibleTimer--;
    }

    if (mIsBig && mTouchCooldown > 0) {
        mTouchCooldown--;
    }

    sead::Vector3f front;
    al::calcQuatFront(&front, this);
    al::connectPoseQT(this, mMtxConnector);
    sead::Quatf quat = al::getQuat(this);
    quat.setMul(mPoseOffsetQuat, quat);
    sead::Vector3f up;
    al::calcQuatUp(&up, quat);
    sead::Quatf newQuat;
    al::makeQuatUpFront(&newQuat, up, front);
    al::setQuat(this, newQuat);

    if (mIsBig) {
        mSpinMtx->getBase(mStemDir, 0);
        al::normalizeOrDirZ(&mStemDir);
    }

    if (al::isNerve(this, &NrvPackunFlowerSupportFreeze) ||
        al::isNerve(this, &NrvPackunFlowerPressDown) ||
        al::isNerve(this, &NrvPackunFlowerBlowDown)) {
        return;
    }

    if (!al::isNerve(this, &NrvPackunFlowerSupportFreeze) &&
        !al::isNerve(this, &NrvPackunFlowerPressDown) &&
        !al::isNerve(this, &NrvPackunFlowerBlowDown)) {
        mMicRumbler->update();
    }
}

/**
 * @brief Checks whether the plant can currently attack.
 * @return Whether the plant is neither squashed nor blown away.
 */
bool PackunFlower::isEnableAttack() {
    return !al::isNerve(this, &NrvPackunFlowerPressDown) &&
           !al::isNerve(this, &NrvPackunFlowerBlowDown);
}

/**
 * @brief Checks whether the Big Piranha Plant is reeling from a hit.
 * @return Whether the plant is damaged, swooning or trampled.
 */
bool PackunFlower::isDamaged() {
    return al::isNerve(this, &NrvPackunFlowerDamage) ||
           al::isNerve(this, &NrvPackunFlowerSwoon) ||
           al::isNerve(this, &NrvPackunFlowerTrampled);
}

/**
 * @brief Pushes and bites other actors.
 * @param pSelf Sensor of the plant.
 * @param pOther Sensor of the other actor.
 */
void PackunFlower::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isNerve(this, &NrvPackunFlowerPressDown) ||
        al::isNerve(this, &NrvPackunFlowerBlowDown)) {
        return;
    }

    if (al::isSensorEnemyBody(pOther)) {
        if (mIsBig) {
            al::sendMsgEnemyAttackFire(pOther, pSelf);
        }

        al::sendMsgPushAndKillVelocityToTarget(this, pSelf, pOther);
        return;
    }

    if (((al::isSensorPlayer(pOther) || al::isSensorKoopaJr(pOther) ||
          al::isSensorDoorKey(pOther)) &&
         al::isSensorEnemyAttack(pSelf)) ||
        al::isSensorKickKoura(pOther)) {
        al::sendMsgPush(pOther, pSelf);

        if (isDamaged() || al::isNerve(this, &NrvPackunFlowerSwoonEnd)) {
            return;
        }

        al::sendMsgEnemyAttack(pOther, pSelf);
        return;
    }

    if ((al::isSensorPlayer(pOther) || al::isSensorKoopaJr(pOther) ||
         al::isSensorDoorKey(pOther) || al::isSensorKickKoura(pOther)) &&
        al::isSensorName(pSelf, "Stem") &&
        al::isHitCylinderSensor(pOther, pSelf, mStemDir, 250.0f)) {
        al::sendMsgPush(pOther, pSelf);
        return;
    }

    if (GameDataFunction::isSingleMode(GameDataHolderAccessor(this)) &&
        !al::isSensorPlayerType(pOther) && !al::isSensorPlessie(pOther)) {
        al::sendMsgPush(pOther, pSelf);
    }
}

/**
 * @brief Handles tramples, attacks, blow-downs and being eaten.
 * @param pMsg Received message.
 * @param pOther Sensor of the sender.
 * @param pSelf Sensor of the plant.
 * @return Whether the message was handled.
 */
bool PackunFlower::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                              al::HitSensor* pSelf) {
    if (al::isNerve(this, &NrvPackunFlowerPressDown) ||
        al::isNerve(this, &NrvPackunFlowerBlowDown)) {
        return false;
    }

    if (mIsBig) {
        if (al::isMsgKeyThrow(pMsg)) {
            EnemyStateUtil::requestBlowDown(pMsg, pOther, pSelf, mStateBlowDown, true);
            al::addTransOffsetLocalDir(this, 250.0f, 1);
            rc::addScoreCombo(this, pOther, pMsg, 100.0f);
            al::setNerve(this, &NrvPackunFlowerBlowDown);
            return true;
        }

        if (!al::isSensorName(pSelf, "Body") && !al::isSensorName(pSelf, "Root") &&
            (!al::isSensorName(pSelf, "Stem") ||
             !al::isHitCylinderSensor(pOther, pSelf, mStemDir, 250.0f))) {
            return false;
        }

        if ((al::isMsgPlayerTrampleForCrossoverSensor(pMsg, pOther, pSelf) ||
             al::isMsgBallTrample(pMsg) || al::isMsgPlayerBodyAttackReflect(pMsg)) &&
            al::getActorVelocity(pOther).y < 0.0f) {
            if (mHitState == HitState::Healthy) {
                rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
                mInvincibleTimer =
                    GameDataFunction::isSingleMode(GameDataHolderAccessor(this)) ? 30 : 25;
                rc::addScoreCombo(this, pOther, pMsg, 100.0f);
                al::setNerve(this, &NrvPackunFlowerTrampled);
                return true;
            }

            if (mInvincibleTimer > 0) {
                return false;
            }

            rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
            rc::setAppearItemFactorByMsg(this, pMsg, pOther);
            rc::addScoreCombo(this, pOther, pMsg, 100.0f);
            al::setNerve(this, &NrvPackunFlowerPressDown);
            return true;
        }

        if (al::isMsgPlayerObjHipDropReflectAll(pMsg)) {
            rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
            rc::setAppearItemFactorByMsg(this, pMsg, pOther);
            rc::addScoreCombo(this, pOther, pMsg, 100.0f);
            al::setNerve(this, &NrvPackunFlowerPressDown);
            return true;
        }

        if (al::isMsgPlayerCooperationHipDrop(pMsg) || al::isMsgPlayerInvincibleAttack(pMsg) ||
            al::isMsgPlayerGiantAttack(pMsg) || al::isMsgLaserAttack(pMsg) ||
            al::isMsgPlayerGiantHipDrop(pMsg) || al::isMsgExplosion(pMsg)) {
            EnemyStateUtil::requestBlowDown(pMsg, pOther, pSelf, mStateBlowDown, true);
            al::addTransOffsetLocalDir(this, 250.0f, 1);
            rc::addScoreCombo(this, pOther, pMsg, 100.0f);
            al::setNerve(this, &NrvPackunFlowerBlowDown);
            return true;
        }

        if (!EnemyStateUtil::isMsgBlowDown(pMsg) && !al::isMsgPlayerBoomerangReflect(pMsg) &&
            !al::isMsgKickKouraReflect(pMsg)) {
            return false;
        }

        if (mHitState == HitState::Damaged || al::isMsgBlockUpperPunch(pMsg)) {
            if (al::isMsgPlayerBoomerangReflect(pMsg) || al::isMsgKickKouraReflect(pMsg) ||
                mInvincibleTimer > 0) {
                return false;
            }

            if (mIsBlowBackDir) {
                rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
                rc::startHitReactionBlowHitMessage(pMsg, this, pOther, pSelf);
                rc::setAppearItemFactorByMsg(al::getSensorHost(pSelf), pMsg, pOther);
                sead::Vector3f blowDir;
                al::calcQuatFront(&blowDir, this);
                blowDir *= -sBlowDownParam.mSpeed;
                blowDir.y = sBlowDownParam.mJumpSpeed;
                mStateBlowDown->setBlowDir(blowDir);
            } else {
                EnemyStateUtil::requestBlowDown(pMsg, pOther, pSelf, mStateBlowDown, true);
            }

            al::addTransOffsetLocalDir(this, 250.0f, 1);
            rc::addScoreCombo(this, pOther, pMsg, 100.0f);
            al::setNerve(this, &NrvPackunFlowerBlowDown);
            return true;
        }

        if (al::isMsgPlayerBoomerangAttack(pMsg) || al::isMsgKickKouraAttack(pMsg) ||
            mInvincibleTimer > 0) {
            return false;
        }

        rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        rc::addScoreCombo(this, pOther, pMsg, 100.0f);
        mInvincibleTimer = GameDataFunction::isSingleMode(GameDataHolderAccessor(this)) ? 30 : 25;
        al::setNerve(this, &NrvPackunFlowerDamage);
        return true;
    }

    if (!al::isSensorEnemyBody(pSelf)) {
        return false;
    }

    if (GameDataFunction::isSingleMode(GameDataHolderAccessor(this)) &&
        al::isMsgEnemyAttack(pMsg)) {
        al::onCollide(this);
        mStateBlowDown->setBlowDir(al::getSensorHost(pOther));
        al::setNerve(this, &NrvPackunFlowerBlowDown);
        al::connectPoseQT(this, mMtxConnector);
        sead::Quatf quat = al::getQuat(this);
        quat.setMul(mPoseOffsetQuat, quat);
        sead::Vector3f up;
        al::calcQuatUp(&up, quat);
        al::setTrans(this, al::getTrans(this) + up * 100.0f);
        rc::setAppearItemFactorByMsg(this, pMsg, pOther);
        return true;
    }

    if (EnemyStateUtil::isMsgBlowDown(pMsg) && !checkCollision(pMsg, pOther, pSelf)) {
        EnemyStateUtil::requestBlowDown(pMsg, pOther, pSelf, mStateBlowDown, false);
        al::setNerve(this, &NrvPackunFlowerBlowDown);
        al::connectPoseQT(this, mMtxConnector);
        sead::Quatf quat = al::getQuat(this);
        quat.setMul(mPoseOffsetQuat, quat);
        sead::Vector3f up;
        al::calcQuatUp(&up, quat);
        al::setTrans(this, al::getTrans(this) + up * 100.0f);
        rc::setAppearItemFactorByMsg(this, pMsg, pOther);
        rc::addScoreCombo(this, pOther, pMsg, 100.0f);
        return true;
    }

    if ((al::isMsgTrampleAll(pMsg) || al::isMsgPlayerObjHipDropAll(pMsg)) &&
        !checkCollision(pMsg, pOther, pSelf)) {
        rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        rc::setAppearItemFactorByMsg(this, pMsg, pOther);
        rc::addScoreCombo(this, pOther, pMsg, 100.0f);
        al::setNerve(this, &NrvPackunFlowerPressDown);
        return true;
    }

    if (rc::isMsgPackunEatStart(pMsg)) {
        return true;
    }

    if (rc::isMsgPackunEat(pMsg)) {
        rc::setAppearItemFactorByMsg(this, pMsg, pOther);
        rc::addScoreCombo(this, pOther, pMsg, 100.0f);
        tryAppearTrace();
        kill();
        return true;
    }

    return false;
}

/**
 * @brief Checks whether collision lies between the attacking player and the plant.
 * @param pMsg Received message.
 * @param pOther Sensor of the attacker.
 * @param pSelf Sensor of the plant.
 * @return Whether the attack is blocked by collision.
 */
bool PackunFlower::checkCollision(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                                  al::HitSensor* pSelf) {
    if (al::isMsgExplosion(pMsg) || al::isMsgPlayerCooperationHipDrop(pMsg) ||
        al::isMsgPlayerGiantHipDrop(pMsg) || !al::isSensorPlayerOrPlayerWeapon(pOther)) {
        return false;
    }

    return alCollisionUtil::checkStrikeArrow(this, al::getSensorPos(pOther),
                                             al::getSensorPos(pSelf) - al::getSensorPos(pOther),
                                             nullptr, nullptr) != 0;
}

/** @brief Makes the remains appear in the current pose when enabled. */
void PackunFlower::tryAppearTrace() {
    if (!mIsRemainTrace) {
        return;
    }

    if (mIsBig) {
        static_cast<PackunTraceBig*>(mTrace)->setBaseQuat(mPoseOffsetQuat);
    } else {
        static_cast<PackunTrace*>(mTrace)->setBaseQuat(mPoseOffsetQuat);
    }

    mTrace->appear();
}

/**
 * @brief Freezes (or, for the big plant, damages) the plant when it is touched.
 * @param pMsg Received message.
 * @param pPointer Screen pointer.
 * @param pTarget Screen point target of the plant.
 * @return Whether the message was handled.
 */
bool PackunFlower::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                         al::ScreenPointTarget* pTarget) {
    if (al::isNerve(this, &NrvPackunFlowerPressDown)) {
        return false;
    }

    if (al::isNerve(this, &NrvPackunFlowerBlowDown)) {
        return false;
    }

    if (al::isNerve(this, &NrvPackunFlowerTrampled)) {
        return false;
    }

    if (al::isNerve(this, &NrvPackunFlowerDamage)) {
        return false;
    }

    if (mTouchCooldown > 0) {
        return false;
    }

    if (mIsBig && al::isMsgTouchAssistTrig(pMsg)) {
        mTouchCooldown = 60;

        if (!al::isNerve(this, &NrvPackunFlowerSwoon) &&
            !al::isNerve(this, &NrvPackunFlowerSupportFreezeSwoon)) {
            rc::addScoreCombo(this, pPointer, pMsg, 100.0f);
        }

        al::setNerve(this, &NrvPackunFlowerDamage);
        return true;
    }

    if (mStateSupportFreeze->receiveMsgScreenPoint(pMsg, pPointer, pTarget)) {
        if (al::isNerve(this, &NrvPackunFlowerSwoon)) {
            mMicRumbler->stopAndReset();
            al::setNerve(this, &NrvPackunFlowerSupportFreezeSwoon);
        }

        if (!al::isNerve(this, &NrvPackunFlowerSupportFreeze) &&
            !al::isNerve(this, &NrvPackunFlowerSupportFreezeSwoon)) {
            mMicRumbler->stopAndReset();
            al::setNerve(this, &NrvPackunFlowerSupportFreeze);
        }

        return true;
    }

    return false;
}

/** @brief Sleeps until a player comes close. */
void PackunFlower::exeSleep() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Sleep");
    }

    if (al::isNearPlayer(this, 1000.0f)) {
        al::setNerve(this, &NrvPackunFlowerFind);
    }
}

/** @brief Plays the wake-up reaction. */
void PackunFlower::exeFind() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Find");
    }

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvPackunFlowerWait);
    }
}

/** @brief Waits and turns towards a nearby player (fast when the player is behind). */
void PackunFlower::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait");
    }

    if (al::isNearPlayer(this, 1000.0f)) {
        sead::Vector3f dirToPlayer = al::findNearestPlayerPos(this) - al::getTrans(this);
        al::normalizeOrDirZ(&dirToPlayer);
        sead::Vector3f frontDir;
        al::calcFrontDir(&frontDir, this);

        if (frontDir.dot(dirToPlayer) < 0.0f) {
            al::setNerve(this, &NrvPackunFlowerTurnFast);
        } else {
            al::setNerve(this, &NrvPackunFlowerTurn);
        }
    }
}

/** @brief Turns towards the player and attacks once the player is in sight. */
void PackunFlower::exeTurn() {
    if (al::isFirstStep(this)) {
        mTurnDegree = mIsBig ? 3.5f : 4.0f;
    }

    sead::Vector3f dirToPlayer = al::findNearestPlayerPos(this) - al::getTrans(this);
    al::normalizeOrDirZ(&dirToPlayer);
    sead::Vector3f front;
    al::calcQuatFront(&front, this);
    sead::Vector3f newFront = front;
    al::turnVecToVecCosOnPlane(&newFront, front, dirToPlayer, mUpDir,
                               cos(mTurnDegree / 180.0f * 3.14));
    sead::Quatf quat;
    al::makeQuatUpFront(&quat, mUpDir, newFront);
    al::setQuat(this, quat);

    if (isInSight()) {
        al::setNerve(this, &NrvPackunFlowerAttack);
    }
}

/**
 * @brief Checks whether the nearest player is close and in front of the plant.
 * @return Whether the player is in sight.
 */
bool PackunFlower::isInSight() {
    sead::Vector3f playerPos = al::findNearestPlayerPos(this);

    if (al::isFar(this, playerPos, mIsBig ? 800.0f : 500.0f)) {
        return false;
    }

    sead::Vector3f dirToPlayer = playerPos - al::getTrans(this);
    sead::Vector3f front;
    al::calcQuatFront(&front, this);
    return al::isNearAngleDegreeHV(dirToPlayer, front, mUpDir, 5.0f, 45.0f);
}

/** @brief Quickly turns around towards a player behind the plant. */
void PackunFlower::exeTurnFast() {
    sead::Vector3f dirToPlayer = al::findNearestPlayerPos(this) - al::getTrans(this);
    al::normalizeOrDirZ(&dirToPlayer);

    if (al::isFirstStep(this)) {
        sead::Vector3f sideDir;
        al::calcSideDir(&sideDir, this);

        if (sideDir.dot(dirToPlayer) > 0.0f) {
            al::startAction(this, "TurnLeft");
        } else {
            al::startAction(this, "TurnRight");
        }

        mTurnDegree = mIsBig ? 4.0f : 4.8f;
    }

    sead::Vector3f front;
    al::calcQuatFront(&front, this);
    sead::Vector3f newFront = front;
    al::turnVecToVecCosOnPlane(&newFront, front, dirToPlayer, mUpDir,
                               cos(mTurnDegree / 180.0f * 3.14));
    sead::Quatf quat;
    al::makeQuatUpFront(&quat, mUpDir, newFront);
    al::setQuat(this, quat);

    if (isInSight()) {
        al::setNerve(this, &NrvPackunFlowerAttack);
        return;
    }

    if (al::isActionEnd(this)) {
        al::startAction(this, "Wait");
        al::setNerve(this, &NrvPackunFlowerTurn);
    }
}

/** @brief Bites towards the player. */
void PackunFlower::exeAttack() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Attack");
    }

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvPackunFlowerAfterAttack);
    }
}

/** @brief Rests for a moment after biting. */
void PackunFlower::exeAfterAttack() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait");
    }

    if (al::isGreaterEqualStep(this, 40)) {
        al::setNerve(this, &NrvPackunFlowerWait);
    }
}

/** @brief Gets squashed and dies, leaving the remains and an item. */
void PackunFlower::exePressDown() {
    if (al::isFirstStep(this)) {
        al::invalidateClipping(this);
        al::startAction(this, "PressDown");
        al::changeEnvTextureStamp(this);
        mMicRumbler->stopAndReset();
    }

    if (al::isActionEnd(this)) {
        al::validateClipping(this);
        al::resetEnvTexture(this);
        tryAppearTrace();
        al::appearItem(this);
        al::startHitReactionDeath(this);
        kill();
    }
}

/** @brief Gets blown away by the blow-down state, leaving the remains. */
void PackunFlower::exeBlowDown() {
    if (al::isFirstStep(this)) {
        al::onCollide(this);
        mMicRumbler->stopAndReset();
        tryAppearTrace();
    }

    if (al::updateNerveState(this)) {
        al::appearItem(this);
        al::startHitReactionDeath(this);
        kill();
    }
}

/** @brief Stays frozen by the touch screen until released. */
void PackunFlower::exeSupportFreeze() {
    if (al::updateNerveState(this)) {
        if (mIsBig) {
            mHitState = HitState::Healthy;
        }

        al::setNerve(this, &NrvPackunFlowerWait);
    }
}

/** @brief Stays frozen while swooning, then keeps swooning. */
void PackunFlower::exeSupportFreezeSwoon() {
    if (al::updateNerveState(this)) {
        al::setNerve(this, &NrvPackunFlowerSwoon);
    }
}

/** @brief Gets trampled (with a special reaction while biting), then swoons. */
void PackunFlower::exeTrampled() {
    if (al::isFirstStep(this)) {
        if (al::isActionPlaying(this, "Attack") && al::getActionFrame(this) > 83.0f &&
            al::getActionFrame(this) < 172.0f) {
            al::startAction(this, "TrampledAttack");
        } else {
            al::startAction(this, "Trampled");
        }

        if (mIsBig) {
            mHitState = HitState::Damaged;
        }
    }

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvPackunFlowerSwoon);
    }
}

/** @brief Swoons for a while. */
void PackunFlower::exeSwoon() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Swoon");
    }

    if (al::isGreaterEqualStep(this, 300)) {
        al::setNerve(this, &NrvPackunFlowerSwoonEnd);
    }
}

/** @brief Recovers from swooning. */
void PackunFlower::exeSwoonEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "SwoonEnd");

        if (mIsBig) {
            mHitState = HitState::Healthy;
        }
    }

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvPackunFlowerWait);
    }
}

/** @brief Reacts to a hit, then swoons. */
void PackunFlower::exeDamage() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Damage");

        if (mIsBig) {
            mHitState = HitState::Damaged;
        }
    }

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvPackunFlowerSwoon);
    }
}
