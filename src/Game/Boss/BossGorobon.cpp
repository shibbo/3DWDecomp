#include "Boss/BossGorobon.hpp"

#include <attributes.h>
#include <cmath>
#include <math/seadQuat.h>

#include "Boss/BossGorobonRock.hpp"
#include "Boss/BossStateDemoStart.hpp"
#include "Enemy/EnemyStateUtil.hpp"
#include "Enemy/Gorobon.hpp"
#include "Enemy/MeraWanwan.hpp"
#include "Enemy/TargetFinder.hpp"
#include "Enemy/WalkerStateParam.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/Joint/JointAimInfo.hpp"
#include "Library/Joint/JointControllerKeeper.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/LiveActor/Common/LiveActorGroup.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Obj/BreakModel.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/JointAimUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ScoreUtil.hpp"

namespace {
NERVE_DECL(BossGorobon, JumpStart);
NERVE_DECL(BossGorobon, DemoBattleStart);
NERVE_DECL(BossGorobon, PrepareBattleStart);
NERVE_DECL(BossGorobon, Down);
NERVE_DECL(BossGorobon, SpinStart);
NERVE_DECL(BossGorobon, Spin);
NERVE_DECL(BossGorobon, Walk);
NERVE_DECL(BossGorobon, WalkSlowDown);
NERVE_DECL(BossGorobon, Damage);
NERVE_DECL(BossGorobon, Jump);
NERVE_DECL(BossGorobon, Land);
NERVE_DECL(BossGorobon, Wait);
NERVE_DECL(BossGorobon, SpinEnd);

/** @brief Slows the walk down before spinning (shares exeWalkSlowDown with WalkSlowDown). */
class BossGorobonNrvWalkSlowDownSpin : public al::Nerve {
public:
    void execute(al::NerveKeeper* pKeeper) const override {
        pKeeper->getParent<BossGorobon>()->exeWalkSlowDown();
    }
};

NERVES_MAKE_NOSTRUCT(BossGorobon, JumpStart, DemoBattleStart, PrepareBattleStart, Down, SpinStart,
                     Spin, Walk, WalkSlowDown, WalkSlowDownSpin, Damage, Jump, Land, Wait, SpinEnd)

/** @brief Target search and movement parameters shared by every BossGorobon. */
struct BossGorobonParam {
    BossGorobonParam() {
        mTargetFinderParam._0 = 6000.0f;
        mTargetFinderParam._4 = 180.0f;
        mTargetFinderParam._8 = 90.0f;
        mWalkerStateParam.mGravity = 3.0f;
        mWalkerStateParam.mAirFriction = 0.99f;
        mWalkerStateParam.mGroundFriction = 0.5f;
    }

    TargetFinderParam mTargetFinderParam;
    WalkerStateParam mWalkerStateParam;
};

BossGorobonParam sParam;

constexpr s32 cRockNum = 15;
constexpr s32 cMeraWanwanNum = 2;
constexpr f32 cGorobonFallOffsetY = -1200.0f;

/**
 * @brief Returns the smaller of two angles.
 * @param a First angle in degrees.
 * @param b Second angle in degrees.
 * @return The smaller angle.
 */
f32 minDegree(f32 a, f32 b) {
    return b < a ? b : a;
}

/**
 * @brief Returns the larger of two angles.
 * @param a First angle in degrees.
 * @param b Second angle in degrees.
 * @return The larger angle.
 */
f32 maxDegree(f32 a, f32 b) {
    return a < b ? b : a;
}
}  // namespace

/**
 * @brief Creates the boss with full health, facing +Z.
 * @param pName Actor name.
 */
BossGorobon::BossGorobon(const char* pName) : al::LiveActor(pName) {}

/**
 * @brief Reads placement, creates the Gorobon/rock/MeraWanwan pools, demo state, eye aim and
 * break model.
 * @param rInfo Actor placement and scene initialization information.
 */
