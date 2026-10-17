#include "Boss/Punpun.hpp"

#include <prim/seadSafeString.h>
#include <time/seadTickTime.h>

#include "Boss/GateKeeperStateDemo.hpp"
#include "Boss/PunpunDivision.hpp"
#include "Boss/PunpunParam.hpp"
#include "Boss/PunpunShuriken.hpp"
#include "Enemy/ActorJointLookController.hpp"
#include "Enemy/EnemyStateUtil.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/Joint/JointControllerKeeper.hpp"
#include "Library/LiveActor/Common/LiveActorGroup.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Movement/RumbleCalculator.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Rail/RailUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "MapObj/DisasterModeController.hpp"
#include "Project/Bgm/BgmPlayingRequest.hpp"
#include "Util/DemoUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ScoreUtil.hpp"

namespace {
NERVE_DECL(Punpun, DemoAppear)
NERVE_DECL(Punpun, PreDemoAppearDelay)
NERVE_DECL(Punpun, Die)

/** @brief Knocked down by fire balls (shares exePressDown). */
class PunpunNrvFireBallDown : public al::Nerve {
public:
    void execute(al::NerveKeeper* pKeeper) const override {
        pKeeper->getParent<Punpun>()->exePressDown();
    }
};

NERVE_DECL(Punpun, DiePressDown)

/** @brief Knocked down by a tail or spin attack (shares exePressDown). */
class PunpunNrvTailDown : public al::Nerve {
public:
    void execute(al::NerveKeeper* pKeeper) const override {
        pKeeper->getParent<Punpun>()->exePressDown();
    }
};

NERVE_DECL(Punpun, PressDown)
NERVE_DECL(Punpun, Hide)
NERVE_DECL(Punpun, DivideWait)
NERVE_DECL(Punpun, Divide)
NERVE_DECL(Punpun, DivideAppear)
NERVE_DECL(Punpun, ThrowWait)
NERVE_DECL(Punpun, ThrowSign)
NERVE_DECL(Punpun, Throw)
NERVE_DECL(Punpun, ThrowEnd)
NERVE_DECL(Punpun, DivideMove)

NERVES_MAKE_NOSTRUCT(Punpun, DemoAppear, PreDemoAppearDelay, Die, FireBallDown, DiePressDown,
                     TailDown, PressDown, Hide, DivideWait, Divide, DivideAppear, ThrowWait,
                     ThrowSign, Throw, ThrowEnd, DivideMove)

/** @brief Number of clones per level (row) and number of hits taken (column). */
const s32 sDivisionNum[2][3] = {{2, 3, 5}, {5, 7, 8}};

/**
 * @brief Gets the largest number of clones a level can split into.
 * @param level Punpun level (0 or 1).
 * @return Maximum clone count.
 */
inline s32 getDivisionNumMax(s32 level) {
    s32 numMax = 0;
    for (s32 i = 0; i < 3; i++) {
        if (numMax < sDivisionNum[level][i]) {
            numMax = sDivisionNum[level][i];
        }
    }

    return numMax;
}

sead::Vector2f sLookLimit(180.0f, 180.0f);
sead::Vector2f sHeadLookRange(-30.0f, 30.0f);
sead::Vector2f sEyeLookRange(-5.25f, 5.25f);
GateKeeperStateDemoParam sDemoParam("DemoAppear", 0);
ActorJointLookControllerParam sHeadLookParam(2.0f, sHeadLookRange, false, nullptr, nullptr);
}  // namespace

/**
 * @brief Creates Punpun together with its shuriken and squash rumble.
 * @param pName Actor name.
 */
Punpun::Punpun(const char* pName)
    : al::LiveActor(pName), mShuriken(new PunpunShuriken("プンプンの手裏剣")),
      mRumble(new al::RumbleCalculatorCosMultLinear(2.5f, 2.0f, 0.1f, 30)),
      mRandom(static_cast<u32>(sead::TickTime().toTicks())) {}

/**
 * @brief Gets how many clones split off at the current level and damage count.
 * @return Number of clones.
 */
