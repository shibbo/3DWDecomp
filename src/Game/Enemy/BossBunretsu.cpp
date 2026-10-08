#include "Enemy/BossBunretsu.hpp"

#include <math/seadMathCalcCommon.h>
#include <math/seadQuat.h>

#include "Boss/BossStateDemoStart.hpp"
#include "Enemy/BossBunretsuChip.hpp"
#include "Enemy/BossBunretsuCore.hpp"
#include "Enemy/RingBeamerBeam.hpp"
#include "Enemy/TargetFinder.hpp"
#include "Library/Actor/ActorSensorController.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorAnimUtil.hpp"
#include "Library/LiveActor/Util/ActorClippingUtil.hpp"
#include "Library/LiveActor/Util/ActorFlagUtil.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorModelUtil.hpp"
#include "Library/LiveActor/Util/ActorMovementUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Screen/ScreenPointTarget.hpp"
#include "Library/StageSwitch/StageSwitchFunc.hpp"
#include "MapObj/DoubleMario.hpp"
#include "Player/Normal/PlayerAliveWatcher.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Scene/PlayerStocker.hpp"
#include "Util/ProjectMsgUtil.hpp"

namespace {
NERVE_DECL(BossBunretsu, Wait)
NERVE_DECL(BossBunretsu, Demo)
NERVE_DECL(BossBunretsu, BreakupWait)
NERVE_DECL(BossBunretsu, Appear)
NERVE_DECL(BossBunretsu, HipDropFall)
NERVE_DECL(BossBunretsu, HipDropEnd)
NERVE_DECL(BossBunretsu, Breakup)

/** @brief Gathers the body again with every chunk revived (shares exeGather with Gather). */
class BossBunretsuNrvGatherRecover : public al::Nerve {
public:
    void execute(al::NerveKeeper* pKeeper) const override {
        pKeeper->getParent<BossBunretsu>()->exeGather();
    }
};

NERVE_DECL(BossBunretsu, Gather)
NERVE_DECL(BossBunretsu, JumpStart)
NERVE_DECL(BossBunretsu, Trample)
NERVE_DECL(BossBunretsu, BattleStart)
NERVE_DECL(BossBunretsu, HipDrop)
NERVE_DECL(BossBunretsu, Jump)
NERVE_DECL(BossBunretsu, JumpEnd)
NERVE_DECL(BossBunretsu, GatherEnd)
NERVE_DECL(BossBunretsu, GatherEndFall)

/** @brief Lands after gathering again (shares exeJumpEnd with JumpEnd). */
class BossBunretsuNrvGatherLand : public al::Nerve {
public:
    void execute(al::NerveKeeper* pKeeper) const override {
        pKeeper->getParent<BossBunretsu>()->exeJumpEnd();
    }
};

// Non-const nerve objects share one .data block; the const ones sit right after their vtables.
BossBunretsuNrvWait NrvBossBunretsuWait;
BossBunretsuNrvDemo NrvBossBunretsuDemo;
const BossBunretsuNrvBreakupWait NrvBossBunretsuBreakupWait{};
BossBunretsuNrvAppear NrvBossBunretsuAppear;
BossBunretsuNrvHipDropFall NrvBossBunretsuHipDropFall;
BossBunretsuNrvHipDropEnd NrvBossBunretsuHipDropEnd;
const BossBunretsuNrvBreakup NrvBossBunretsuBreakup{};
BossBunretsuNrvGatherRecover NrvBossBunretsuGatherRecover;
BossBunretsuNrvGather NrvBossBunretsuGather;
const BossBunretsuNrvJumpStart NrvBossBunretsuJumpStart{};
BossBunretsuNrvTrample NrvBossBunretsuTrample;
const BossBunretsuNrvBattleStart NrvBossBunretsuBattleStart{};
BossBunretsuNrvHipDrop NrvBossBunretsuHipDrop;
BossBunretsuNrvJump NrvBossBunretsuJump;
BossBunretsuNrvJumpEnd NrvBossBunretsuJumpEnd;
BossBunretsuNrvGatherEnd NrvBossBunretsuGatherEnd;
const BossBunretsuNrvGatherEndFall NrvBossBunretsuGatherEndFall{};
const BossBunretsuNrvGatherLand NrvBossBunretsuGatherLand{};

TargetFinderParam sTargetFinderParam(5000.0f, 120.0f, 80.0f, 60, 20000.0f, -1.0f, -1.0f, -1.0f,
                                     false);
RingBeamerParam sRingBeamerParamLv2(4.0f, 250, "Emit2");
RingBeamerParam sRingBeamerParamLv3(8.0f, 250, "Emit3");

constexpr s32 cDoubleMarioNum = 4;
constexpr s32 cRingBeamNum = 8;
constexpr s32 cBgmStartStep = 634;
constexpr f32 cJumpGravity = 2.3f;
constexpr f32 cJumpSpeed = 55.0f;
constexpr f32 cJumpDistanceMax = 1000.0f;
constexpr f32 cTerritoryRadius = 1400.0f;
constexpr f32 cHipDropHeight = 1400.0f;
constexpr f32 cHipDropGravity = 3.8f;

}  // namespace