void BossGorobon::init(const al::ActorInitInfo& rInfo) {
    al::initActor(this, rInfo);
    al::invalidateClipping(this);
    mTargetFinder = new TargetFinder(this, &sParam.mTargetFinderParam);
    mTargetFinder->setFrontDir(&mFrontDir);
    al::tryGetLinksTrans(&mStageCenterPos, rInfo, "StageCenterPos");
    al::tryGetArg(&mStageRadius, rInfo, "StageRadius");
    al::tryGetArg(&mOffsetRadiusGorobon, rInfo, "OffsetRadiusGorobon");
    al::tryGetArg(&mIsGateKeeper, rInfo, "IsGateKeeper");

    if (mIsGateKeeper) {
        al::initNerve(this, &NrvBossGorobonJumpStart, 0);
    } else {
        al::initNerve(this, &NrvBossGorobonDemoBattleStart, 1);
    }

    mGorobonNum = al::calcLinkChildNum(rInfo, "GorobonAppear");
    mGorobonGroup = new al::DeriveActorGroup<Gorobon>("ゴロボンリスト", mGorobonNum);
    for (s32 i = 0; i < mGorobonNum; i++) {
        auto* gorobon = new Gorobon("ゴロボン");
        gorobon->mIsBossStage = true;
        al::initLinksActor(gorobon, rInfo, "GorobonAppear", i);
        gorobon->initAtBossStage();
        mGorobonGroup->registerActor(gorobon);
    }

    mGorobonGroup->makeActorDeadAll();

    mRockGroup = new al::DeriveActorGroup<BossGorobonRock>("ボスゴロボン岩リスト", cRockNum);
    for (s32 i = 0; i < cRockNum; i++) {
        auto* rock = new BossGorobonRock("ボスゴロボン岩");
        al::initCreateActorWithPlacementInfo(rock, rInfo);
        mRockGroup->registerActor(rock);
    }

    if (mIsGateKeeper) {
        mMeraWanwanNum = al::calcLinkChildNum(rInfo, "MeraWanwanAppear");
        mMeraWanwanGroup =
            new al::DeriveActorGroup<MeraWanwan>("メラワンワンリスト", cMeraWanwanNum);
        for (s32 i = 0; i < cMeraWanwanNum; i++) {
            auto* meraWanwan = new MeraWanwan("メラワンワン");
            al::initLinksActor(meraWanwan, rInfo, "MeraWanwanAppear", i);
            meraWanwan->setFindDistance(2400.0f);
            meraWanwan->_1C9 = false;
            meraWanwan->kill();
            mMeraWanwanGroup->registerActor(meraWanwan);
        }
    }

    if (!mIsGateKeeper) {
        mDemoStartInfo = new BossDemoStartInfo(al::initAnimCamera(this, rInfo), this,
                                               "DemoBattleStart", 60, nullptr);
        mDemoStartInfo->_18 = 10;
        mStateDemoStart = new BossStateDemoStart(this, rInfo, mDemoStartInfo);
        al::initNerveState(this, mStateDemoStart, &NrvBossGorobonDemoBattleStart, "開始デモ");
    }

    al::createAndSetColliderSpecialPurpose(this, "MoveLimit");
    al::trySyncStageSwitchAppear(this);
    al::offStageSwitch(this, "SwitchBossGorobonSpinStartOn");
    al::onStageSwitch(this, "SwitchBossGorobonSpinEndOn");
    al::setEffectFollowMtxPtr(this, "Walk", &mWalkEffectMtx);
    al::initJointControllerKeeper(this, 2);
    mEyeAimInfo = new al::JointAimInfo;
    mEyeAimInfo->setBaseAimLocalDir(sead::Vector3f::ex);
    mEyeAimInfo->setBaseSideLocalDir(sead::Vector3f::ez);
    mEyeAimInfo->setBaseUpLocalDir(sead::Vector3f::ey);
    mEyeAimInfo->setEnableBackAim(true);
    mEyeAimInfo->setLimitDegreeRect(32.0f, 9.0f, 0.0f, 0.0f);
    al::initJointAimController(this, mEyeAimInfo, "EyeL");
    al::initJointAimController(this, mEyeAimInfo, "EyeR");
    mBreakModel = new al::BreakModel(this, "ボスゴロボン壊れモデル", "BossGorobonBreak", nullptr,
                                     nullptr, "Break", true);
    al::initCreateActorNoPlacementInfo(mBreakModel, rInfo);

    if (mIsGateKeeper) {
        *al::getTransPtr(this) += sead::Vector3f(0.0f, 330.0f, 0.0f);
    }

    setColorAnim(this, "BossGorobonBodyColor");
    setVisibilityAnim();
}