inline s32 Punpun::getDivisionNum() const {
    return sDivisionNum[mLevel][mDamageCount];
}

/** @brief Kills every clone that is still alive. */
inline void Punpun::killAliveDivisions() {
    for (s32 i = 0; i < mDivisionGroup->getActorCount(); i++) {
        if (al::isAlive(mDivisionGroup->getActor(i))) {
            mDivisionGroup->getActor(i)->kill();
        }
    }
}

/**
 * @brief Checks whether Punpun is moving away or splitting into clones.
 * @return Whether Punpun is in one of the divide nerves.
 */
inline bool Punpun::isDividing() const {
    return al::isNerve(this, &NrvPunpunDivideMove) || al::isNerve(this, &NrvPunpunDivideWait) ||
           al::isNerve(this, &NrvPunpunDivide);
}

/**
 * @brief Checks whether Punpun is knocked down or dying.
 * @return Whether Punpun is in one of the damage nerves.
 */
inline bool Punpun::isDamaged() const {
    return al::isNerve(this, &NrvPunpunPressDown) || al::isNerve(this, &NrvPunpunFireBallDown) ||
           al::isNerve(this, &NrvPunpunTailDown) || al::isNerve(this, &NrvPunpunDiePressDown) ||
           al::isNerve(this, &NrvPunpunDie);
}

/**
 * @brief Initializes the model, the clones, the shuriken and the opening demo.
 * @param rInfo Actor init info.
 */
void Punpun::init(const al::ActorInitInfo& rInfo) {
    if (rInfo.getActorSceneInfo().isSingleMode) {
        al::initActorWithArchiveName(this, rInfo, "PunpunFur", nullptr);
    } else {
        al::initActorWithArchiveName(this, rInfo, "Punpun", nullptr);
    }

    al::hideModel(this);
    al::calcFrontDir(&mInitFront, this);
    mDivideStartTrans.set(al::getTrans(this));
    mIsSingleMode = rInfo.getActorSceneInfo().isSingleMode;

    if (al::isObjectName(rInfo, "PunpunLv2")) {
        mLevel = 1;
    }

    auto* pDivisionGroup = new DivisionGroup("分身リスト", getDivisionNumMax(mLevel));
    mDivisionGroup = pDivisionGroup;
    for (s32 i = 0; i < pDivisionGroup->getMaxActorCount(); i++) {
        auto* pDivision = new PunpunDivision("プンプン[分身]");
        al::initCreateActorWithPlacementInfo(pDivision, rInfo);
        pDivisionGroup->registerActor(pDivision);
    }

    al::initCreateActorWithPlacementInfo(mShuriken, rInfo);
    mLookController = makePunpunLookController(this);

    if (mIsSingleMode) {
        al::invalidateHitSensors(this);
        al::initNerve(this, &NrvPunpunPreDemoAppearDelay, 1);
        mStateDemo = new GateKeeperStateDemo(this, rInfo, &sDemoParam,
                                             al::initAnimCamera_RS(this, rInfo, "Anim"));
    } else {
        al::initNerve(this, &NrvPunpunDemoAppear, 1);
        mStateDemo =
            new GateKeeperStateDemo(this, rInfo, &sDemoParam, al::initAnimCamera(this, rInfo));
    }

    al::initNerveState(this, mStateDemo, &NrvPunpunDemoAppear, "開始デモ");
    al::tryGetArg(&mDivideDistance, rInfo, "DistDiv");
    al::trySyncStageSwitchAppear(this);
}

/**
 * @brief Creates the controller that turns Punpun's head and eyes towards the player.
 * @param pActor Actor whose joints are turned.
 * @return The new look controller.
 */
ActorJointLookController* Punpun::makePunpunLookController(const al::LiveActor* pActor) {
    auto* pController = new ActorJointLookController(pActor, 3);
    al::initJointControllerKeeper(pActor, pController->mParams.capacity());
    pController->setLimit(sLookLimit);
    pController->appendJoint("Head", sead::Vector3f::ex, &sHeadLookParam);

    auto* pEyeParam = new ActorJointLookControllerParam(2.0f, sEyeLookRange, false, nullptr,
                                                        al::getJointMtxPtr(pActor, "Head"));
    pController->appendJoint("EyeL", sead::Vector3f::ey, pEyeParam);
    pController->appendJoint("EyeR", sead::Vector3f::ey, pEyeParam);
    return pController;
}

