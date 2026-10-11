#include "Enemy/Kuribon.hpp"

#include <math/seadMathCalcCommon.h>

#include "Enemy/ActorJointLookController.hpp"
#include "Enemy/ActorMicRumbler.hpp"
#include "Enemy/ActorStateSupportFreeze.hpp"
#include "Enemy/EnemyStateBlowDown.hpp"
#include "Enemy/EnemyStateUtil.hpp"
#include "Enemy/KuribonStateReverse.hpp"
#include "Enemy/TargetFinder.hpp"
#include "Enemy/WalkerStateChase.hpp"
#include "Enemy/WalkerStateFindPlayer.hpp"
#include "Enemy/WalkerStateFunction.hpp"
#include "Enemy/WalkerStateParam.hpp"
#include "Enemy/WalkerStateRouteDokanMove.hpp"
#include "Enemy/WalkerStateWander.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/Joint/JointControllerKeeper.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Thread/Functor.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ScoreUtil.hpp"

// Nerve that shares the execute function of another nerve.
#define KURIBON_NERVE_SHARED_DECL(Action, ExeFunc)                                                 \
    class KuribonNrv##Action : public al::Nerve {                                                  \
    public:                                                                                        \
        void execute(al::NerveKeeper* pKeeper) const override {                                    \
            (pKeeper->getParent<Kuribon>())->exe##ExeFunc();                                       \
        }                                                                                          \
    };

namespace {
NERVE_DECL(Kuribon, Wait)
NERVE_DECL(Kuribon, Wander)
NERVE_DECL(Kuribon, Chase)
NERVE_DECL(Kuribon, FindPlayer)
NERVE_DECL(Kuribon, RouteDokan)
NERVE_DECL(Kuribon, Reverse)
NERVE_DECL(Kuribon, BlowDown)
NERVE_DECL(Kuribon, SupportFreeze)
KURIBON_NERVE_SHARED_DECL(SupportFreezeReverse, SupportFreeze)
NERVE_DECL(Kuribon, WaitGateKeeper)
NERVE_DECL(Kuribon, PressDown)
KURIBON_NERVE_SHARED_DECL(PressDownReverse, PressDown)
NERVE_DECL(Kuribon, Attack)
NERVE_DECL(Kuribon, RouteDokanDeath)
// Non-const nerve objects: the game keeps them in .data in this order.
KuribonNrvWait NrvKuribonWait;
KuribonNrvWander NrvKuribonWander;
KuribonNrvChase NrvKuribonChase;
KuribonNrvFindPlayer NrvKuribonFindPlayer;
KuribonNrvRouteDokan NrvKuribonRouteDokan;
KuribonNrvReverse NrvKuribonReverse;
KuribonNrvBlowDown NrvKuribonBlowDown;
KuribonNrvSupportFreeze NrvKuribonSupportFreeze;
KuribonNrvSupportFreezeReverse NrvKuribonSupportFreezeReverse;
KuribonNrvWaitGateKeeper NrvKuribonWaitGateKeeper;
KuribonNrvPressDown NrvKuribonPressDown;
KuribonNrvPressDownReverse NrvKuribonPressDownReverse;
KuribonNrvAttack NrvKuribonAttack;
KuribonNrvRouteDokanDeath NrvKuribonRouteDokanDeath;

typedef al::FunctorV0M<Kuribon*, void (Kuribon::*)()> KuribonFunctor;

sead::Vector2f sLookLimit(90.0f, 90.0f);
TargetFinderParam sTargetFinderParam(1500.0f, 180.0f, 85.0f, 90, -1.0f, 500.0f, 500.0f, 2000.0f,
                                     true);
TargetFinderParam sTargetFinderParamGateKeeper(10000.0f, 180.0f, 85.0f, 90, -1.0f, 500.0f, 500.0f,
                                               2000.0f, true);
WalkerStateParam sWalkerStateParam(2.25f, 0.98f, 0.87f, 500.0f, 1500.0f, 70.0f, 80.0f, 150.0f);
WalkerStateWanderParam sWanderParamBig(40, 400, 0.3f, 0.5f, 200.0f, 800.0f, true, "Walk", "Wait");
WalkerStateWanderParam sWanderParam(10, 120, 0.2f, 6.0f, 20.0f, 500.0f, true, "Walk", "Wait");
WalkerStateChaseParam sChaseParamBig(1.0f, 130.0f, 500.0f, 2.0f, 5.0f, false, true, "Run", "Wait",
                                     -1.0f);
WalkerStateChaseParam sChaseParam(1.1f, 130.0f, 500.0f, 2.8f, 5.0f, false, true, "Run", "Wait",
                                  -1.0f);
WalkerStateFindPlayerParam sFindPlayerParamBig(30, 5.0f, false, "Turn");
WalkerStateFindPlayerParam sFindPlayerParam(30, 7.5f, false, "Turn");
KuribonStateReverseParam sReverseParam;
KuribonStateReverseParam sReverseParamBig(2.0f, 0.98f, 0.92f, 60);
ActorStateSupportFreezeParam sSupportFreezeParam(true, 15, false, true, 120,
                                                 sead::Vector3f(0.0f, 150.0f, 0.0f));
ActorStateSupportFreezeParam sSupportFreezeParamBig(true, 15, false, true, 120,
                                                    sead::Vector3f(0.0f, 450.0f, 0.0f));
EnemyStateBlowDownParam sBlowDownParam(false);
ActorJointLookControllerParam sLookParamEyeL(1.5f, sead::Vector2f(-3.0f, 9.5f), false, nullptr,
                                             nullptr);
ActorJointLookControllerParam sLookParamEyeR(1.5f, sead::Vector2f(-9.5f, 3.0f), false, nullptr,
                                             nullptr);
ActorJointLookControllerParam sLookParamVertical(0.5f, sead::Vector2f(-14.0f, 4.0f), true, nullptr,
                                                 nullptr);

/**
 * @brief Copies a vector as a plain copy of its storage.
 * @param pDst Destination vector.
 * @param rSrc Source vector.
 */
inline void copyVector(sead::Vector3f* pDst, const sead::Vector3f& rSrc) {
    pDst->e = rSrc.e;
}

/**
 * @brief Scales a horizontal vector to a length, falling back to the Z axis when it is zero.
 * @param pVec Horizontal vector to scale (y must be zero).
 * @param length Length of the result.
 */
inline void setLengthHOrDirZ(sead::Vector3f* pVec, f32 length) {
    if (pVec->x == 0.0f && pVec->z == 0.0f) {
        pVec->set(0.0f, 0.0f, length);
        return;
    }

    pVec->normalize();
    pVec->x *= length;
    pVec->z *= length;
}
}  // namespace

