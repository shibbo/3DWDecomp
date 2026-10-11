#include "Player/Player.hpp"

#include <math/seadMathCalcCommon.h>
#include <prim/seadSafeString.h>

#include "Library/Math/MathUtil.hpp"
#include "Player/Giga/PlayerActionGraph.hpp"
#include "Player/Giga/PlayerActionGraphBuilder.hpp"
#include "Player/Giga/PlayerActionGraphRestarter.hpp"
#include "Player/IUsePlayerAnimator.hpp"
#include "Player/IUsePlayerCheckArea.hpp"
#include "Player/IUsePlayerCollision.hpp"
#include "Player/IUsePlayerCollisionCheckedObserver.hpp"
#include "Player/IUsePlayerEffect.hpp"
#include "Player/IUsePlayerEventReceiver.hpp"
#include "Player/IUsePlayerFlag.hpp"
#include "Player/Normal/PlayerAmiiboDirector.hpp"
#include "Player/Normal/PlayerClimbAirAttackInhibitor.hpp"
#include "Player/Normal/PlayerConstParam.hpp"
#include "Player/Normal/PlayerCounterAfterPunch.hpp"
#include "Player/Normal/PlayerEquipmentDirector.hpp"
#include "Player/Normal/PlayerFigureDirector.hpp"
#include "Player/Normal/PlayerGiantDirector.hpp"
#include "Player/Normal/PlayerGigaDirector.hpp"
#include "Player/Normal/PlayerInvincibleState.hpp"
#include "Player/Normal/PlayerKiller.hpp"
#include "Player/Normal/PlayerProperty.hpp"
#include "Player/Normal/PlayerSpinJumpChecker.hpp"
#include "Player/Normal/PlayerSwimSquatInhibitor.hpp"
#include "Player/Normal/PlayerTrigger.hpp"
#include "Player/Normal/SinkSandControl.hpp"
#include "Player/PlayerActionFunc.hpp"
#include "Player/PlayerActualMove.hpp"
#include "Player/PlayerAirTurnChecker.hpp"
#include "Player/PlayerBindControl.hpp"
#include "Player/PlayerCeilingCheck.hpp"
#include "Player/PlayerCollisionFunc.hpp"
#include "Player/PlayerCollisionSize.hpp"
#include "Player/PlayerContinuousJump.hpp"
#include "Player/PlayerDamageControl.hpp"
#include "Player/PlayerDamageInvalidater.hpp"
#include "Player/PlayerFigureChangeObserver.hpp"
#include "Player/PlayerFlightDurationInhibitor.hpp"
#include "Player/PlayerForwardBent.hpp"
#include "Player/PlayerGlideInhibitor.hpp"
#include "Player/PlayerHeightChecker.hpp"
#include "Player/PlayerHorizontalSpeedAverage.hpp"
#include "Player/PlayerInkChecker.hpp"
#include "Player/PlayerInvincibleDash.hpp"
#include "Player/PlayerLandingChecker.hpp"
#include "Player/PlayerLandingInformer.hpp"
#include "Player/PlayerLifeControl.hpp"
#include "Player/PlayerLongFallCheck.hpp"
#include "Player/PlayerPropellerInhibitor.hpp"
#include "Player/PlayerRaccoonDogFallTask.hpp"
#include "Player/PlayerSimpleFlag.hpp"
#include "Player/PlayerSizeTrigger.hpp"
#include "Player/PlayerSubAction.hpp"
#include "Player/PlayerSuperDashResetter.hpp"
#include "Player/PlayerWallJumpInfo.hpp"

namespace {
/// Sensor trigger set when the player's punch hits something.
constexpr PlayerTrigger::ESensorTrigger cTriggerPunchHit =
    static_cast<PlayerTrigger::ESensorTrigger>(4);
/// Collision trigger set when the player's punch hits a wall.
constexpr PlayerTrigger::ECollisionTrigger cTriggerPunchWall =
    static_cast<PlayerTrigger::ECollisionTrigger>(2);
/// Effect index of the giga cat's climbing effects.
constexpr s32 cGigaClimbEffectIndex = 9;
}  // namespace