/** @brief Updates the look controller, the KoopaJr hit cooldown and the squash rumble. */
void Punpun::control() {
    mLookController->update();

    if (mKoopaJrHitCooldown > 0) {
        mKoopaJrHitCooldown--;
    }

    if (!mRumble->isEnd()) {
        mRumble->calc();
        al::setScaleY(this, mRumble->getValueY() + 1.0f);
        if (mRumble->isEnd()) {
            al::setScaleY(this, 1.0f);
        }
    }
}

/** @brief Kills Punpun, turns on the dead switch and starts the after-battle music. */
void Punpun::kill() {
    al::LiveActor::kill();
    al::tryOnSwitchDeadOn(this);

    if (mIsSingleMode) {
        DisasterModeController* pController = DisasterModeController::tryGetController(this);
        al::BgmPlayingRequest request("AfterBattleSingleMode");
        if (pController != nullptr && pController->isDisasterMode()) {
            request._18 = 163300;
        }

        al::startBgm(this, request);
        al::resumeOceanBgm(this, -1);
    } else {
        al::startBgm(this, "AfterBattle", -1, 0, -1, -1);
    }
}

/**
 * @brief Pushes or attacks the player (and KoopaJr) on contact.
 * @param pSelf Own sensor.
 * @param pOther Touched sensor.
 */
void Punpun::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (isDividing()) {
        return;
    }

    if (isDamaged()) {
        if (al::isSensorEnemyBody(pSelf) &&
            (al::isSensorPlayer(pOther) || al::isSensorKoopaJr(pOther))) {
            al::sendMsgPush(pOther, pSelf);
        }
    }

    if (al::isNerve(this, &NrvPunpunDemoAppear) || isDamaged()) {
        return;
    }

    if (mIsSingleMode &&
        (al::isSensorHostName(pOther, "DoorKey") || al::isSensorHostName(pOther, "コウラ"))) {
        al::sendMsgPush(pOther, pSelf);
    }

    if (al::isSensorEnemyAttack(pSelf) &&
        (al::isSensorPlayer(pOther) || al::isSensorKoopaJr(pOther))) {
        if (!mIsSingleMode || !al::isSensorNpc(pSelf)) {
            al::sendMsgEnemyAttackForCrossoverSensor(pOther, pSelf);
        }

        al::sendMsgPush(pOther, pSelf);
    }
}

/**
 * @brief Takes fire ball hits and stomps.
 * @param pMsg Received message.
 * @param pOther Sender sensor.
 * @param pSelf Own sensor.
 * @return Whether the message was handled.
 */