/**
 * @brief Constructs a Kuribon.
 * @param pName Actor name.
 */
Kuribon::Kuribon(const char* pName) : al::LiveActor(pName) {}

/**
 * @brief Initializes the model, states, eye look controller and nerves.
 * @param rInfo Placement info of the actor.
 */
void Kuribon::init(const al::ActorInitInfo& rInfo) {
    al::tryGetArg(&mIsGateKeeper, rInfo, "IsGateKeeper");
    al::tryGetArg(&mActiveDistance, rInfo, "ActiveDistance");
    al::tryGetArg(&mDeactiveDistance, rInfo, "DeactiveDistance");
    if (mActiveDistance < 0.0f) {
        mActiveDistance = 3500.0f;
    }

    if (mDeactiveDistance < 0.0f) {
        mDeactiveDistance = 4000.0f;
    }

    const char* objectName;
    al::getObjectName(&objectName, rInfo);
    if (al::isEqualString(objectName, "KuribonBig")) {
        al::initActorWithArchiveName(this, rInfo, "KuribonBig", nullptr);
        mIsBig = true;
        mJointLookController = new ActorJointLookController(this, 4);
        al::initJointControllerKeeper(this, 4);
        mJointLookController->appendJoint("EyeL", sead::Vector3f::ey, &sLookParamEyeL);
        mJointLookController->appendJoint("EyeL", -sead::Vector3f::ez, &sLookParamVertical);
        mJointLookController->appendJoint("EyeR", sead::Vector3f::ey, &sLookParamEyeR);
        mJointLookController->appendJoint("EyeR", -sead::Vector3f::ez, &sLookParamVertical);
        mJointLookController->setLimit(sLookLimit);
    } else {
        al::initActorWithArchiveName(this, rInfo, "Kuribon", nullptr);
        mIsBig = false;
    }

    copyVector(&mInitTrans, al::getTrans(this));
    mTargetFinder = new TargetFinder(
        this, mIsGateKeeper ? &sTargetFinderParamGateKeeper : &sTargetFinderParam);
    mMicRumbler = new ActorMicRumbler(this, nullptr);
    mStateWander = new WalkerStateWander(this, al::getFrontPtr(this), &sWalkerStateParam,
                                         mIsBig ? &sWanderParamBig : &sWanderParam, mTargetFinder);
    mStateChase = new WalkerStateChase(this, al::getFrontPtr(this), mTargetFinder,
                                       &sWalkerStateParam, mIsBig ? &sChaseParamBig : &sChaseParam,
                                       true, nullptr);
    mStateFindPlayer = new WalkerStateFindPlayer(
        this, al::getFrontPtr(this), mTargetFinder, &sWalkerStateParam,
        mIsBig ? &sFindPlayerParamBig : &sFindPlayerParam, nullptr);
    mStateRouteDokanMove = new WalkerStateRouteDokanMove(this, rInfo, &sWalkerStateParam, nullptr);
    mStateReverse = new KuribonStateReverse(this, &sWalkerStateParam,
                                            mIsBig ? &sReverseParamBig : &sReverseParam);
    mStateBlowDown = new EnemyStateBlowDown(this, &sBlowDownParam);
    mStateSupportFreeze = new ActorStateSupportFreeze(
        this, mIsBig ? &sSupportFreezeParamBig : &sSupportFreezeParam);
    if (mIsBig) {
        mStateSupportFreeze->setScaleAnimTypeHard();
    }

    al::initNerve(this, &NrvKuribonWait, 8);
    al::initNerveState(this, mStateWander, &NrvKuribonWander, "[state]徘徊");
    al::initNerveState(this, mStateChase, &NrvKuribonChase, "[state]追いかけ");
    al::initNerveState(this, mStateFindPlayer, &NrvKuribonFindPlayer, "[state]プレーヤー発見");
    al::initNerveState(this, mStateRouteDokanMove, &NrvKuribonRouteDokan,
                       "[state]ルート土管移動");
    al::initNerveState(this, mStateReverse, &NrvKuribonReverse, "[state]ひっくり返り");
    al::initNerveState(this, mStateBlowDown, &NrvKuribonBlowDown, "[state]吹き飛ばし");
    al::initNerveState(this, mStateSupportFreeze, &NrvKuribonSupportFreeze, "[state]フリーズ");
    al::initNerveState(this, mStateSupportFreeze, &NrvKuribonSupportFreezeReverse,
                       "[state]フリーズ Reverse");
    if (mIsGateKeeper) {
        al::setNerve(this, &NrvKuribonWaitGateKeeper);
    }

    al::createAndSetColliderSpecialPurpose(this, "MoveLimit");
    al::listenStageSwitchOn(this, "SwitchResetPosition",
                            KuribonFunctor(this, &Kuribon::resetInitTrans));
    makeActorAppeared();
}