/**
 * @brief Starts a body color animation, frozen on the variant's color frame.
 * @param pActor Actor whose color animation is set (the boss or its break model).
 * @param pAnimName Color animation name.
 */
void BossGorobon::setColorAnim(al::LiveActor* pActor, const char* pAnimName) {
    al::startMclAnim(pActor, pAnimName);
    al::setMclAnimFrameRate(pActor, 0.0f);
    if (mIsGateKeeper) {
        al::setMclAnimFrame(pActor, 1.0f);
    } else {
        al::setMclAnimFrame(pActor, 0.0f);
    }
}

/** @brief Shows the damage parts matching the remaining hit points. */
void BossGorobon::setVisibilityAnim() {
    switch (mHitPoint) {
    case 0:
        al::startVisAnim(this, "Damage3");
        break;
    case 1:
        al::startVisAnim(this, "Damage2");
        break;
    case 2:
        al::startVisAnim(this, "Damage1");
        break;
    case 3:
        al::startVisAnim(this, "Damage0");
        break;
    }
}

/**
 * @brief Attacks players, shoots enemies while spinning and rams them while walking.
 * @param pSelf Boss sensor.
 * @param pOther Touched sensor.
 */
void BossGorobon::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isNerve(this, &NrvBossGorobonDemoBattleStart) ||
        al::isNerve(this, &NrvBossGorobonPrepareBattleStart)) {
        return;
    }

    if (al::isSensorEnemyAttack(pSelf) && al::isSensorPlayer(pOther)) {
        if (!al::isNerve(this, &NrvBossGorobonDown)) {
            al::sendMsgEnemyAttack(pOther, pSelf);
        }

        al::sendMsgPush(pOther, pSelf);
    }

    if (al::isNerve(this, &NrvBossGorobonSpinStart) || al::isNerve(this, &NrvBossGorobonSpin)) {
        if (al::isSensorEnemyAttack(pSelf) && al::isSensorEnemyBody(pOther)) {
            rc::sendMsgBossGorobonSpinShot(pOther, pSelf);
        }

        return;
    }

    if (al::isNerve(this, &NrvBossGorobonWalk) || al::isNerve(this, &NrvBossGorobonWalkSlowDown) ||
        al::isNerve(this, &NrvBossGorobonWalkSlowDownSpin)) {
        if (al::isSensorEnemyAttack(pSelf) && al::isSensorEnemyBody(pOther)) {
            rc::sendMsgBossGorobonAttack(pOther, pSelf);
        }

        return;
    }

    if (al::isSensorEnemyAttack(pSelf) && al::isSensorEnemyBody(pOther)) {
        al::sendMsgPush(pOther, pSelf);
    }
}

/**
 * @brief Bounces player attacks and takes damage from Gorobon hits.
 * @param pMsg Received message.
 * @param pOther Sender sensor.
 * @param pSelf Boss sensor.
 * @return Whether the message was handled.
 */
bool BossGorobon::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                             al::HitSensor* pSelf) {
    if (al::isMsgPlayerBoomerangReflect(pMsg) || EnemyStateUtil::isMsgBlowDown(pMsg)) {
        if (mInvincibleTimer == 0 && !al::isMsgPlayerInvincibleAttack(pMsg)) {
            rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
            mInvincibleTimer = 30;
        }

        return true;
    }

    if (isInvincibleNerve() || !rc::isMsgGorobonAttack(pMsg)) {
        return false;
    }

    mHitPoint--;
    if (mHitPoint == 0) {
        sead::Vector3f cameraDir = {0.0f, 0.0f, 0.0f};
        al::calcCameraDir(&cameraDir, this);
        cameraDir.y = 0.0f;
        al::normalize(&cameraDir);
        mFrontDir = cameraDir;
    }

    if (mHitPoint == 0) {
        al::startHitReaction(this, "ダメージ[ラスト]");
    } else {
        al::startHitReaction(this, "ダメージ");
    }

    sead::Quatf quat = al::getQuat(this);
    al::makeQuatFrontNoSupport(&quat, mFrontDir);
    al::setQuat(this, quat);
    al::setVelocityZero(this);
    mRollDegree = 0.0f;
    mTiltDegree = 0.0f;
    mBreakModel->appear();
    setColorAnim(mBreakModel, "BossGorobonBreakBodyColor");

    if (rc::tryFindRelativeControlUserId(pOther) >= 0) {
        rc::addScore(this, pOther, 0.0f, 2 - mHitPoint);
    }

    if (mHitPoint != 0) {
        if (mHitPoint == 1) {
            mWalkSpeed = 8.4f;
        }

        al::setNerve(this, &NrvBossGorobonDamage);
    } else {
        al::setNerve(this, &NrvBossGorobonDown);
    }

    return true;
}

