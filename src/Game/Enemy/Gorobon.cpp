#include "Enemy/Gorobon.hpp"

#include <attributes.h>

#include <math/seadQuat.h>

#include "Enemy/ActorStateSupportFreeze.hpp"
#include "Enemy/EnemyStateUtil.hpp"
#include "Enemy/TargetFinder.hpp"
#include "Enemy/WalkerStateParam.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Joint/JointAimInfo.hpp"
#include "Library/Joint/JointControllerKeeper.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Obj/BreakModel.hpp"
#include "Library/Obj/PartsFunction.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Thread/Functor.hpp"
#include "Project/Collision/CollisionUtil.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "MapObj/BallStateFunction.hpp"
#include "MapObj/ItemStatePlayerHold.hpp"
#include "MapObj/ItemStatePlayerHoldParam.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/JointAimUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ScoreUtil.hpp"

// Nerve with an end callback.
#define GOROBON_NERVE_END_DECL(Action, EndFunc)                                                    \
    class GorobonNrv##Action : public al::Nerve {                                                  \
    public:                                                                                        \
        void execute(al::NerveKeeper* pKeeper) const override {                                    \
            (pKeeper->getParent<Gorobon>())->exe##Action();                                        \
        }                                                                                          \
                                                                                                   \
        void executeOnEnd(al::NerveKeeper* pKeeper) const override {                               \
            (pKeeper->getParent<Gorobon>())->end##EndFunc();                                       \
        }                                                                                          \
    };

// Nerve that shares the execute function of another nerve.
#define GOROBON_NERVE_SHARED_DECL(Action, ExeFunc)                                                 \
    class GorobonNrv##Action : public al::Nerve {                                                  \
    public:                                                                                        \
        void execute(al::NerveKeeper* pKeeper) const override {                                    \
            (pKeeper->getParent<Gorobon>())->exe##ExeFunc();                                       \
        }                                                                                          \
    };

namespace {
NERVE_DECL(Gorobon, WaitGround)
NERVE_DECL(Gorobon, WaitGroundWithStageSwitch)
GOROBON_NERVE_END_DECL(PlayerHold, Hold)
NERVE_DECL(Gorobon, SupportFreeze)
GOROBON_NERVE_END_DECL(Walk, Walk)
NERVE_DECL(Gorobon, SinkMagma)
GOROBON_NERVE_SHARED_DECL(RockFall, RockThrow)
NERVE_DECL(Gorobon, RockThrow)
NERVE_DECL(Gorobon, Break)
NERVE_DECL(Gorobon, Appear)
NERVE_DECL(Gorobon, RockSpinShot)
NERVE_DECL(Gorobon, RockStartAgain)
NERVE_DECL(Gorobon, RockWait)
NERVE_DECL(Gorobon, RockStart)
NERVE_DECL(Gorobon, Death)
NERVE_DECL(Gorobon, AppearSign)
NERVE_DECL(Gorobon, Land)
NERVE_DECL(Gorobon, DemoWait)
NERVE_DECL(Gorobon, RockEnd)
NERVE_DECL(Gorobon, Recovery)
NERVE_DECL(Gorobon, LandRecovery)
// Non-const nerve objects: the game keeps them in .data in this order.
GorobonNrvWaitGround NrvGorobonWaitGround;
GorobonNrvWaitGroundWithStageSwitch NrvGorobonWaitGroundWithStageSwitch;
GorobonNrvPlayerHold NrvGorobonPlayerHold;
GorobonNrvSupportFreeze NrvGorobonSupportFreeze;
GorobonNrvWalk NrvGorobonWalk;
GorobonNrvSinkMagma NrvGorobonSinkMagma;
GorobonNrvRockFall NrvGorobonRockFall;
GorobonNrvRockThrow NrvGorobonRockThrow;
GorobonNrvBreak NrvGorobonBreak;
GorobonNrvAppear NrvGorobonAppear;
GorobonNrvRockSpinShot NrvGorobonRockSpinShot;
GorobonNrvRockStartAgain NrvGorobonRockStartAgain;
GorobonNrvRockWait NrvGorobonRockWait;
GorobonNrvRockStart NrvGorobonRockStart;
GorobonNrvDeath NrvGorobonDeath;
GorobonNrvAppearSign NrvGorobonAppearSign;
GorobonNrvLand NrvGorobonLand;
GorobonNrvDemoWait NrvGorobonDemoWait;
GorobonNrvRockEnd NrvGorobonRockEnd;
GorobonNrvRecovery NrvGorobonRecovery;
GorobonNrvLandRecovery NrvGorobonLandRecovery;

typedef al::FunctorV0M<Gorobon*, void (Gorobon::*)()> GorobonFunctor;

ItemStatePlayerHoldParam sPlayerHoldParam(
    sead::Vector3f(150.0f, 0.0f, 0.0f), sead::Vector3f(150.0f, 0.0f, 0.0f),
    sead::Vector3f(150.0f, 0.0f, 0.0f), sead::Vector3f(170.0f, 0.0f, 0.0f),
    sead::Vector3f(150.0f, 0.0f, 0.0f), sead::Vector3f(160.0f, 0.0f, 0.0f),
    sead::Vector3f(160.0f, 0.0f, 0.0f), sead::Vector3f(160.0f, 0.0f, 0.0f),
    sead::Vector3f(170.0f, 0.0f, 0.0f), sead::Vector3f(160.0f, 0.0f, 0.0f),
    sead::Vector3f(0.0f, 0.0f, 0.0f));

/** @brief Freeze, target search and movement parameters shared by every Gorobon. */
struct GorobonParam {
    GorobonParam()
        : mSupportFreezeParam(true, 15, false, true, 120, sead::Vector3f(0.0f, 30.0f, 0.0f)) {
        mTargetFinderParam._0 = 5000.0f;
        mTargetFinderParam._4 = 180.0f;
        mTargetFinderParam._8 = 90.0f;
        mWalkerStateParam.mGravity = 3.0f;
        mWalkerStateParam.mAirFriction = 0.99f;
        mWalkerStateParam.mGroundFriction = 0.3f;
    }

