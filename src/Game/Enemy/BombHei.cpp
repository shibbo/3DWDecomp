#include "Enemy/BombHei.hpp"

#include <math/seadMatrix.h>

#include "Enemy/ActorStateSupportFreeze.hpp"
#include "Enemy/Bomb.hpp"
#include "Enemy/BombStateExplosion.hpp"
#include "Enemy/EnemyStateUtil.hpp"
#include "Enemy/TargetFinder.hpp"
#include "Enemy/WalkerStateChase.hpp"
#include "Enemy/WalkerStateFindPlayer.hpp"
#include "Enemy/WalkerStateParam.hpp"
#include "Enemy/WalkerStateWander.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Joint/JointControllerKeeper.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Math/MatrixUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/StageSwitch/StageSwitchFunc.hpp"
#include "Library/Thread/Functor.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ScoreUtil.hpp"

namespace {
NERVE_DECL(BombHei, Wander)
NERVE_DECL(BombHei, FindPlayer)
NERVE_DECL(BombHei, Chase)
NERVE_DECL(BombHei, Explosion)
// Freeze while the fuse is lit: the countdown keeps running.
class BombHeiNrvSupportFreezeCountDown : public al::Nerve {
public:
    void execute(al::NerveKeeper* pKeeper) const override {
        (pKeeper->getParent<BombHei>())->exeSupportFreeze();
    }
};
NERVE_DECL(BombHei, SupportFreeze)
NERVE_DECL(BombHei, Jump)
NERVE_DECL(BombHei, LaunchReady)
NERVE_DECL(BombHei, Generate)
NERVE_DECL(BombHei, Launch)
NERVE_DECL(BombHei, LaunchLand)

BombHeiNrvWander NrvBombHeiWander;
BombHeiNrvFindPlayer NrvBombHeiFindPlayer;
BombHeiNrvChase NrvBombHeiChase;
BombHeiNrvExplosion NrvBombHeiExplosion;
BombHeiNrvSupportFreezeCountDown NrvBombHeiSupportFreezeCountDown;
BombHeiNrvSupportFreeze NrvBombHeiSupportFreeze;
BombHeiNrvJump NrvBombHeiJump;
BombHeiNrvLaunchReady NrvBombHeiLaunchReady;
BombHeiNrvGenerate NrvBombHeiGenerate;
BombHeiNrvLaunch NrvBombHeiLaunch;
BombHeiNrvLaunchLand NrvBombHeiLaunchLand;

typedef al::FunctorV0M<BombHei*, void (BombHei::*)()> BombHeiFunctor;

const sead::Vector3f sLaunchReadyOffset(0.0f, 150.0f, 100.0f);
const sead::Vector3f sLaunchVelocity(0.0f, 20.0f, 10.0f);
const sead::Vector3f sGenerateOffset(0.0f, 50.0f, 0.0f);
const sead::Vector3f sGenerateVelocity(0.0f, 25.0f, 8.0f);
TargetFinderParam sTargetFinderParam(750.0f, 120.0f, 80.0f, 60, 750.0f, -1.0f, -1.0f, -1.0f,
                                     false);
WalkerStateParam sWalkerStateParam(2.25f, 0.98f, 0.85f, 500.0f, 750.0f, 70.0f, 80.0f, 150.0f);
WalkerStateWanderParam sWanderParam(120, 150, 0.4f, 2.5f, 20.0f, 750.0f, true, "Walk", "Wait");
WalkerStateFindPlayerParam sFindPlayerParam(30, 6.0f, false, "Walk");
WalkerStateChaseParam sChaseParam(0.9f, 60.0f, 60.0f, 3.5f, 60.0f, true, true, "Run", "Wait",
                                  -1.0f);
ActorStateSupportFreezeParam sSupportFreezeParam(true, 15, false, true, 120,
                                                 sead::Vector3f(0.0f, 150.0f, 0.0f));
}  // namespace

/**
 * @brief Constructs a BombHei.
 * @param pName Actor name.
 */
BombHei::BombHei(const char* pName) : al::LiveActor(pName) {}

/**
 * @brief Initializes the model, states, the Bomb it turns into and the appear switch.
 * @param rInfo Placement info of the actor.
 */