bool Punpun::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) {
    if (al::isNerve(this, &NrvPunpunDemoAppear) || isDividing() || isDamaged()) {
        return false;
    }

    if (!al::isSensorEnemyBody(pSelf)) {
        return false;
    }

    if (al::isMsgEnemyAttack(pMsg)) {
        return al::getSensorHost(pOther) != mShuriken;
    }

    bool isKoopaJrSpin =
        al::isMsgPlayerSpinAttack(pMsg) && al::isSensorHostName(pOther, "KoopaJr");
    if (al::isMsgKillerAttack(pMsg) || al::isMsgPlayerFireBallAttack(pMsg) ||
        al::isMsgPlayerBoomerangReflect(pMsg) || isKoopaJrSpin) {
        if (isKoopaJrSpin) {
            s32 cooldown = mKoopaJrHitCooldown;
            mKoopaJrHitCooldown = 20;
            if (cooldown > 0) {
                return true;
            }
        }

        mFireBallHitCount++;
        rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        al::startMclAnim(this, "ReactionFireball");
        al::startSe(this, "PgFireBallHit");

        if (mFireBallHitCount >= 3) {
            if (receiveDamage(pOther)) {
                al::setNerve(this, &NrvPunpunDie);
            } else {
                al::setNerve(this, &NrvPunpunFireBallDown);
            }
        } else {
            mRumble->start(0);
        }

        return true;
    }

    bool isSingleModeAttack =
        mIsSingleMode &&
        (al::isMsgPlayerOnlyInvincibleAttack(pMsg) || al::isMsgPlayerKouraAttack(pMsg) ||
         al::isMsgKickKouraAttack(pMsg) || al::isMsgNekoAttack(pMsg) ||
         al::isMsgBallAttack(pMsg) || al::isMsgKeyThrow(pMsg) ||
         (al::isMsgExplosion(pMsg) && al::isSensorHostName(pOther, "KoopaJr")));
    if (EnemyStateUtil::isMsgPressDownForCrossoverSensor(pMsg, pOther, pSelf) ||
        al::isMsgPlayerBodyAttackReflect(pMsg) || al::isMsgPlayerClimbAttack(pMsg) ||
        al::isMsgPlayerObjHipDropReflectAll(pMsg) || al::isMsgPlayerSpinAttack(pMsg) ||
        al::isMsgPlayerTailAttack(pMsg) || isSingleModeAttack) {
        rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        bool isDead = receiveDamage(pOther);
        bool isBlow = al::isMsgExplosion(pMsg) || al::isMsgPlayerClimbAttack(pMsg) ||
                      al::isMsgPlayerSpinAttack(pMsg) || al::isMsgPlayerTailAttack(pMsg);

        if (isDead) {
            if (isBlow || isSingleModeAttack) {
                al::setNerve(this, &NrvPunpunDie);
            } else {
                al::setNerve(this, &NrvPunpunDiePressDown);
            }
        } else if (isBlow || isSingleModeAttack) {
            al::setNerve(this, &NrvPunpunTailDown);
        } else {
            al::setNerve(this, &NrvPunpunPressDown);
        }

        return true;
    }

    return false;
}

/**
 * @brief Counts a hit, gives score and removes the clones and the shuriken.
 * @param pOther Attacking sensor.
 * @return Whether this was the last hit.
 */
bool Punpun::receiveDamage(al::HitSensor* pOther) {
    rc::addScore(this, pOther, 100.0f, mDamageCount);
    mLookController->stopLook();
    mDamageCount++;
    mFireBallHitCount = 0;
    mDividePatternIndex = 0;
    killAliveDivisions();

    if (al::isAlive(mShuriken)) {
        mShuriken->kill();
    }

    return mDamageCount > 2;
}

/**
 * @brief Lets the touch screen stop Punpun unless it is splitting.
 * @param pMsg Received message.
 * @param pPointer Screen pointer.
 * @param pTarget Touched target.
 * @return Whether the message was handled.
 */
bool Punpun::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                   al::ScreenPointTarget* pTarget) {
    if (isDividing()) {
        return false;
    }

    return al::isMsgTouchAssist(pMsg);
}

/**
 * @brief Pairs Punpun with Bunbun, which then drives the player during the demo.
 * @param pBunbun Partner boss.
 */
void Punpun::setBunbun(Bunbun* pBunbun) {
    mBunbun = pBunbun;
    mStateDemo->setIsControlPlayer(false);
}

/** @brief Plays the opening demo and starts the battle music afterwards. */
void Punpun::exeDemoAppear() {
    if (al::isFirstStep(this)) {
        al::invalidateShadow(mShuriken);
        if (mIsSingleMode) {
            rc::setDemoAudioType(this, alSeFunction::DemoType(3));
        }
    }

    if (mIsSingleMode) {
        al::pauseBgm(this, "Phase1", 60);
    }

    updateShurikenPose();

    if (al::isAlive(mShuriken) && al::isActionPlaying(mShuriken, sDemoParam.mActionName) &&
        static_cast<s32>(al::getActionFrame(mShuriken)) == 175) {
        al::validateShadow(mShuriken);
    }

    if (mStateDemo->isDemoFirstStep()) {
        mShuriken->appearDemo();
    }

    if (al::updateNerveStateAndNextNerve(this, &NrvPunpunHide)) {
        al::tryOnStageSwitch(this, "SwitchStartDemoEndOn");
        if (mIsSingleMode) {
            al::startBgm(this, "PunpunSingleMode", -1, 0, -1, -1);
            al::validateHitSensors(this);
        } else {
            al::startBgm(this, "Punpun", -1, 0, -1, -1);
        }

        al::invalidateShadow(mShuriken);
    }
}

