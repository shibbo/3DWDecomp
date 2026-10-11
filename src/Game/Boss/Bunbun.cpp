#include "Boss/Bunbun.hpp"

#include "Boss/BunbunStateShellAttack.hpp"
#include "Boss/BunbunStateSpinAttack.hpp"
#include "Boss/GateKeeperStateDemo.hpp"
#include "Boss/Punpun.hpp"
#include "Enemy/ActorJointLookController.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/Joint/JointControllerKeeper.hpp"
#include "Library/LiveActor/ActorParamHolderUtil.hpp"
#include "Library/LiveActor/Util/ActorSceneInfo.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Movement/RumbleCalculator.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Play/Placement/PlacementHolder.hpp"
#include "Library/Play/Placement/PlacementInfo.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "MapObj/DisasterModeController.hpp"
#include "Project/Bgm/BgmPlayingRequest.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/DemoUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ScoreUtil.hpp"

namespace {
NERVE_DECL(Bunbun, Demo)
NERVE_DECL(Bunbun, ShellAttack)
NERVE_DECL(Bunbun, SpinAttack)
NERVE_DECL(Bunbun, MysteryBoxWait)
NERVE_DECL(Bunbun, Recover)
NERVE_DECL(Bunbun, Tired)
NERVE_DECL(Bunbun, RecoverSign)
NERVE_DECL(Bunbun, PressDown)
NERVE_DECL(Bunbun, Die)
NERVE_DECL(Bunbun, Down)
NERVE_DECL(Bunbun, Wait)
NERVE_DECL(Bunbun, Warp)
NERVE_DECL(Bunbun, DiePressDown)
NERVE_DECL(Bunbun, WarpWait)
NERVE_DECL(Bunbun, PreDemoDelay)

NERVES_MAKE_NOSTRUCT(Bunbun, Demo, ShellAttack, SpinAttack, MysteryBoxWait, Recover, Tired,
                     RecoverSign, PressDown, Die, Down, Wait, Warp, DiePressDown, WarpWait,
                     PreDemoDelay)

/** @brief Number of stomps Bunbun takes before it is defeated. */
constexpr s32 cHpMax = 3;

sead::Vector2f sLookLimit(180.0f, 180.0f);
sead::Vector2f sHeadLookRange(-20.0f, 20.0f);
sead::Vector2f sEyeLookRange(-5.0f, 5.0f);
GateKeeperStateDemoParam sDemoParam("DemoAppear", 0);
ActorJointLookControllerParam sHeadLookParam(2.0f, sHeadLookRange, false, nullptr, nullptr);
}  // namespace

/**
 * @brief Creates Bunbun with full health.
 * @param pName Actor name.
 */
Bunbun::Bunbun(const char* pName) : al::LiveActor(pName) {}

/**
 * @brief Initializes the model, the attack states, the linked Punpun, the warp points and the
 * opening demo.
 * @param rInfo Actor init info.
 */
