#include "NPC/NekoDisaster.hpp"

#include <attributes.h>
#include <prim/seadSafeString.h>

#include "Enemy/ActorStateSupportFreeze.hpp"
#include "Enemy/WalkerStateJump.hpp"
#include "Enemy/WalkerStateParam.hpp"
#include "NPC/NpcStateChase.hpp"
#include "NPC/NpcStateFunction.hpp"
#include "NPC/NpcStateParam.hpp"
#include "NPC/NpcStateWander.hpp"
#include "NPC/NpcTargetFinder.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Library/Clipping/ClippingJudge.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorClippingUtil.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorModelUtil.hpp"
#include "Library/LiveActor/Util/ActorMovementUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorResourceUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Screen/ScreenPointTarget.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Yaml/ByamlIter.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"

namespace {
NERVE_DECL(NekoDisaster, Wander)
NERVE_DECL(NekoDisaster, Chase)
NERVE_DECL(NekoDisaster, SupportFreeze)
NERVE_DECL(NekoDisaster, Attack)
NERVE_DECL(NekoDisaster, Fall)
NERVE_DECL(NekoDisaster, RunAway)
NERVE_DECL(NekoDisaster, HitReact)
NERVE_DECL(NekoDisaster, Find)
NERVE_DECL(NekoDisaster, FindEnd)
NERVE_DECL(NekoDisaster, ChaseEnd)
NERVE_DECL(NekoDisaster, Stun)
NERVE_DECL(NekoDisaster, StunEnd)
NERVE_DECL(NekoDisaster, Wait)
NERVE_DECL(NekoDisaster, RunAwayEnd)
NERVE_DECL(NekoDisaster, SeekPlacementPosition)
NERVE_DECL(NekoDisaster, FindFace)
NERVE_DECL(NekoDisaster, Startle)

NERVES_MAKE_NOSTRUCT(NekoDisaster, Find, FindEnd, Startle)

// Non-const nerve objects: the game merges them into one block.
NekoDisasterNrvWander NrvNekoDisasterWander;
NekoDisasterNrvChase NrvNekoDisasterChase;
NekoDisasterNrvSupportFreeze NrvNekoDisasterSupportFreeze;
NekoDisasterNrvAttack NrvNekoDisasterAttack;
NekoDisasterNrvFall NrvNekoDisasterFall;
NekoDisasterNrvRunAway NrvNekoDisasterRunAway;
NekoDisasterNrvHitReact NrvNekoDisasterHitReact;
NekoDisasterNrvChaseEnd NrvNekoDisasterChaseEnd;
NekoDisasterNrvStun NrvNekoDisasterStun;
NekoDisasterNrvStunEnd NrvNekoDisasterStunEnd;
NekoDisasterNrvWait NrvNekoDisasterWait;
NekoDisasterNrvRunAwayEnd NrvNekoDisasterRunAwayEnd;
NekoDisasterNrvSeekPlacementPosition NrvNekoDisasterSeekPlacementPosition;
NekoDisasterNrvFindFace NrvNekoDisasterFindFace;

NpcStateTurnParam sTurnParam(0.0f, 0.0f, 5.0f, -1.0f, true, true, 0);
NpcStateRumbleParam sRumbleParam(30, 2.0f, 1.5f, 0.2f, 1.0f);
NpcStateParam sStateParam(2.25f, 0.98f, 0.89f, 300.0f, 800.0f, 80.0f, 40.0f, 110.0f, 0, 0.0f);
NpcStateParam sFallStateParam(1.5f, 0.98f, 0.89f, 300.0f, 800.0f, 80.0f, 40.0f, 110.0f, 5, 0.3f);
ActorStateSupportFreezeParam sSupportFreezeParam(true, 15, false, true, 120,
                                                 sead::Vector3f(0.0f, 100.0f, 0.0f));
WalkerStateParam sWalkerStateParam(1.0f, 0.99f, 0.9f, 500.0f, 1500.0f, 70.0f, 80.0f, 150.0f);
WalkerStateJumpParam sJumpParam(8.0f, "Attack", true);
NpcTargetFinderParam sTargetFinderParam(700.0f, 50.0f, 40.0f, 90, 450.0f, 300.0f, 100.0f, 800.0f,
                                        false, 10);

/**
 * @brief Check whether an actor is far enough outside of the view to be ignored.
 * @param pActor Actor to check.
 * @return Whether the actor is clipped by the view frustum.
 */
bool isClipFrustum(al::LiveActor* pActor) {
    return al::getClippingJudge(pActor)->isJudgedToClipFrustum(al::getTrans(pActor), 1.0f, 200.0f,
                                                                1);
}

/**
 * @brief Stop the forward movement of a cat about to fall off a cliff or into water.
 * @param pNeko The cat.
 */
ALWAYS_INLINE void limitMoveToGround(NekoDisaster* pNeko) {
    if ((pNeko->getDisasterParam()->mIsEnableCliffCheck &&
         NpcStateFunction::isFallNextMove(pNeko, 100.0f, 150.0f, sStateParam.getFallCheckDrop(),
                                          false)) ||
        (pNeko->getDisasterParam()->mIsEnableShoreCheck && al::isOnGround(pNeko, 0, 0.0f) &&
         rc::isInWaterAreaNoSink(pNeko))) {
        sead::Vector3f* velocity = al::getVelocityPtr(pNeko);
        al::verticalizeVec(velocity, al::getFront(pNeko), *velocity);
    }
}
}  // namespace