/**
 * @brief Creates the boss with three hit points and thirty body chunks.
 * @param pName Actor name.
 */
BossBunretsu::BossBunretsu(const char* pName) : al::LiveActor(pName) {}

/**
 * @brief Reads the boss level, creates the core, chunks, gather model, Double Mario items and
 * ring beams, and sets up the opening demo.
 * @param rInfo Actor placement and scene initialization information.
 */
void BossBunretsu::init(const al::ActorInitInfo& rInfo) {
    const char* objectName = nullptr;
    const char* suffix;

    if (al::tryGetObjectName(&objectName, rInfo) &&
        al::isEqualString("BossBunretsuLv2", objectName)) {
        mLevel = 2;
        suffix = "Lv2";
    } else if (al::tryGetObjectName(&objectName, rInfo) &&
               al::isEqualString("BossBunretsuLv3", objectName)) {
        mLevel = 3;
        suffix = "Lv3";
    } else {
        suffix = nullptr;
    }

    al::initActorWithArchiveName(this, rInfo, "BossBunretsu", suffix);
    al::initNerve(this, &NrvBossBunretsuWait, 1);

    mSensorControllerList = new al::ActorSensorControllerList(4);
    mSensorControllerList->addSensor(this, "Body");
    mSensorControllerList->addSensor(this, "Attack");
    mSensorControllerList->addSensor(this, "AttackL");
    mSensorControllerList->addSensor(this, "AttackR");

    mDemoStartInfo = new BossDemoStartInfo(al::initAnimCamera(this, rInfo), this,
                                           "DemoBattleStart", 60, nullptr);
    mDemoStartInfo->_18 = 30;
    mStateDemoStart = new BossStateDemoStart(this, rInfo, mDemoStartInfo);
    al::initNerveState(this, mStateDemoStart, &NrvBossBunretsuDemo, "開始デモ");

    mCore = new BossBunretsuCore(this, "分裂ボス コア");
    mCore->init(rInfo);
    mCore->makeActorDead();

    mChips = new BossBunretsuChip*[mChipNum];
    for (s32 i = 0; i < mChipNum; i++) {
        mChips[i] = new BossBunretsuChip(this, i, "分裂ボス 肉片");
        mChips[i]->init(rInfo);
        mChips[i]->makeActorDead();
    }

    mTargetFinder = new TargetFinder(this, &sTargetFinderParam);
    mCameraInfo = al::initObjectCamera(this, rInfo, nullptr);
    al::trySyncStageSwitchAppear(this);
    al::invalidateClipping(this);

    mTransformModel = new al::LiveActor("ボスブンレツ集合中の専用モデル");
    al::initActorWithArchiveName(mTransformModel, rInfo, "BossBunretsuChipTransform", nullptr);
    mTransformModel->makeActorDead();
    al::invalidateClipping(mTransformModel);

    mDoubleMarioNum = calcAppearDoubleMarioNum(rInfo);
    mDoubleMarios = new DoubleMario*[mDoubleMarioNum];
    for (s32 i = 0; i < mDoubleMarioNum; i++) {
        mDoubleMarios[i] = new DoubleMario("ダブルマリオアイテム[ボスブンレツ]", nullptr, true);
        mDoubleMarios[i]->init(rInfo);
        mDoubleMarios[i]->makeActorDead();
    }

    if (mLevel >= 2) {
        mRingBeamNum = cRingBeamNum;
        mRingBeams = new RingBeamerBeam*[cRingBeamNum];
        for (s32 i = 0; i < mRingBeamNum; i++) {
            mRingBeams[i] =
                new RingBeamerBeam("リングビーム", this, RingBeamerType_BossBunretsu, false);
            al::initCreateActorNoPlacementInfo(mRingBeams[i], rInfo);
            mRingBeams[i]->setRingBeamerParam(mLevel == 2 ? &sRingBeamerParamLv2 :
                                                            &sRingBeamerParamLv3);
            mRingBeams[i]->setColorFrame(mLevel - 1);
        }
    }

    mInitTrans = al::getTrans(this);
    al::getTransPtr(this)->z += -1000.0f;
    al::resetPosition(this, false);

    al::startMclAnimAndSetFrameAndStop(this, "BossBunretsuColor", mLevel - 1);
    al::startMtpAnimAndSetFrameAndStop(this, "BossBunretsuColor", mLevel - 1);
    al::startMtpAnimAndSetFrameAndStop(mCore, "BossBunretsuColor", mLevel - 1);
    al::startMclAnimAndSetFrameAndStop(mTransformModel, "BossBunretsuColor", mLevel - 1);
    al::startMtpAnimAndSetFrameAndStop(mTransformModel, "BossBunretsuColor", mLevel - 1);

    for (s32 i = 0; i < mChipNum; i++) {
        al::startMclAnimAndSetFrameAndStop(mChips[i], "BossBunretsuColor", mLevel - 1);
        al::startMtpAnimAndSetFrameAndStop(mChips[i], "BossBunretsuColor", mLevel - 1);
    }
}