void BombHei::init(const al::ActorInitInfo& rInfo) {
    if (rInfo.getActorSceneInfo().isSingleMode) {
        al::initActorWithArchiveName(this, rInfo, "BombHeiFur", nullptr);
    } else {
        al::initActorWithArchiveName(this, rInfo, "BombHei", nullptr);
    }

    al::initNerve(this, &NrvBombHeiWander, 6);
    mTargetFinder = new TargetFinder(this, &sTargetFinderParam);
    mStateWander = new WalkerStateWander(this, al::getFrontPtr(this), &sWalkerStateParam,
                                         &sWanderParam, nullptr);
    mStateFindPlayer =
        new WalkerStateFindPlayer(this, al::getFrontPtr(this), mTargetFinder, &sWalkerStateParam,
                                  &sFindPlayerParam, nullptr);
    mStateChase = new WalkerStateChase(this, al::getFrontPtr(this), mTargetFinder,
                                       &sWalkerStateParam, &sChaseParam, false, nullptr);
    mStateExplosion = new BombStateExplosion(this, true, nullptr);
    mStateSupportFreeze = new ActorStateSupportFreeze(this, &sSupportFreezeParam);
    al::initNerveState(this, mStateWander, &NrvBombHeiWander, "うろつき");
    al::initNerveState(this, mStateFindPlayer, &NrvBombHeiFindPlayer, "プレーヤー発見");
    al::initNerveState(this, mStateChase, &NrvBombHeiChase, "追いかけ");
    al::initNerveState(this, mStateExplosion, &NrvBombHeiExplosion, "爆発");
    al::initNerveState(this, mStateSupportFreeze, &NrvBombHeiSupportFreezeCountDown,
                       "フリーズ[カウントダウン中]");
    al::addNerveState(this, mStateSupportFreeze, &NrvBombHeiSupportFreeze, "フリーズ");
    al::initJointControllerKeeper(this, 1);
    al::initJointLocalXRotator(this, &mSpringAngle, "Zenmai");
    mIsSingleMode = rInfo.getActorSceneInfo().isSingleMode;

    mBomb = new Bomb("バクダン", false);
    al::initCreateActorWithPlacementInfo(mBomb, rInfo);
    mBomb->makeActorDead();
    if (al::listenStageSwitchOn(this, "SwitchAppear",
                                BombHeiFunctor(this, &BombHei::makeActorAppeared))) {
        makeActorDead();
    } else {
        makeActorAppeared();
    }

    const sead::Vector3f& trans = al::getTrans(this);
    mInitTrans.x = trans.x;
    mInitTrans.y = trans.y;
    mInitTrans.z = trans.z;
    const sead::Vector3f& front = al::getFront(this);
    mInitFront.x = front.x;
    mInitFront.y = front.y;
    mInitFront.z = front.z;
}

/** @brief Tells the floor it is being walked on. */
void BombHei::control() {
    al::HitSensor* groundSensor = al::tryGetCollidedGroundSensor(this);
    if (groundSensor != nullptr) {
        al::sendMsgEnemyFloorTouch(groundSensor, al::getHitSensor(this, "Body"));
    }
}

/** @brief Appears with the fuse unlit and the spring reset. */
void BombHei::makeActorAppeared() {
    mStateExplosion->reset();
    al::validateClipping(this);
    mControlUserId = -1;
    mCountDown = -1;
    mSpringAngle = 0.0f;
    mComboCounter = nullptr;
    al::LiveActor::makeActorAppeared();
    al::startMclAnim(this, "Default");
}

/** @brief Disappears together with its Bomb when the kill stage switch turns on. */
void BombHei::killBySwitch() {
    if (al::isAlive(this)) {
        al::startHitReactionDisappear(this);
        al::LiveActor::kill();
    }

    if (al::isAlive(mBomb)) {
        al::startHitReactionDisappear(mBomb);
        mBomb->kill();
    }
}

/** @brief Kills itself and its Bomb silently so that it can be respawned. */
void BombHei::killForRespawn() {
    if (al::isAlive(this)) {
        al::LiveActor::kill();
    }

    if (al::isAlive(mBomb)) {
        mBomb->kill();
    }
}

/**
 * @brief Damages everything around while exploding, otherwise pushes enemies, players and objects.
 * @param pSelf Sensor of the BombHei.
 * @param pOther Sensor that was hit.
 */
void BombHei::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isNerve(this, &NrvBombHeiExplosion)) {
        mStateExplosion->attackSensor(pSelf, pOther, mComboCounter);
        return;
    }

    if (!al::isSensorName(pSelf, "Body")) {
        return;
    }

    if (al::isSensorEnemyBody(pOther)) {
        al::sendMsgPushAndKillVelocityToTarget(this, pSelf, pOther);
        return;
    }

    if (al::isSensorPlayer(pOther) || al::isSensorKoopaJr(pOther)) {
        al::sendMsgPush(pOther, pSelf);
        return;
    }

    if (GameDataFunction::isSingleMode(this) &&
        (al::isSensorNpc(pOther) || al::isSensorMapObj(pOther))) {
        al::sendMsgPush(pOther, pSelf);
    }
}