/**
 * @brief Construct the disaster cat mode.
 * @param pHost Host cat of the mode.
 */
NekoDisaster::NekoDisaster(Neko* pHost) : IUseNekoModeActor("NekoDisaster"), mHost(pHost) {
    mParam = new NekoDisasterParam();
}

/**
 * @brief Initialize the disaster cat and its states.
 * @param rInfo Placement information.
 * @param colorType Coat color of the cat.
 * @param pTargetFinder Target finder shared with the other modes of the cat.
 */
void NekoDisaster::init(const al::ActorInitInfo& rInfo, neko::ColorType colorType,
                        NpcTargetFinder* pTargetFinder) {
    NekoDisasterParam* param = mParam;
    s32 startBehavior;
    if (al::tryGetArg(&startBehavior, rInfo, "StartBehavior")) {
        param->mStartBehavior = static_cast<NekoDisasterParam::StartBehavior>(startBehavior);
    }

    al::tryGetArg(&param->mWanderRange, rInfo, "WanderRange");
    al::tryGetArg(&param->mChaseRange, rInfo, "ChaseRange");
    al::tryGetArg(&param->mIsEnableCliffCheck, rInfo, "IsEnableCliffCheck");
    al::tryGetArg(&param->mIsEnableShoreCheck, rInfo, "IsEnableShoreCheck");
    al::tryGetArg(&param->mIsDisabledPR, rInfo, "isDisabledPR");
    al::tryGetArg(&param->mIsDisablePlessieChase, rInfo, "isDisablePlessieChase");
    al::tryGetStringArg(&param->mComment, rInfo, "Comment");
    mColorType = colorType;
    al::initActorWithArchiveName(this, rInfo,
                                 mColorType == 7 ? "NekoParentDisaster" : "NekoDisaster", nullptr);

    const al::Resource* modelResource = al::getModelResource(this);
    al::ByamlIter initIter;
    al::tryGetActorInitFileIter(&initIter, modelResource, "InitNeko", nullptr);

    mTargetFinder = pTargetFinder;
    pTargetFinder->setSearchTypes(npc::NpcFindTargetType_Player);
    mTargetFinder->setTargetTypePriority(npc::NpcFindTargetType_Player, 1);
    al::initNerve(this, &NrvNekoDisasterWander, 5);

    mWanderParam = new NpcStateWanderParam(
        300, 400, 0.33f, 4.0f, 20.0f, 800.0f, 0.5f, 150.0f, 1.0f, mParam->mIsEnableCliffCheck,
        mParam->mIsEnableShoreCheck, "Walk", "StartleWait", true, 110, 3000.0f);
    mChaseParam = new NpcStateChaseParam(1.2f, 130.0f, 500.0f, 5.5f, -1.0f, false,
                                         mParam->mIsEnableCliffCheck, mParam->mIsEnableShoreCheck,
                                         "Run", "StartleWait",
                                         mTargetFinder->getParam()->getChaseRange());
    mStateWander = new NpcStateWander(this, al::getFrontPtr(this), &sStateParam, mWanderParam);
    mStateChase = new NpcStateChase(this, al::getFrontPtr(this), mTargetFinder, &sStateParam,
                                    mChaseParam, false, nullptr);
    mStateSupportFreeze = new ActorStateSupportFreeze(this, &sSupportFreezeParam);
    al::initNerveState(this, mStateWander, &NrvNekoDisasterWander, "[state]Wander");
    al::initNerveState(this, mStateChase, &NrvNekoDisasterChase, "[state]Chase");
    al::initNerveState(this, mStateSupportFreeze, &NrvNekoDisasterSupportFreeze,
                       "[state]SupportFreeze");
    mStateAttack = new WalkerStateJump(this, &sWalkerStateParam, &sJumpParam);
    al::initNerveState(this, mStateAttack, &NrvNekoDisasterAttack, "[state]Attack");
    al::createAndSetColliderSpecialPurpose(this, "NekoMoveLimit");
    al::tryStartActionIfNotPlaying(this, "StartleWait");
    tryStartDefaultBehavior();
}

