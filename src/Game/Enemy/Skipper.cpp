#include "Enemy/Skipper.hpp"

#include <math/seadVector.h>

#include "Enemy/ActorStateSupportFreeze.hpp"
#include "Enemy/EnemyStateBlowDown.hpp"
#include "Enemy/EnemyStateUtil.hpp"
#include "Enemy/SkipperTrampoline.hpp"
#include "Enemy/TargetFinder.hpp"
#include "Enemy/WalkerStateChase.hpp"
#include "Enemy/WalkerStateFunction.hpp"
#include "Enemy/WalkerStateJump.hpp"
#include "Enemy/WalkerStateParam.hpp"
#include "Enemy/WalkerStateWander.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/ActorAreaFunction.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/StageSwitch/StageSwitchFunc.hpp"
#include "Library/Thread/Functor.hpp"
#include "Project/AreaObj/AreaObjUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ScoreUtil.hpp"

namespace {
NERVE_DECL(Skipper, Wander)
NERVE_DECL(Skipper, BlowDown)
NERVE_DECL(Skipper, Chase)
NERVE_DECL(Skipper, SupportFreeze)
NERVE_DECL(Skipper, After)
NERVE_DECL(Skipper, RecoverEnd)
NERVE_DECL(Skipper, Flip)
NERVE_DECL(Skipper, AttackSuccess)
NERVE_DECL(Skipper, Trampled)
NERVE_DECL(Skipper, Fall)
NERVE_DECL(Skipper, TurnToPlayer)
NERVE_DECL(Skipper, Find)
NERVE_DECL(Skipper, StandByWander)
NERVE_DECL(Skipper, Angry)
NERVE_DECL(Skipper, Land)

SkipperNrvLand NrvSkipperLand;
SkipperNrvWander NrvSkipperWander;
SkipperNrvBlowDown NrvSkipperBlowDown;
SkipperNrvChase NrvSkipperChase;
SkipperNrvSupportFreeze NrvSkipperSupportFreeze;
SkipperNrvAfter NrvSkipperAfter;
SkipperNrvRecoverEnd NrvSkipperRecoverEnd;
SkipperNrvFlip NrvSkipperFlip;
SkipperNrvAttackSuccess NrvSkipperAttackSuccess;
SkipperNrvTrampled NrvSkipperTrampled;
SkipperNrvFall NrvSkipperFall;
SkipperNrvTurnToPlayer NrvSkipperTurnToPlayer;
SkipperNrvFind NrvSkipperFind;
SkipperNrvStandByWander NrvSkipperStandByWander;
SkipperNrvAngry NrvSkipperAngry;

typedef al::FunctorV0M<Skipper*, void (Skipper::*)()> SkipperFunctor;

EnemyStateBlowDownParam sBlowDownParam(false);
TargetFinderParam sTargetFinderParam(1500.0f, 120.0f, 80.0f, 60, 5000.0f, -1.0f, -1.0f, -1.0f,
                                     false);
WalkerStateParam sWalkerStateParam(2.5f, 0.98f, 0.86f, 250.0f, 700.0f, 180.0f, 70.0f, 150.0f);
WalkerStateChaseParam sChaseParam(0.7f, 110.0f, 270.0f, 5.0f, 0.0f, false, true, "Run", "Wait",
                                  -1.0f);
WalkerStateJumpParam sTrampolineJumpParam(30.0f, "JumpTrampoline", false);
WalkerStateWanderParam sWanderParam(95, 173, 0.35f, 2.0f, 10.0f, 750.0f, true, "Walk", "Wait");
ActorStateSupportFreezeParam sSupportFreezeParam(true, 15, false, true, 120,
                                                 sead::Vector3f(0.0f, 150.0f, 0.0f));
}  // namespace

/**
 * @brief Constructs a Skipper.
 * @param pName Actor name.
 */
Skipper::Skipper(const char* pName) : al::LiveActor(pName) {}

/**
 * @brief Initializes the model, states, nerves, the trampoline and the chase area.
 * @param rInfo Placement info of the actor.
 */