/**
 * @brief Creates the player's action logic and all of its helpers.
 * @param pBuilder Creates the action graph.
 * @param pInput The player's controller input.
 * @param pInputArranger Arranges the input to the camera.
 * @param pAnimator The player's animations.
 * @param pEffect The player's effects.
 * @param pAudio The player's sounds.
 * @param pBind Binds the player to objects (pipes, cannons, ...).
 * @param pCheckArea Checks the areas the player is in.
 * @param pCollision The player's collision.
 * @param pCollisionPartsArray The collision parts the player touches.
 * @param pSnapWallInfo The wall the player snaps to.
 * @param pCheckSphere Collision sphere checks.
 * @param pCheckSphereMove Moving collision sphere checks.
 * @param pCheckArrow Collision arrow checks.
 * @param pFireBallLauncher Launches fire balls.
 * @param pBoomerangLauncher Launches boomerangs.
 * @param pAttack The player's attack sensors.
 * @param pGetPos The player's position.
 * @param pBindEndParamGetter Parameters of the end of a bind.
 * @param pCollisionPartsMtx Unused.
 * @param pWaterFlowField The water flow the player is in.
 * @param pReaction The player's reactions.
 * @param pWaterSurfaceInfo The water surface the player is near.
 * @param pStoneStatue The statue (Tanooki statue) state.
 * @param pHipDropObserver Gets told about hip drops.
 * @param pEventReceiver Receives the player's events.
 * @param pCharaQuery Which character the player is.
 * @param pHolded Being held by someone.
 * @param pWallClimbInfo The wall the player climbs.
 * @param pFlag1 A player flag.
 * @param pFlag2 A player flag.
 * @param pFlag3 A player flag.
 * @param pFlag4 A player flag.
 * @param pWallClimbInput Wall climbing input.
 * @param pPushed Being pushed.
 * @param pDoubleMarioSpeed Speed of the double cherry clones.
 * @param pDoubleMarioCheck Whether this is a double cherry clone.
 * @param pSensorControl The player's sensors.
 * @param pFlag5 A player flag.
 * @param pClimbFlag Keeps the climbing collision size while on.
 * @param pFlagControl Switches player flags.
 * @param pMoveSpeedScaler Scales the move speed.
 * @param pFlag6 A player flag.
 * @param pFlag7 A player flag.
 * @param pGetPosConst The player's position (read only).
 * @param pClimbJumpInfo Climb jump parameters.
 * @param pConstParam The player's tuning values.
 * @param isGigaValid Whether the giga form exists (Bowser's Fury).
 * @param pActor The player's actor.
 */