/**
 * @brief Start wandering, going back to the placement position or waiting.
 * @return Whether a behavior was started.
 */
bool NekoDisaster::tryStartDefaultBehavior() {
    bool isActive = neko::isActive(this, 2000.0f);
    if (isActive && neko::trySetNerve(this, &NrvNekoDisasterWander)) {
        return true;
    }

    if (!neko::isInChaseRange(this, mStateWander->getWanderCenter()) &&
        neko::trySetNerve(this, &NrvNekoDisasterSeekPlacementPosition)) {
        return true;
    }

    if (!isActive && neko::trySetNerve(this, &NrvNekoDisasterWait)) {
        return true;
    }

    return false;
}

/**
 * @brief Update the cool times and start falling when the ground disappears.
 */
void NekoDisaster::control() {
    if (mHitReactCoolTime > 0) {
        mHitReactCoolTime--;
    }

    if (mPackunEatCoolTime > 0) {
        mPackunEatCoolTime--;
    }

    bool isEnableFall = !al::isNerve(this, &NrvNekoDisasterHitReact) &&
                        !al::isNerve(this, &NrvNekoDisasterStun);
    al::HitSensor* groundSensor = al::tryGetCollidedGroundSensor(this);
    if (groundSensor != nullptr) {
        al::HitSensor* bodySensor = al::getHitSensor(this, "Body");
        al::sendMsgEnemyFloorTouch(groundSensor, bodySensor);
        if (rc::sendMsgEnemyFloorTouchTrampoline(groundSensor, bodySensor)) {
            al::setVelocity(this, 0.0f, 30.0f, 0.0f);
            neko::trySetNerve(this, &NrvNekoDisasterFall);
            return;
        }
    }

    if (isEnableFall && !al::isNerve(this, &NrvNekoDisasterAttack) &&
        !al::isNerve(this, &NrvNekoDisasterFall) && !neko::checkGround(this)) {
        al::setNerve(this, &NrvNekoDisasterFall);
    }
}

/**
 * @brief Check whether the cat reacts to its surroundings.
 * @return Whether the cat is neither reacting to a hit nor stunned.
 */
bool NekoDisaster::isInteractive() const {
    return !al::isNerve(this, &NrvNekoDisasterHitReact) && !al::isNerve(this, &NrvNekoDisasterStun);
}

/**
 * @brief Check whether the cat is attacking.
 * @return Whether the cat is attacking.
 */
bool NekoDisaster::isAttack() const {
    return al::isNerve(this, &NrvNekoDisasterAttack);
}

/**
 * @brief Clip the cat, and its host if this mode is the active one.
 */
void NekoDisaster::startClipped() {
    if (mHost->getModeActor() == this) {
        mHost->tryStartClipped();
    }

    al::LiveActor::startClipped();
}

/**
 * @brief Unclip the cat, and its host if this mode is the active one.
 */
void NekoDisaster::endClipped() {
    mReactWaitTime = 60;
    if (mHost->getModeActor() == this) {
        mHost->tryEndClipped();
    }

    al::LiveActor::endClipped();
}

/**
 * @brief Make this mode the active one of the host cat.
 * @param rReason Why the mode gets attached.
 */