    ActorStateSupportFreezeParam mSupportFreezeParam;
    TargetFinderParam mTargetFinderParam;
    WalkerStateParam mWalkerStateParam;
};

GorobonParam sParam;

/**
 * @brief Copies a vector as a plain copy of its storage.
 * @param pDst Destination vector.
 * @param rSrc Source vector.
 */
inline void copyVector(sead::Vector3f* pDst, const sead::Vector3f& rSrc) {
    pDst->e = rSrc.e;
}

/**
 * @brief Makes a matrix with an identity rotation and the given translation.
 * @param pMtx Matrix to set.
 * @param rTrans Translation of the matrix.
 */
inline void makeMtxTrans(sead::Matrix34f* pMtx, const sead::Vector3f& rTrans) {
    pMtx->m[0][0] = 1.0f;
    pMtx->m[0][1] = 0.0f;
    pMtx->m[0][2] = 0.0f;
    pMtx->m[1][0] = 0.0f;
    pMtx->m[1][1] = 1.0f;
    pMtx->m[1][2] = 0.0f;
    pMtx->m[2][0] = 0.0f;
    pMtx->m[2][1] = 0.0f;
    pMtx->m[2][2] = 1.0f;
    pMtx->setTranslation(rTrans);
}

/**
 * @brief Copies a quaternion as a plain copy of its storage.
 * @param pDst Destination quaternion.
 * @param rSrc Source quaternion.
 */
inline void copyQuat(sead::Quatf* pDst, const sead::Quatf& rSrc) {
    static_cast<sead::BaseQuat<f32>&>(*pDst) = rSrc;
}
}  // namespace

/**
 * @brief Constructs a Gorobon with its combo counter.
 * @param pName Actor name.
 */
Gorobon::Gorobon(const char* pName) : al::LiveActor(pName) {}

/**
 * @brief Initializes the model, break model, states, effects and eye aim controllers.
 * @param rInfo Placement info of the actor.
 */
void Gorobon::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "Gorobon", nullptr);
    al::initNerve(this, &NrvGorobonWaitGround, 2);
    al::offCollide(this);

    mTargetFinder = new TargetFinder(this, &sParam.mTargetFinderParam);
    mTargetFinder->setFrontDir(&mFrontDir);
    copyVector(&mInitTrans, al::getTrans(this));
    copyQuat(&mInitQuat, al::getQuat(this));

    mBreakModel = new al::BreakModel(this, "ゴロボン壊れモデル", "GorobonBreak", nullptr, nullptr,
                                     "Break", true);
    al::initCreateActorNoPlacementInfo(mBreakModel, rInfo);

    copyVector(&mAppearSignPos, al::getTrans(this));
    mAppearSignPos.y += -60.0f;
    al::setEffectFollowPosPtr(this, "AppearSign", &mAppearSignPos);
    al::addTransOffsetLocalDir(this, -100.0f, 1);
    al::tryGetArg(&mIsRecover, rInfo, "IsRecover");
    al::tryGetArg(&mRebirthTime, rInfo, "RebirthTime");
    al::tryGetArg(&mIsEnableAppearSign, rInfo, "IsEnableAppearSign");
    if (al::isValidStageSwitch(this, "SwitchStart")) {
        al::setNerve(this, &NrvGorobonWaitGroundWithStageSwitch);
    }

    al::listenStageSwitchOnKill(this, GorobonFunctor(this, &Gorobon::killBySwitch));
    al::setEffectFollowPosPtr(this, "Walk", al::getTransPtr(this));
    al::setEffectFollowPosPtr(this, "Sweat", al::getTransPtr(this));
    al::setEffectFollowMtxPtr(this, "HitCollision", &mEffectMtx);
    al::setEffectFollowMtxPtr(this, "LandSmoke", &mEffectMtx);
    al::initJointControllerKeeper(this, 3);
    if (al::isExistJoint(this, "JointRoot")) {
        al::initJointLocalXRotator(this, &mRotateX, "JointRoot");
    }

    mStatePlayerHold = new ItemStatePlayerHold(this, &sPlayerHoldParam, true, false);
    mStateSupportFreeze = new ActorStateSupportFreeze(this, &sParam.mSupportFreezeParam);
    al::initNerveState(this, mStatePlayerHold, &NrvGorobonPlayerHold,
                       "[state]プレイヤーに持たれる");
    al::initNerveState(this, mStateSupportFreeze, &NrvGorobonSupportFreeze, "[state]フリーズ");
    mStatePlayerHold->initColliderControl();
    if (!mIsBossStage && GameDataFunction::isCurrentStageGateKeeper(this)) {
        mStatePlayerHold->setFlag23(true);
    }

    al::hideModelIfShow(this);

    mJointAimInfo = new al::JointAimInfo();
    mJointAimInfo->setBaseAimLocalDir(sead::Vector3f::ex);
    mJointAimInfo->setBaseSideLocalDir(sead::Vector3f::ez);
    mJointAimInfo->setBaseUpLocalDir(sead::Vector3f::ey);
    mJointAimInfo->setEnableBackAim(true);
    mJointAimInfo->setLimitDegreeRect(34.0f, 11.0f, 0.0f, 0.0f);
    al::initJointAimController(this, mJointAimInfo, "EyeL");
    al::initJointAimController(this, mJointAimInfo, "EyeR");
    makeActorAppeared();
}