/**
 * @brief Handles restoring, explosions, attacks that turn it into a Bomb, pushes and stomps.
 * @param pMsg Received message.
 * @param pOther Sensor that sent the message.
 * @param pSelf Sensor of the BombHei that received it.
 * @return Whether the message was handled.
 */
bool BombHei::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) {
    if (rc::isMsgAskControlUserId(pMsg, mControlUserId)) {
        return true;
    }

    if (al::isMsgRestore(pMsg)) {
        makeActorAppeared();
        mIsRestored = true;
        al::resetPosition(this, mInitTrans, false);
        al::setFront(this, mInitFront);
        al::setNerve(this, &NrvBombHeiWander);
        return true;
    }

    if (al::isNerve(this, &NrvBombHeiExplosion) || al::isNerve(this, &NrvBombHeiJump)) {
        return false;
    }

    if (al::isSensorName(pSelf, "Body")) {
        if (!al::isNerve(this, &NrvBombHeiLaunchReady)) {
            if (al::isMsgExplosion(pMsg) || al::isMsgKillerAttack(pMsg) ||
                al::isMsgPlayerFireBallAttack(pMsg) || al::isMsgEnemyAttackFire(pMsg) ||
                al::isMsgPlayerGiantAttack(pMsg) || al::isMsgPlayerGiantHipDrop(pMsg) ||
                al::isMsgPlayerCooperationHipDrop(pMsg) || al::isMsgLaserAttack(pMsg) ||
                al::isMsgGigaEnemyAttack(pMsg) ||
                (mIsSingleMode && rc::isMsgBobsledBodyAttack(pMsg))) {
                if (al::isMsgEnemyAttackFire(pMsg) && al::isSensorPlessie(pOther)) {
                    return false;
                }

                if (!al::isMsgGigaEnemyAttack(pMsg)) {
                    rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
                }

                if (al::isMsgPlayerGiantAttack(pMsg) || al::isMsgLaserAttack(pMsg) ||
                    al::isMsgGigaEnemyAttack(pMsg)) {
                    al::setNerve(this, &NrvBombHeiExplosion);
                } else {
                    al::setNerve(this, &NrvBombHeiJump);
                }

                if (al::isMsgExplosion(pMsg)) {
                    mControlUserId = rc::tryFindRelativeControlUserId(pOther);
                    mComboCounter = rc::tryGetMsgComboCount(pMsg);
                }

                return true;
            }

            if (al::tryReceiveMsgPushAndAddVelocity(this, pMsg, pOther, pSelf, 5.0f)) {
                if (mIsSingleMode && al::isSensorHostName(pOther, "GigaBell")) {
                    al::setNerve(this, &NrvBombHeiExplosion);
                }

                return true;
            }
        }

        if (!isLaunching() &&
            (al::isMsgPlayerBoomerangAttack(pMsg) || al::isMsgPlayerClimbAttack(pMsg) ||
             al::isMsgPlayerClimbSlidingAttack(pMsg) || al::isMsgPlayerClimbRollingAttack(pMsg) ||
             al::isMsgPlayerInvincibleAttack(pMsg) || al::isMsgPlayerSlidingAttack(pMsg) ||
             al::isMsgPlayerBodyAttack(pMsg) || al::isMsgPlayerBodyLanding(pMsg) ||
             al::isMsgPlayerTailAttack(pMsg) || al::isMsgPlayerSpinAttack(pMsg) ||
             al::isMsgBallAttack(pMsg) || al::isMsgBallTrample(pMsg) ||
             al::isMsgKickKouraAttack(pMsg) || al::isMsgPlayerKouraAttack(pMsg) ||
             al::isMsgKeyThrow(pMsg) || rc::isMsgSkateShoesAttack(pMsg))) {
            rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
            al::startSe(this, "Blow");
            changeToBomb(pMsg, pOther);
            return true;
        }
    }

    if (!al::isSensorName(pSelf, "Trample") || isLaunching()) {
        return false;
    }

    if (al::isMsgPlayerTrample(pMsg) && !(rc::getPlayerVelocity(pOther).y < 0.0f)) {
        return false;
    }

    if (!EnemyStateUtil::tryRequestPressDown(pMsg, pOther, pSelf, false)) {
        return false;
    }

    al::startSe(this, "Stomped");
    changeToBomb(pMsg, pOther);
    return true;
}

/**
 * @brief Checks whether it is being launched by a launcher or a generator.
 * @return Whether it is getting ready for, or flying in, a launch.
 */