/**
 * @brief Checks whether the current nerve ignores Gorobon hits.
 * @return Whether the boss cannot be damaged right now.
 */
bool BossGorobon::isInvincibleNerve() {
    if (al::isNerve(this, &NrvBossGorobonSpinStart)) {
        return !mIsDamageableSpinStart;
    }

    return al::isNerve(this, &NrvBossGorobonDemoBattleStart) ||
           al::isNerve(this, &NrvBossGorobonPrepareBattleStart) ||
           al::isNerve(this, &NrvBossGorobonSpin) || al::isNerve(this, &NrvBossGorobonDamage) ||
           al::isNerve(this, &NrvBossGorobonDown);
}

/**
 * @brief Accepts touch assist from the pointer.
 * @param pMsg Received message.
 * @param pPointer Screen pointer.
 * @param pTarget Pointed target.
 * @return Whether the message is a touch assist.
 */
bool BossGorobon::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                        al::ScreenPointTarget* pTarget) {
    return al::isMsgTouchAssist(pMsg);
}

/** @brief Appears jumping in as a gatekeeper, or hidden for the battle start demo. */
void BossGorobon::appear() {
    if (mIsGateKeeper) {
        al::setAdditionalCameraLookAtPosPtr(this, this, al::getTransPtr(this));
        al::setNerve(this, &NrvBossGorobonJumpStart);
    } else {
        al::setNerve(this, &NrvBossGorobonDemoBattleStart);
        al::hideModelIfShow(this);
    }

    al::LiveActor::appear();
}

/** @brief Counts down invincibility and drives the eyes toward the player. */
void BossGorobon::control() {
    if (mInvincibleTimer > 0) {
        mInvincibleTimer--;
    }

    if (al::isNerve(this, &NrvBossGorobonDemoBattleStart) ||
        al::isNerve(this, &NrvBossGorobonDamage) || al::isNerve(this, &NrvBossGorobonDown) ||
        al::isNerve(this, &NrvBossGorobonSpinStart) || al::isNerve(this, &NrvBossGorobonSpin) ||
        al::isNerve(this, &NrvBossGorobonSpinEnd)) {
        mEyeAimInfo->subPowerRate(0.1f);
    } else {
        JointAimUtil::updateEyeJointInfo(this, mEyeAimInfo, 1500.0f, 0.2f);
    }
}

/** @brief Plays the battle start demo, landing and releasing the Gorobons. */
void BossGorobon::exeDemoBattleStart() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "DemoBattleStart");
        al::offCollide(this);
        al::showModelIfHide(this);
        *al::getTransPtr(this) += sead::Vector3f(0.0f, 330.0f, 0.0f);
    }

    if (al::isStep(this, 30)) {
        mGorobonGroup->appearAll();
        for (s32 i = 0; i < mGorobonGroup->mNumActors; i++) {
            mGorobonGroup->getDeriveActor(i)->mIsBossDemo = true;
        }

        al::startHitReaction(this, "地面衝突");
    }

    if (al::isStep(this, 296) && !mIsGateKeeper) {
        al::startBgm(this, "Boss", -1, 0, -1, -1);
    }

    if (al::updateNerveState(this)) {
        if (al::isLessStep(this, 296)) {
            al::startBgm(this, "Boss", -1, 0, -1, -1);
        }

        if (mStateDemoStart->_44) {
            al::clearSklAnimInterpole(this);
            al::tryKillEmitterAndParticleAll(this);
        }

        al::setNerve(this, &NrvBossGorobonPrepareBattleStart);
    }
}