/** @brief Keeps the shuriken in Punpun's right hand. */
void Punpun::updateShurikenPose() {
    al::updatePoseMtx(mShuriken, al::getJointMtxPtr(this, "WristR"));
}

/** @brief Waits a moment before the opening demo in single mode. */
void Punpun::exePreDemoAppearDelay() {
    if (al::isGreaterEqualStep(this, 20)) {
        al::setNerve(this, &NrvPunpunDemoAppear);
    }
}

/** @brief Flies (hidden) to the rail point farthest from the player. */
void Punpun::exeDivideMove() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "LightWait");
        al::invalidateShadow(this);
        al::invalidateHitSensors(this);

        if (al::getRailPointNum(this) == 1) {
            al::setRailPosToRailPoint(this, 0);
        } else {
            sead::Vector3f playerPos = al::findNearestPlayerPos(this);
            s32 pointIndex = -1;
            f32 farthestDistance = -1.0f;
            for (s32 i = 0; i < al::getRailPointNum(this); i++) {
                sead::Vector3f pointPos = {0.0f, 0.0f, 0.0f};
                al::calcRailPointPos(&pointPos, this, i);
                if (al::isNear(pointPos, al::getRailPos(this), 0.001f)) {
                    continue;
                }

                if (pointIndex < 0 || farthestDistance < (pointPos - playerPos).length()) {
                    farthestDistance = (pointPos - playerPos).length();
                    pointIndex = i;
                }
            }

            al::setRailPosToRailPoint(this, pointIndex);
        }

        al::setVelocityToDirection(this, al::getRailPos(this) - al::getTrans(this), 15.0f);
        killAliveDivisions();
    }

    if (al::isNear(al::getRailPos(this), al::getTrans(this), 0.001f) ||
        !al::isNearAngleRadian(al::getVelocity(this), al::getRailPos(this) - al::getTrans(this),
                               1.0f)) {
        al::setVelocityZero(this);
        al::resetPosition(this, al::getRailPos(this), false);
        al::setNerve(this, &NrvPunpunDivideWait);
    }
}

/** @brief Waits briefly at the new spot before splitting. */
void Punpun::exeDivideWait() {
    al::setNerveAtStep(this, &NrvPunpunDivide, 10);
}