/** @brief Breaks the Gorobon without recovery when its kill switch turns on. */
void Gorobon::killBySwitch() {
    if (al::isNerve(this, &NrvGorobonPlayerHold)) {
        rc::requestPlayerRelease(mHolderSensor);
    }

    mIsRecover = false;
    al::setNerve(this, &NrvGorobonBreak);
}

/**
 * @brief Checks whether the Gorobon is broken, dead or still hidden in the ground.
 * @return Whether the Gorobon is in an inactive nerve.
 */
inline bool Gorobon::isNerveInactive() const {
    return al::isNerve(this, &NrvGorobonDeath) || al::isNerve(this, &NrvGorobonWaitGround) ||
           al::isNerve(this, &NrvGorobonWaitGroundWithStageSwitch) ||
           al::isNerve(this, &NrvGorobonBreak) || al::isNerve(this, &NrvGorobonSinkMagma);
}

/** @brief Updates cool times, magma contact, death areas and the eye aim. */
void Gorobon::control() {
    if (mIsBossDemo) {
        mJointAimInfo->setPowerRate(0.0f);
        return;
    }

    if (mHitCoolTime > 0) {
        mHitCoolTime--;
    }

    if (mReflectCoolTime > 0) {
        mReflectCoolTime--;
    }

    if (mAttackCoolTime > 0) {
        mAttackCoolTime--;
    }

    if (isNerveInactive()) {
        return;
    }

    if (rc::isCollidedDamageFire(this)) {
        mIsSinkByFire = !al::isNerve(this, &NrvGorobonWalk);
        al::setNerve(this, &NrvGorobonSinkMagma);
        return;
    }

    al::tryKillByDeathArea(this);
    if (al::isNerve(this, &NrvGorobonAppear) || al::isNerve(this, &NrvGorobonLand) ||
        al::isNerve(this, &NrvGorobonWalk) || al::isNerve(this, &NrvGorobonRecovery)) {
        JointAimUtil::updateEyeJointInfo(this, mJointAimInfo, 1500.0f, 0.2f);
        return;
    }

    if (al::isNerve(this, &NrvGorobonLandRecovery)) {
        JointAimUtil::updateEyeJointInfo(this, mJointAimInfo, 1500.0f, 0.2f);
        return;
    }

    mJointAimInfo->subPowerRate(0.1f);
}

/** @brief Updates the collider through the hold state while the Gorobon is carried. */
void Gorobon::updateCollider() {
    if (mStatePlayerHold->isDead()) {
        al::LiveActor::updateCollider();
        return;
    }

    mStatePlayerHold->updateCollider(al::getHitSensor(this, "Body"));
}

/**
 * @brief Attacks enemies, map objects and players touched by the Gorobon.
 * @param pSelf Sensor of the Gorobon.
 * @param pOther Sensor that was touched.
 */
void Gorobon::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isNerve(this, &NrvGorobonRockFall) || isNerveInactive()) {
        return;
    }

    if (al::isNerve(this, &NrvGorobonPlayerHold)) {
        rc::sendMsgBubbleVanish(pOther, pSelf);
        if (al::isSensorEnemyBody(pSelf) && al::isSensorPlayer(pOther) &&
            al::getSensorHost(pOther) != al::getSensorHost(mHolderSensor)) {
            al::sendMsgPush(pOther, pSelf);
        }

        return;
    }

    if (isRockState()) {
        if (al::isSensorHoldObj(pSelf) && al::isSensorMapObj(pOther)) {
            al::sendMsgBallItemGet(pOther, pSelf);
        }

        if (al::isSensorEnemyBody(pSelf) && al::isSensorEnemyBody(pOther)) {
            if (al::isNerve(this, &NrvGorobonRockThrow)) {
                if (rc::sendMsgGorobonAttack(pOther, pSelf, mComboCounter)) {
                    al::setNerve(this, &NrvGorobonBreak);
                    return;
                }

                if (al::sendMsgBallAttack(pOther, pSelf, mComboCounter) ||
                    al::sendMsgEnemyAttack(pOther, pSelf)) {
                    return;
                }
            } else {
                al::sendMsgPush(pOther, pSelf);
            }

            if (rc::sendMsgBubbleVanish(pOther, pSelf)) {
                return;
            }
        }

        if (al::isSensorEnemyBody(pSelf) && al::isSensorPlayer(pOther)) {
            al::sendMsgPush(pOther, pSelf);
        }

        return;
    }

    if (al::isSensorName(pSelf, "Body") && al::isSensorEnemyBody(pOther)) {
        if (al::isNerve(this, &NrvGorobonWalk)) {
            if (rc::sendMsgBubbleVanish(pOther, pSelf)) {
                return;
            }

            if (al::sendMsgEnemyAttack(pOther, pSelf)) {
                al::startHitReactionHitEffect(this, "エネミーアタックヒット", pSelf, pOther);
                return;
            }
        }

        al::sendMsgPush(pOther, pSelf);
    }

    if (al::isSensorEnemyBody(pSelf) && al::isSensorMapObj(pOther)) {
        al::sendMsgBallAttackCollide(pOther, pSelf);
    }

    if (al::isSensorEnemyAttack(pSelf) && al::isSensorPlayer(pOther)) {
        if (!al::isNerve(this, &NrvGorobonAppear)) {
            al::sendMsgEnemyAttack(pOther, pSelf);
        }

        al::sendMsgPush(pOther, pSelf);
    }
}

/**
 * @brief Checks whether the Gorobon is curled up as a rock (including while carried or thrown).
 * @return Whether the Gorobon is in a rock nerve.
 */
bool Gorobon::isRockState() const {
    return al::isNerve(this, &NrvGorobonRockStart) || al::isNerve(this, &NrvGorobonRockStartAgain) ||
           al::isNerve(this, &NrvGorobonRockWait) || al::isNerve(this, &NrvGorobonRockEnd) ||
           al::isNerve(this, &NrvGorobonPlayerHold) || al::isNerve(this, &NrvGorobonRockThrow) ||
           al::isNerve(this, &NrvGorobonRockFall);
}