/** @brief Restores collision, releases the Gorobons and starts walking. */
void BossGorobon::exePrepareBattleStart() {
    al::startAction(this, "Walk");
    al::onCollide(this);
    al::resetPosition(this, false);
    for (s32 i = 0; i < mGorobonGroup->mNumActors; i++) {
        mGorobonGroup->getDeriveActor(i)->mIsBossDemo = false;
    }

    al::setAdditionalCameraLookAtPosPtr(this, this, al::getTransPtr(this));
    al::setNerve(this, &NrvBossGorobonWalk);
}

/** @brief Rolls toward the player, then slows down to jump or spin. */
void BossGorobon::exeWalk() {
    if (al::isFirstStep(this)) {
        al::tryStartActionIfNotPlaying(this, "Walk");
        al::tryEmitEffect(this, "Walk", nullptr);
    }

    mWalkEffectMtx.makeT(al::getTrans(this));
    sead::Quatf quat;
    sead::Matrix34f rotateMtx;
    if (quat.makeVectorRotation(sead::Vector3f(0.0f, 0.0f, 1.0f), mFrontDir)) {
        rotateMtx.makeQT(quat, sead::Vector3f(0.0f, 0.0f, 0.0f));
    } else {
        rotateMtx.makeIdentity();
    }

    mWalkEffectMtx = mWalkEffectMtx * rotateMtx;

    chasePlayer();
    checkGorobonPosition();

    if (al::isLessEqualStep(this, 90)) {
        return;
    }

    if (mGorobonGroup->calcAliveActorNum() != 0) {
        if (al::isGreaterEqualStep(this, 180) && mHitPoint == 1) {
            al::tryDeleteEffect(this, "Walk");
            al::setNerve(this, &NrvBossGorobonWalkSlowDownSpin);
        }

        return;
    }

    if (mRollDegree < mWalkSpeed * 0.5f) {
        al::tryDeleteEffect(this, "Walk");
        al::setNerve(this, &NrvBossGorobonWalkSlowDown);
        mRollDegree = mWalkSpeed * 0.5f;
    }
}

/** @brief Turns toward the target, moves, and updates the rolling/tilting pose. */
void BossGorobon::chasePlayer() {
    sead::Vector3f prevFrontDir = mFrontDir;
    mTargetFinder->update();
    if (mTargetFinder->isExistTarget()) {
        al::turnDirectionToTargetDegree(this, &mFrontDir, mTargetFinder->getTargetPos(),
                                        mHitPoint > 1 ? 0.5f : 1.0f);
    }

    al::getGravity(this);
    f32 speed;
    if (al::isNerve(this, &NrvBossGorobonWalk)) {
        speed = al::calcNerveValue(this, 90, 0.0f, mWalkSpeed);
    } else {
        speed = al::calcNerveValue(this, 240, mWalkSpeed, 2.0f);
    }

    f32 rollSpeed = speed * 0.5f;
    mRollDegree += rollSpeed;
    if (al::isCollidedGround(this)) {
        al::getOnGroundNormal(this, 0);
        al::addVelocityToDirection(this, mFrontDir, speed);
        al::addVelocityToGravityFittedGround(this, 3.0f, 0);
    } else {
        al::addVelocityToGravity(this, 3.0f);
    }

    al::scaleVelocity(this, 0.5f);
    f32 turnDegree = al::calcAngleOnPlaneDegree(mFrontDir, prevFrontDir, sead::Vector3f::ey);
    if (turnDegree >= 0.75f && !al::isNerve(this, &NrvBossGorobonWalkSlowDownSpin) &&
        !al::isNerve(this, &NrvBossGorobonWalkSlowDown)) {
        mTiltDegree = minDegree(mTiltDegree + 0.1f, 30.0f);
    } else if (al::isNerve(this, &NrvBossGorobonWalkSlowDownSpin) ||
               al::isNerve(this, &NrvBossGorobonWalkSlowDown)) {
        if (mTiltDegree > 0.0f) {
            mTiltDegree = maxDegree(mTiltDegree - 1.0f, 0.0f);
        } else {
            mTiltDegree = minDegree(mTiltDegree + 1.0f, 0.0f);
        }
    } else if ((turnDegree < 0.75f && turnDegree > 0.0f) ||
               (turnDegree >= -0.75f && turnDegree < 0.0f)) {
        if (mTiltDegree > 0.0f) {
            mTiltDegree = maxDegree(mTiltDegree - 0.1f, 0.0f);
        } else {
            mTiltDegree = minDegree(mTiltDegree + 0.1f, 0.0f);
        }
    } else {
        mTiltDegree = maxDegree(mTiltDegree - 0.1f, -30.0f);
    }

    sead::Quatf quat = al::getQuat(this);
    mRollDegree = mRollDegree >= 360.0f ? mRollDegree - 360.0f : mRollDegree;
    al::makeQuatFrontNoSupport(&quat, mFrontDir);
    al::rotateQuatLocalDirDegree(&quat, quat, 0, mRollDegree);
    al::rotateQuatLocalDirDegree(&quat, quat, 2, mTiltDegree);
    al::setQuat(this, quat);
    al::holdSeWithParam(this, "PgRotate", rollSpeed, nullptr);
}