void Skipper::init(const al::ActorInitInfo& rInfo) {
    al::initActor(this, rInfo);
    mTargetFinder = new TargetFinder(this, &sTargetFinderParam);
    mStateBlowDown = new EnemyStateBlowDown(this, &sBlowDownParam);
    mStateChase = new WalkerStateChase(this, al::getFrontPtr(this), mTargetFinder,
                                       &sWalkerStateParam, &sChaseParam, false, nullptr);
    mStateWander = new WalkerStateWander(this, al::getFrontPtr(this), &sWalkerStateParam,
                                         &sWanderParam, nullptr);
    mStateSupportFreeze = new ActorStateSupportFreeze(this, &sSupportFreezeParam);

    al::initNerve(this, &NrvSkipperWander, 5);
    al::initNerveState(this, mStateBlowDown, &NrvSkipperBlowDown, "吹き飛び死亡");
    al::initNerveState(this, mStateChase, &NrvSkipperChase, "追いかけ");
    al::initNerveState(this, mStateWander, &NrvSkipperWander, "うろつき");
    al::initNerveState(this, mStateSupportFreeze, &NrvSkipperSupportFreeze, "フリーズ");

    mTrampoline = new SkipperTrampoline("SkipperTrampoline");
    al::initCreateActorWithPlacementInfo(mTrampoline, rInfo);
    al::setTrans(mTrampoline, al::getTrans(this));
    mTrampoline->makeActorDead();

    mChaseAreaGroup = al::createLinkAreaGroup(this, rInfo, "ChaseArea",
                                              "追いかけ有効エリアグループ", "子供エリア");
    al::createAndSetColliderSpecialPurpose(this, "MoveLimit");
    al::listenStageSwitchOnKill(this, SkipperFunctor(this, &Skipper::killSwitch));
    makeActorAppeared();
}

/**
 * @brief Checks whether the Skipper is stomped, blown away or turned into its trampoline.
 * @return Whether the Skipper is already being defeated.
 */
inline bool Skipper::isNerveDownOrAfter() const {
    return al::isNerve(this, &NrvSkipperTrampled) || al::isNerve(this, &NrvSkipperBlowDown) ||
           al::isNerve(this, &NrvSkipperAfter);
}

/** @brief Kills the Skipper and its trampoline when the kill switch turns on. */
void Skipper::killSwitch() {
    if (!isNerveDownOrAfter()) {
        al::startHitReactionDisappear(this);
    }

    mTrampoline->kill();
    kill();
}

/** @brief Kills the Skipper unless it is already being defeated, and its trampoline. */
void Skipper::kill() {
    if (!isNerveDownOrAfter()) {
        al::startHitReactionDisappear(this);
        al::LiveActor::kill();
    }

    if (al::isAlive(mTrampoline)) {
        mTrampoline->kill();
    }
}

/**
 * @brief Checks whether the Skipper is not already being defeated or flipped into a trampoline.
 * @return Whether the Skipper can still be killed normally.
 */
bool Skipper::isEnableKill() const {
    return !isNerveDownOrAfter();
}

/** @brief Handles kill areas and bouncing on trampolines. */
void Skipper::control() {
    EnemyStateUtil::tryKillByAreaOrMaterialCodeWithHitReaction(this);
    if (al::isNerve(this, &NrvSkipperAfter) || !al::isCollidedGround(this)) {
        return;
    }

    al::HitSensor* groundSensor = al::tryGetCollidedGroundSensor(this);
    if (groundSensor == nullptr) {
        return;
    }

    al::sendMsgEnemyFloorTouch(groundSensor, al::getHitSensor(this, "Body"));
    if (rc::sendMsgEnemyFloorTouchTrampoline(groundSensor, al::getHitSensor(this, "Body")) &&
        al::getVelocity(this).y <= 0.0f) {
        al::addVelocity(this, sead::Vector3f::ey * 50.0f);
        al::startAction(this, "FallStart");
    }
}

/**
 * @brief Attacks players and pushes other enemies.
 * @param pSelf Sensor of the Skipper.
 * @param pOther Sensor that was touched.
 */