/** @brief Moves the Kuribon back to its initial position when it is walking around. */
void Kuribon::resetInitTrans() {
    if (al::isDead(this)) {
        return;
    }

    if (al::isNerve(this, &NrvKuribonPressDown) || al::isNerve(this, &NrvKuribonPressDownReverse) ||
        al::isNerve(this, &NrvKuribonBlowDown) || al::isNerve(this, &NrvKuribonRouteDokanDeath) ||
        al::isNerve(this, &NrvKuribonRouteDokan)) {
        return;
    }

    al::resetPosition(this, mInitTrans, false);
    al::setNerve(this, &NrvKuribonWait);
}

/** @brief Appears and starts waiting. */
void Kuribon::appear() {
    al::LiveActor::appear();
    al::setNerve(this, &NrvKuribonWait);
}

/** @brief Updates the eye look controller, the mic rumbler and the kill areas. */
void Kuribon::control() {
    if (mJointLookController != nullptr) {
        if (al::isNerve(this, &NrvKuribonWait) || al::isNerve(this, &NrvKuribonWander)) {
            mJointLookController->setLookAtNearestPlayer(800.0f);
        } else if (al::isNerve(this, &NrvKuribonChase) ||
                   al::isNerve(this, &NrvKuribonFindPlayer)) {
            if (mTargetFinder->isExistTarget()) {
                mJointLookController->setLookTarget(mTargetFinder->getTargetPos());
            } else {
                mJointLookController->setLookAtNearestPlayer(800.0f);
            }
        } else {
            mJointLookController->stopLook();
        }

        mJointLookController->update();
    }

    if (!al::isNerve(this, &NrvKuribonSupportFreeze) &&
        !al::isNerve(this, &NrvKuribonSupportFreezeReverse) &&
        !al::isNerve(this, &NrvKuribonPressDown) &&
        !al::isNerve(this, &NrvKuribonPressDownReverse) &&
        !al::isNerve(this, &NrvKuribonBlowDown) && !mIsBig) {
        mMicRumbler->update();
    }

    EnemyStateUtil::tryKillByAreaOrMaterialCodeWithHitReaction(this);
}