/** @brief Splits into clones that spread around the start position. */
void Punpun::exeDivide() {
    if (al::isFirstStep(this)) {
        al::faceToDirection(this, mInitFront);
        s32 divisionNum = getDivisionNum();
        s32 posNum = divisionNum + 1;
        s32 offset = mIsUseOwnRandom ? mRandom.getU32(posNum) : al::getRandom(posNum);
        mDivideStartTrans.set(al::getTrans(this));

        sead::Vector3f dir = sead::Vector3f::ez;
        f32 angleStep = 360.0f / (posNum > 0 ? posNum : 1);
        if (mLevel != 0) {
            const sead::Vector3f& rPos =
                PunpunParam::getDividePos(mDamageCount, mDividePatternIndex, offset);
            mDivideTargetTrans.x = rPos.x + mDivideStartTrans.x;
            mDivideTargetTrans.y = rPos.y + mDivideStartTrans.y;
            mDivideTargetTrans.z = rPos.z + mDivideStartTrans.z;
        } else {
            if (posNum == 3) {
                dir = -sead::Vector3f::ez;
            }

            al::rotateVectorDegreeY(&dir, angleStep * offset);
            al::normalize(&dir);
            mDivideTargetTrans.x = mDivideDistance * dir.x + mDivideStartTrans.x;
            mDivideTargetTrans.y = mDivideDistance * dir.y + mDivideStartTrans.y;
            mDivideTargetTrans.z = mDivideDistance * dir.z + mDivideStartTrans.z;
        }

        for (s32 i = 0; i < divisionNum; i++) {
            PunpunDivision* pDivision = mDivisionGroup->getDeriveActor(i);
            pDivision->startDivide();

            sead::Vector3f target;
            if (mLevel != 0) {
                const sead::Vector3f& rPos = PunpunParam::getDividePos(
                    mDamageCount, mDividePatternIndex, al::modi(posNum + i + 1 + offset, posNum));
                target.x = rPos.x + mDivideStartTrans.x;
                target.y = rPos.y + mDivideStartTrans.y;
                target.z = rPos.z + mDivideStartTrans.z;
            } else {
                al::rotateVectorDegreeY(&dir, angleStep);
                al::normalize(&dir);
                target.x = mDivideDistance * dir.x + mDivideStartTrans.x;
                target.y = mDivideDistance * dir.y + mDivideStartTrans.y;
                target.z = mDivideDistance * dir.z + mDivideStartTrans.z;
            }

            pDivision->setDivideTargetTrans(target);
            al::faceToDirection(pDivision, mInitFront);
            al::resetPosition(pDivision, mDivideStartTrans, false);
        }

        s32 patternIndex = mDividePatternIndex;
        s32 patternNum = PunpunParam::getDividePosPatternNum(mDamageCount);
        mDividePatternIndex = al::modi(patternIndex + patternNum + 1, patternNum);
    }

    f32 rate = al::calcNerveSquareOutRate(this, 105);
    al::lerpVec(al::getTransPtr(this), mDivideStartTrans, mDivideTargetTrans, rate);
    for (s32 i = 0; i < getDivisionNum(); i++) {
        PunpunDivision* pDivision = mDivisionGroup->getDeriveActor(i);
        al::lerpVec(al::getTransPtr(pDivision), mDivideStartTrans,
                    pDivision->getDivideTargetTrans(), rate);
    }

    if (al::isGreaterEqualStep(this, 105)) {
        al::setNerve(this, &NrvPunpunDivideAppear);
        for (s32 i = 0; i < getDivisionNum(); i++) {
            mDivisionGroup->getDeriveActor(i)->startDivideAppear();
        }
    }
}

/** @brief Reappears with the shuriken in hand after splitting. */
void Punpun::exeDivideAppear() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Appear");
        al::validateShadow(this);
        al::validateHitSensors(this);
        mShuriken->appear();
        al::resetPosition(mShuriken, al::getTrans(this), false);
        al::validateShadow(mShuriken);
    }

    updateShurikenPose();

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvPunpunThrowWait);
    }
}

/** @brief Watches the player, then makes every clone start its throw. */
void Punpun::exeThrowWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "ThrowWait");
        al::startAction(mShuriken, "ThrowWait");
    }

    updateShurikenPose();
    mLookController->setLookAtNearestPlayer(-1.0f);

    if (al::isGreaterEqualStep(this, 30)) {
        for (s32 i = 0; i < getDivisionNum(); i++) {
            mDivisionGroup->getDeriveActor(i)->tryStartThrow();
        }

        al::setNerve(this, &NrvPunpunThrowSign);
        mLookController->stopLook();
    }
}

/** @brief Winds up the throw while turning towards the player. */
void Punpun::exeThrowSign() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "ThrowSign");
        al::startAction(mShuriken, "ThrowSign");
    }

    al::turnToTarget(this, al::findNearestPlayerPos(this), getThrowWaitTurnSpeed());
    updateShurikenPose();

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvPunpunThrow);
    }
}

/** @brief Throws the shuriken forwards. */
void Punpun::exeThrow() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Throw");
        sead::Vector3f front = {0.0f, 0.0f, 0.0f};
        al::calcFrontDir(&front, this);
        mShuriken->shoot(front, mLevel == 1);
    }

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvPunpunThrowEnd);
    }
}