void Skipper::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isNerve(this, &NrvSkipperRecoverEnd) && al::isSensorEnemyAttack(pSelf)) {
        if (al::isSensorPlayer(pOther) && al::getVelocity(this).y < -10.0f) {
            al::sendMsgEnemyAttack(pOther, pSelf);
            return;
        }

        if (al::isSensorEnemy(pOther)) {
            al::sendMsgBallAttack(pOther, pSelf, nullptr);
            return;
        }
    }

    if (isNerveAttackable()) {
        if (al::isSensorEnemyBody(pSelf) && al::isSensorEnemyBody(pOther)) {
            al::sendMsgPushAndKillVelocityToTarget(this, pSelf, pOther);
        }

        if (al::isSensorEnemyAttack(pSelf) && al::isSensorPlayer(pOther)) {
            al::sendMsgPush(pOther, pSelf);
            if (!al::isNerve(this, &NrvSkipperFlip) && al::sendMsgEnemyAttack(pOther, pSelf)) {
                al::faceToTarget(this, al::getSensorPos(pOther));
                al::setNerve(this, &NrvSkipperAttackSuccess);
            }
        }
    }
}

/**
 * @brief Checks whether the Skipper is in a nerve that can attack.
 * @return Whether the Skipper can attack.
 */
bool Skipper::isNerveAttackable() const {
    return !al::isNerve(this, &NrvSkipperTrampled) && !al::isNerve(this, &NrvSkipperBlowDown) &&
           !al::isNerve(this, &NrvSkipperAfter) && !al::isNerve(this, &NrvSkipperRecoverEnd);
}

/**
 * @brief Handles goal kills, stomps, push messages and blow-down attacks.
 * @param pMsg Received message.
 * @param pOther Sensor that sent the message.
 * @param pSelf Sensor of the Skipper.
 * @return Whether the message was handled.
 */
bool Skipper::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) {
    if (isEnableKill() && al::isMsgGoalKill(pMsg)) {
        al::startHitReactionDeath(this);
        al::setAppearItemFactor(this, "直接攻撃", pOther);
        al::appearItemTiming(this, "Coin");
        rc::addScoreCombo(this, pOther, pMsg, 100.0f);
        kill();
        return true;
    }

    if (isEnableDown()) {
        if (al::tryReceiveMsgPushAndAddVelocityH(this, pMsg, pOther, pSelf, 5.0f)) {
            return true;
        }

        if (al::isSensorEnemyBody(pSelf)) {
            if (EnemyStateUtil::tryRequestPressDownAndNextNerve(pMsg, pOther, pSelf, this,
                                                                &NrvSkipperTrampled, true)) {
                rc::addScoreCombo(this, pOther, pMsg, 100.0f);
                return true;
            }

            if ((EnemyStateUtil::isMsgBlowDown(pMsg) && !al::isMsgExplosion(pMsg)) ||
                al::isMsgPlayerInvincibleAttack(pMsg)) {
                rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
                rc::addScoreCombo(this, pOther, pMsg, 100.0f);
                al::setNerve(this, &NrvSkipperFlip);
                return true;
            }
        }

        if (al::isMsgExplosion(pMsg)) {
            return true;
        }
    }

    return false;
}

/**
 * @brief Checks whether the Skipper can be knocked down or frozen.
 * @return Whether the Skipper can be knocked down.
 */
bool Skipper::isEnableDown() const {
    return !al::isNerve(this, &NrvSkipperTrampled) && !al::isNerve(this, &NrvSkipperBlowDown) &&
           !al::isNerve(this, &NrvSkipperFlip) && !al::isNerve(this, &NrvSkipperAfter);
}

/**
 * @brief Handles touch attacks (flip) and touch freezing.
 * @param pMsg Received message.
 * @param pPointer Screen pointer that sent the message.
 * @param pTarget Screen point target of the Skipper.
 * @return Whether the message was handled.
 */
bool Skipper::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                    al::ScreenPointTarget* pTarget) {
    if (!isEnableDown()) {
        return false;
    }

    if (al::isMsgTouchAssistTrig(pMsg)) {
        al::startHitReactionBlowHitDirect(this);
        al::setNerve(this, &NrvSkipperFlip);
        rc::addScoreCombo(this, pPointer, pMsg, 100.0f);
        return true;
    }

    if (!mStateSupportFreeze->receiveMsgScreenPoint(pMsg, pPointer, pTarget)) {
        return false;
    }

    if (!al::isNerve(this, &NrvSkipperSupportFreeze)) {
        al::setNerve(this, &NrvSkipperSupportFreeze);
    }
    return true;
}

/**
 * @brief Checks whether the target left the chase area while the Skipper is still inside it.
 * @param pTarget Chased actor.
 * @return Whether the chase should stop.
 */