/**
 * @brief Pushes other enemies and map objects, attacks players and enters route pipes.
 * @param pSelf Sensor of the Kuribon.
 * @param pOther Sensor that was hit.
 */
void Kuribon::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isSensorEnemyBody(pOther)) {
        if (isPushStrong()) {
            al::sendMsgPushStrong(pOther, pSelf);
        } else {
            al::sendMsgPushAndKillVelocityToTarget(this, pSelf, pOther);
        }
    }

    if (al::isSensorEnemyAttack(pSelf) &&
        (al::isSensorPlayer(pOther) || al::isSensorKoopaJr(pOther))) {
        if (!isEnablePush() || al::isNerve(this, &NrvKuribonRouteDokan)) {
            return;
        }

        al::sendMsgPush(pOther, pSelf);
        if (!isEnableAttack()) {
            return;
        }

        if (al::sendMsgEnemyAttackForCrossoverSensor(pOther, pSelf) &&
            !al::isNerve(this, &NrvKuribonAttack)) {
            if (!mIsBig) {
                al::faceToTarget(this, al::getSensorPos(pOther));
            }

            al::setNerve(this, &NrvKuribonAttack);
        }

        return;
    }

    if (al::isSensorMapObj(pOther)) {
        al::sendMsgPush(pOther, pSelf);
        return;
    }

    if (isEnableRotueDokan() && mStateRouteDokanMove->tryStart(pSelf, pOther)) {
        al::setNerve(this, &NrvKuribonRouteDokan);
    }
}

/**
 * @brief Checks whether the Kuribon is flipped and still in the air.
 * @return Whether the Kuribon is in the air after being flipped.
 */
inline bool Kuribon::isReverseInAir() const {
    return al::isNerve(this, &NrvKuribonReverse) && mStateReverse->isInAir();
}

/**
 * @brief Checks whether the Kuribon pushes other enemies strongly.
 * @return Whether it is flying after a flip or being ejected from a route pipe.
 */
bool Kuribon::isPushStrong() const {
    if (isReverseInAir()) {
        return true;
    }

    if (al::isNerve(this, &NrvKuribonRouteDokan)) {
        return mStateRouteDokanMove->isEject();
    }

    return false;
}

/**
 * @brief Checks whether the Kuribon can push players.
 * @return Whether players can be pushed.
 */
bool Kuribon::isEnablePush() const {
    if (mIsBig && isReverseInAir()) {
        return true;
    }

    if (al::isNerve(this, &NrvKuribonBlowDown) || al::isNerve(this, &NrvKuribonReverse)) {
        return false;
    }

    return isEnableDown();
}

/**
 * @brief Checks whether the Kuribon can attack players.
 * @return Whether players can be attacked.
 */
bool Kuribon::isEnableAttack() const {
    if (mIsBig && isReverseInAir()) {
        return false;
    }

    if (isEnableKick()) {
        return false;
    }

    return !al::isNerve(this, &NrvKuribonBlowDown);
}

/**
 * @brief Checks whether the Kuribon can enter a route pipe.
 * @return Whether a route pipe move can start.
 */
bool Kuribon::isEnableRotueDokan() const {
    if (mIsBig) {
        return false;
    }

    if (al::isNerve(this, &NrvKuribonPressDown)) {
        return false;
    }

    if (al::isNerve(this, &NrvKuribonBlowDown)) {
        return false;
    }

    return !al::isNerve(this, &NrvKuribonRouteDokanDeath);
}

/**
 * @brief Handles pushes, route pipe attacks, kicks, stomps, flips and blow downs.
 * @param pMsg Received message.
 * @param pOther Sensor that sent the message.
 * @param pSelf Sensor of the Kuribon.
 * @return Whether the message was handled.
 */