/** @brief Breaks every Gorobon that fell far below the stage. */
void BossGorobon::checkGorobonPosition() {
    for (s32 i = 0; i < mGorobonNum; i++) {
        Gorobon* gorobon = mGorobonGroup->getDeriveActor(i);
        if (al::isAlive(gorobon) &&
            al::getTrans(gorobon).y < mStageCenterPos.y + cGorobonFallOffsetY) {
            gorobon->forceBreak();
        }
    }
}

/** @brief Keeps chasing while rolling slows down, then jumps or starts spinning. */
void BossGorobon::exeWalkSlowDown() {
    chasePlayer();
    if (mRollDegree <= mWalkSpeed * 0.5f) {
        sead::Quatf quat = al::getQuat(this);
        al::makeQuatFrontNoSupport(&quat, mFrontDir);
        al::setQuat(this, quat);
        if (al::isNerve(this, &NrvBossGorobonWalkSlowDown)) {
            al::setNerve(this, &NrvBossGorobonJumpStart);
        } else if (al::isNerve(this, &NrvBossGorobonWalkSlowDownSpin)) {
            mIsDamageableSpinStart = true;
            al::setNerve(this, &NrvBossGorobonSpinStart);
        }
    }
}

/** @brief Crouches before jumping. */
void BossGorobon::exeJumpStart() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "JumpStart");
        al::setVelocityZero(this);
    }

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvBossGorobonJump);
    }
}

/** @brief Jumps upward and falls until landing. */
void BossGorobon::exeJump() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Jump");
        sead::Vector3f upDir;
        al::calcUpDir(&upDir, this);
        al::setVelocityToDirection(this, upDir, 55.0f);
    }

    al::addVelocityToGravity(this, 3.0f);
    if (al::isCollidedGround(this) && al::getVelocity(this).y <= 0.0f) {
        al::setVelocityZero(this);
        al::setNerve(this, &NrvBossGorobonLand);
    }
}

/** @brief Lands, reviving Gorobons and calling MeraWanwans. */
void BossGorobon::exeLand() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Land");
        al::startHitReaction(this, "地面衝突");
        revivalGorobon();
        tryAppearMeraWanwan();
    }

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvBossGorobonWait);
    }
}