inline bool Skipper::isTargetLeftChaseArea(const al::LiveActor* pTarget) const {
    return mChaseAreaGroup != nullptr &&
           !al::tryIsInAreaObj(mChaseAreaGroup, al::getTrans(pTarget)) &&
           al::tryIsInAreaObj(mChaseAreaGroup, al::getTrans(this));
}

/**
 * @brief Checks whether a found target may be chased with respect to the chase area.
 * @param pTarget Found actor.
 * @return Whether the target may be chased.
 */
inline bool Skipper::isEnableChaseTarget(const al::LiveActor* pTarget) const {
    return mChaseAreaGroup == nullptr ||
           (al::tryIsInAreaObj(mChaseAreaGroup, al::getTrans(pTarget)) &&
            al::tryIsInAreaObj(mChaseAreaGroup, al::getTrans(this))) ||
           !al::tryIsInAreaObj(mChaseAreaGroup, al::getTrans(this));
}

/** @brief Waits for the run animation to finish before wandering again. */
void Skipper::exeStandByWander() {
    WalkerStateFunction::calcPassiveMovement(this, &sWalkerStateParam);
    if (al::isActionPlaying(this, "Run") &&
        al::getActionFrame(this) == al::getActionFrameMax(this, "Run") - 1.0f) {
        al::setNerve(this, &NrvSkipperWander);
    }
}

/** @brief Wanders around and turns to a found player. */
void Skipper::exeWander() {
    al::updateNerveState(this);
    if (mStateWander->isWait()) {
        al::setVelocityZeroH(this);
    }

    if (!al::isOnGround(this, 5, 0.0f)) {
        al::setNerve(this, &NrvSkipperFall);
        return;
    }

    mTargetFinder->update();
    if (mTargetFinder->isFoundTarget() && isEnableChaseTarget(mTargetFinder->getTarget()) &&
        al::isGreaterEqualStep(this, sWanderParam.mWaitTime) && al::isOnGround(this, 0, 0.0f)) {
        al::setNerve(this, &NrvSkipperTurnToPlayer);
    }
}

/** @brief Turns to face the found player. */
void Skipper::exeTurnToPlayer() {
    if (al::isFirstStep(this)) {
        al::setVelocityZero(this);
        al::startAction(this, "Walk");
    }

    WalkerStateFunction::calcPassiveMovement(this, &sWalkerStateParam);
    if (!al::isOnGround(this, 5, 0.0f)) {
        al::setNerve(this, &NrvSkipperFall);
        return;
    }

    if (al::turnDirectionToTargetDegree(this, al::getFrontPtr(this),
                                        al::getTrans(mTargetFinder->getTarget()), 8.0f)) {
        al::setNerve(this, &NrvSkipperFind);
    }
}

/** @brief Plays the find reaction before chasing. */
void Skipper::exeFind() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Find");
    }

    WalkerStateFunction::calcPassiveMovement(this, &sWalkerStateParam);
    if (!al::isOnGround(this, 5, 0.0f)) {
        al::setNerve(this, &NrvSkipperFall);
        return;
    }

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvSkipperChase);
    }
}

/** @brief Chases the target until it is lost or leaves the chase area. */
void Skipper::exeChase() {
    const al::LiveActor* target = mTargetFinder->getTarget();
    if (target == nullptr) {
        al::setNerve(this, &NrvSkipperWander);
        return;
    }

    if (isTargetLeftChaseArea(target)) {
        if (al::isActionPlaying(this, "Run")) {
            al::setNerve(this, &NrvSkipperStandByWander);
            return;
        }

        al::setNerve(this, &NrvSkipperWander);
        return;
    }

    if (!al::isOnGround(this, 5, 0.0f)) {
        al::setNerve(this, &NrvSkipperFall);
        return;
    }

    if (al::updateNerveState(this)) {
        al::setNerve(this, &NrvSkipperAngry);
    }
}

/** @brief Plays the angry reaction, then resumes chasing. */
void Skipper::exeAngry() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Angry");
    }

    const al::LiveActor* target = mTargetFinder->getTarget();
    if (target == nullptr || isTargetLeftChaseArea(target)) {
        al::setNerve(this, &NrvSkipperWander);
        return;
    }

    WalkerStateFunction::calcPassiveMovement(this, &sWalkerStateParam);
    if (!al::isOnGround(this, 5, 0.0f)) {
        al::setNerve(this, &NrvSkipperFall);
        return;
    }

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvSkipperChase);
    }
}