bool Kuribon::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) {
    if (!al::isNerve(this, &NrvKuribonPressDown) &&
        !al::isNerve(this, &NrvKuribonPressDownReverse) &&
        !al::isNerve(this, &NrvKuribonBlowDown) && !isPushStrong()) {
        if (al::tryReceiveMsgPushAndAddVelocity(this, pMsg, pOther, pSelf,
                                                al::isMsgPushStrong(pMsg) ? 15.0f : 1.0f)) {
            return true;
        }
    }

    if (!al::isSensorEnemyBody(pSelf)) {
        return false;
    }

    if (al::isNerve(this, &NrvKuribonRouteDokan) && mStateRouteDokanMove->isMove()) {
        if (rc::isMsgRouteDokanPlayerAttack(pMsg)) {
            rc::addScoreCombo(this, pOther, pMsg, 100.0f);
            al::setAppearItemAttackerSensor(this, pOther);
            al::appearItemTiming(this, "ルート土管死亡");
            rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
            al::setNerve(this, &NrvKuribonRouteDokanDeath);
            return true;
        }

        if (EnemyStateUtil::isMsgRouteDokanAttack(pMsg)) {
            rc::addScoreCombo(this, pOther, pMsg, 100.0f);
            if (rc::tryFindRelativeControlUserId(pOther) != -1) {
                al::setAppearItemAttackerSensor(this, pOther);
                al::appearItemTiming(this, "ルート土管死亡");
            }

            al::setNerve(this, &NrvKuribonRouteDokanDeath);
            return true;
        }

        return false;
    }

    if (isEnableKick() &&
        (al::isMsgPlayerObjRollingAttack(pMsg) || al::isMsgPlayerKick(pMsg) ||
         al::isMsgBallAttack(pMsg) || al::isMsgBallTrample(pMsg))) {
        rc::addScoreCombo(this, pOther, pMsg, 100.0f);
        startKick(pMsg, pOther, pSelf);
        return true;
    }

    if (isEnableDamage() && al::isMsgPlayerObjHipDropReflectAll(pMsg)) {
        rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        rc::setAppearItemFactorByMsg(al::getSensorHost(pSelf), pMsg, pOther);
        rc::addScoreCombo(this, pOther, pMsg, 100.0f);
        if (al::isNerve(this, &NrvKuribonReverse)) {
            al::setNerve(this, &NrvKuribonPressDownReverse);
        } else {
            al::setNerve(this, &NrvKuribonPressDown);
        }

        return true;
    }

    if (isEnableDamage() && EnemyStateUtil::isMsgPressDownForCrossoverSensor(pMsg, pOther, pSelf)) {
        if (mIsBig && al::isMsgPlayerTrample(pMsg) &&
            al::getVelocity(al::getSensorHost(pOther)).y > 0.0f) {
            return false;
        }

        if (isEnableKick()) {
            rc::addScoreCombo(this, pOther, pMsg, 100.0f);
            startKick(pMsg, pOther, pSelf);
            return true;
        }

        if (isEnableReverse()) {
            rc::addScoreCombo(this, pOther, pMsg, 100.0f);
            rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
            startReverse(al::getSensorHost(pOther));
            return true;
        }
    }

    if (isEnableReverse() &&
        (al::isMsgPlayerSlidingAttack(pMsg) || al::isMsgBallAttack(pMsg) ||
         al::isMsgBallTrample(pMsg) || al::isMsgPlayerBodyLanding(pMsg) ||
         al::isMsgPlayerClimbAttack(pMsg) || al::isMsgPlayerSpinAttack(pMsg) ||
         al::isMsgPlayerTailAttack(pMsg))) {
        rc::addScoreCombo(this, pOther, pMsg, 100.0f);
        rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        startReverse(al::getSensorHost(pOther));
        return true;
    }

    if (isEnableDamage() && al::isMsgEnemyAttack(pMsg)) {
        EnemyStateUtil::requestBlowDown(pMsg, pOther, pSelf, mStateBlowDown, true);
        rc::setAppearItemFactorByMsg(this, pMsg, pOther);
        rc::addScoreCombo(this, pOther, pMsg, 100.0f);
        al::setNerve(this, &NrvKuribonBlowDown);
        return true;
    }

    if (!isEnableDown() && !isEnableKick()) {
        return false;
    }

    if (!mIsBig) {
        if (EnemyStateUtil::tryRequestBlowDownAndNextNerve(pMsg, pOther, pSelf, mStateBlowDown,
                                                           &NrvKuribonBlowDown, true)) {
            rc::addScoreCombo(this, pOther, pMsg, 100.0f);
            return true;
        }

        return false;
    }

    if (al::isMsgPlayerCooperationHipDrop(pMsg) || al::isMsgPlayerInvincibleAttack(pMsg) ||
        al::isMsgPlayerGiantAttack(pMsg) || al::isMsgLaserAttack(pMsg) ||
        al::isMsgPlayerGiantHipDrop(pMsg) || al::isMsgExplosion(pMsg)) {
        rc::addScoreCombo(this, pOther, pMsg, 100.0f);
        startKick(pMsg, pOther, pSelf);
        return true;
    }

    if (EnemyStateUtil::isMsgBlowDown(pMsg)) {
        if (isEnableReverse()) {
            rc::addScoreCombo(this, pOther, pMsg, 100.0f);
            rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
            startReverse(al::getSensorHost(pOther));
        } else {
            rc::addScoreCombo(this, pOther, pMsg, 100.0f);
            startKick(pMsg, pOther, pSelf);
        }

        return true;
    }

    return false;
}