void Bunbun::init(const al::ActorInitInfo& rInfo) {
    if (rInfo.getActorSceneInfo().isSingleMode) {
        al::initActorWithArchiveName(this, rInfo, "BunbunFur", nullptr);
    } else {
        al::initActor(this, rInfo);
    }

    al::tryGetArg(reinterpret_cast<s32*>(&mAttackType), rInfo, "AttackType");
    mIsSingleMode = rInfo.getActorSceneInfo().isSingleMode;

    s32 scenarioId = -1;
    al::tryGetArg(&scenarioId, rInfo, "ScenarioID");
    bool isAppear = true;
    bool isUseAnimCamera = true;
    if (scenarioId >= 0 && mIsSingleMode) {
        s32 zoneNo = mPlacementHolder->getZoneNo();
        isAppear = !SingleModeDataFunction::isScenarioComplete(GameDataHolderAccessor(this),
                                                               zoneNo - 1, scenarioId - 1);
        // Scenario battles in single mode play the demo through an RS camera ticket.
        isUseAnimCamera = !mIsSingleMode;
    }

    al::initNerve(this, &NrvBunbunDemo, 3);
    if (isUseAnimCamera) {
        mStateDemo =
            new GateKeeperStateDemo(this, rInfo, &sDemoParam, al::initAnimCamera(this, rInfo));
    } else {
        mStateDemo = new GateKeeperStateDemo(this, rInfo, &sDemoParam,
                                             al::initAnimCamera_RS(this, rInfo, "Anim"));
        mStateDemo->setIsCameraTicket();
    }

    mStateShellAttack = new BunbunStateShellAttack(this, rInfo);
    mStateSpinAttack = new BunbunStateSpinAttack(this, rInfo);
    al::initNerveState(this, mStateDemo, &NrvBunbunDemo, "[状態]開始デモ");
    al::initNerveState(this, mStateShellAttack, &NrvBunbunShellAttack, "[状態]コウラアタック");
    al::initNerveState(this, mStateSpinAttack, &NrvBunbunSpinAttack, "[状態]スピンアタック");

    if (al::calcLinkChildNum(rInfo, "Punpun") != 0) {
        mPunpun = new Punpun("プンプン");
        al::initLinksActor(mPunpun, rInfo, "Punpun", 0);
        mPunpun->setBunbun(this);
    }

    sead::Vector3f lookAtOffset = sead::Vector3f::zero;
    al::tryGetArg(&lookAtOffset.x, rInfo, "AnimLookAtOffsetX");
    al::tryGetArg(&lookAtOffset.y, rInfo, "AnimLookAtOffsetY");
    al::tryGetArg(&lookAtOffset.z, rInfo, "AnimLookAtOffsetZ");
    mStateDemo->setLookAtOffset(lookAtOffset);

    mIsUseMysteryBox = false;
    al::tryGetArg(&mIsUseMysteryBox, rInfo, "IsUseMysteryBox");
    if (mIsUseMysteryBox) {
        mHp = 1;
        al::setNerve(this, &NrvBunbunMysteryBoxWait);
    } else {
        al::hideModel(this);
    }

    if (mAttackType == AttackType_KouraThrow) {
        s32 pointNum = al::calcLinkChildNum(rInfo, "MovePoint");
        mWarpPointNum = pointNum;
        mWarpPoints = new sead::Vector3f[pointNum];
        for (s32 i = 0; i < mWarpPointNum; i++) {
            al::PlacementInfo placementInfo;
            al::getLinksInfoByIndex(&placementInfo, al::getPlacementInfo(rInfo), "MovePoint", i);
            al::tryGetTrans(&mWarpPoints[i], placementInfo);
        }
    }

    mRumble = new al::RumbleCalculatorCosMultLinear(2.5f, 2.0f, 0.1f, 30);
    mLookController = new ActorJointLookController(this, 3);
    al::initJointControllerKeeper(this, mLookController->mParams.capacity());
    mLookController->setLimit(sLookLimit);
    mLookController->appendJoint("Head", sead::Vector3f::ex, &sHeadLookParam);

    auto* pEyeParam = new ActorJointLookControllerParam(2.0f, sEyeLookRange, false, nullptr,
                                                        al::getJointMtxPtr(this, "Head"));
    mLookController->appendJoint("EyeL", sead::Vector3f::ey, pEyeParam);
    mLookController->appendJoint("EyeR", sead::Vector3f::ey, pEyeParam);

    if (!mIsUseMysteryBox && !mIsSingleMode) {
        mShellAttackCamera = al::initObjectCamera(this, rInfo, "ShellAttack");
    }

    mMoveParam = new BunbunMoveParam{al::findActorParamMove(this, "Wait")};

    if (isAppear) {
        al::trySyncStageSwitchAppear(this);
    } else {
        makeActorDead();
    }
}

/** @brief Kills Bunbun, turns on the dead switch and releases the camera look-at position. */
void Bunbun::kill() {
    al::tryOnSwitchDeadOn(this);
    al::setAdditionalCameraLookAtPosPtr(this, this, nullptr);
    al::LiveActor::kill();
}

/** @brief Updates the hit cooldowns, the squash rumble and the look controller. */
void Bunbun::control() {
    if (mExplosionHitCooldown > 0) {
        mExplosionHitCooldown--;
    }

    if (mKoopaJrHitCooldown > 0) {
        mKoopaJrHitCooldown--;
    }

    if (!mRumble->isEnd()) {
        mRumble->calc();
        f32 scale = mRumble->getValueY() + 1.0f;
        al::setScaleY(this, scale);
        mStateSpinAttack->setRumbleScale(scale);
    }

    mLookController->update();
}

/**
 * @brief Pushes and attacks the player (and KoopaJr) on contact.
 * @param pSelf Own sensor.
 * @param pOther Touched sensor.
 */