/** @brief Respawns every dead Gorobon at a free spot of the stage. */
void BossGorobon::revivalGorobon() {
    for (s32 i = 0; i < mGorobonNum; i++) {
        Gorobon* gorobon = mGorobonGroup->getDeriveActor(i);
        if (al::isAlive(gorobon)) {
            continue;
        }

        f32 gorobonRadius = al::getSensorRadius(gorobon, "Body") + mOffsetRadiusGorobon;
        f32 bossRadius = al::getSensorRadius(this, "Body");
        s32 userNum = rc::getControlUserNumMax();
        f32 bossDistance = gorobonRadius + bossRadius;
        f32 gorobonDistance = gorobonRadius + gorobonRadius;
        f32 playerDistance = gorobonRadius + 100.0f;
        sead::Vector3f pos = mStageCenterPos;
        bool isFound = false;
        for (s32 j = 0; j < 400; j++) {
            u16 index = j;
            f32 radius = mStageRadius / 20.0f * (index / 20);
            f32 angle = sead::Mathf::deg2rad((index % 20) * 18.0f);
            f32 offsetX = std::cos(angle) * radius;
            f32 offsetZ = radius * std::sin(angle);
            pos.x = mStageCenterPos.x + offsetX;
            pos.y = mStageCenterPos.y + 0.0f;
            pos.z = offsetZ + mStageCenterPos.z;
            if (al::calcDistanceH(this, pos) < bossDistance) {
                continue;
            }

            bool isNearPlayer = false;
            for (s32 k = 0; k < userNum; k++) {
                al::LiveActor* player = rc::findPlayerActorFirstByUserId(this, k);
                if (player != nullptr && !rc::isPlayerDeadOrBubble(player) &&
                    al::calcDistanceH(player, pos) < playerDistance) {
                    isNearPlayer = true;
                    break;
                }
            }

            if (isNearPlayer) {
                continue;
            }

            bool isNearGorobon = false;
            for (s32 k = 0; k < mGorobonNum; k++) {
                Gorobon* other = mGorobonGroup->getDeriveActor(k);
                if (al::isAlive(other) && i != k &&
                    al::calcDistanceH(other, pos) < gorobonDistance) {
                    isNearGorobon = true;
                    break;
                }
            }

            if (isNearGorobon) {
                continue;
            }

            isFound = true;
            break;
        }

        if (!isFound) {
            continue;
        }

        al::resetPosition(gorobon, pos, false);
        gorobon->initAtBossStage();
        gorobon->appear();
    }
}

/** @brief Calls both MeraWanwans back while the gatekeeper is damaged (inlined into exeLand). */
ALWAYS_INLINE void BossGorobon::tryAppearMeraWanwan() {
    if (!mIsGateKeeper || mHitPoint <= 0) {
        return;
    }

    switch (mHitPoint) {
    case 1:
    case 2:
    case 3:
        for (s32 i = 0; i < cMeraWanwanNum; i++) {
            MeraWanwan* meraWanwan = mMeraWanwanGroup->getDeriveActor(i);
            if (al::isDead(meraWanwan)) {
                al::invalidateClipping(meraWanwan);
                meraWanwan->appearByBossGorobon();
            }
        }

        break;
    }
}

/** @brief Waits a moment after landing before walking again. */
void BossGorobon::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait");
        al::setVelocityZero(this);
    }

    if (al::isGreaterEqualStep(this, 30)) {
        al::setNerve(this, &NrvBossGorobonWalk);
    }
}

/** @brief Starts spinning and flips the spin stage switches. */
void BossGorobon::exeSpinStart() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "SpinStart");
        al::setVelocityZero(this);
        al::onStageSwitch(this, "SwitchBossGorobonSpinStartOn");
        al::offStageSwitch(this, "SwitchBossGorobonSpinEndOn");
    }

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvBossGorobonSpin);
    }
}

/** @brief Spins toward the target while throwing rocks. */
void BossGorobon::exeSpin() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Spin");
    }

    createBossGorobonRock();
    mTargetFinder->update();
    if (mTargetFinder->isExistTarget()) {
        al::turnDirectionToTargetDegree(this, &mFrontDir, mTargetFinder->getTargetPos(), 1.5f);
    }

    f32 speed = al::calcNerveValue(this, 60, 0.0f, mWalkSpeed * 0.3f);
    if (al::isCollidedGround(this)) {
        al::addVelocityToDirection(this, mFrontDir, speed);
        al::addVelocityToGravityFittedGround(this, 3.0f, 0);
    } else {
        al::addVelocityToGravity(this, 3.0f);
    }

    al::scaleVelocity(this, 0.5f);
    if (al::isGreaterEqualStep(this, 420)) {
        al::setNerve(this, &NrvBossGorobonSpinEnd);
    }
}