void NekoDisaster::startAttach(const NekoAttachReason& rReason) {
    mTargetFinder->clearTarget();
    mTargetFinder->setParam(&sTargetFinderParam);
    mTargetFinder->setSearchTypes(npc::NpcFindTargetType_Player);
    mTargetFinder->setTargetTypePriority(npc::NpcFindTargetType_Player, 1);
    tryStartDefaultBehavior();
    al::tryEmitEffect(this, "DisasterAura", nullptr);
}

/**
 * @brief Kill the cat.
 * @param isDeleteParticle Whether the aura particles are deleted immediately.
 */
void NekoDisaster::startKill(bool isDeleteParticle) {
    if (isDeleteParticle) {
        al::tryDeleteEffectAndParticle(this, "DisasterAura");
    } else {
        al::tryDeleteEffect(this, "DisasterAura");
    }

    kill();
}

/**
 * @brief Push, touch and attack the actors the cat collides with.
 * @param pSelf Sensor of the cat.
 * @param pOther Sensor of the other actor.
 */
void NekoDisaster::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (isInteractive()) {
        mTargetFinder->attackSensor(pSelf, pOther);
    }

    if (al::isSensorEnemyBody(pSelf)) {
        al::sendMsgNpcTouch(pOther, pSelf);
        if (al::isSensorNpc(pOther) && al::isSensorName(pOther, "NPCDisasterAvoid")) {
            mRunAwaySensor = pOther;
            if (!al::isNerve(this, &NrvNekoDisasterRunAway)) {
                al::setNerve(this, &NrvNekoDisasterRunAway);
            }

            return;
        }

        if (al::isSensorEnemyBody(pOther) || al::isSensorNpc(pOther)) {
            al::sendMsgPush(pOther, pSelf);
            return;
        }

        if (al::isSensorRide(pOther) && al::isSensorPlessie(pOther)) {
            al::sendMsgPush(pSelf, pOther);
        }
    }

    if (al::isSensorEnemyAttack(pSelf) &&
        (al::isSensorPlayer(pOther) || al::isSensorKoopaJr(pOther))) {
        al::sendMsgPush(pOther, pSelf);
        if (isEnableAttack() &&
            (al::sendMsgEnemyAttackForCrossoverSensor(pOther, pSelf) ||
             al::sendMsgNekoAttack(pOther, pSelf)) &&
            !al::isNerve(this, &NrvNekoDisasterAttack)) {
            al::faceToTarget(this, al::getSensorPos(pOther));
            al::setNerve(this, &NrvNekoDisasterAttack);
        }
    }
}

/**
 * @brief Check whether the cat can start an attack.
 * @return Whether the cat is neither reacting to a hit nor stunned.
 */
bool NekoDisaster::isEnableAttack() const {
    return !al::isNerve(this, &NrvNekoDisasterHitReact) && !al::isNerve(this, &NrvNekoDisasterStun);
}

/**
 * @brief React to pushes, attacks and other messages.
 * @param pMsg Received message.
 * @param pOther Sensor of the sender.
 * @param pSelf Sensor of the cat.
 * @return Whether the message was handled.
 */