void Bunbun::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isSensorEnemyAttack(pSelf)) {
        al::sendMsgPush(pOther, pSelf);

        if (al::isNerve(this, &NrvBunbunShellAttack) && !mStateShellAttack->isEnableAttack()) {
            return;
        }

        if (al::isNerve(this, &NrvBunbunSpinAttack) && !mStateSpinAttack->isEnableAttack()) {
            return;
        }

        if (!isEnableAttack()) {
            return;
        }

        if (al::isSensorName(pSelf, "ArmL") || al::isSensorName(pSelf, "ArmR")) {
            if (!al::isNerve(this, &NrvBunbunRecover)) {
                return;
            }
        } else if (al::isNerve(this, &NrvBunbunTired) ||
                   al::isNerve(this, &NrvBunbunRecoverSign)) {
            return;
        }

        al::sendMsgEnemyAttackForCrossoverSensor(pOther, pSelf);
        return;
    }

    if (!mIsSingleMode || !isEnableAttack()) {
        return;
    }

    if (!al::isSensorNpc(pSelf) && !al::isSensorPlayer(pOther) && !al::isSensorKoopaJr(pOther)) {
        al::sendMsgEnemyAttackForCrossoverSensor(pOther, pSelf);
    }

    al::sendMsgPush(pOther, pSelf);
}

/**
 * @brief Checks whether Bunbun can attack or be attacked (not in the demo, not knocked down).
 * @return Whether Bunbun is in an attackable nerve.
 */
bool Bunbun::isEnableAttack() const {
    bool isNotDemo;
    if (mIsSingleMode) {
        isNotDemo = !al::isNerve(this, &NrvBunbunPreDemoDelay);
    } else {
        isNotDemo = !al::isNerve(this, &NrvBunbunDemo);
    }

    if (al::isNerve(this, &NrvBunbunPressDown) || al::isNerve(this, &NrvBunbunDown) ||
        al::isNerve(this, &NrvBunbunDiePressDown) || al::isNerve(this, &NrvBunbunDie)) {
        return false;
    }

    return isNotDemo;
}

/**
 * @brief Takes stomps, fire balls, explosions and attacks.
 * @param pMsg Received message.
 * @param pOther Sender sensor.
 * @param pSelf Own sensor.
 * @return Whether the message was handled.
 */
bool Bunbun::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) {
    if (!isEnableAttack()) {
        return false;
    }

    if (al::isNerve(this, &NrvBunbunShellAttack)) {
        return mStateShellAttack->receiveMsg(pMsg, pOther, pSelf);
    }

    if (al::isMsgPlayerTrampleForCrossoverSensor(pMsg, pOther, pSelf) ||
        al::isMsgPlayerObjHipDropReflectAll(pMsg) || al::isMsgPlayerBodyAttackReflect(pMsg)) {
        rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        startDown(pOther);
        al::setNerve(this, &NrvBunbunPressDown);
        return true;
    }

    if (al::isMsgExplosion(pMsg) && mExplosionHitCooldown <= 0) {
        mFireBallHitCount++;
        al::startMclAnim(this, "ReactionFireball");
        al::startSe(this, "PgFireBallHit");
        rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        if (!mStateSpinAttack->isDead()) {
            mStateSpinAttack->requestReactionAnim();
        }

        mExplosionHitCooldown = 30;
        if (mFireBallHitCount >= 3) {
            startDown(pOther);
            if (mHp <= 0) {
                al::setNerve(this, &NrvBunbunDie);
            } else {
                al::setNerve(this, &NrvBunbunDown);
            }
        }

        mRumble->start(0);
        return true;
    }

    bool isKoopaJrSpin =
        al::isMsgPlayerSpinAttack(pMsg) && al::isSensorHostName(pOther, "KoopaJr");
    if (al::isMsgPlayerBoomerangReflect(pMsg) || al::isMsgPlayerFireBallAttack(pMsg) ||
        isKoopaJrSpin) {
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
        if (!mStateSpinAttack->isDead()) {
            mStateSpinAttack->requestReactionAnim();
        }

        if (mFireBallHitCount >= 3) {
            startDown(pOther);
            if (mHp <= 0) {
                al::setNerve(this, &NrvBunbunDie);
            } else {
                al::setNerve(this, &NrvBunbunDown);
            }
        } else {
            al::startSe(this, "PgFireBallHit");
        }

        mRumble->start(0);
        return true;
    }

    bool isSingleModeAttack = false;
    if (mIsSingleMode) {
        isSingleModeAttack = al::isMsgPlayerKouraAttack(pMsg) || al::isMsgKickKouraAttack(pMsg) ||
                             al::isMsgPlayerOnlyInvincibleAttack(pMsg) ||
                             al::isMsgNekoAttack(pMsg) || al::isMsgBallAttack(pMsg);
        if (al::isMsgKeyThrow(pMsg)) {
            al::sendMsgPush(pOther, pSelf);
            isSingleModeAttack = true;
        }
    }

    if (al::isMsgPlayerClimbAttack(pMsg) || al::isMsgPlayerTailAttack(pMsg) ||
        al::isMsgPlayerSpinAttack(pMsg) || isSingleModeAttack) {
        rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        startDown(pOther);
        if (mHp <= 0) {
            al::setNerve(this, &NrvBunbunDie);
        } else {
            al::setNerve(this, &NrvBunbunDown);
        }

        return true;
    }

    return false;
}