/**
 * @brief Gets the number of Double Mario items the boss creates.
 * @param rInfo Actor placement information (unused).
 * @return Number of Double Mario items.
 */
s32 BossBunretsu::calcAppearDoubleMarioNum(const al::ActorInitInfo& rInfo) {
    return cDoubleMarioNum;
}

/** @brief Removes leftover Double Mario items and starts the opening demo with a hidden body. */
void BossBunretsu::appear() {
    PlayerStockerFunction::killAllAppearDoubleItem(this);
    al::setNerve(this, &NrvBossBunretsuDemo);
    mCore->startDemo();
    al::LiveActor::appear();
    al::hideModelIfShow(this);
    al::invalidateScreenPointTargetAll(this);
}

/** @brief Turns the dead switch on, releases the camera look-at position and dies. */
void BossBunretsu::kill() {
    al::tryOnSwitchDeadOn(this);
    al::setAdditionalCameraLookAtPosPtr(this, this, nullptr);
    al::LiveActor::kill();
}

/** @brief Updates the animations unless the body is split up. */
void BossBunretsu::calcAnim() {
    if (al::isNerve(this, &NrvBossBunretsuBreakupWait)) {
        return;
    }

    al::LiveActor::calcAnim();
}

/** @brief Moves the camera look-at position between the boss (or its core) and the arena. */
void BossBunretsu::control() {
    if (al::isNerve(this, &NrvBossBunretsuWait) || al::isNerve(this, &NrvBossBunretsuAppear) ||
        al::isNerve(this, &NrvBossBunretsuDemo)) {
        return;
    }

    al::LiveActor* lookAtActor = al::isAlive(mCore) ? static_cast<al::LiveActor*>(mCore) : this;
    const sead::Vector3f& trans = al::getTrans(lookAtActor);
    sead::Vector3f lookAtPos = trans;
    lookAtPos.y = al::lerpValue(0.5f, mInitTrans.y, lookAtPos.y);
    mCameraLookAtPos = lookAtPos;
}