/** @brief Throws a rock every 20 steps, aimed at a player or at a random spot. */
void BossGorobon::createBossGorobonRock() {
    if (al::isLessEqualStep(this, 30) || al::isGreaterEqualStep(this, 390)) {
        return;
    }

    if (al::getNerveStep(this) % 20 != 0) {
        return;
    }

    BossGorobonRock* rock = mRockGroup->tryFindDeadDeriveActor();
    if (rock == nullptr) {
        return;
    }

    s32 playerNum = rc::calcActivePlayerNum(this);
    bool isNoPlayer = playerNum < 1;
    bool isEarlyStep = al::isLessEqualStep(this, 120);
    bool isAimShot = al::getRandom() < 0.3f;
    bool isExistPlayer = playerNum > 0;
    bool isRandomAim = isEarlyStep || isNoPlayer;
    al::LiveActor* nearestPlayer = nullptr;
    al::LiveActor* randomPlayer = nullptr;
    bool isExistRandomPlayer = false;
    if (isExistPlayer) {
        nearestPlayer = rc::findNearestActivePlayerActor(this);
        randomPlayer = rc::findRandomPlayerActor(this);
        isExistRandomPlayer = randomPlayer != nullptr;
    }

    sead::Vector3f dir;
    f32 distance;
    if (isRandomAim && isExistRandomPlayer) {
        distance = al::calcDistanceH(this, randomPlayer);
        dir.setSub(al::getTrans(randomPlayer), al::getTrans(this));
        al::normalize(&dir);
        al::verticalizeVec(&dir, sead::Vector3f::ey, dir);
        al::rotateVectorDegreeY(&dir, al::getRandom(340.0f) + 10.0f);
    } else if (!isRandomAim && isExistPlayer && isAimShot) {
        al::LiveActor* target = al::getRandom() < 0.5f ? randomPlayer : nearestPlayer;
        dir = al::getTrans(target) - al::getTrans(this);
        distance = dir.length();
        al::normalize(&dir);
        al::verticalizeVec(&dir, sead::Vector3f::ey, dir);
        al::rotateVectorDegreeY(&dir, al::getRandom(20.0f) + -10.0f);
    } else {
        f32 degree = al::getRandom(20) * 18.0f;
        if (isExistRandomPlayer) {
            distance = al::calcDistanceH(this, randomPlayer);
        } else {
            distance = al::getRandom(20) * 100.0f + 100.0f;
        }

        f32 angle = sead::Mathf::deg2rad(degree);
        dir = {std::cos(angle), 0.0f, std::sin(angle)};
    }

    al::setVelocity(rock, dir * 13.0f + sead::Vector3f(0.0f, distance * 0.012f + 5.0f, 0.0f));
    const sead::Vector3f& trans = al::getTrans(this);
    f32 attackRadius = al::getSensorRadius(this, "Attack");
    sead::Vector3f pos = sead::Vector3f(trans.x, trans.y - attackRadius, trans.z) + dir * 120.0f;
    al::resetPosition(rock, pos, false);
    rock->appear();
}

/** @brief Stops spinning and resumes walking. */
void BossGorobon::exeSpinEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "SpinEnd");
        al::setVelocityZero(this);
        al::calcFrontDir(&mFrontDir, this);
    }

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvBossGorobonWalk);
        al::offStageSwitch(this, "SwitchBossGorobonSpinStartOn");
        al::onStageSwitch(this, "SwitchBossGorobonSpinEndOn");
    }
}

/** @brief Plays the damage reaction, then spins (damageable only after a slowdown). */
void BossGorobon::exeDamage() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Damage");
        setVisibilityAnim();
    }

    if (al::isActionEnd(this)) {
        mIsDamageableSpinStart = false;
        al::setNerve(this, &NrvBossGorobonSpinStart);
    }
}

/** @brief Breaks every minion, then dies and turns on the defeat switch. */
void BossGorobon::exeDown() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Down");
        al::tryDeleteEffect(this, "Walk");
        setVisibilityAnim();
        for (s32 i = 0; i < mGorobonGroup->mNumActors; i++) {
            mGorobonGroup->getDeriveActor(i)->forceBreak();
        }

        if (mIsGateKeeper) {
            for (s32 i = 0; i < mMeraWanwanGroup->mNumActors; i++) {
                mMeraWanwanGroup->getDeriveActor(i)->forceDead();
            }
        }

        if (!mIsGateKeeper) {
            al::stopBgm(this, "Boss", 20, -1);
        }
    }

    if (al::isActionEnd(this)) {
        if (!mIsGateKeeper) {
            al::startBgm(this, "AfterBattle", -1, 0, -1, -1);
        }

        al::setAdditionalCameraLookAtPosPtr(this, this, nullptr);

        al::startHitReactionDeath(this);
        al::LiveActor::kill();
        al::onStageSwitch(this, "SwitchBossGorobonDeadOn");
    }
}

/** @brief Destroys the boss. */
BossGorobon::~BossGorobon() = default;