Player::Player(
    IUsePlayerActionGraphBuilder* pBuilder, IUsePlayerInput* pInput,
    IUsePlayerInputArranger* pInputArranger, IUsePlayerAnimator* pAnimator,
    IUsePlayerEffect* pEffect, IUsePlayerAudio* pAudio, IUsePlayerBind* pBind,
    IUsePlayerCheckArea* pCheckArea, IUsePlayerCollision* pCollision,
    IUsePlayerCollisionPartsArray* pCollisionPartsArray, IUsePlayerSnapWallInfo* pSnapWallInfo,
    IUsePlayerCollisionCheckSphere* pCheckSphere,
    IUsePlayerCollisionCheckSphereMove* pCheckSphereMove,
    IUsePlayerCollisionCheckArrow* pCheckArrow, IUsePlayerFireBallLauncher* pFireBallLauncher,
    IUsePlayerFireBallLauncher* pBoomerangLauncher, IUsePlayerAttack* pAttack,
    IUsePlayerGetPos* pGetPos, IUsePlayerBindEndParamGetter* pBindEndParamGetter,
    IUseCollisionPartsMtx* pCollisionPartsMtx, IUsePlayerWaterFlowField* pWaterFlowField,
    IUsePlayerReaction* pReaction, const IUsePlayerWaterSurfaceInfo* pWaterSurfaceInfo,
    IUsePlayerStoneStatue* pStoneStatue, IUsePlayerHipDropObserver* pHipDropObserver,
    IUsePlayerEventReceiver* pEventReceiver, const IUsePlayerCharaQuery* pCharaQuery,
    IUsePlayerHolded* pHolded, const IUsePlayerWallClimbInfo* pWallClimbInfo,
    const IUsePlayerFlag* pFlag1, const IUsePlayerFlag* pFlag2, const IUsePlayerFlag* pFlag3,
    const IUsePlayerFlag* pFlag4, IUsePlayerWallClimbInput* pWallClimbInput,
    const IUsePlayerPushed* pPushed, const IUsePlayerDoubleMarioSpeed* pDoubleMarioSpeed,
    const IUsePlayerDoubleMarioCheck* pDoubleMarioCheck, IUsePlayerSensorControl* pSensorControl,
    const IUsePlayerFlag* pFlag5, const IUsePlayerFlag* pClimbFlag,
    IUsePlayerFlagControl* pFlagControl, const IUsePlayerMoveSpeedScaler* pMoveSpeedScaler,
    const IUsePlayerFlag* pFlag6, const IUsePlayerFlag* pFlag7,
    const IUsePlayerGetPos* pGetPosConst, const IUsePlayerClimbJumpInfo* pClimbJumpInfo,
    const PlayerConstParam* pConstParam, bool isGigaValid, PlayerActor* pActor)
    : mProperty(new PlayerProperty()), mCheckSphere(pCheckSphere), mCheckArrow(pCheckArrow),
      mInput(pInput), mInputArranger(pInputArranger), mCollision(pCollision),
      mCollisionPartsArray(pCollisionPartsArray), mSnapWallInfo(pSnapWallInfo),
      mCollisionSize(nullptr), mAnimator(pAnimator), mTrigger(new PlayerTrigger()),
      mAudio(pAudio), mBindControl(nullptr), mCheckArea(pCheckArea),
      mHeightChecker(new PlayerHeightChecker(pConstParam)),
      mDamageInvalidater(new PlayerDamageInvalidater()), _80(false),
      mFigureDirector(new PlayerFigureDirector(pAudio, isGigaValid)),
      mFigureChangeObserver(nullptr), mKiller(nullptr), mActionGraph(nullptr), mBind(pBind),
      mLifeControl(nullptr),
      mInvincibleState(new PlayerInvincibleState(pAnimator, pEffect, mDamageInvalidater,
                                                 mFigureDirector, pConstParam, pEventReceiver)),
      mSubAction(nullptr), mGetPos(pGetPos), mEffect(pEffect), mIsActionShifted(false),
      mCeilingCheck(
          new PlayerCeilingCheck(pCheckSphereMove, mProperty, mFigureDirector, pConstParam)),
      mBindEndParamGetter(pBindEndParamGetter), mLandingChecker(nullptr),
      mContinuousJump(new PlayerContinuousJump(pConstParam)), mReaction(pReaction),
      mAirTurnChecker(new PlayerAirTurnChecker()), mDamageControl(nullptr),
      mInvincibleDash(new PlayerInvincibleDash()), mRestarter(nullptr),
      mCollisionCheckedObserver(nullptr), mWaterFlowField(pWaterFlowField),
      mWaterSurfaceInfo(pWaterSurfaceInfo), mForwardBent(nullptr),
      mWallJumpInfo(new PlayerWallJumpInfo()),
      mHorizontalSpeedAverage(new PlayerHorizontalSpeedAverage(pConstParam)),
      mSizeTrigger(nullptr), mEquipmentDirector(new PlayerEquipmentDirector()),
      mStoneStatue(pStoneStatue), mHipDropObserver(pHipDropObserver),
      mRaccoonDogFallTask(new PlayerRaccoonDogFallTask()), mFlag180(new PlayerSimpleFlag),
      mLongFallCheck(new PlayerLongFallCheck()),
      mPropellerInhibitor(new PlayerPropellerInhibitor()),
      mActualMove(new PlayerActualMove(mProperty)),
      mLandingInformer(new PlayerLandingInformer()),
      mGlideInhibitor(new PlayerGlideInhibitor()),
      mCounterAfterPunch(new PlayerCounterAfterPunch()), mPropellerJumpPhase(nullptr),
      mAttack(pAttack), mSwimSquatInhibitor(new PlayerSwimSquatInhibitor(pConstParam)),
      mEventReceiver(pEventReceiver), mCharaQuery(pCharaQuery), mHolded(pHolded),
      mWallClimbInfo(pWallClimbInfo), mFlag1f0(new PlayerSimpleFlag),
      mFlag1f8(new PlayerSimpleFlag), mSinkSandFlag(new PlayerSimpleFlag),
      mDashChecker(nullptr), mFlag1(pFlag1), mFlag2(pFlag2), mFlag3(pFlag3), mFlag4(pFlag4),
      mWallClimbInput(pWallClimbInput),
      mSinkSandControl(new SinkSandControl(pCheckArea, mSinkSandFlag, pConstParam, mProperty)),
      mInkChecker(
          new PlayerInkChecker(pActor, mProperty, pConstParam, mSinkSandFlag, isGigaValid)),
      mPushed(pPushed), mDoubleMarioSpeed(pDoubleMarioSpeed),
      mDoubleMarioCheck(pDoubleMarioCheck),
      mClimbAirAttackInhibitor(new PlayerClimbAirAttackInhibitor(pConstParam)),
      mFlightDurationInhibitor(new PlayerFlightDurationInhibitor(mEquipmentDirector)),
      mSensorControl(pSensorControl), mSpinJumpChecker(new PlayerSpinJumpChecker(pInput)),
      mGiantDirector(new PlayerGiantDirector(mProperty, pConstParam, pEventReceiver)),
      mGigaDirector(nullptr), mAmiiboDirector(nullptr),
      mSuperDashResetter(new PlayerSuperDashResetter()), mFlag5(pFlag5), mKnockDown(nullptr),
      mFlag2b0(new PlayerSimpleFlag), mClimbFlag(pClimbFlag), mFlagControl(pFlagControl),
      mFlag2c8(new PlayerSimpleFlag), mMoveSpeedScaler(pMoveSpeedScaler), mFlag6(pFlag6),
      mFlag7(pFlag7), mGetPosConst(pGetPosConst), mClimbJumpInfo(pClimbJumpInfo),
      mConstParam(pConstParam), mActor(pActor), mIsFirstUpdate(true),
      mIsValidCeilingCheck(true), mIsGigaValid(isGigaValid), _310(nullptr), _318(0),
      mUpdateCount(0) {
    mHeightChecker->setCollisionCheckArrow(pCheckArrow);

    PlayerCollisionSize* collisionSize = new PlayerCollisionSize(mCollision);
    mCollisionSize = collisionSize;
    mSizeTrigger = new PlayerSizeTrigger(mFigureDirector, collisionSize);
    mLandingChecker = new PlayerLandingChecker(mCollision);

    if (isGigaValid) {
        mGigaDirector = new PlayerGigaDirector(mCollision, mInput, mProperty, pEventReceiver,
                                               mFigureDirector, mSinkSandFlag);
    }

    mLifeControl = new PlayerLifeControl(mFigureDirector, mDamageInvalidater, pAudio, pConstParam,
                                         pEventReceiver, pDoubleMarioCheck, mGigaDirector);
    mSubAction = new PlayerSubAction(pInput, pAnimator, pAudio, pEffect, mFigureDirector,
                                     mCharaQuery, pFireBallLauncher, pBoomerangLauncher, pAttack,
                                     mCeilingCheck, mEquipmentDirector, pClimbFlag, mCollision,
                                     pConstParam, pEventReceiver, mGiantDirector, mGigaDirector,
                                     pActor);
    mForwardBent = new PlayerForwardBent(mProperty, mWaterFlowField, mFigureDirector, pConstParam);

    mActionGraph = pBuilder->create(this);
    mContinuousJump->setActionGraph(mActionGraph);
    mDamageControl = new PlayerDamageControl(mActionGraph, mTrigger, mEquipmentDirector,
                                             mDamageInvalidater, mEffect, mLifeControl);
    mInvincibleDash->init(mActionGraph, mInvincibleState, mInput, mProperty, pConstParam);
    mLongFallCheck->init(mProperty, mActionGraph, mCollision, pConstParam);

    mLandingInformer->appendObserver(mRaccoonDogFallTask);
    mPropellerInhibitor->setActionGraph(mActionGraph);
    mLandingInformer->appendObserver(mPropellerInhibitor);
    mLandingInformer->appendObserver(mWallJumpInfo);
    mLandingInformer->appendObserver(mFlightDurationInhibitor);
    mLandingInformer->setLandingChecker(mLandingChecker);
    mLandingInformer->setTrigger(mTrigger);
    mLandingInformer->setActionGraph(mActionGraph);
    mLandingInformer->setEventReceiver(mEventReceiver);
    mHorizontalSpeedAverage->setProperty(mProperty);
    mHorizontalSpeedAverage->setCollision(mCollision);

    mFigureChangeObserver = new PlayerFigureChangeObserver;
    mFigureDirector->setChangeObserver(mFigureChangeObserver);
}