/**
 * @brief Pushes and attacks players touching the body (only the bottom while hip dropping).
 * @param pSelf Own sensor.
 * @param pOther Other actor's sensor.
 */
void BossBunretsu::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (!al::isSensorEnemyAttack(pSelf) || !al::isSensorPlayer(pOther) || !isAttackable()) {
        return;
    }

    if ((al::isNerve(this, &NrvBossBunretsuHipDropFall) ||
         al::isNerve(this, &NrvBossBunretsuHipDropEnd)) &&
        !al::isSensorName(pSelf, "Attack")) {
        return;
    }

    al::sendMsgPush(pOther, pSelf);
    al::sendMsgEnemyAttackForCrossoverSensor(pOther, pSelf);
}

/**
 * @brief Checks whether the body can hurt players.
 * @return False while being trampled.
 */
bool BossBunretsu::isAttackable() const {
    return !al::isNerve(this, &NrvBossBunretsuTrample);
}

/**
 * @brief Bounces fire balls and boomerangs off the body.
 * @param pMsg Received message.
 * @param pOther Sender's sensor.
 * @param pSelf Own sensor.
 * @return Whether the message was handled.
 */
bool BossBunretsu::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                              al::HitSensor* pSelf) {
    if (!al::isSensorEnemyBody(pSelf)) {
        return false;
    }

    if (al::isMsgPlayerFireBallAttack(pMsg) || al::isMsgPlayerBoomerangBreak(pMsg)) {
        rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        return true;
    }

    return false;
}

/**
 * @brief Accepts touch assist from the touch screen.
 * @param pMsg Received message.
 * @param pPointer Screen pointer (unused).
 * @param pTarget Screen point target (unused).
 * @return Whether the message is a touch assist.
 */
bool BossBunretsu::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                         al::ScreenPointTarget* pTarget) {
    return al::isMsgTouchAssist(pMsg);
}

/** @brief Splits the body into its chunks and pops out a Double Mario item if one is missing. */
void BossBunretsu::startBreakup() {
    mJumpCount = 0;
    mCore->startBreakup();

    for (s32 i = 0; i < mActiveChipNum; i++) {
        mChips[i]->startBreakup();
    }

    s32 doubleMarioNum = PlayerAliveWatcher::getPlayerAliveWatcher(this)->calcDoubleMarioNum();
    for (s32 i = 0; i < mDoubleMarioNum; i++) {
        if (!al::isDead(mDoubleMarios[i])) {
            doubleMarioNum++;
        }
    }

    if (doubleMarioNum < mDoubleMarioNum) {
        for (s32 i = 0; i < mDoubleMarioNum; i++) {
            if (al::isDead(mDoubleMarios[i])) {
                mDoubleMarios[i]->appearPopUpFront();
                al::resetPosition(mDoubleMarios[i], al::getTrans(this), false);
                break;
            }
        }
    }

    al::setNerve(this, &NrvBossBunretsuBreakup);
}

/**
 * @brief Gathers the chunks at the core again, reviving dead chunks up to the body size.
 * @param isRecover Whether every chunk is revived (otherwise at least five chunks gather).
 */