/**
 * @brief Handles hold, throw, attack and push messages.
 * @param pMsg Received message.
 * @param pOther Sensor that sent the message.
 * @param pSelf Sensor of the Gorobon.
 * @return Whether the message was handled.
 */
bool Gorobon::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) {
    if (rc::isMsgAskControlUserId(pMsg, mHolderSensor)) {
        return true;
    }

    if (al::isNerve(this, &NrvGorobonRockFall) || al::isNerve(this, &NrvGorobonRockSpinShot) ||
        al::isNerve(this, &NrvGorobonAppear) || isNerveInactive()) {
        return false;
    }

    if (isRockState() && al::isSensorEnemyBody(pSelf) &&
        (al::isMsgPlayerFireBallAttack(pMsg) ||
         EnemyStateUtil::isMsgPressDownForCrossoverSensor(pMsg, pOther, pSelf) ||
         al::isMsgPlayerBoomerangReflect(pMsg) || al::isMsgPlayerTailAttack(pMsg) ||
         al::isMsgPlayerClimbAttack(pMsg) || al::isMsgPlayerSpinAttack(pMsg) ||
         al::isMsgPlayerObjHipDropReflectAll(pMsg) || al::isMsgPlayerRollingReflect(pMsg) ||
         al::isMsgPlayerBodyAttackReflect(pMsg) || al::isMsgPlayerBodyLanding(pMsg))) {
        if (mHitCoolTime == 0) {
            if (al::isNerve(this, &NrvGorobonPlayerHold)) {
                al::startAction(this, "RockStartAgain");
                mHoldTime = 0;
            } else if (!al::isNerve(this, &NrvGorobonRockThrow)) {
                al::setNerve(this, &NrvGorobonRockStartAgain);
            }

            rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
            startSeHit();
            mHitCoolTime = 22;
        }

        if (al::isMsgPlayerBodyAttackReflect(pMsg)) {
            mReflectCoolTime = 22;
        }

        return true;
    }

    if (al::isNerve(this, &NrvGorobonPlayerHold)) {
        if (!mStatePlayerHold->receiveMsg(pMsg, pOther, pSelf)) {
            return false;
        }

        if (al::isMsgPlayerRelease(pMsg) || al::isMsgHoldCancel(pMsg) ||
            al::isMsgWarpStart(pMsg)) {
            al::setNerve(this, &NrvGorobonRockThrow);
        } else {
            al::setNerve(this, &NrvGorobonRockFall);
        }

        sead::Vector3f front = rc::getPlayerFront(mHolderSensor);
        al::calcSideDir(&mSideDir, al::getSensorHost(mHolderSensor));
        al::verticalizeVec(&front, sead::Vector3f::ey, front);
        front *= 16.0f;
        front += sead::Vector3f(0.0f, 50.0f, 0.0f);
        copyVector(&mThrowVelocity, front);
        al::invalidateClipping(this);
        return true;
    }

    if (rc::isMsgBossGorobonSpinShot(pMsg)) {
        al::startHitReaction(this, "スピン攻撃を受けた");
        al::setNerve(this, &NrvGorobonRockSpinShot);
        mFrontDir.setSub(al::getTrans(this), al::getTrans(al::getSensorHost(pOther)));
        al::normalize(&mFrontDir);
        return true;
    }

    if (rc::isMsgBossGorobonAttack(pMsg) && !al::isNerve(this, &NrvGorobonRockThrow)) {
        al::setNerve(this, &NrvGorobonBreak);
        return true;
    }

    if (rc::isMsgGorobonAttack(pMsg)) {
        rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        rc::addScoreCombo(this, pOther, pMsg, 100.0f);
        rc::setAppearItemFactorByMsg(this, pMsg, pOther);
        al::appearItem(this);
        al::setNerve(this, &NrvGorobonBreak);
        return false;
    }

    if (al::isSensorEnemyBody(pOther) && al::isSensorName(pSelf, "Body") &&
        !al::isNerve(this, &NrvGorobonRockThrow) && !al::isNerve(this, &NrvGorobonRockWait)) {
        if (al::isNerve(this, &NrvGorobonWalk) && rc::isMsgMeraWanwanPush(pMsg)) {
            sead::Vector3f dir = {0.0f, 0.0f, 0.0f};
            al::calcDirBetweenSensorsH(&dir, pOther, pSelf);
            if (al::isNearZero(dir, 0.001f)) {
                dir = -mFrontDir;
            }

            al::setVelocityToDirection(this, dir, 15.0f);
            return true;
        }

        if (al::tryReceiveMsgPushAndAddVelocity(this, pMsg, pOther, pSelf, 2.0f)) {
            return true;
        }
    }

    if (isEnableHold(pOther) && mStatePlayerHold->tryStartCarryUp(pMsg, pOther, false)) {
        mHolderSensor = pOther;
        al::setNerve(this, &NrvGorobonPlayerHold);
        return true;
    }

    if (al::isNerve(this, &NrvGorobonLand) || al::isNerve(this, &NrvGorobonSupportFreeze) ||
        al::isNerve(this, &NrvGorobonWalk) || al::isNerve(this, &NrvGorobonRecovery) ||
        al::isNerve(this, &NrvGorobonLandRecovery)) {
        if (!al::isSensorEnemyBody(pSelf)) {
            return false;
        }

        if (al::isMsgPlayerTrampleForCrossoverSensor(pMsg, pOther, pSelf) ||
            al::isMsgPlayerObjHipDropReflectAll(pMsg) || al::isMsgPlayerFireBallAttack(pMsg) ||
            al::isMsgPlayerTailAttack(pMsg) || al::isMsgPlayerClimbAttack(pMsg) ||
            al::isMsgPlayerBodyAttack(pMsg) || al::isMsgPlayerBodyLanding(pMsg) ||
            al::isMsgPlayerCooperationHipDrop(pMsg) || al::isMsgPlayerGiantHipDrop(pMsg) ||
            al::isMsgPlayerSlidingAttack(pMsg) || al::isMsgPlayerSpinAttack(pMsg) ||
            al::isMsgPlayerBoomerangReflect(pMsg) || al::isMsgPlayerInvincibleAttack(pMsg) ||
            al::isMsgExplosion(pMsg)) {
            rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
            startSeHit();
            mHitCoolTime = 22;
            al::setNerve(this, &NrvGorobonRockStart);
            return true;
        }
    }

    return false;
}