/**
 * @brief Initializes the action graph.
 */
void Player::init() {
    mActionGraph->init();
}

/**
 * @brief Runs one frame of the player: updates the helpers, shifts and moves the current action.
 */
void Player::update() {
    mIsActionShifted = false;
    mGiantDirector->update();

    if (mGigaDirector != nullptr) {
        mGigaDirector->update();
    }

    if (mAmiiboDirector != nullptr) {
        mAmiiboDirector->update();
    }

    mSpinJumpChecker->update();
    mSwimSquatInhibitor->update();
    mClimbAirAttackInhibitor->update();
    mHeightChecker->update(mProperty->getTrans(), mProperty->getGroundUp());
    mContinuousJump->update();
    mDamageInvalidater->update();
    mFigureDirector->update();
    updateHelpMario();
    updateCollisionSize();

    if (mBindControl != nullptr) {
        mBindControl->update();
    }

    mSizeTrigger->update();
    mDamageControl->update();
    mIsActionShifted |= mActionGraph->checkShift();

    if (!mIsFirstUpdate && !mIsActionShifted) {
        al::isNear(sead::Mathf::abs(mProperty->getFront().y), 1.0f, 0.1f);
        mActionGraph->checkShiftCondition();
        mIsActionShifted |= mActionGraph->checkShift();
    }

    mIsFirstUpdate = false;

    if (mIsActionShifted) {
        mEventReceiver->onChangeAction();
    }

    mLongFallCheck->update();
    mSuperDashResetter->update(mProperty, mActionGraph);

    if (mSinkSandControl->isInSinkSand() &&
        (mGigaDirector == nullptr || !mGigaDirector->isGiga())) {
        f32 sinkHeight = mSinkSandControl->getSurfaceHeight() - mProperty->getTrans().y;
        mCollision->push({0.0f, sinkHeight, 0.0f});
    }

    mActionGraph->move();
    mSinkSandControl->update(mProperty->getTrans());
    mInkChecker->update();
    mTrigger->clearCollisionTrigger();
    notifyCollisionCheckedObserver();
    mGlideInhibitor->update(mTrigger);
    mCounterAfterPunch->update(mTrigger);
    mActualMove->calc();
    mSubAction->update();

    if (mIsValidCeilingCheck) {
        mCeilingCheck->update();
    }

    arrangeVelocity();
    mInvincibleDash->update();
    mForwardBent->update();
    mHorizontalSpeedAverage->update();
    mLandingChecker->update();
    mLandingInformer->update();
    mFlightDurationInhibitor->update();
    mActionGraph->update();
    mInvincibleState->update();
    updateEffect();
    updateSound();
    mTrigger->clearSensorTrigger();
    mFigureChangeObserver->clear();
    mUpdateCount++;
}