void BossBunretsu::startGather(bool isRecover) {
    al::resetPosition(this, al::getTrans(mCore), false);
    al::onCollide(this);
    al::LiveActor* player = al::tryFindNearestPlayerActor(this);

    if (player != nullptr) {
        const sead::Vector3f& playerTrans = al::getTrans(player);
        const sead::Vector3f& trans = al::getTrans(this);
        sead::Vector3f front = {playerTrans.x - trans.x, 0.0f, playerTrans.z - trans.z};
        al::normalizeOrDirZ(&front);
        al::setFront(this, front);
    }

    s32 aliveNum = 0;
    for (s32 i = 0; i < mChipNum; i++) {
        if (isRecover) {
            if (!al::isDead(mChips[i])) {
                aliveNum++;
            }

            mChips[i]->startGather();
        } else if (!al::isDead(mChips[i])) {
            aliveNum++;
            mChips[i]->startGather();
        }
    }

    mActiveChipNum = isRecover ? mChipNum : aliveNum > 5 ? aliveNum : 5;
    s32 reviveNum = mActiveChipNum - aliveNum;

    for (s32 i = 0; i < mChipNum; i++) {
        if (al::isDead(mChips[i])) {
            mChips[i]->startGather();
            if (reviveNum <= 1) {
                break;
            }

            reviveNum--;
        }
    }

    al::setNerve(this, isRecover ? static_cast<const al::Nerve*>(&NrvBossBunretsuGatherRecover) :
                                   &NrvBossBunretsuGather);
}

/**
 * @brief Picks the next jump target near the targeted player and starts jumping.
 * @return Whether a target was found.
 */
bool BossBunretsu::startJump() {
    mTargetFinder->refindTarget();
    mTargetFinder->update();

    if (mTargetFinder->getTarget() == nullptr) {
        return false;
    }

    if (mIsFirstJump) {
        mIsFirstJump = false;
        mJumpTarget = al::getTrans(this);
    } else {
        mJumpTarget = mTargetFinder->getTargetPos();
        sead::Vector3f toTarget = mJumpTarget - al::getTrans(this);
        toTarget.y = 0.0f;

        if (toTarget.squaredLength() > cJumpDistanceMax * cJumpDistanceMax) {
            f32 length = toTarget.length();
            mJumpTarget = al::getTrans(this) + toTarget * (cJumpDistanceMax / length);
        }

        sead::Vector3f fromInit = mJumpTarget - mInitTrans;
        fromInit.y = 0.0f;

        if (fromInit.squaredLength() > cTerritoryRadius * cTerritoryRadius) {
            f32 length = fromInit.length();
            mJumpTarget = mInitTrans + fromInit * (cTerritoryRadius / length);
        }
    }

    al::setNerve(this, &NrvBossBunretsuJumpStart);
    return true;
}

/**
 * @brief Takes one hit point and lets the chunks react.
 * @return Whether the boss was defeated.
 */
bool BossBunretsu::receiveDamage() {
    mHitPoint--;

    if (mHitPoint <= 0) {
        for (s32 i = 0; i < mActiveChipNum; i++) {
            mChips[i]->onDeath();
        }

        return true;
    }

    for (s32 i = 0; i < mActiveChipNum; i++) {
        mChips[i]->onDamage();
    }

    return false;
}

/**
 * @brief Gets how many hits the boss has taken.
 * @return Number of received hits.
 */
s32 BossBunretsu::receivedDamageNum() const {
    return 3 - mHitPoint;
}

/** @brief Kills the boss after its defeat. */
void BossBunretsu::startDeath() {
    kill();
}

/**
 * @brief Checks whether the gathered body is made of few chunks.
 * @return Whether fewer than eleven chunks are active.
 */
bool BossBunretsu::isBodySizeSmall() const {
    return mActiveChipNum < 11;
}

/** @brief Waits for a target and jumps at it. */
void BossBunretsu::exeWait() {
    if (al::isFirstStep(this)) {
        if (!al::isActionPlaying(this, "Wait")) {
            al::startAction(this, "Wait");
        }

        mTargetFinder->refindTarget();
    }

    if (al::isGreaterEqualStep(this, 0)) {
        startJump();
    }
}