/** @brief Plays the sound effects of the Gorobon being hit. */
void Gorobon::startSeHit() {
    al::startSe(this, "PgTrample");
    al::startSe(this, "PgRockHit");
}

/**
 * @brief Checks whether the given sensor may pick the Gorobon up.
 * @param pSensor Sensor that tries to hold the Gorobon.
 * @return Whether the Gorobon can be held.
 */
bool Gorobon::isEnableHold(al::HitSensor* pSensor) {
    if (al::isNerve(this, &NrvGorobonRockWait) || al::isNerve(this, &NrvGorobonRockEnd) ||
        al::isNerve(this, &NrvGorobonRockThrow)) {
        if (mHolderSensor == nullptr ||
            al::getSensorHost(pSensor) != al::getSensorHost(mHolderSensor)) {
            return true;
        }

        return mReflectCoolTime == 0;
    }

    return false;
}

/**
 * @brief Handles touch screen messages: curling up and support freezing.
 * @param pMsg Received message.
 * @param pPointer Screen pointer that touched the Gorobon.
 * @param pTarget Touched screen point target.
 * @return Whether the message was handled.
 */
bool Gorobon::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                    al::ScreenPointTarget* pTarget) {
    if ((al::isNerve(this, &NrvGorobonLand) || al::isNerve(this, &NrvGorobonSupportFreeze) ||
         al::isNerve(this, &NrvGorobonWalk) || al::isNerve(this, &NrvGorobonRecovery) ||
         al::isNerve(this, &NrvGorobonLandRecovery)) &&
        al::isMsgTouchAssistTrig(pMsg)) {
        startSeHit();
        al::setNerve(this, &NrvGorobonRockStart);
        return true;
    }

    if (isRockState() && al::isMsgTouchAssistTrig(pMsg)) {
        startSeHit();
        if (al::isNerve(this, &NrvGorobonPlayerHold)) {
            al::startAction(this, "RockStartAgain");
            mHoldTime = 0;
        } else if (!al::isNerve(this, &NrvGorobonRockThrow)) {
            al::setNerve(this, &NrvGorobonRockStartAgain);
        }

        return true;
    }

    if (al::isMsgTouchAssist(pMsg)) {
        if (al::isNerve(this, &NrvGorobonWalk) || al::isNerve(this, &NrvGorobonLand) ||
            al::isNerve(this, &NrvGorobonLandRecovery)) {
            mStateSupportFreeze->setTouchActor(pPointer);
            al::setNerve(this, &NrvGorobonSupportFreeze);
        }

        return true;
    }

    return false;
}

/** @brief Resets the Gorobon to its initial pose so the boss can make it appear. */
void Gorobon::initAtBossStage() {
    al::invalidateClipping(this);
    al::setTransY(this, mInitTrans.y);
    al::addTransOffsetLocalDir(this, -100.0f, 1);
    al::setQuat(this, mInitQuat);
    al::setVelocityZero(this);
    copyVector(&mFrontDir, sead::Vector3f::ez);
    mRotateX = 0.0f;
    al::setNerve(this, &NrvGorobonAppear);
    al::startAction(this, "WaitGround");
    mIsBossStage = true;
    mIsEnableAppearSign = false;
    al::hideSilhouetteModel(this);
}

/** @brief Breaks the Gorobon if it is currently active. */
void Gorobon::forceBreak() {
    if (al::isDead(this) || isNerveInactive()) {
        return;
    }

    if (al::isNerve(this, &NrvGorobonPlayerHold)) {
        rc::requestPlayerRelease(mHolderSensor);
    }

    al::setNerve(this, &NrvGorobonBreak);
}

/** @brief Makes the Gorobon appear already curled up as a rock. */
void Gorobon::appearRock() {
    al::LiveActor::appear();
    al::showModelIfHide(this);
    al::onCollide(this);
    al::setNerve(this, &NrvGorobonRockWait);
}

/** @brief Breaks into pieces, then waits to respawn or dies. */
void Gorobon::exeBreak() {
    al::setVelocityZero(this);
    al::startHitReactionBreak(this);
    al::setQuat(this, sead::Quatf::unit);
    al::startSe(this, "Break");
    al::appearBreakModelRandomRotateY(mBreakModel);
    if (!mIsBossStage && mIsRecover) {
        al::setNerve(this, &NrvGorobonDeath);
        return;
    }

    kill();
}

/** @brief Sinks into magma, then waits to respawn or dies. */
void Gorobon::exeSinkMagma() {
    if (al::isFirstStep(this)) {
        al::offCollide(this);
        al::invalidateClipping(this);
        al::setVelocity(this, sead::Vector3f(0.0f, -4.0f, 0.0f));
        al::hideSilhouetteModel(this);
        al::startAction(this, "Sink");
    }

    if (al::isActionEnd(this)) {
        al::tryEmitEffect(this, "Die", nullptr);
        al::startSe(this, "PgDie");
        if (mIsSinkByFire) {
            rc::addScore(this, mHolderSensor, 150.0f, 0);
        }

        if (!mIsBossStage && mIsRecover) {
            al::setNerve(this, &NrvGorobonDeath);
            return;
        }

        kill();
    }
}