bool NekoDisaster::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                              al::HitSensor* pSelf) {
    if (rc::isMsgAskControlUserId(pMsg, mControlSensor)) {
        return true;
    }

    if (!al::isSensorPlayer(pOther) &&
        al::tryReceiveMsgPushAndAddVelocityH(this, pMsg, pOther, pSelf, 2.0f)) {
        return true;
    }

    if (al::isMsgNpcTouch(pMsg) || rc::isMsgImozoTouch(pMsg)) {
        al::pushAndAddVelocityH(this, pOther, pSelf, 2.0f);
        return true;
    }

    if (al::isMsgPlayerInvincibleAttack(pMsg) || al::isMsgGigaBellPush(pMsg) ||
        al::isMsgDisasterSpikeAttack(pMsg) || al::isMsgGigaEnemyAttack(pMsg) ||
        al::isMsgExplosion(pMsg)) {
        if (!mHost->tryStartHide()) {
            return false;
        }

        if (al::isMsgPlayerInvincibleAttack(pMsg)) {
            rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        } else if (al::isMsgExplosion(pMsg)) {
            al::startHitReactionHitEffect(this, "ＮＰＣヒット", pOther, pSelf);
        }

        return true;
    }

    if (al::isMsgPlayerTrampleForCrossoverSensor(pMsg, pOther, pSelf) ||
        al::isMsgPlayerObjHipDropReflectAll(pMsg) || al::isMsgPlayerBodyAttackReflect(pMsg) ||
        al::isMsgPlayerObjStatueDrop(pMsg)) {
        if (al::isSensorRide(pOther)) {
            return false;
        }

        if (mHitReactCoolTime <= 0) {
            al::setNerve(this, &NrvNekoDisasterHitReact);
            al::startSe(this, "PgTrample");
            rc::requestHitReactionToAttackerNpc(pSelf, pOther);
            mHitReactCoolTime = 30;
            al::calcDirBetweenSensorsH(&mHitReactVelocity, pOther, pSelf);
            mHitReactVelocity.y = 0.1f;
            al::normalize(&mHitReactVelocity);
            mHitReactVelocity *= 20.0f;
            return true;
        }
    }

    if (neko::isMsgHitReaction(this, pMsg, pOther, pSelf) || al::isMsgKickKouraAttack(pMsg) ||
        al::isMsgPlayerObjHipDropAll(pMsg) || al::isMsgKickKouraReflect(pMsg) ||
        al::isMsgBlockUpperPunch(pMsg)) {
        if (mHitReactCoolTime > 0) {
            return false;
        }

        if (neko::isMsgNpcAttackerHitReaction(this, pMsg, pOther, pSelf) ||
            al::isMsgPlayerObjHipDropAll(pMsg)) {
            rc::requestHitReactionToAttackerNpc(pSelf, pOther);
        } else if (rc::isMsgPackunEat(pMsg)) {
            sead::Vector3f dir;
            al::calcDirBetweenSensors(&dir, pSelf, pOther);
            dir *= al::getSensorRadius(pSelf);
            al::startHitReactionHitEffect(this, "ＮＰＣヒット", al::getSensorPos(pSelf) + dir);
        } else {
            al::startHitReactionHitEffect(this, "ＮＰＣヒット", pOther, pSelf);
        }

        al::LiveActor* target = mTargetFinder->getTarget();
        if (target != nullptr && al::calcDistance(this, target) < 300.0f) {
            al::calcDirToActor(&mHitReactVelocity, mTargetFinder->getTarget(), this);
            mHitReactVelocity *= 20.0f;
        } else {
            f32 speed = pOther != nullptr && al::isSensorRide(pOther) ? 30.0f : 20.0f;
            al::calcDirBetweenSensorsH(&mHitReactVelocity, pOther, pSelf);
            mHitReactVelocity.y = 0.1f;
            al::normalize(&mHitReactVelocity);
            mHitReactVelocity *= speed;
        }

        if (al::isMsgPlayerFireBallAttack(pMsg)) {
            mHitReactCoolTime = 8;
        } else if (al::isMsgPlayerClimbAttack(pMsg)) {
            mHitReactCoolTime = 30;
        } else if (al::isMsgNekoAttack(pMsg) || al::isMsgExplosion(pMsg)) {
            mHitReactCoolTime = 20;
        } else if (pOther != nullptr && al::isSensorRide(pOther)) {
            mHitReactCoolTime = 60;
        } else {
            mHitReactCoolTime = 20;
        }
        al::startSe(this, "PgHit");
        al::setNerve(this, &NrvNekoDisasterHitReact);
        return true;
    }

    if (neko::isMsgMeraWanwanTrackAttack(this, pMsg, pOther, nullptr) ||
        rc::isMsgNeedleRollerHit(pMsg)) {
        if (mHost != nullptr && mHost->tryStartHide()) {
            return true;
        }
    }

    if (rc::isMsgJumpPanelAction(pMsg)) {
        al::setVelocity(this, 0.0f, rc::isMsgJumpPanelActionAndSuperJump(pMsg) ? 80.0f : 50.0f,
                        0.0f);
        al::setNerve(this, &NrvNekoDisasterFall);
        return true;
    }

    if (rc::isMsgPackunEatStart(pMsg) && mPackunEatCoolTime <= 0) {
        mPackunEatCoolTime = 120;
        return true;
    }

    return false;
}