/** @brief Does nothing. */
void BossBunretsu::exeAppear() {}

/** @brief Plays the opening demo and starts the boss music. */
void BossBunretsu::exeDemo() {
    if (al::isFirstStep(this)) {
        mTransformModel->appear();
        al::setTrans(mCore, al::getTrans(this));
        al::setTrans(mTransformModel, al::getTrans(this));
        al::showModelIfHide(this);
        al::startAction(this, "DemoBattleStart");
        al::startAction(mTransformModel, "DemoBattleStart");
    }

    if (al::isStep(this, cBgmStartStep)) {
        al::startBgm(this, "Boss", -1, 0, -1, -1);
    }

    if (al::updateNerveState(this)) {
        if (al::isLessStep(this, cBgmStartStep)) {
            al::startBgm(this, "Boss", -1, 0, -1, -1);
        }

        if (mStateDemoStart->isSkipped()) {
            al::tryDeleteEmitterAndParticleAll(this);
        }

        al::setNerve(this, &NrvBossBunretsuBattleStart);
        mTransformModel->kill();
        mCore->kill();
    }
}

/** @brief Points the camera at the boss and waits a moment before the battle. */
void BossBunretsu::exeBattleStart() {
    if (al::isFirstStep(this)) {
        al::setAdditionalCameraLookAtPosPtr(this, this, &mCameraLookAtPos);
        al::startAction(this, "Wait");
    }

    if (al::isGreaterEqualStep(this, 60)) {
        al::setNerve(this, &NrvBossBunretsuWait);
    }
}

/** @brief Turns to the jump target and decides between another jump and a hip drop. */
void BossBunretsu::exeJumpStart() {
    s32 jumpCount = mJumpCount;

    if (al::isFirstStep(this)) {
        al::setVelocityZero(this);
        if (jumpCount == 0) {
            al::startAction(this, "JumpStart");
        }

        return;
    }

    bool isFaced = faceToTarget();
    if (jumpCount == 0 && !(isFaced & al::isActionEnd(this))) {
        return;
    }

    mJumpCount++;
    s32 jumpNum = mHitPoint == 2 ? 5 : mHitPoint == 1 ? 7 : 3;

    if (jumpNum <= mJumpCount) {
        al::setNerve(this, &NrvBossBunretsuHipDrop);
    } else {
        al::setNerve(this, &NrvBossBunretsuJump);
    }
}

/**
 * @brief Turns the boss towards the jump target.
 * @return Whether the boss faces the target.
 */
bool BossBunretsu::faceToTarget() {
    sead::Vector3f dir = mJumpTarget - al::getTrans(this);
    bool isFaced;

    if (al::normalizeOrZero(&dir)) {
        isFaced = true;
    } else {
        sead::Vector3f front;
        al::calcFrontDir(&front, this);
        isFaced = al::turnDirectionDegree(this, &front, dir, 10.0f);
        sead::Quatf quat;
        al::makeQuatUpFront(&quat, -al::getGravity(this), front);
        al::updatePoseQuat(this, quat);
    }

    return isFaced;
}

/** @brief Jumps towards the jump target and lands. */
void BossBunretsu::exeJump() {
    if (al::isFirstStep(this)) {
        sead::Vector3f velocity;
        calcJumpVelocity(&velocity, mJumpTarget, cJumpGravity, cJumpSpeed);
        al::setVelocity(this, velocity);
        al::startAction(this, "Jump");
        return;
    }

    faceToTarget();

    if (al::isCollidedGround(this) && al::getVelocityPtr(this)->y < 0.0f) {
        al::setNerve(this, &NrvBossBunretsuJumpEnd);
        al::setVelocityZero(this);
        return;
    }

    al::addVelocityToGravity(this, cJumpGravity);
}