/**
 * @brief Checks whether the Kuribon can be kicked away.
 * @return Whether the Kuribon lies flipped on the ground or is frozen while flipped.
 */
bool Kuribon::isEnableKick() const {
    if (mIsBig && al::isNerve(this, &NrvKuribonReverse) && mStateReverse->isRecover() &&
        al::getNerveStep(mStateReverse) > 65) {
        return false;
    }

    if (al::isNerve(this, &NrvKuribonReverse) && !mStateReverse->isInAir()) {
        return true;
    }

    return al::isNerve(this, &NrvKuribonSupportFreezeReverse);
}

/**
 * @brief Blows the Kuribon away after a kick.
 * @param pMsg Kick message.
 * @param pOther Sensor that kicked.
 * @param pSelf Sensor of the Kuribon.
 */
void Kuribon::startKick(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) {
    EnemyStateUtil::requestBlowDown(pMsg, pOther, pSelf, mStateBlowDown, true);
    al::setNerve(this, &NrvKuribonBlowDown);
}

/**
 * @brief Checks whether the Kuribon can take damage.
 * @return Whether it can be damaged.
 */
bool Kuribon::isEnableDamage() const {
    if (mIsBig && isReverseInAir()) {
        return false;
    }

    if (al::isNerve(this, &NrvKuribonPressDown)) {
        return false;
    }

    if (al::isNerve(this, &NrvKuribonBlowDown)) {
        return false;
    }

    return !al::isNerve(this, &NrvKuribonRouteDokanDeath);
}

/**
 * @brief Checks whether the Kuribon can be flipped.
 * @return Whether it is neither blown away nor already flipped.
 */
bool Kuribon::isEnableReverse() const {
    if (al::isNerve(this, &NrvKuribonBlowDown)) {
        return false;
    }

    return !al::isNerve(this, &NrvKuribonReverse);
}

/**
 * @brief Flips the Kuribon away from an attacker (or toward a nearby other player).
 * @param pAttacker Actor that flipped the Kuribon.
 */
void Kuribon::startReverse(const al::LiveActor* pAttacker) {
    bool isBig = mIsBig;
    f32 speed = isBig ? 35.0f : 18.0f;
    const sead::Vector3f& trans = al::getTrans(this);
    bool isTowardPlayer = false;
    {
        const al::LiveActor* playerList[4];
        s32 playerNum = al::calcPlayerListOrderByDistance(this, playerList, 4);
        for (s32 i = 0; i < playerNum; i++) {
            if (playerList[i] == pAttacker) {
                continue;
            }

            const sead::Vector3f& playerTrans = al::getTrans(playerList[i]);
            if (!mIsBig && (playerTrans - trans).squaredLength() < 3000.0f * 3000.0f) {
                pAttacker = playerList[i];
                isTowardPlayer = true;
            }

            break;
        }
    }

    f32 angle = isBig ? sead::Mathf::deg2rad(45.0f) : sead::Mathf::deg2rad(25.0f);
    sead::Vector3f velocity = al::getTrans(this);
    velocity -= al::getTrans(pAttacker);
    velocity.y = 0.0f;
    setLengthHOrDirZ(&velocity, speed * sinf(angle));
    if (isTowardPlayer) {
        velocity.x = -velocity.x;
        velocity.z = -velocity.z;
    }

    velocity.y = speed * cosf(angle);
    al::setVelocity(this, velocity);
    velocity.y = 0.0f;
    velocity.normalize();
    al::faceToDirection(this, velocity);
    al::setNerve(this, &NrvKuribonReverse);
}