/**
 * @brief React to the touch screen pointer.
 * @param pMsg Received message.
 * @param pPointer The screen pointer.
 * @param pTarget The touched screen point target.
 * @return Whether the cat got frozen by the pointer.
 */
bool NekoDisaster::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                         al::ScreenPointTarget* pTarget) {
    mTargetFinder->receiveMsgScreenPoint(pMsg, pPointer, pTarget);
    if (al::isScreenPointTargetName(pTarget, "Eye")) {
        return false;
    }

    if (!canFreeze()) {
        return false;
    }

    if (mStateSupportFreeze->receiveMsgScreenPoint(pMsg, pPointer, pTarget)) {
        if (!al::isNerve(this, &NrvNekoDisasterSupportFreeze)) {
            al::setNerve(this, &NrvNekoDisasterSupportFreeze);
        }

        return true;
    }

    return false;
}

/**
 * @brief Check whether the cat can be frozen by the touch screen pointer.
 * @return Whether the cat can be frozen.
 */
bool NekoDisaster::canFreeze() const {
    return !al::isNerve(this, &NrvNekoDisasterHitReact) &&
           !al::isNerve(this, &NrvNekoDisasterStun) &&
           !al::isNerve(this, &NrvNekoDisasterStunEnd) &&
           !al::isNerve(this, &NrvNekoDisasterSupportFreeze);
}

/**
 * @brief Wait in place, then react to a target or start the default behavior.
 */
void NekoDisaster::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait");
        mReactWaitTime = 60;
    }

    NpcStateFunction::calcPassiveMovement(this, &sStateParam);
    mTargetFinder->update();
    if (mReactWaitTime-- <= 0 && !tryStartReactToTarget()) {
        tryStartDefaultBehavior();
    }
}

/**
 * @brief Start facing or chasing the current target if it is close enough.
 * @return Whether a reaction was started.
 */
bool NekoDisaster::tryStartReactToTarget() {
    if (isRunAway()) {
        return false;
    }

    if (mTargetFinder->getTarget() == nullptr) {
        return false;
    }

    if (isClipFrustum(this)) {
        return false;
    }

    if (!neko::isInRange(mTargetFinder->getTarget(), mStateWander->getWanderCenter(),
                         getChaseRange())) {
        return false;
    }

    if (!(mTargetFinder->getTarget() != nullptr && mTargetFinder->isTargetValid()) &&
        !mTargetFinder->isInSenseAreaTarget()) {
        return false;
    }

    if (isWait() || isStun()) {
        return neko::trySetNerve(this, &NrvNekoDisasterFindFace);
    }

    return neko::trySetNerve(this, &NrvNekoDisasterChase);
}

/**
 * @brief Wander around the placement position.
 */
void NekoDisaster::exeWander() {
    if (al::isFirstStep(this)) {
        mReactWaitTime = 60;
    }

    mTargetFinder->update();
    al::updateNerveState(this);
    if (mReactWaitTime-- <= 0 && (tryStartReactToTarget() || tryStartDefaultBehavior())) {
        return;
    }

    neko::setAnimationRate(this);
}

/**
 * @brief Walk back to the position of the host cat.
 */
void NekoDisaster::exeSeekPlacementPosition() {
    if (al::isFirstStep(this)) {
        al::tryStartActionIfNotPlaying(this, "Walk");
        mReactWaitTime = 60;
    }

    mTargetFinder->update();
    if (mReactWaitTime-- <= 0 && tryStartReactToTarget()) {
        return;
    }

    if (al::isNear(this, al::getTrans(mHost), 30.0f)) {
        tryStartDefaultBehavior();
        return;
    }

    al::turnDirectionToTargetDegree(this, al::getFrontPtr(this), al::getTrans(mHost),
                                    mWanderParam->getTurnDegree());
    al::addVelocityToDirection(this, al::getFront(this), mWanderParam->getWalkAccel());
    limitMoveToGround(this);
    NpcStateFunction::calcPassiveMovement(this, &sStateParam);
    neko::setAnimationRate(this);
}

/**
 * @brief Fall until the cat lands on the ground.
 */