/**
 * @brief Makes the white (helper) forms invincible when the figure changes.
 */
void Player::updateHelpMario() {
    if (!mFigureChangeObserver->isChanged()) {
        return;
    }

    if (mFigureDirector->getFigure() == EPlayerFigure::RaccoonDogWhite) {
        mDamageInvalidater->invalidateForHelp();
    } else if (mFigureDirector->getFigure() == EPlayerFigure::ClimbWhite) {
        mDamageInvalidater->invalidateForHelp();
    } else {
        mDamageInvalidater->validateForHelp();
    }
}

/**
 * @brief Fits the collision size to the figure and to the cat's climbing animations.
 */
void Player::updateCollisionSize() {
    if (mFigureChangeObserver->isChanged()) {
        if (mFigureDirector->getFigure() == EPlayerFigure::Mini) {
            mCollisionSize->changeShort();
        } else {
            mCollisionSize->changeSuper();
        }
    }

    if (!PlayerActionFunc::isClimb(mFigureDirector) || mClimbFlag->isOn()) {
        mCollisionSize->changeClimbSuper();
        return;
    }

    if (mAnimator->isAnim("Move") || mAnimator->isAnim("TurnPoint") ||
        mAnimator->isAnim("Wait") || mAnimator->isAnim("DashSign") ||
        mAnimator->isAnim("Brake") || mAnimator->isAnim("Turn") ||
        mAnimator->isAnim("SquatEnd") || mAnimator->isAnim("Grooming")) {
        mCollisionSize->changeClimbShort();
        return;
    }

    if (PlayerActionFunc::isClimbGiga(mFigureDirector)) {
        if (mAnimator->isAnim("GigaMove") || mAnimator->isAnim("GigaTurnPoint") ||
            mAnimator->isAnim("GigaWait") || mAnimator->isAnim("GigaDashSign") ||
            mAnimator->isAnim("GigaBrake") || mAnimator->isAnim("GigaTurn") ||
            mAnimator->isAnim("GigaSquatEnd") || mAnimator->isAnim("GigaGrooming")) {
            mCollisionSize->changeClimbShort();
            return;
        }
    }

    if (mCeilingCheck->hasSpaceToStandUp()) {
        mCollisionSize->changeClimbSuper();
    }
}