/**
 * @brief Calculates the velocity of a jump with a fixed vertical speed that lands on a target.
 * @param pVelocity Output velocity.
 * @param rTarget Landing position.
 * @param gravity Gravity applied every frame.
 * @param jumpSpeed Vertical start speed.
 * @return Frames until landing, or -1 if the target cannot be reached.
 */
f32 BossBunretsu::calcJumpVelocity(sead::Vector3f* pVelocity, const sead::Vector3f& rTarget,
                                   f32 gravity, f32 jumpSpeed) {
    sead::Vector3f diff = rTarget - al::getTrans(this);
    sead::Vector3f dir = {diff.x, 0.0f, diff.z};
    f32 lengthSq = dir.squaredLength();
    f32 time = -1.0f;
    sead::Vector3f velocity = {0.0f, jumpSpeed, 0.0f};

    if (!al::isNearZero(lengthSq, 0.001f)) {
        f32 length = sead::Mathf::sqrt(lengthSq);
        dir *= 1.0f / length;
        f32 discriminant = jumpSpeed * jumpSpeed + gravity * -2.0f * diff.y;
        bool isReachable = false;

        if (!(discriminant < 0.0f)) {
            f32 root = sead::Mathf::sqrt(discriminant);
            f32 landTime = (-jumpSpeed - root) / -gravity;
            if (landTime < 0.0f) {
                landTime = (root - jumpSpeed) / -gravity;
            }

            if (landTime > 0.0f) {
                f32 speed = length / landTime;
                velocity.x = dir.x * speed;
                velocity.z = dir.z * speed;
                time = landTime;
                isReachable = true;
            }
        }

        if (!isReachable) {
            f32 speed = jumpSpeed * 0.1f;
            velocity.x = speed * dir.x;
            velocity.z = speed * dir.z;
        }
    }

    *pVelocity = velocity;
    return time;
}

/** @brief Lands, fires a ring beam and jumps again. */
void BossBunretsu::exeJumpEnd() {
    if (al::isFirstStep(this)) {
        s32 index = mJumpCount - 1;

        if (index >= 0 && mRingBeams != nullptr && mJumpCount <= mRingBeamNum) {
            mRingBeams[index]->setActiveWithPosture(al::getTrans(this), al::getQuat(this));
        }

        al::setVelocityZero(this);
        if (al::isNerve(this, &NrvBossBunretsuJumpEnd)) {
            al::startAction(this, "JumpLand");
        } else {
            al::startAction(this, "GatherLand");
        }
    }

    if (al::isActionEnd(this) && !startJump()) {
        al::setNerve(this, &NrvBossBunretsuWait);
    }
}

/** @brief Jumps onto the jump target to hip drop on it. */
void BossBunretsu::exeHipDrop() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "HipDropStart");
        f32 time = sead::Mathf::sqrt(cHipDropHeight * 2.0f / cHipDropGravity);
        f32 speedY = time * cHipDropGravity;
        const sead::Vector3f& trans = al::getTrans(this);
        sead::Vector3f velocity = {mJumpTarget.x - trans.x, 0.0f, mJumpTarget.z - trans.z};

        if (!al::isNearZero(time, 0.001f)) {
            velocity *= 1.0f / time;
        } else {
            velocity.set(0.0f, 0.0f, 0.0f);
        }

        velocity.y = speedY;
        al::setVelocity(this, velocity);
        mHipDropStep = time;
        return;
    }

    faceToTarget();

    if (al::isGreaterStep(this, mHipDropStep)) {
        al::setNerve(this, &NrvBossBunretsuHipDropFall);
        return;
    }

    if (al::isCollidedGround(this) && al::getVelocityPtr(this)->y < 0.0f) {
        return;
    }

    al::addVelocityToGravity(this, cJumpGravity);
}