bool BombHei::isLaunching() const {
    return al::isNerve(this, &NrvBombHeiLaunchReady) || al::isNerve(this, &NrvBombHeiLaunch) ||
           al::isNerve(this, &NrvBombHeiGenerate);
}

/**
 * @brief Disappears and leaves a Bomb behind in its place.
 * @param pMsg Message that defeated it.
 * @param pOther Sensor of the attacker.
 */
void BombHei::changeToBomb(const al::SensorMsg* pMsg, al::HitSensor* pOther) {
    mBomb->appearTrampled(this, pMsg);
    kill();
    rc::addScoreCombo(this, pOther, pMsg, 0.0f);
}

/**
 * @brief Handles touch screen attacks, which turn it into a Bomb, and freezing by touch.
 * @param pMsg Received message.
 * @param pPointer Screen pointer that sent the message.
 * @param pTarget Screen point target of the BombHei.
 * @return Whether the message was handled.
 */
bool BombHei::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                    al::ScreenPointTarget* pTarget) {
    if (al::isNerve(this, &NrvBombHeiExplosion) || al::isNerve(this, &NrvBombHeiJump)) {
        return false;
    }

    if (al::isMsgTouchAssistTrig(pMsg)) {
        al::startHitReactionBlowHitDirect(this);
        al::startSe(this, "Blow");
        mBomb->appearTrampled(this, pMsg);
        kill();
        rc::addScoreCombo(this, pPointer, pMsg, 0.0f);
        return true;
    }

    if (al::isNerve(this, &NrvBombHeiFindPlayer)) {
        return false;
    }

    if (!mStateSupportFreeze->receiveMsgScreenPoint(pMsg, pPointer, pTarget)) {
        return false;
    }

    if (al::isNerve(this, &NrvBombHeiSupportFreeze) ||
        al::isNerve(this, &NrvBombHeiSupportFreezeCountDown)) {
        return true;
    }

    if (al::isNerve(this, &NrvBombHeiChase)) {
        al::setNerve(this, &NrvBombHeiSupportFreezeCountDown);
    } else {
        al::setNerve(this, &NrvBombHeiSupportFreeze);
    }

    return true;
}

/**
 * @brief Appears on top of a launcher, ready to be launched.
 * @param pLauncher Launcher actor.
 */
void BombHei::appearLaunchReady(const al::LiveActor* pLauncher) {
    sead::Matrix34f mtx;
    al::makeMtxRT(&mtx, pLauncher);
    sead::Vector3f trans;
    al::calcTransLocalOffsetByMtx(&trans, mtx, sLaunchReadyOffset);
    mtx.setTranslation(trans);
    al::updatePoseMtx(this, &mtx);
    al::resetPosition(this, false);
    al::setVelocityZero(this);
    appear();
    al::setNerve(this, &NrvBombHeiLaunchReady);
}

/**
 * @brief Appears from a generator and jumps out of it.
 * @param pGenerator Generator actor.
 */
void BombHei::appearGenerate(const al::LiveActor* pGenerator) {
    sead::Matrix34f mtx;
    al::makeMtxRT(&mtx, pGenerator);
    sead::Vector3f trans;
    al::calcTransLocalOffsetByMtx(&trans, mtx, sGenerateOffset);
    mtx.setTranslation(trans);
    al::updatePoseMtx(this, &mtx);
    al::resetPosition(this, false);
    al::setVelocityZero(this);
    appear();
    al::setNerve(this, &NrvBombHeiGenerate);
}

/** @brief Launches it from the launcher. */
void BombHei::launch() {
    al::setNerve(this, &NrvBombHeiLaunch);
}

/**
 * @brief Checks whether both the BombHei and its Bomb are dead.
 * @return Whether both are dead.
 */
bool BombHei::isDeadWithBomb() const {
    return al::isDead(this) && al::isDead(mBomb);
}

/**
 * @brief Lights the fuse unless it is already burning.
 * @return Whether the countdown was started.
 */
bool BombHei::tryStartCountDown() {
    if (mCountDown >= 0) {
        return false;
    }

    mCountDown = 240;
    al::invalidateClipping(this);
    return true;
}

/**
 * @brief Advances the fuse countdown, blinking and playing the fuse sound near the end.
 * @return Whether the countdown ended and it started exploding.
 */
bool BombHei::updateCountDown() {
    if (mCountDown < 0) {
        return false;
    }

    mCountDown--;
    if (mCountDown <= 0) {
        al::setNerve(this, &NrvBombHeiExplosion);
        return true;
    }

    if (mCountDown == 120) {
        al::startMclAnim(this, "Blink");
    }

    if (240 - mCountDown >= 63) {
        al::holdSe(this, "PgFuseLv");
    }

    if (mCountDown <= 120) {
        al::holdSe(this, "Blink");
    }

    return false;
}