/** @brief Hides the Gorobon at its initial position until the rebirth time has passed. */
void Gorobon::exeDeath() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "WaitGround");
        al::setVelocityZero(this);
        al::offCollide(this);
        al::hideModelIfShow(this);
        al::hideSilhouetteModel(this);
        al::invalidateHitSensors(this);
        al::invalidateClipping(this);
        al::resetPosition(this, mInitTrans, false);
        al::addTransOffsetLocalDir(this, -100.0f, 1);
        al::setQuat(this, mInitQuat);
        copyVector(&mFrontDir, sead::Vector3f::ez);
        mRotateX = 0.0f;
        mIsSinkByFire = false;
    }

    if (al::isGreaterEqualStep(this, mRebirthTime)) {
        al::validateHitSensors(this);
        if (!mIsBossStage) {
            al::validateClipping(this);
        }

        al::setNerve(this, &NrvGorobonWaitGround);
    }
}

/** @brief Waits in the ground until a player comes near. */
void Gorobon::exeWaitGround() {
    if (al::isFirstStep(this)) {
        al::hideSilhouetteModel(this);
        al::startAction(this, "WaitGround");
    }

    if (rc::calcActivePlayerNum(this) >= 1 &&
        rc::tryFindNearestActivePlayerActorInSphere(this, 750.0f) != nullptr) {
        if (mIsEnableAppearSign) {
            al::setNerve(this, &NrvGorobonAppearSign);
            return;
        }

        al::setNerve(this, &NrvGorobonAppear);
    }
}

/** @brief Waits in the ground until the start switch turns on. */
void Gorobon::exeWaitGroundWithStageSwitch() {
    if (al::isFirstStep(this)) {
        al::hideSilhouetteModel(this);
        al::startAction(this, "WaitGround");
    }

    if (al::isOnStageSwitch(this, "SwitchStart")) {
        if (mIsEnableAppearSign) {
            al::setNerve(this, &NrvGorobonAppearSign);
            return;
        }

        al::setNerve(this, &NrvGorobonAppear);
    }
}

/** @brief Shows the appearance sign for a while before jumping out. */
void Gorobon::exeAppearSign() {
    if (al::isFirstStep(this)) {
        al::tryEmitEffect(this, "AppearSign", nullptr);
        al::startHitReaction(this, "出現予兆");
        al::showModelIfHide(this);
    }

    al::holdSe(this, "PgAppearSignLv");
    if (al::isGreaterEqualStep(this, 60)) {
        al::setNerve(this, &NrvGorobonAppear);
    }
}

/** @brief Jumps out of the ground towards the nearest player. */
void Gorobon::exeAppear() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Appear");
        al::showModelIfHide(this);
        al::invalidateClipping(this);

        sead::Vector3f up;
        al::calcUpDir(&up, this);
        al::setVelocity(this, up * 39.0f);

        al::LiveActor* player = rc::findNearestActivePlayerActor(this);
        if (player != nullptr && !mIsBossDemo) {
            al::faceToTarget(this, player);
        }
    }

    al::addVelocityToGravity(this, 2.0f);
    if (al::getVelocity(this).y < 0.0f && al::isNoCollide(this)) {
        al::onCollide(this);
        al::showSilhouetteModel(this);
    }

    if (al::isOnGround(this, 0, 0.0f)) {
        startEffect();
        al::setVelocityZero(this);
        al::setNerve(this, &NrvGorobonLand);
    }
}

/** @brief Places the collision effect matrix on the touched surface and starts its effect. */
void Gorobon::startEffect() {
    if (rc::isCollidedDamageFire(this)) {
        return;
    }

    sead::Vector3f normal = {0.0f, 0.0f, 0.0f};
    sead::Vector3f pos = {0.0f, 0.0f, 0.0f};
    if (!BallStateFunction::getCollidedNormalAndPos(this, &normal, &pos)) {
        return;
    }

    makeMtxTrans(&mEffectMtx, pos);

    sead::Quatf quat;
    sead::Matrix34f rotateMtx;
    if (quat.makeVectorRotation(sead::Vector3f(0.0f, 1.0f, 0.0f), normal)) {
        rotateMtx.fromQuat(quat);
    } else {
        rotateMtx.makeIdentity();
    }
    mEffectMtx = mEffectMtx * rotateMtx;

    if (al::isCollidedGround(this)) {
        al::startHitReaction(this, "地面衝突");
        return;
    }

    if (al::getNerveStep(this) % 15 == 0) {
        al::startHitReaction(this, "コリジョンヒット");
    }
}

/** @brief Plays the landing animation, then walks (or waits during the boss demo). */
void Gorobon::exeLand() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Land");
    }

    BallStateFunction::sendMsgToCollision(this, true);
    if (al::isActionEnd(this)) {
        if (mIsBossDemo && mIsBossStage) {
            al::setNerve(this, &NrvGorobonDemoWait);
            return;
        }

        al::setNerve(this, &NrvGorobonWalk);
    }
}

/** @brief Waits until the boss demo is over. */
void Gorobon::exeDemoWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait");
    }

    if (mIsBossDemo) {
        return;
    }

    al::setNerve(this, &NrvGorobonWalk);
}