/** @brief Gets stomped, then flips over. */
void Skipper::exeTrampled() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Trampled");
        al::setVelocityZero(this);
    }

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvSkipperFlip);
    }
}

/** @brief Jumps up while flipping over, then turns into the trampoline. */
void Skipper::exeFlip() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Flip");
        al::setVelocity(this, sead::Vector3f::ey * 40.0f);
    }

    WalkerStateFunction::calcPassiveMovement(this, &sWalkerStateParam);
    if (al::isActionEnd(this) && al::getVelocityPtr(this)->y < 0.01f) {
        al::setNerve(this, &NrvSkipperAfter);
    }
}

/** @brief Hides the Skipper while its trampoline is out, and recovers when it ends. */
void Skipper::exeAfter() {
    if (al::isFirstStep(this)) {
        al::invalidateHitSensors(this);
        al::offCollide(this);
        al::hideModelIfShow(this);
        al::invalidateClipping(this);
        al::setVelocityZero(this);
        al::resetPosition(mTrampoline, al::getTrans(this), false);
        al::copyPose(mTrampoline, this);
        mTrampoline->makeActorAppeared();
    }

    if (mTrampoline->isEnd()) {
        al::validateHitSensors(this);
        al::onCollide(this);
        al::showModelIfHide(this);
        al::validateClipping(this);
        al::setTrans(this, al::getTrans(mTrampoline));
        al::setVelocity(this, al::getVelocity(mTrampoline));
        mTrampoline->kill();
        al::copyPose(this, mTrampoline);
        al::resetPosition(this, false);
        al::setNerve(this, &NrvSkipperRecoverEnd);
    }
}

/** @brief Plays the recovery animation until landing. */
void Skipper::exeRecoverEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "RecoverEnd");
    }

    WalkerStateFunction::calcPassiveMovement(this, &sWalkerStateParam);
    if (al::isOnGround(this, 0, 0.0f)) {
        al::setNerve(this, &NrvSkipperLand);
    }
}

/** @brief Lands and resets the wander center. */
void Skipper::exeLand() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Land");
        mStateWander->setWanderCenter(al::getTrans(this));
    }

    WalkerStateFunction::calcPassiveMovement(this, &sWalkerStateParam);
    if (!al::isOnGround(this, 5, 0.0f)) {
        al::setNerve(this, &NrvSkipperFall);
        return;
    }

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvSkipperWander);
    }
}

/** @brief Falls until it lands. */
void Skipper::exeFall() {
    if (al::isFirstStep(this) && !al::isActionPlaying(this, "FallStart")) {
        al::startAction(this, "Fall");
    }

    if (al::isActionPlaying(this, "FallStart") && al::isActionEnd(this)) {
        al::startAction(this, "Fall");
    }

    WalkerStateFunction::calcPassiveMovement(this, &sWalkerStateParam);
    if (al::isOnGround(this, 0, 0.0f)) {
        al::setNerve(this, &NrvSkipperLand);
    }
}

/** @brief Gets blown away and dies, dropping a coin. */
void Skipper::exeBlowDown() {
    if (al::updateNerveState(this)) {
        al::appearItemTiming(this, "Coin");
        al::startHitReactionDeath(this);
        kill();
    }
}

/** @brief Stays frozen until the freeze state ends, then wanders from the current position. */
void Skipper::exeSupportFreeze() {
    if (al::updateNerveState(this)) {
        mStateWander->setWanderCenter(al::getTrans(this));
        al::setNerve(this, &NrvSkipperWander);
    }
}

/** @brief Plays the attack success animation, then chases or wanders. */
void Skipper::exeAttackSuccess() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "AttackSuccess");
        al::setVelocityZeroH(this);
    }

    WalkerStateFunction::calcPassiveMovement(this, &sWalkerStateParam);
    if (!al::isActionEnd(this)) {
        return;
    }

    mTargetFinder->update();
    if (mTargetFinder->isExistTarget()) {
        al::setNerve(this, &NrvSkipperChase);
    } else {
        al::setNerve(this, &NrvSkipperWander);
    }
}

/**
 * @brief Checks whether the Skipper is stomped or blown away.
 * @return Whether the Skipper is down.
 */
bool Skipper::isNerveDown() const {
    return al::isNerve(this, &NrvSkipperTrampled) || al::isNerve(this, &NrvSkipperBlowDown);
}