/** @brief Recovers from the throw, removes the clones and hides again. */
void Punpun::exeThrowEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "ThrowEnd");
    }

    if (al::isActionPlaying(this, "ThrowEnd") && al::isActionEnd(this)) {
        al::startAction(this, "Wait");
    }

    mLookController->setLookAtNearestPlayer(-1.0f);

    if (al::isStep(this, 80)) {
        mDivisionGroup->killAll();
    }

    if (al::isGreaterEqualStep(this, 90)) {
        al::setNerve(this, &NrvPunpunHide);
        mLookController->stopLook();
    }
}

/** @brief Hides together with the shuriken before moving. */
void Punpun::exeHide() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Hide");
        al::startAction(mShuriken, "Hide");
    }

    if (al::isActionEnd(this)) {
        if (al::isAlive(mShuriken)) {
            mShuriken->kill();
        }

        al::setNerve(this, &NrvPunpunDivideMove);
    }
}

/** @brief Plays the knock-down animation matching the attack, then moves away. */
void Punpun::exePressDown() {
    if (al::isFirstStep(this)) {
        if (al::isNerve(this, &NrvPunpunFireBallDown)) {
            al::startAction(this, "FireBallDown");
        } else if (al::isNerve(this, &NrvPunpunTailDown)) {
            al::startAction(this, "TailDown");
        } else {
            al::startAction(this, "PressDown");
        }

        if (!al::isNerve(this, &NrvPunpunPressDown)) {
            al::faceToDirection(this, mInitFront);
        }
    }

    if (al::isStep(this, 4)) {
        al::startHitReactionPressDown(this);
    }

    al::setNerveAtActionEnd(this, &NrvPunpunDivideMove);
}

/** @brief Gets stomped for the last time and turns back to the initial front. */
void Punpun::exeDiePressDown() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "DiePressDown");
        sead::Vector3f front = {0.0f, 0.0f, 0.0f};
        al::calcFrontDir(&front, this);
        mDieTurnDegree = al::calcAngleOnPlaneDegree(mInitFront, front, -sead::Vector3f::ey);
        if (mDieTurnDegree < -135.0f) {
            mDieTurnDegree += 360.0f;
        }

        s32 turnFrames = al::getActionFrameMax(this, "DiePressDown") - 55.0f - 80.0f;
        mDieTurnSpeed = sead::Mathf::abs(al::normalize(1.0f, 0.0f, static_cast<f32>(turnFrames)) * mDieTurnDegree);

        if (mIsSingleMode) {
            al::stopBgm(this, "PunpunSingleMode", 20, -1);
        } else {
            al::stopBgm(this, "Punpun", -1, -1);
        }
    }

    if (al::isStep(this, 4)) {
        al::startHitReactionPressDown(this);
    }

    if (mIsSingleMode) {
        al::pauseOceanBgm(this, -1);
    }

    if (al::isGreaterEqualStep(this, 55)) {
        f32 degree = al::converge(mDieTurnDegree, 0.0f, mDieTurnSpeed);
        al::rotateQuatYDirDegree(this, mDieTurnDegree - degree);
        mDieTurnDegree = degree;
    }

    if (al::isActionEnd(this)) {
        kill();
    }
}

/** @brief Plays the normal defeat animation. */
void Punpun::exeDie() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Die");
        if (mIsSingleMode) {
            al::stopBgm(this, "PunpunSingleMode", 20, -1);
        } else {
            al::stopBgm(this, "Punpun", 20, -1);
        }

        al::faceToDirection(this, mInitFront);
    }

    if (mIsSingleMode) {
        al::pauseOceanBgm(this, -1);
    }

    if (al::isStep(this, 0)) {
        al::startHitReaction(this, "通常死亡");
    }

    if (al::isActionEnd(this)) {
        kill();
    }
}

/**
 * @brief Gets how fast Punpun turns towards the player before throwing.
 * @return Turn speed in degrees per frame.
 */
f32 Punpun::getThrowWaitTurnSpeed() {
    return 3.0f;
}