void NekoDisaster::exeFall() {
    if (al::isFirstStep(this)) {
        al::invalidateClipping(this);
    }

    NpcStateFunction::calcPassiveMovement(this, &sFallStateParam);
    mTargetFinder->update();
    if (al::isOnGround(this, 0, 0.0f)) {
        if (tryStartReactToTarget() || tryStartDefaultBehavior()) {
            al::validateClipping(this);
        }

        return;
    }

    if (al::getVelocity(this).y > 0.0f) {
        return;
    }

    if (al::isGreaterStep(this, 10)) {
        al::tryStartActionIfNotPlaying(this, "Fall");
    }
}

/**
 * @brief Get startled and turn towards the current target.
 */
void NekoDisaster::exeStartle() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Startle");
    }

    mTargetFinder->update();
    al::LiveActor* target = mTargetFinder->getTarget();
    if (target != nullptr) {
        al::turnDirectionToTargetDegree(this, al::getFrontPtr(this), al::getTrans(target),
                                        mChaseParam->getTurnDegree());
    }

    if (al::isActionEnd(this) && !tryStartReactToTarget() && tryStartDefaultBehavior()) {
        return;
    }

    NpcStateFunction::calcPassiveMovement(this, &sStateParam);
    al::setVelocityZeroH(this);
}

/**
 * @brief Turn towards the current target before noticing it.
 */
void NekoDisaster::exeFindFace() {
    if (al::isFirstStep(this)) {
        al::tryStartActionIfNotPlaying(this, "Walk");
    }

    mTargetFinder->update();
    if (mTargetFinder->getTarget() == nullptr) {
        tryStartDefaultBehavior();
        return;
    }

    if (mTargetFinder->isTargetValid() ||
        al::turnDirectionToTargetDegree(this, al::getFrontPtr(this), mTargetFinder->getTargetPos(),
                                        5.5f)) {
        al::setNerve(this, &NrvNekoDisasterFind);
        return;
    }

    NpcStateFunction::calcPassiveMovement(this, &sStateParam);
    al::addVelocity(this, al::getFront(this) * 0.25f);
    limitMoveToGround(this);
    neko::setAnimationRate(this);
}

/**
 * @brief Notice the current target.
 */
void NekoDisaster::exeFind() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "FindStart");
    }

    NpcStateFunction::calcPassiveMovement(this, &sStateParam);
    if (al::isActionPlaying(this, "FindStart")) {
        if (!al::isActionEnd(this)) {
            return;
        }

        if (!al::isOnGround(this, 0, 0.0f)) {
            al::startAction(this, "FindLoop");
            return;
        }
    } else if (!al::isOnGround(this, 0, 0.0f)) {
        return;
    }

    al::setNerve(this, &NrvNekoDisasterFindEnd);
}

/**
 * @brief Finish noticing the target, then chase it.
 */
void NekoDisaster::exeFindEnd() {
    if (al::isFirstStep(this)) {
        al::tryStartActionIfNotPlaying(this, "FindEnd");
    }

    NpcStateFunction::calcPassiveMovement(this, &sStateParam);
    mTargetFinder->update();
    if (!al::isActionEnd(this)) {
        return;
    }

    if (mTargetFinder->getTarget() != nullptr && mTargetFinder->isTargetInChaseRange()) {
        al::setNerve(this, &NrvNekoDisasterChase);
        return;
    }

    tryStartDefaultBehavior();
}

/**
 * @brief Chase the current target.
 */
void NekoDisaster::exeChase() {
    if (al::isFirstStep(this)) {
        al::tryStartActionIfNotPlaying(this, "Run");
    }

    if (al::updateNerveState(this)) {
        if (al::isGreaterEqualStep(this, 30)) {
            al::setNerve(this, &NrvNekoDisasterChaseEnd);
        } else {
            al::setNerve(this, &NrvNekoDisasterWander);
        }

        return;
    }

    if (mTargetFinder->getTarget() == nullptr) {
        al::setNerve(this, &NrvNekoDisasterChaseEnd);
        return;
    }

    if (!neko::isInChaseRange(this, mStateWander->getWanderCenter()) || isClipFrustum(this)) {
        al::setNerve(this, &NrvNekoDisasterChaseEnd);
        return;
    }

    neko::setAnimationRate(this);
}

/**
 * @brief Stop chasing, then react to a new target or start the default behavior.
 */