/**
 * @brief Checks whether the Kuribon can be knocked down.
 * @return Whether it can be knocked down.
 */
bool Kuribon::isEnableDown() const {
    if (mIsBig && isReverseInAir()) {
        return false;
    }

    if (al::isNerve(this, &NrvKuribonPressDown)) {
        return false;
    }

    if (al::isNerve(this, &NrvKuribonBlowDown)) {
        return false;
    }

    if (al::isNerve(this, &NrvKuribonRouteDokanDeath)) {
        return false;
    }

    return !al::isNerve(this, &NrvKuribonReverse);
}

/**
 * @brief Handles touch screen hits and freezes.
 * @param pMsg Received message.
 * @param pPointer Screen pointer that touched.
 * @param pTarget Touched screen point target.
 * @return Whether the message was handled.
 */
bool Kuribon::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                    al::ScreenPointTarget* pTarget) {
    if (al::isNerve(this, &NrvKuribonPressDown) ||
        al::isNerve(this, &NrvKuribonPressDownReverse) ||
        al::isNerve(this, &NrvKuribonBlowDown) || al::isNerve(this, &NrvKuribonRouteDokanDeath) ||
        al::isNerve(this, &NrvKuribonRouteDokan)) {
        return false;
    }

    if (al::isMsgTouchAssistTrig(pMsg) && !mIsBig &&
        !al::isNerve(this, &NrvKuribonSupportFreeze) && !al::isNerve(this, &NrvKuribonBlowDown) &&
        !isReverseStart()) {
        if (!al::isNerve(this, &NrvKuribonReverse) &&
            !al::isNerve(this, &NrvKuribonSupportFreezeReverse)) {
            rc::addScore(this, pPointer, 0.0f, 0);
        }

        startScreenHit();
        return true;
    }

    if (!mStateSupportFreeze->receiveMsgScreenPoint(pMsg, pPointer, pTarget)) {
        return false;
    }

    if (isReverseStart()) {
        return true;
    }

    if (al::isNerve(this, &NrvKuribonReverse)) {
        mMicRumbler->stopAndReset();
        mStateReverse->cancel();
        al::setNerve(this, &NrvKuribonSupportFreezeReverse);
        return true;
    }

    if (al::isNerve(this, &NrvKuribonSupportFreeze) ||
        al::isNerve(this, &NrvKuribonSupportFreezeReverse)) {
        return true;
    }

    mMicRumbler->stopAndReset();
    al::setNerve(this, &NrvKuribonSupportFreeze);
    return true;
}

/**
 * @brief Checks whether the Kuribon was blown away by a kick.
 * @return Whether it is blown away.
 */
bool Kuribon::isAfterKick() const {
    return al::isNerve(this, &NrvKuribonBlowDown);
}

/**
 * @brief Checks whether the Kuribon has just been flipped.
 * @return Whether it is at the start of a flip.
 */
bool Kuribon::isReverseStart() const {
    if (!al::isNerve(this, &NrvKuribonReverse)) {
        return false;
    }

    return al::isLessEqualStep(this, mIsBig ? 200 : 40);
}

/** @brief Flips the Kuribon away from the camera after a touch screen hit. */
void Kuribon::startScreenHit() {
    bool isBig = mIsBig;
    f32 speed = isBig ? 35.0f : 18.0f;
    sead::Vector3f velocity;
    al::calcCameraLookDir(&velocity, this);
    f32 angle = isBig ? sead::Mathf::deg2rad(45.0f) : sead::Mathf::deg2rad(25.0f);
    velocity.y = 0.0f;
    setLengthHOrDirZ(&velocity, speed * sinf(angle));
    velocity.y = speed * cosf(angle);
    al::setVelocity(this, velocity);
    velocity.y = 0.0f;
    velocity.normalize();
    al::faceToDirection(this, velocity);
    al::setNerve(this, &NrvKuribonReverse);
}