/**
 * @brief Tells the observer that the collision was checked this frame.
 */
void Player::notifyCollisionCheckedObserver() {
    if (mCollisionCheckedObserver != nullptr) {
        mCollisionCheckedObserver->notifyCollisionChecked();
    }
}

/**
 * @brief Cuts the velocity against the collision and bounces off ceilings and punched walls.
 */
void Player::arrangeVelocity() {
    sead::Vector3f velocity = mProperty->getVelocity();
    sead::Vector3f up = mProperty->getUpDir();
    mCollision->cutVelocity(_80 ? 5 : 1);

    if (mTrigger->isOn(cTriggerPunchHit) || mTrigger->isOn(cTriggerPunchWall)) {
        reflectCeiling(mConstParam->getPunchReflectPower());
    } else {
        f32 upSpeed = velocity.dot(up);
        bool isOnCeiling = mCollision->isOnCeiling();

        if (!(upSpeed > 0.0f && isOnCeiling)) {
            return;
        }

        reflectCeiling(0.0f);
    }

    s32 inhibitFrame = mConstParam->getGlideInhibitFrameAfterPunch();
    mFlightDurationInhibitor->inhibitForAShortTime(inhibitFrame);
    mGlideInhibitor->requestInhibit(inhibitFrame);
    mEventReceiver->onPunchHit();
}

/**
 * @brief Keeps the white tanooki glow and the giga cat's climbing effects in sync with the figure.
 */
void Player::updateEffect() {
    EPlayerFigure figure = mFigureDirector->getFigure();
    bool isEmitting = mEffect->isEmitting("RaccoonDogWhite");

    if (figure == EPlayerFigure::ClimbWhite || figure == EPlayerFigure::RaccoonDogWhite) {
        if (!isEmitting) {
            mEffect->emitEffect("RaccoonDogWhite");
        }
    } else if (isEmitting) {
        mEffect->stopEffect("RaccoonDogWhite");
    }

    if (!mIsGigaValid) {
        return;
    }

    figure = mFigureDirector->getFigure();
    isEmitting = mEffect->isEmittingIdx("GigaClimbMane", cGigaClimbEffectIndex);

    if (figure == EPlayerFigure::ClimbGiga) {
        if (!isEmitting) {
            mEffect->emitEffectIdx("GigaClimbMane", cGigaClimbEffectIndex);
            mEffect->emitEffectSubActor("GigaClimbTail", cGigaClimbEffectIndex, "尻尾");
        }
    } else if (isEmitting) {
        mEffect->stopEffectIdx("GigaClimbMane", cGigaClimbEffectIndex);
        mEffect->stopEffectSubActor("GigaClimbTail", cGigaClimbEffectIndex, "尻尾");
    }
}

/**
 * @brief Checks for a waterfall at the player's body center (the result is unused).
 */
void Player::updateSound() {
    const PlayerProperty* property = mProperty;
    sead::Vector3f pos = property->getTrans();
    EPlayerFigure figure = mFigureDirector->getFigure();
    f32 tall = PlayerCollisionFunc::calcTall(property, mConstParam);
    sead::Vector3f offset = property->getGroundUp() * tall;

    if (figure == EPlayerFigure::Mini) {
        offset *= 0.5f;
    }

    pos += offset;
    mCheckArea->isInWaterFall(pos);
}

/**
 * @brief Sets the collision sphere checks.
 * @param pCheckSphere The new collision sphere checks.
 */
void Player::setCollisionCheckSphere(IUsePlayerCollisionCheckSphere* pCheckSphere) {
    mCheckSphere = pCheckSphere;
}

/**
 * @brief Sets the collision arrow checks (also used by the height checker).
 * @param pCheckArrow The new collision arrow checks.
 */
void Player::setCollisionCheckArrow(IUsePlayerCollisionCheckArrow* pCheckArrow) {
    mCheckArrow = pCheckArrow;
    mHeightChecker->setCollisionCheckArrow(pCheckArrow);
}

/**
 * @brief Sets who gets told about the damage the player takes.
 * @param pObserver The damage observer.
 */
void Player::setDamageObserver(IUsePlayerDamageObserver* pObserver) {
    mDamageControl->setDamageObserver(pObserver);
}

/**
 * @brief Stops checking the space above the player.
 */
void Player::invalidateCeilingCheck() {
    mIsValidCeilingCheck = false;
    mCeilingCheck->clear();
}