/** @brief Rolls towards the found target, breaking blocks it runs into. */
void Gorobon::exeWalk() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Walk");
        al::tryEmitEffect(this, "Walk", nullptr);
        if (!mIsBossStage) {
            al::validateClipping(this);
        }

        sead::Vector3f front = sead::Vector3f::ez;
        al::calcFrontDir(&front, this);
        copyVector(&mFrontDir, front);
        al::validateHitSensor(this, "BlockBreak");
    }

    mTargetFinder->update();
    if (mTargetFinder->isExistTarget()) {
        al::turnDirectionToTargetDegree(this, &mFrontDir, mTargetFinder->getTargetPos(), 1.5f);
    }

    f32 speed = al::calcNerveValue(this, 60, 0.0f, mIsBossStage ? 2.1f : 1.8f);
    al::addVelocityToDirection(this, mFrontDir, speed);
    if (al::isCollidedGround(this)) {
        al::addVelocityToGravityFittedGround(this, 3.0f, 0);
    } else {
        al::addVelocityToGravity(this, 9.8f);
    }

    al::scaleVelocity(this, 0.75f);

    al::HitSensor* collidedSensor = nullptr;
    if (al::isOnGround(this, 0, 0.0f)) {
        collidedSensor = al::tryGetCollidedGroundSensor(this);
    } else if (al::isCollidedWall(this)) {
        collidedSensor = al::tryGetCollidedWallSensor(this);
    } else if (al::isCollidedCeiling(this)) {
        collidedSensor = al::tryGetCollidedCeilingSensor(this);
    }

    if (collidedSensor != nullptr) {
        al::sendMsgBallAttackCollide(collidedSensor, al::getHitSensor(this, "BlockBreak"));
        if (al::isOnGround(this, 0, 0.0f)) {
            al::sendMsgBallTrampleCollide(collidedSensor, al::getHitSensor(this, "BlockBreak"));
        }
    }

    f32 rotateSpeed = speed + speed;
    mRotateX = rotateSpeed + mRotateX;
    sead::Quatf quat = al::getQuat(this);
    al::makeQuatFrontNoSupport(&quat, mFrontDir);
    al::setQuat(this, quat);
    al::holdSeWithParam(this, "PgRotate", rotateSpeed);
}

/** @brief Stops the walk effect and disables the block break sensor. */
void Gorobon::endWalk() {
    al::tryDeleteEffect(this, "Walk");
    al::invalidateHitSensor(this, "BlockBreak");
}

/** @brief Curls up into a rock. */
void Gorobon::exeRockStart() {
    if (al::isFirstStep(this)) {
        al::setVelocityZero(this);
        if (!mIsBossStage) {
            al::validateClipping(this);
        }

        al::startAction(this, "RockStart");
        mRotateX = 0.0f;
    }

    al::addVelocityToGravity(this, 3.0f);
    if (al::isOnGround(this, 0, 0.0f)) {
        al::setVelocityZero(this);
        BallStateFunction::sendMsgToCollision(this, true);
    }

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvGorobonRockWait);
    }
}

/** @brief Curls up into a rock again after being hit while curled up. */
void Gorobon::exeRockStartAgain() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "RockStartAgain");
    }

    al::addVelocityToGravity(this, 3.0f);
    if (al::isOnGround(this, 0, 0.0f)) {
        al::setVelocityZero(this);
        BallStateFunction::sendMsgToCollision(this, true);
    }

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvGorobonRockWait);
    }
}

/** @brief Rolls passively as a rock for a while. */
void Gorobon::exeRockWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "RockWait");
        copyVector(&mPrevTrans, al::getTrans(this));
    }

    al::addVelocityToGravity(this, 9.8f);
    al::scaleVelocity(this, 0.99f);
    if (al::isCollidedGround(this)) {
        BallStateFunction::sendMsgToCollision(this, true);
        al::scaleVelocity(this, 0.3f);
        sead::Vector3f groundNormal = al::getOnGroundNormal(this, 0);
        if (!al::isNearDirection(groundNormal, sead::Vector3f::ey, 0.01f)) {
            mRotateX = BallStateFunction::calcRotateSpeed(this, mPrevTrans) + mRotateX;
        }
    }

    copyVector(&mPrevTrans, al::getTrans(this));
    if (al::isGreaterEqualStep(this, 300)) {
        al::setNerve(this, &NrvGorobonRockEnd);
    }
}

/** @brief Starts uncurling from the rock form. */
void Gorobon::exeRockEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "RockEnd");
    }

    al::addVelocityToGravity(this, 3.0f);
    if (al::isOnGround(this, 0, 0.0f)) {
        al::setVelocityZero(this);
        BallStateFunction::sendMsgToCollision(this, true);
    }

    if (al::isGreaterEqualStep(this, 120)) {
        al::setNerve(this, &NrvGorobonRecovery);
    }
}

/** @brief Jumps back up out of the rock form, facing the nearest player. */
void Gorobon::exeRecovery() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Recovery");
        al::invalidateClipping(this);
        al::setVelocityZero(this);
        al::setQuat(this, sead::Quatf::unit);
        copyVector(&mFrontDir, sead::Vector3f::ez);
        mRotateX = 0.0f;

        al::LiveActor* player = rc::tryFindNearestActivePlayerActorInSphere(this, 750.0f);
        if (player != nullptr) {
            al::faceToTarget(this, player);
        }

        sead::Vector3f up;
        al::calcUpDir(&up, this);
        al::setVelocity(this, up * 39.0f);
    }

    al::addVelocityToGravity(this, 3.0f);
    if (al::isCollidedGround(this) && al::getVelocity(this).y <= 0.0f) {
        startEffect();
        al::setVelocityZero(this);
        al::setNerve(this, &NrvGorobonLandRecovery);
    }
}

/** @brief Lands after recovering, then walks again. */
void Gorobon::exeLandRecovery() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "LandRecovery");
    }

    BallStateFunction::sendMsgToCollision(this, true);
    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvGorobonWalk);
    }
}