/** @brief Waits until a player comes near. */
void Kuribon::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait");
        al::setVelocityToGravity(this, sWalkerStateParam.mGravity);
    }

    WalkerStateFunction::calcPassiveMovement(this, &sWalkerStateParam);
    mTargetFinder->update();
    if (isActive(mActiveDistance)) {
        al::setNerve(this, &NrvKuribonWander);
    }
}

/**
 * @brief Checks whether the Kuribon should be active.
 * @param distance Distance to the nearest player within which it is active.
 * @return Whether it is in the air or near a player.
 */
bool Kuribon::isActive(f32 distance) const {
    if (!al::isOnGround(this, 0, 0.0f)) {
        return true;
    }

    return al::isNearPlayer(this, distance);
}

/** @brief Waits as a gate keeper until a target is found. */
void Kuribon::exeWaitGateKeeper() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait");
        al::setVelocityToGravity(this, sWalkerStateParam.mGravity);
    }

    WalkerStateFunction::calcPassiveMovement(this, &sWalkerStateParam);
    mTargetFinder->update();
    if (mTargetFinder->isExistTarget()) {
        al::setNerve(this, &NrvKuribonFindPlayer);
    }
}

/** @brief Wanders around and goes back to waiting when no player is near. */
void Kuribon::exeWander() {
    al::updateNerveStateAndNextNerve(this, &NrvKuribonFindPlayer);
    if (!isActive(mDeactiveDistance)) {
        al::setNerve(this, &NrvKuribonWait);
    }
}

/** @brief Turns toward a found player and then chases it. */
void Kuribon::exeFindPlayer() {
    al::updateNerveStateAndNextNerve(this, &NrvKuribonChase);
}

/** @brief Plays the attack animation. */
void Kuribon::exeAttack() {
    if (al::isFirstStep(this)) {
        al::setVelocityZero(this);
        al::startAction(this, "Attack");
    }

    WalkerStateFunction::calcPassiveMovement(this, &sWalkerStateParam);
    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvKuribonWait);
    }
}

/** @brief Chases the target and wanders around the place where it was lost. */
void Kuribon::exeChase() {
    if (al::updateNerveStateAndNextNerve(this, &NrvKuribonWander)) {
        mStateWander->setWanderCenter(al::getTrans(this));
    }
}

/** @brief Lies flipped until it recovers. */
void Kuribon::exeReverse() {
    al::updateNerveStateAndNextNerve(this, &NrvKuribonWait);
}

/** @brief Moves through a route pipe. */
void Kuribon::exeRouteDokan() {
    if (al::updateNerveState(this)) {
        al::setNerve(this, &NrvKuribonWait);
    }
}

/** @brief Dies after being hit inside a route pipe. */
void Kuribon::exeRouteDokanDeath() {
    if (al::isFirstStep(this)) {
        al::startHitReaction(this, "ルート土管死亡");
        kill();
    }
}

/** @brief Gets stomped flat and dies. */
void Kuribon::exePressDown() {
    if (al::isFirstStep(this)) {
        al::startHitReactionHit(this);
        al::startAction(this, al::isNerve(this, &NrvKuribonPressDownReverse) ? "PressDownReverse" :
                                                                               "PressDown");
        al::setVelocityZero(this);
        al::invalidateClipping(this);
        mMicRumbler->stopAndReset();
    }

    if (al::isActionEnd(this)) {
        al::appearItem(this);
        al::startHitReactionDeath(this);
        kill();
    }
}

/** @brief Gets blown away and dies. */
void Kuribon::exeBlowDown() {
    if (al::isFirstStep(this)) {
        al::startHitReactionHit(this);
        mMicRumbler->stopAndReset();
    }

    if (al::updateNerveState(this)) {
        al::appearItem(this);
        al::startHitReactionDeath(this);
        kill();
    }
}

/** @brief Stays frozen and goes back to the previous behavior afterwards. */
void Kuribon::exeSupportFreeze() {
    if (al::updateNerveState(this)) {
        if (al::isNerve(this, &NrvKuribonSupportFreezeReverse)) {
            al::setNerve(this, &NrvKuribonReverse);
        } else {
            al::setNerve(this, &NrvKuribonWait);
        }
    }
}