/** @brief Falls down quickly after hanging in the air. */
void BossBunretsu::exeHipDropFall() {
    if (al::isFirstStep(this)) {
        al::setVelocityZero(this);
        al::startAction(this, "HipDrop");
    }

    if (!al::isGreaterStep(this, 5)) {
        return;
    }

    if (al::isCollidedGround(this)) {
        al::setNerve(this, &NrvBossBunretsuHipDropEnd);
        return;
    }

    al::addVelocityToGravity(this, cHipDropGravity);
}

/** @brief Lands from the hip drop, fires a ring beam and splits up. */
void BossBunretsu::exeHipDropEnd() {
    if (al::isFirstStep(this)) {
        if (mRingBeams != nullptr && mJumpCount < mRingBeamNum) {
            mRingBeams[mJumpCount]->setActiveWithPosture(al::getTrans(this), al::getQuat(this));
        }

        al::setVelocityZero(this);
        al::startAction(this, "HipDropLand");
    }

    if (al::isActionEnd(this)) {
        startBreakup();
    }
}

/** @brief Gets trampled by a player and splits up. */
void BossBunretsu::exeTrample() {
    if (al::isFirstStep(this)) {
        al::setVelocityZero(this);
        al::startAction(this, "Trampled");
    }

    if (al::isActionEnd(this)) {
        startBreakup();
    }
}

/** @brief Hides the body and disables it while the chunks fly apart. */
void BossBunretsu::exeBreakup() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait");
        al::setVelocityZero(this);
        al::hideModelIfShow(this);
        al::invalidateScreenPointTargetAll(this);
        al::invalidateHitSensors(this);
        al::offCollide(this);
    }

    if (al::isGreaterEqualStep(this, 5)) {
        al::setNerve(this, &NrvBossBunretsuBreakupWait);
    }
}

/** @brief Does nothing (the core and chunks act on their own). */
void BossBunretsu::exeBreakupWait() {}

/** @brief Plays the gather model's transformation at the core. */
void BossBunretsu::exeGather() {
    if (al::isFirstStep(this)) {
        mTransformModel->appear();
        al::setTrans(mTransformModel, al::getTrans(mCore));
        if (al::isNerve(this, &NrvBossBunretsuGatherRecover)) {
            al::startAction(mTransformModel, "TransformRecover");
        } else {
            al::startAction(mTransformModel, "Transform");
        }
    }

    if (al::isActionEnd(mTransformModel)) {
        al::setNerve(this, &NrvBossBunretsuGatherEnd);
    }
}

/** @brief Shows the gathered body again, scaled by the number of chunks. */
void BossBunretsu::exeGatherEnd() {
    if (al::isFirstStep(this)) {
        mTransformModel->kill();
        mCore->endGather();

        for (s32 i = 0; i < mChipNum; i++) {
            mChips[i]->endGather();
        }

        f32 scale = mActiveChipNum < 11 ? 0.6f : mActiveChipNum < 21 ? 0.7f : 1.0f;
        mSensorControllerList->setAllSensorScale(scale);
        al::setScaleAll(this, scale);
        al::showModelIfHide(this);
        al::validateScreenPointTargetAll(this);
        al::validateHitSensors(this);
        al::startAction(this, "GatherEnd");
    }

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvBossBunretsuGatherEndFall);
    }
}

/** @brief Falls down after gathering. */
void BossBunretsu::exeGatherEndFall() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "GatherLoop");
    }

    al::scaleVelocity(this, 0.998f);

    if (al::isCollidedGround(this)) {
        al::setNerve(this, &NrvBossBunretsuGatherLand);
        return;
    }

    al::addVelocityToGravity(this, cJumpGravity);
}

/**
 * @brief Checks whether the boss can be damaged.
 * @return Whether the boss is waiting.
 */
bool BossBunretsu::isReceivableAttack() const {
    return al::isNerve(this, &NrvBossBunretsuWait);
}

BossBunretsu::~BossBunretsu() = default;