/** @brief Gets carried by the player and recovers by itself after a while. */
void Gorobon::exePlayerHold() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "RockWait");
        al::startSe(this, "HoldItem");
        mComboCounter->reset();
        al::tryEmitEffect(this, "Sweat", nullptr);
        al::startSe(this, "PgLiftUp");
        mHoldTime = 0;
    }

    al::holdSe(this, "PgSweatLv");
    al::updateNerveState(this);

    sead::Quatf quat = al::getQuat(this);
    sead::Vector3f front = {0.0f, 0.0f, 0.0f};
    al::calcFrontDir(&front, al::getSensorHost(mHolderSensor));
    al::makeQuatFrontNoSupport(&quat, front);
    al::setQuat(this, quat);

    if (mHoldTime == 300) {
        al::startAction(this, "RockEnd");
    }

    if (mHoldTime >= 420) {
        al::setNerve(this, &NrvGorobonRecovery);
        al::onCollide(this);
        rc::requestPlayerRelease(mHolderSensor);
        return;
    }

    mHoldTime++;
}

/** @brief Stops the sweat effect when the Gorobon is no longer carried. */
void Gorobon::endHold() {
    al::tryDeleteEffect(this, "Sweat");
}

/** @brief Flies away after being thrown or released by the player. */
void Gorobon::exeRockThrow() {
    if (al::isFirstStep(this)) {
        al::onCollide(this);
        al::setVelocity(this, mThrowVelocity);
        al::startSe(this, "Thrown");
        mReflectCoolTime = 12;
        mAttackCoolTime = 12;
        mIsRotateForward = false;

        sead::Vector3f dir = mThrowVelocity;
        dir.y = 0.0f;
        al::normalizeOrZero(&dir);
        if (!al::isNearZero(dir, 0.001f)) {
            sead::Quatf quat = al::getQuat(this);
            al::makeQuatFrontNoSupport(&quat, dir);
            al::setQuat(this, quat);
        }

        sead::Vector3f trans = al::getTrans(this);
        sead::Vector3f hitPos = {0.0f, 0.0f, 0.0f};
        const sead::Vector3f holderOffset(0.0f, 350.0f, 0.0f);
        if (alCollisionUtil::getFirstPolyOnArrow(this, &hitPos, nullptr, al::getTrans(this),
                                                 al::getColliderRadius(this) * dir, nullptr,
                                                 nullptr)) {
            sead::Vector3f holderPos =
                al::getTrans(al::getSensorHost(mHolderSensor)) + holderOffset;
            al::resetPosition(this, holderPos + (al::getTrans(this) - hitPos), false);
        } else {
            al::resetPosition(this, al::getTrans(al::getSensorHost(mHolderSensor)) + holderOffset,
                              false);
        }

        al::setTrans(this, trans);
    }

    al::addVelocityToGravity(this, 3.0f);
    mRotateX += mIsRotateForward ? 4.0f : -4.0f;
    if (al::isNerve(this, &NrvGorobonRockFall)) {
        if (!al::isGreaterEqualStep(this, 90) && !al::isCollided(this)) {
            return;
        }

        if (rc::isCollidedDamageFire(this)) {
            al::setNerve(this, &NrvGorobonSinkMagma);
            return;
        }

        al::setNerve(this, &NrvGorobonBreak);
        return;
    }

    if (al::isCollidedGround(this)) {
        al::HitSensor* groundSensor = al::tryGetCollidedGroundSensor(this);
        if (groundSensor != nullptr) {
            bool isAttack =
                al::sendMsgBallAttackCollide(groundSensor, al::getHitSensor(this, "BallAttack"));
            startEffect();
            if (isAttack) {
                return;
            }

            al::getVelocityPtr(this)->x = 0.0f;
            al::getVelocityPtr(this)->z = 0.0f;
            mComboCounter->reset();
            copyVector(&mSideDir, sead::Vector3f::zero);
            al::setNerve(this, &NrvGorobonRockWait);
            return;
        }
    }

    if (al::isCollidedWall(this)) {
        al::HitSensor* wallSensor = al::tryGetCollidedWallSensor(this);
        if (wallSensor != nullptr) {
            al::sendMsgBallAttackCollide(wallSensor, al::getHitSensor(this, "BallAttack"));
        }
    }

    if (al::isCollidedCeiling(this)) {
        al::HitSensor* ceilingSensor = al::tryGetCollidedCeilingSensor(this);
        if (ceilingSensor != nullptr) {
            al::sendMsgBallAttackCollide(ceilingSensor, al::getHitSensor(this, "BallAttack"));
        }
    }

    startEffect();
}

/** @brief Gets shot away spinning by the boss Gorobon. */
void Gorobon::exeRockSpinShot() {
    if (al::isFirstStep(this)) {
        al::invalidateClipping(this);
        al::setVelocity(this, sead::Vector3f(mFrontDir.x * 40.0f, 45.0f, mFrontDir.z * 40.0f));
        al::startAction(this, "RockWait");
        al::setQuat(this, sead::Quatf::unit);
    }

    al::scaleVelocity(this, 0.99f);
    al::addVelocityToGravity(this, 3.0f);
    sead::Quatf* quat = al::getQuatPtr(this);
    al::rotateQuatYDirDegree(quat, *quat, -18.0f);
    if ((al::isCollidedGround(this) && al::isGreaterEqualStep(this, 10)) ||
        al::isCollidedWall(this) || al::isCollidedCeiling(this) ||
        al::isGreaterEqualStep(this, 240)) {
        al::setNerve(this, &NrvGorobonBreak);
    }
}

/** @brief Stays frozen by the support touch until the freeze state ends. */
void Gorobon::exeSupportFreeze() {
    BallStateFunction::sendMsgToCollision(this, true);
    if (al::updateNerveState(this)) {
        al::setNerve(this, &NrvGorobonWalk);
    }
}