/**
 * @brief Gets who gets told about the player's hip drops.
 * @return The hip drop observer.
 */
IUsePlayerHipDropObserver* Player::getHipDropObserver() {
    return mHipDropObserver;
}

/**
 * @brief Gets the player's height above the ground.
 * @return The height checker.
 */
IUsePlayerHeightChecker* Player::getHeightChecker() {
    return mHeightChecker;
}

/**
 * @brief Gets the reasons the player can't take damage.
 * @return The damage invalidater.
 */
IUsePlayerDamageInvalidCheck* Player::getDamageInvalidater() const {
    return mDamageInvalidater;
}

/**
 * @brief Gets the actions that run on top of the main one.
 * @return The sub action.
 */
IUsePlayerSubAction* Player::getSubAction() {
    return mSubAction;
}

/**
 * @brief Gets the check for room above the player.
 * @return The ceiling check.
 */
IUsePlayerCeilingCheck* Player::getCeilingCheck() {
    return mCeilingCheck;
}

/**
 * @brief Gets whether the player turns in mid-air.
 * @return The air turn checker.
 */
const IUsePlayerAirTurnCheck* Player::getAirTurnChecker() const {
    return mAirTurnChecker;
}

/**
 * @brief Gets who gets told about the player's mid-air turns.
 * @return The air turn checker's observer side.
 */
IUsePlayerAirTurnObserver* Player::getAirTurnObserver() {
    return mAirTurnChecker;
}

/**
 * @brief Gets the dash while the player is invincible.
 * @return The invincible dash.
 */
const IUsePlayerInvincibleDash* Player::getInvincibleDash() const {
    return mInvincibleDash;
}

/**
 * @brief Gets the water flow the player is in.
 * @return The water flow field.
 */
const IUsePlayerWaterFlowField* Player::getWaterFlowField() const {
    return mWaterFlowField;
}

/**
 * @brief Gets the water surface the player is near.
 * @return The water surface info.
 */
const IUsePlayerWaterSurfaceInfo* Player::getWaterSurfaceInfo() const {
    return mWaterSurfaceInfo;
}

/**
 * @brief Gets the player's forward bend while running.
 * @return The forward bent.
 */
IUsePlayerForwardBent* Player::getForwardBent() {
    return mForwardBent;
}

/**
 * @brief Gets the player's averaged horizontal speed.
 * @return The horizontal speed average.
 */
IUsePlayerHorizontalSpeedAverage* Player::getHorizontalSpeedAverage() {
    return mHorizontalSpeedAverage;
}

/**
 * @brief Gets the triggers of the player's size changes.
 * @return The size trigger.
 */
const IUsePlayerSizeTrigger* Player::getSizeTrigger() const {
    return mSizeTrigger;
}

/**
 * @brief Gets the tanooki slow fall state.
 * @return The raccoon dog fall task.
 */
IUsePlayerRaccoonDogFallTask* Player::getRaccoonDogFallTask() {
    return mRaccoonDogFallTask;
}

/**
 * @brief Gets whether the player has been falling for long.
 * @return The long fall check.
 */
IUsePlayerLongFallCheck* Player::getLongFallCheck() {
    return mLongFallCheck;
}

/**
 * @brief Gets what blocks the propeller box's flight.
 * @return The propeller inhibitor.
 */
IUsePlayerPropellerInhibitor* Player::getPropellerInhibitor() {
    return mPropellerInhibitor;
}

/**
 * @brief Gets how far the player really moved in the last frame.
 * @return The actual move.
 */
const IUsePlayerActualMove* Player::getActualMove() const {
    return mActualMove;
}

/**
 * @brief Gets what blocks gliding.
 * @return The glide inhibitor.
 */
IUsePlayerGlideInhibitor* Player::getGlideInhibitor() {
    return mGlideInhibitor;
}

/**
 * @brief Creates the player's amiibo scanning.
 * @param pActor The player's actor.
 * @param pWatcher Watches all the players' amiibo directors.
 * @param rInfo The actor init info.
 */
void Player::createAmiiboDirector(const PlayerActor* pActor, PlayerAmiiboDirectorWatcher* pWatcher,
                                  const al::ActorInitInfo& rInfo) {
    mAmiiboDirector = new PlayerAmiiboDirector(pActor, pWatcher, rInfo);
}

/**
 * @brief Takes over another player's amiibo scanning.
 * @param pDirector The amiibo director.
 * @param pActor The player's actor.
 */