/**
 * @brief Stops the spin attack, gives score and takes one hit point.
 * @param pOther Attacking sensor.
 */
void Bunbun::startDown(al::HitSensor* pOther) {
    if (mAttackType == AttackType_Spin) {
        al::showModelIfHide(this);
    }

    mStateSpinAttack->endAttack();
    rc::addScore(this, pOther, 100.0f, cHpMax - mHp);
    mHp--;
    mFireBallHitCount = 0;
}

/**
 * @brief Lets the touch screen stop Bunbun.
 * @param pMsg Received message.
 * @param pPointer Screen pointer.
 * @param pTarget Touched target.
 * @return Whether the message was handled.
 */
bool Bunbun::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                   al::ScreenPointTarget* pTarget) {
    return al::isMsgTouchAssist(pMsg);
}

/**
 * @brief Checks whether Bunbun may turn transparent (only the spinning type hides).
 * @return Whether Bunbun uses the spin attack.
 */
bool Bunbun::isEnableTransparent() const {
    return mAttackType == AttackType_Spin;
}

/**
 * @brief Checks whether Bunbun throws its shell.
 * @return Whether Bunbun uses the shell throw attack.
 */
bool Bunbun::isEnableKouraThrow() const {
    return mAttackType == AttackType_KouraThrow;
}

/**
 * @brief Checks whether the opening demo has started.
 * @return Whether the demo has started.
 */
bool Bunbun::isDemoStarted() {
    return mStateDemo->isDemoStarted();
}

/** @brief Plays the opening demo and starts the battle music afterwards. */
void Bunbun::exeDemo() {
    if (al::isFirstStep(this)) {
        setInvalidateClippingFlag();
        if (mIsSingleMode) {
            rc::setDemoAudioType(this, alSeFunction::DemoType(3));
        }
    }

    if (mIsSingleMode) {
        al::pauseBgm(this, "Phase1", 60);
    }

    if (al::updateNerveStateAndNextNerve(this, &NrvBunbunWait)) {
        if (!mIsUseMysteryBox) {
            if (mIsSingleMode) {
                al::startBgm(this, "BunbunSingleMode", -1, 0, -1, -1);
            } else {
                al::startBgm(this, "Bunbun", -1, 0, -1, -1);
            }
        }

        al::tryOnStageSwitch(this, "SwitchStartDemoEndOn");
        al::setAdditionalCameraLookAtPosPtr(this, this, al::getTransPtr(this));
    }
}

/** @brief Keeps Bunbun and its spin attack from being clipped. */
void Bunbun::setInvalidateClippingFlag() {
    al::invalidateClipping(this);
    mStateSpinAttack->setInvalidateClippingFlag();
}

/** @brief Waits a moment before the opening demo. */
void Bunbun::exePreDemoDelay() {
    if (al::isGreaterEqualStep(this, 20)) {
        al::setNerve(this, &NrvBunbunDemo);
    }
}

/** @brief Debug nerve: stands still and looks at the player. */
void Bunbun::exeDebug() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait");
    }

    mLookController->setLookAtNearestPlayer(-1.0f);
}

/** @brief Walks and turns towards the player using the "Wait" movement parameters. */
inline void Bunbun::walkAndTurnToPlayer() {
    const al::ActorParamMove* pMove = mMoveParam->mWaitMove;
    al::walkAndTurnToPlayer(this, pMove->moveAccel, pMove->gravity, pMove->moveFriction,
                            pMove->turnSpeedDegree, false);
}