/**
 * @brief Explodes in kill areas, on kill materials or, in Bowser's Fury, in water.
 * @return Whether it started exploding.
 */
bool BombHei::tryExplosionByAreaOrMaterialCode() {
    if (EnemyStateUtil::isKillByAreaOrMaterialCode(this) ||
        (mIsSingleMode && rc::isInWaterArea(this))) {
        mStateExplosion->setIsAttackToPlayer(false);
        al::setNerve(this, &NrvBombHeiExplosion);
        return true;
    }

    return false;
}

/**
 * @brief Turns the wind-up key on its back.
 * @param speed Rotation in degrees.
 */
void BombHei::rotateSpring(f32 speed) {
    mSpringAngle = al::wrapAngle(mSpringAngle + speed);
}

/** @brief Wanders around until a player is found. */
void BombHei::exeWander() {
    al::updateNerveState(this);
    rotateSpring(2.5f);
    if (tryExplosionByAreaOrMaterialCode()) {
        return;
    }

    if (updateCountDown()) {
        return;
    }

    mTargetFinder->update();
    if (mTargetFinder->isExistTarget()) {
        al::setNerve(this, &NrvBombHeiFindPlayer);
    }
}

/** @brief Lights the fuse and turns to the player that was found. */
void BombHei::exeFindPlayer() {
    if (al::isFirstStep(this)) {
        tryStartCountDown();
    }

    al::updateNerveStateAndNextNerve(this, &NrvBombHeiChase);
    rotateSpring(2.5f);
    if (tryExplosionByAreaOrMaterialCode()) {
        return;
    }

    updateCountDown();
}

/** @brief Runs after the player while the fuse burns. */
void BombHei::exeChase() {
    al::updateNerveState(this);
    rotateSpring(10.0f);
    if (tryExplosionByAreaOrMaterialCode()) {
        return;
    }

    updateCountDown();
}

/** @brief Jumps up after being hit and explodes when landing. */
void BombHei::exeJump() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Jump");
        al::setVelocityToDirection(this, sead::Vector3f::ey, 23.0f);
    }

    al::addVelocityToGravity(this, 1.2f);
    if (al::isOnGround(this, 0, 0.0f) || al::isActionEnd(this)) {
        al::setNerve(this, &NrvBombHeiExplosion);
    }
}

/** @brief Explodes and dies once the explosion is over. */
void BombHei::exeExplosion() {
    if (al::updateNerveState(this)) {
        kill();
    }
}

/** @brief Stays frozen by touch, keeping the fuse burning if it was lit. */
void BombHei::exeSupportFreeze() {
    if (al::isNerve(this, &NrvBombHeiSupportFreezeCountDown) && updateCountDown()) {
        return;
    }

    if (al::updateNerveState(this)) {
        if (al::isNerve(this, &NrvBombHeiSupportFreezeCountDown)) {
            al::setNerve(this, &NrvBombHeiChase);
        } else {
            mStateWander->setWanderCenter(al::getTrans(this));
            al::setNerve(this, &NrvBombHeiWander);
        }
    }
}

/** @brief Waits on the launcher. */
void BombHei::exeLaunchReady() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Set");
    }
}

/** @brief Flies out of the launcher until it lands. */
void BombHei::exeLaunch() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Launch");
        sead::Matrix34f mtx;
        al::makeMtxSRT(&mtx, this);
        mtx.setTranslation(sead::Vector3f::zero);
        al::getVelocityPtr(this)->setMul(mtx, sLaunchVelocity);
    }

    al::addVelocityToGravity(this, 1.2f);
    if (al::isOnGround(this, 0, 0.0f)) {
        al::setVelocityZero(this);
        al::setNerve(this, &NrvBombHeiLaunchLand);
    }
}

/** @brief Lands after being launched and starts wandering. */
void BombHei::exeLaunchLand() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Land");
    }

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvBombHeiWander);
    }
}

/** @brief Jumps out of the generator until it lands. */
void BombHei::exeGenerate() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Launch");
        sead::Matrix34f mtx;
        al::makeMtxSRT(&mtx, this);
        mtx.setTranslation(sead::Vector3f::zero);
        al::getVelocityPtr(this)->setMul(mtx, sGenerateVelocity);
    }

    al::addVelocityToGravity(this, 1.2f);
    if (al::isOnGround(this, 0, 0.0f)) {
        al::setVelocityZero(this);
        al::setNerve(this, &NrvBombHeiLaunchLand);
    }
}