void NekoDisaster::exeChaseEnd() {
    // The first step is checked, but nothing happens on it.
    al::isFirstStep(this);
    mTargetFinder->update();
    if (al::isGreaterEqualStep(this, 20)) {
        if (!tryStartReactToTarget()) {
            tryStartDefaultBehavior();
        }

        return;
    }

    NpcStateFunction::calcPassiveMovement(this, &sStateParam);
    neko::setAnimationRate(this);
}

/**
 * @brief Get knocked back by a hit.
 */
void NekoDisaster::exeHitReact() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "HitReact");
        al::addVelocity(this, mHitReactVelocity);
    }

    NpcStateFunction::calcPassiveMovement(this, &sStateParam);
    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvNekoDisasterStun);
    }
}

/**
 * @brief Stay frozen by the touch screen pointer.
 */
void NekoDisaster::exeSupportFreeze() {
    if (al::updateNerveState(this)) {
        mTargetFinder->update();
        if (!tryStartReactToTarget()) {
            tryStartDefaultBehavior();
        }
    }
}

/**
 * @brief Stay stunned after a hit.
 */
void NekoDisaster::exeStun() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Stun");
    }

    NpcStateFunction::calcPassiveMovement(this, &sStateParam);
    if (al::isGreaterEqualStep(this, 220)) {
        al::setNerve(this, &NrvNekoDisasterStunEnd);
    }
}

/**
 * @brief Recover from a stun.
 */
void NekoDisaster::exeStunEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "StunEnd");
    }

    NpcStateFunction::calcPassiveMovement(this, &sStateParam);
    mTargetFinder->update();
    if (al::isActionEnd(this) && !tryStartReactToTarget()) {
        tryStartDefaultBehavior();
    }
}

/**
 * @brief Jump at the player.
 */
void NekoDisaster::exeAttack() {
    al::updateNerveStateAndNextNerve(this, &NrvNekoDisasterWait);
}

/**
 * @brief Run away from an area the cat must avoid.
 */
void NekoDisaster::exeRunAway() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Run");
    }

    NpcStateFunction::calcPassiveMovement(this, &sStateParam);
    if (mRunAwaySensor == nullptr) {
        al::setNerve(this, &NrvNekoDisasterRunAwayEnd);
        return;
    }

    al::turnDirectionFromTargetDegree(this, al::getFrontPtr(this),
                                      al::getSensorPos(mRunAwaySensor),
                                      mChaseParam->getTurnDegree());
    const sead::Vector3f& front = al::getFront(this);
    f32 accel = mChaseParam->getRunAccel();
    al::addVelocity(this, {accel * front.x, accel * front.y, accel * front.z});
    limitMoveToGround(this);
    neko::setAnimationRate(this);
    mRunAwaySensor = nullptr;
}

/**
 * @brief Stop running away, then start the default behavior.
 */
void NekoDisaster::exeRunAwayEnd() {
    // The first step is checked, but nothing happens on it.
    al::isFirstStep(this);
    if (al::isGreaterEqualStep(this, 20)) {
        tryStartDefaultBehavior();
    }

    NpcStateFunction::calcPassiveMovement(this, &sStateParam);
    neko::setAnimationRate(this);
}

/**
 * @brief Check whether the cat is running away.
 * @return Whether the cat is running away.
 */
bool NekoDisaster::isRunAway() const {
    return al::isNerve(this, &NrvNekoDisasterRunAway) ||
           al::isNerve(this, &NrvNekoDisasterRunAwayEnd);
}

/**
 * @brief Check whether the cat is chasing a target.
 * @return Whether the cat is chasing a target.
 */
bool NekoDisaster::isChase() const {
    return al::isNerve(this, &NrvNekoDisasterChase) ||
           al::isNerve(this, &NrvNekoDisasterChaseEnd);
}

/**
 * @brief Check whether the cat is idle.
 * @return Whether the cat is waiting or wandering.
 */
bool NekoDisaster::isWait() const {
    return al::isNerve(this, &NrvNekoDisasterWait) || al::isNerve(this, &NrvNekoDisasterWander);
}

/**
 * @brief Check whether the cat is stunned.
 * @return Whether the cat is stunned or recovering from a stun.
 */
bool NekoDisaster::isStun() const {
    return al::isNerve(this, &NrvNekoDisasterStun) || al::isNerve(this, &NrvNekoDisasterStunEnd);
}