/** @brief Watches the player for a moment before attacking. */
void Bunbun::exeWait() {
    if (al::isFirstStep(this)) {
        al::setVelocityZero(this);
        al::startAction(this, "Wait");
    }

    mLookController->setLookAtNearestPlayer(-1.0f);
    walkAndTurnToPlayer();

    if (al::isGreaterEqualStep(this, 5)) {
        al::setNerve(this, &NrvBunbunSpinAttack);
    }
}

/** @brief Starts attacking right away after coming out of a mystery box. */
void Bunbun::exeMysteryBoxWait() {
    setInvalidateClippingFlag();
    al::setAdditionalCameraLookAtPosPtr(this, this, al::getTransPtr(this));
    al::setNerve(this, &NrvBunbunSpinAttack);
}

/** @brief Runs the spin attack state, then gets tired. */
void Bunbun::exeSpinAttack() {
    if (al::isFirstStep(this)) {
        mLookController->stopLook();
    }

    al::updateNerveStateAndNextNerve(this, &NrvBunbunTired);
}

/** @brief Stays dizzy for a while after attacking. */
void Bunbun::exeTired() {
    if (al::isFirstStep(this)) {
        mLookController->stopLook();
        al::startAction(this, "Tired");
    }

    walkAndTurnToPlayer();

    if (al::isGreaterEqualStep(this, 150)) {
        al::setNerve(this, &NrvBunbunRecoverSign);
    }
}

/** @brief Shakes off the dizziness while looking at the player. */
void Bunbun::exeRecoverSign() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "RecoverSign");
    }

    mLookController->setLookAtNearestPlayer(-1.0f);
    walkAndTurnToPlayer();

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvBunbunRecover);
    }
}

/** @brief Recovers, then attacks again (or warps away for the shell throwing type). */
void Bunbun::exeRecover() {
    if (al::isFirstStep(this)) {
        switch (mAttackType) {
        case AttackType_Spin:
            al::startAction(this, "Recover");
            break;
        case AttackType_KouraThrow:
            al::startAction(this, "RecoverPose");
            break;
        }
    }

    mLookController->stopLook();
    walkAndTurnToPlayer();

    if (al::isActionEnd(this)) {
        switch (mAttackType) {
        case AttackType_Spin:
            al::setNerve(this, &NrvBunbunSpinAttack);
            break;
        case AttackType_KouraThrow:
            al::setNerve(this, &NrvBunbunWarp);
            break;
        }
    }
}

/** @brief Gets stomped, then retreats into the shell (or dies on the last hit). */
void Bunbun::exePressDown() {
    if (al::isFirstStep(this)) {
        mLookController->stopLook();
        al::setVelocityZero(this);
        al::startAction(this, "PressDown");

        if (mHp <= 0 && !mIsUseMysteryBox) {
            if (mIsSingleMode) {
                al::stopBgm(this, "BunbunSingleMode", 20, -1);
            } else {
                al::stopBgm(this, "Bunbun", 20, -1);
            }
        }
    }

    if (mIsSingleMode) {
        al::pauseOceanBgm(this, -1);
    }

    if (al::isStep(this, 4)) {
        al::startHitReactionHit(this);
    }

    walkAndTurnToPlayer();

    if (al::isActionEnd(this)) {
        if (mHp <= 0) {
            al::setNerve(this, &NrvBunbunDiePressDown);
        } else {
            al::setNerve(this, &NrvBunbunShellAttack);
        }
    }
}

/** @brief Gets knocked down by fire balls, then retreats into the shell. */
void Bunbun::exeDown() {
    if (al::isFirstStep(this)) {
        mLookController->stopLook();
        al::setVelocityZero(this);
        al::startAction(this, "FireBallDown");
    }

    if (al::isStep(this, 4)) {
        al::startHitReactionHit(this);
    }

    walkAndTurnToPlayer();

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvBunbunShellAttack);
    }
}