void Player::setAmiiboDirector(PlayerAmiiboDirector* pDirector, const PlayerActor* pActor) {
    mAmiiboDirector = pDirector;
    pDirector->setPlayerActor(pActor);
}

/**
 * @brief Tests whether the player stands on the floor.
 * @return True if on the floor.
 */
bool Player::isOnGround() const {
    return mCollision->isOnFloor();
}

/**
 * @brief Creates the control of the player's binds.
 * @param pActionGraph The player's action graph.
 * @param pEndBind Gets told when a bind ends.
 * @param pBindNode The bind action's node.
 */
void Player::createBindControl(PlayerActionGraph* pActionGraph, IUsePlayerEndBind* pEndBind,
                               PlayerActionNode* pBindNode) {
    mBindControl = new PlayerBindControl(mBind, pEndBind, pActionGraph, pBindNode);
}

/**
 * @brief Creates what kills the player.
 * @param pActionGraph The player's action graph.
 * @param pDieNode The die action's node.
 */
void Player::createPlayerKiller(PlayerActionGraph* pActionGraph, PlayerActionNode* pDieNode) {
    mKiller = new PlayerKiller(pActionGraph, pDieNode);
}

/**
 * @brief Creates what restarts the action graph.
 * @param pActionGraph The player's action graph.
 * @param pRestartNode The node to restart from.
 */
void Player::createRestarter(PlayerActionGraph* pActionGraph, PlayerActionNode* pRestartNode) {
    PlayerActionGraphRestarter* restarter = new PlayerActionGraphRestarter();
    mRestarter = restarter;
    restarter->setActionGraph(pActionGraph);
    restarter->setRestartNode(pRestartNode);
    restarter->setLandingChecker(mLandingChecker);
}

/**
 * @brief Sets the phase of the propeller jump.
 * @param pPropellerJumpPhase The propeller jump phase.
 */
void Player::createPropellerJumpPhase(IUsePlayerPropellerJumpPhase* pPropellerJumpPhase) {
    mPropellerJumpPhase = pPropellerJumpPhase;
}

/**
 * @brief Sets the dash checker.
 * @param pDashChecker The dash checker.
 */
void Player::createDashChecker(IUsePlayerDashChecker* pDashChecker) {
    mDashChecker = pDashChecker;
}

/**
 * @brief Sets the knock down action.
 * @param pKnockDown The knock down action.
 */
void Player::createKnockDownSetter(PlayerActionKnockDown* pKnockDown) {
    mKnockDown = pKnockDown;
}

/**
 * @brief Gives the giant form the actions it shifts to.
 * @param pActionGraph The player's action graph.
 * @param pStartNode The node played when the giant form starts.
 * @param pEndNode The node played when the giant form ends.
 */
void Player::createGiantAction(PlayerActionGraph* pActionGraph, PlayerActionNode* pStartNode,
                               PlayerActionNode* pEndNode) {
    if (mGiantDirector != nullptr) {
        mGiantDirector->setAction(pActionGraph, pStartNode, pEndNode);
    }
}

/**
 * @brief Gives the giga form the actions it shifts to.
 * @param pActionGraph The player's action graph.
 * @param pStartNode The node played when the giga form starts.
 * @param pEndNode The node played when the giga form ends.
 * @param pClimbStartNode The node played when the giga form starts while climbing.
 */
void Player::createGigaAction(PlayerActionGraph* pActionGraph, PlayerActionNode* pStartNode,
                              PlayerActionNode* pEndNode, PlayerActionNode* pClimbStartNode) {
    if (mGigaDirector != nullptr) {
        mGigaDirector->setAction(pActionGraph, pStartNode, pEndNode, pClimbStartNode);
    }
}

/**
 * @brief Bounces the player off a ceiling: drops the upward velocity and pushes it down.
 * @param power How hard the player is pushed along the gravity.
 */
void Player::reflectCeiling(f32 power) {
    al::verticalizeVec(&mProperty->mVelocity, mProperty->mGravity, mProperty->mVelocity);
    mProperty->mVelocity += mProperty->mGravity * power;

    if (mAnimator->isAnim("Jump")) {
        mAnimator->setAnimFrame(8.0f);
    } else if (mAnimator->isAnim("Jump2")) {
        mAnimator->setAnimFrame(8.0f);
    } else if (mAnimator->isAnim("Jump3")) {
        mAnimator->setAnimFrame(8.0f);
    }

    mAnimator->clearInterpolation();
}