/** @brief Gets stomped for the last time and starts the after-battle music. */
void Bunbun::exeDiePressDown() {
    if (al::isFirstStep(this)) {
        mLookController->stopLook();
        al::setVelocityZero(this);
        turnToCameraDir();
        al::startAction(this, "DiePressDown");

        if (!mIsUseMysteryBox && mIsSingleMode) {
            al::stopBgm(this, "BunbunSingleMode", 20, -1);
            al::tryOnStageSwitch(this, "SwitchDeadImmediateOn");
        }
    }

    if (al::isStep(this, 4)) {
        al::startHitReactionHit(this);
    }

    if (mIsSingleMode) {
        al::pauseOceanBgm(this, -1);
    }

    walkAndTurnToPlayer();

    if (al::isActionEnd(this)) {
        if (!mIsUseMysteryBox) {
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

        kill();
    }
}

/** @brief Turns Bunbun to face the camera. */
void Bunbun::turnToCameraDir() {
    sead::Vector3f cameraDir;
    al::calcCameraDir(&cameraDir, this);
    al::turnToDirection(this, cameraDir, 180.0f);
}

/** @brief Plays the normal defeat animation and starts the after-battle music. */
void Bunbun::exeDie() {
    if (al::isFirstStep(this)) {
        mLookController->stopLook();
        al::setVelocityZero(this);
        turnToCameraDir();
        al::startAction(this, "Die");

        if (!mIsUseMysteryBox) {
            if (mIsSingleMode) {
                al::stopBgm(this, "BunbunSingleMode", 20, -1);
                al::tryOnStageSwitch(this, "SwitchDeadImmediateOn");
            } else {
                al::stopBgm(this, "Bunbun", 20, -1);
            }
        }
    }

    if (mIsSingleMode) {
        al::pauseBgm(this, "Phase1", 60);
    }

    if (al::isStep(this, 4)) {
        al::startHitReactionHit(this);
    }

    walkAndTurnToPlayer();

    if (al::isActionEnd(this)) {
        if (!mIsUseMysteryBox) {
            if (mIsSingleMode) {
                DisasterModeController* pController = DisasterModeController::tryGetController(this);
                al::BgmPlayingRequest request("AfterBattleSingleMode");
                if (pController != nullptr && pController->isDisasterMode()) {
                    request._18 = 163300;
                }

                al::startBgm(this, request);
                al::resumeBgm(this, "Phase1", -1);
            } else {
                al::startBgm(this, "AfterBattle", -1, 0, -1, -1);
            }
        }

        kill();
    }
}

/** @brief Runs the shell attack state with its own camera, then attacks again or warps. */
void Bunbun::exeShellAttack() {
    if (al::isFirstStep(this) && !mIsSingleMode) {
        al::startCamera(this, mShellAttackCamera, -1);
    }

    if (al::updateNerveState(this)) {
        if (!mIsSingleMode) {
            al::endCamera(this, mShellAttackCamera, -1);
        }

        switch (mAttackType) {
        case AttackType_Spin:
            al::setNerve(this, &NrvBunbunSpinAttack);
            break;
        case AttackType_KouraThrow:
            al::setNerve(this, &NrvBunbunWarp);
            break;
        }
    }
}

/** @brief Disappears and flies to the move point farthest from the player. */
void Bunbun::exeWarp() {
    if (al::isFirstStep(this)) {
        al::hideModelIfShow(this);
        al::invalidateHitSensors(this);
        al::startHitReactionDisappear(this);
        mLookController->stopLook();

        f32 farthestDistance;
        for (s32 i = 0; i < mWarpPointNum; i++) {
            f32 distance = (mWarpPoints[i] - al::findNearestPlayerPos(this)).length();
            if (i == 0 || farthestDistance < distance) {
                farthestDistance = distance;
                mWarpPointIndex = i;
            }
        }

        mWarpStartTrans = al::getTrans(this);
        s32 frame = (mWarpPoints[mWarpPointIndex] - mWarpStartTrans).length() / 15.0f;
        mWarpFrame = frame > 1 ? frame : 1;
        al::startAction(this, "LightWait");
        turnToCameraDir();
    }

    sead::Vector3f trans = sead::Vector3f::zero;
    al::lerpVec(&trans, mWarpStartTrans, mWarpPoints[mWarpPointIndex],
                static_cast<f32>(al::getNerveStep(this)) / mWarpFrame);
    al::setTrans(this, trans);

    if (al::isGreaterEqualStep(this, mWarpFrame)) {
        mStateSpinAttack->setKouraPos();
        al::setNerve(this, &NrvBunbunWarpWait);
    }
}

/** @brief Reappears with its shell at the new move point, then attacks again. */
void Bunbun::exeWarpWait() {
    if (al::isFirstStep(this)) {
        al::startHitReactionAppear(this);
        al::showModelIfHide(this);
        al::validateHitSensors(this);
        mStateSpinAttack->appearKoura();
        al::startAction(this, "Appear");
    }

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvBunbunSpinAttack);
    }
}

/** @brief Destroys Bunbun. */
Bunbun::~Bunbun() = default;
