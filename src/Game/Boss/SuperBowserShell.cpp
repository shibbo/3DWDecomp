#include "Boss/SuperBowserShell.hpp"

#include <cmath>
#include <gfx/seadTextWriter.h>
#include <gfx/seadViewport.h>
#include <math/seadMathCalcCommon.h>
#include <math/seadQuat.h>

#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/Controller/PadRumbleFunction.hpp"
#include "Library/Framework/GameFrameworkNx.hpp"
#include "Library/Joint/JointControllerKeeper.hpp"
#include "Library/LiveActor/Util/ActorAnimUtil.hpp"
#include "Library/LiveActor/Util/ActorResourceUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Screen/ScreenFunction.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Yaml/ByamlIter.hpp"
#include "MapObj/DisasterModeController.hpp"
#include "Project/Effect/Core/EffectKeeper.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/DemoUtil.hpp"
#include "Util/PlayerUtil.hpp"

namespace {
NERVE_DECL(SuperBowserShell, NoOp)
NERVE_DECL(SuperBowserShell, GrowOverTime0)
NERVE_DECL(SuperBowserShell, GrowOverTime1)
NERVE_DECL(SuperBowserShell, GrowOverTime2)
NERVE_DECL(SuperBowserShell, GrowOverTime3)
NERVE_DECL(SuperBowserShell, GrowOverTime4)
NERVE_DECL(SuperBowserShell, EarlyRise)
NERVE_DECL(SuperBowserShell, Spin)
NERVE_DECL(SuperBowserShell, Launch)

NERVES_MAKE_NOSTRUCT(SuperBowserShell, NoOp, GrowOverTime0, GrowOverTime1, GrowOverTime2,
                     GrowOverTime3, GrowOverTime4, EarlyRise, Spin, Launch)

/** @brief Frames of peace between two disasters. */
constexpr s32 cPeaceFrames = 18000;
/** @brief Longest a single rising step may last. */
constexpr s32 cStepFramesMax = 3600;
}  // namespace

/**
 * @brief Constructs the Black Sun.
 * @param pName Actor name.
 */
SuperBowserShell::SuperBowserShell(const char* pName) : al::LiveActor(pName) {}

/**
 * @brief Initializes the model, the CoG joint controllers and the step timings, and decides
 * whether the shell has to rise early on this save.
 * @param rInfo Actor init info.
 */
void SuperBowserShell::init(const al::ActorInitInfo& rInfo) {
    al::initNerve(this, &NrvSuperBowserShellNoOp, 0);
    al::initActorWithArchiveName(this, rInfo, "BlackSun", nullptr);
    makeActorAppeared();
    al::onDrawClipping(this);
    initFromYaml();
    al::startAction(this, "WaitPhase00");
    al::tryGetArg(&mModelOffset.y, rInfo, "ModelOffset");
    al::initJointControllerKeeper(this, 3);
    al::initJointLocalAxisRotator_RS(this, sead::Vector3f::ey, &mCoGRotation, "CoG", true);
    al::initJointLocalTransController(this, &mCoGJointPos, "CoG");
    al::initJointLocalTransController(this, &mModelOffset, "AllRoot");
    mRainFrames = 1200;

    s32 phase = SingleModeDataFunction::getUnlockedPhase(GameDataHolderAccessor(this));
    bool isFirstVisit = false;
    if (phase == 1) {
        isFirstVisit = !SingleModeDataFunction::hasSeenCutscene(GameDataHolderAccessor(this), 1);
    }

    if (phase == 5) {
        mRainFrames = 600;
    }

    bool isFinalVisit = false;
    if (phase == 8) {
        isFinalVisit = !SingleModeDataFunction::hasSeenCutscene(GameDataHolderAccessor(this), 29);
    }

    if (isFirstVisit || isFinalVisit) {
        mIsEarlyRiseNext = true;
    }

    al::validateHitSensors(this);
}

/**
 * @brief Reads the tuning parameters from the "InitBlackSun" actor init file.
 */
void SuperBowserShell::initFromYaml() {
    al::ByamlIter fileIter;
    al::ByamlIter iter;
    if (!al::tryGetActorInitFileIter(&fileIter, this, "InitBlackSun", nullptr)) {
        return;
    }

    fileIter.tryGetIterByKey(&iter, "Black Sun");
    iter.tryGetFloatByKey(&mAmbientSpinSpeed, "Ambient Spin Speed");
    iter.tryGetIntByKey(&mLaunchFrames, "Launch Frames");
    iter.tryGetIntByKey(&mFloatFrames, "Float Frames");
    iter.tryGetFloatByKey(&mFloatHeight, "Float Height");
    iter.tryGetFloatByKey(&mCoGBaseHeightYStart, "CoG Base Height Y Start");
    iter.tryGetFloatByKey(&mCoGBaseHeightYEnd, "CoG Base Height Y End");
    iter.tryGetFloatByKey(&mCoGBaseHeightYMaxSpeed, "CoG Base Height Y Max Speeds");
    iter.tryGetFloatByKey(&mCoGBaseHeightYEarlyRiseRate, "CoG Base Height Y Early Rise Rate");
    iter.tryGetFloatByKey(&mForeshadowSpinSpeed, "Foreshadow Spin Speed");
    iter.tryGetFloatByKey(&mForeshadowSpinSpeedRain, "Foreshadow Spin Speed Rain");
    iter.tryGetIntByKey(&mForeshadowSpinAccelFrames, "Foreshadow Spin Accel Frames");
    iter.tryGetIntByKey(&mForeshadowSpinDecelFrames, "Foreshadow Spin Decel Frames");
    iter.tryGetFloatByKey(&mRevSpinSpeed, "Rev Spin Speed");
    iter.tryGetFloatByKey(&mRevSpinAccel, "Rev Spin Accel");
    iter.tryGetIntByKey(&mRevSpinFrames, "Rev Spin Frames");
    iter.tryGetIntByKey(&mRevSpinMaxSpeedFrame, "Rev Spin Max Speed Frame");
    iter.tryGetIntByKey(&mRevEndFrame, "Rev End Frame");
}

/**
 * @brief Pushes the player, Bowser Jr. and Plessie out of the shell's body.
 * @param pSelf Own sensor.
 * @param pOther Touched sensor.
 */
void SuperBowserShell::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (!al::isSensorEnemyBody(pSelf)) {
        return;
    }

    if (al::isSensorPlayer(pOther) || al::isSensorKoopaJr(pOther) || al::isSensorPlessie(pOther)) {
        al::sendMsgPushStrong(pOther, pSelf);
    }
}

/**
 * @brief Step 0: the shell is hidden below the lake, appears and starts changing to step 1.
 */
void SuperBowserShell::exeGrowOverTime0() {
    s32 frames = calculatePhaseFrames(0);
    if (al::isFirstStep(this)) {
        if (rc::isPlayerInCloudBonus(this)) {
            al::setNerve(this, &NrvSuperBowserShellGrowOverTime0);
            return;
        }

        al::showModelIfHide(this);
        al::startAction(this, "WaitPhase00");
        recalculatePhaseChangeFrames();
        if (mIsSynced) {
            if (frames > mAppearFrame && frames < mChangePhase01Frame) {
                al::startAction(this, "Appear");
                al::setActionFrame(this, frames - mAppearFrame);
            } else if (frames > mChangePhase01Frame) {
                al::startAction(this, "ChangePhase01");
                al::setActionFrame(this, frames - mChangePhase01Frame);
            }
        }
    }

    if (updateFreeze(frames >= mAppearFrame)) {
        return;
    }

    if (frames >= mAppearFrame && frames < mChangePhase01Frame &&
        !al::isActionPlaying(this, "Appear")) {
        al::startAction(this, "Appear");
    }

    if (frames == mChangePhase01Frame) {
        al::startAction(this, "ChangePhase01");
    }

    if (frames >= mChangePhase01Frame + 1800) {
        al::setNerve(this, &NrvSuperBowserShellGrowOverTime1);
    }

    updateAmbientSpin();
    updateCoGBaseHeightY(!mIsSynced);
    mIsSynced = false;
}

/**
 * @brief Counts the frames elapsed in a rising step.
 * @param phase Step index (0 to 4).
 * @return The frames elapsed since the start of the step, or -1 if unavailable.
 */
s32 SuperBowserShell::calculatePhaseFrames(s32 phase) const {
    DisasterModeController* controller = DisasterModeController::tryGetController(this);
    if (controller == nullptr) {
        return -1;
    }

    switch (phase) {
    case 0:
        return controller->getDisasterFrames();
    case 1:
        return controller->getDisasterFrames() - mStep0Frames;
    case 2:
        return controller->getDisasterFrames() - mStep0Frames - mStepFrames;
    case 3:
        return controller->getDisasterFrames() - mStep0Frames - mStepFrames * 2;
    case 4:
        return controller->getDisasterFrames() - mStep0Frames - mStepFrames * 3;
    default:
        return -1;
    }
}

/**
 * @brief Splits the frames left before the disaster between the five rising steps.
 */
void SuperBowserShell::recalculatePhaseChangeFrames() {
    DisasterModeController* controller = DisasterModeController::tryGetController(this);
    s32 peaceFramesLeft = cPeaceFrames - controller->getPeaceFrames();
    s32 preRainFrames = controller->getPreRainFrames();
    s32 framesLeft = sead::Mathi::max(peaceFramesLeft, 0);
    s32 step0Frames = 0;
    s32 stepFrames = 0;
    s32 step4Frames = 0;
    s32 leftoverFrames = 0;
    if (framesLeft < preRainFrames) {
        mWaitSkipFrames = framesLeft;
    } else {
        mWaitSkipFrames = preRainFrames;
        framesLeft -= preRainFrames;
        if (framesLeft / 5 <= 1200) {
            step0Frames = framesLeft / 5;
            stepFrames = framesLeft / 5;
            step4Frames = framesLeft / 5;
        } else {
            step0Frames = 1200;
            framesLeft -= 1200;
            step4Frames = cStepFramesMax - mRainFrames;
            if (framesLeft / 4 <= step4Frames) {
                stepFrames = framesLeft / 4;
                step4Frames = framesLeft / 4;
            } else {
                framesLeft -= step4Frames;
                if (framesLeft / 3 <= 3000) {
                    stepFrames = framesLeft / 3;
                } else {
                    stepFrames = 3000;
                    leftoverFrames = framesLeft - 9000;
                }
            }
        }
    }

    mAppearFrame = 1200 - step0Frames;
    mChangePhase01Frame = 1800 - step0Frames;
    mChangePhaseFrame = 3000 - stepFrames;
    mLeftoverFrames = leftoverFrames;
    mStep0Frames = cStepFramesMax - step0Frames;
    mStepFrames = cStepFramesMax - stepFrames;
    mStep4Frames = cStepFramesMax - step4Frames;
}

/**
 * @brief Freezes the shell's animation while the disaster timer does not advance.
 * @param isActive Whether the current animation may be frozen.
 * @return True if the shell is frozen.
 */
bool SuperBowserShell::updateFreeze(bool isActive) {
    DisasterModeController* controller = DisasterModeController::tryGetController(this);
    if (controller == nullptr) {
        return false;
    }

    if (controller->isTimeStopped()) {
        return true;
    }

    bool isFrozen = true;
    if (controller->getDisasterFramesOffset() >=
            controller->calcProsperityStartAdditionalFramesMax() &&
        !controller->isBlackSunFloating() && !rc::isPlayerInCloudBonus(this) &&
        !rc::isActiveDemo(this)) {
        GameDataHolderAccessor accessor(this);
        isFrozen = SingleModeDataFunction::getDisasterModePostBossPeaceFrames(accessor) > 0;
    }

    bool isPlaying = !(isFrozen && isActive);
    al::setActionFrameRate(this, isPlaying);
    return isFrozen;
}

/**
 * @brief Turns the shell by its ambient spin speed.
 */
void SuperBowserShell::updateAmbientSpin() {
    rotateByDegrees(mAmbientSpinSpeed);
}

/**
 * @brief Moves the CoG height towards the height matching the disaster timer.
 * @param isLimitSpeed Whether to ease towards the target height at a limited speed.
 */
void SuperBowserShell::updateCoGBaseHeightY(bool isLimitSpeed) {
    DisasterModeController* controller = DisasterModeController::tryGetController(this);
    if (controller == nullptr) {
        return;
    }

    f32 rate;
    if (mIsEarlyRise) {
        s32 disasterFrames = controller->calcDisasterFrames();
        s32 prosperityFrames = controller->calcFramesOfProsperity();
        s32 additionalFrames = controller->calcProsperityStartAdditionalFramesMax();
        f32 riseRate = static_cast<f32>(disasterFrames) /
                       static_cast<f32>(additionalFrames + prosperityFrames - mRainFrames);
        rate = al::lerpValue(riseRate, mCoGBaseHeightYEarlyRiseRate, 1.0f);
    } else {
        s32 appearFrame = mChangePhase01Frame;
        s32 prosperityFrames = controller->calcFramesOfProsperity();
        rate = static_cast<f32>(controller->getDisasterFrames() - appearFrame) /
               static_cast<f32>(prosperityFrames - appearFrame - mRainFrames);
    }

    rate = sead::Mathf::clamp(rate, 0.0f, 1.0f);
    f32 height = al::lerpValue(rate, mCoGBaseHeightYStart, mCoGBaseHeightYEnd);
    if (isLimitSpeed) {
        f32 delta = al::lerpValue(0.1f, mCoGBaseHeightY, height) - mCoGBaseHeightY;
        if (delta > mCoGBaseHeightYMaxSpeed) {
            delta = mCoGBaseHeightYMaxSpeed;
        }

        height = mCoGBaseHeightY + delta;
    }

    mCoGBaseHeightY = height;
    updateCoGJointPosition();
}

/**
 * @brief Step 1: the shell waits, then starts changing to step 2.
 */
void SuperBowserShell::exeGrowOverTime1() {
    s32 frames = calculatePhaseFrames(1);
    if (al::isFirstStep(this)) {
        if (rc::isPlayerInCloudBonus(this)) {
            al::setNerve(this, &NrvSuperBowserShellGrowOverTime1);
            return;
        }

        al::showModelIfHide(this);
        al::startAction(this, "WaitPhase01");
        al::tryStartSeWithParam(this, "RaiseOverTime", 0.0f);
        recalculatePhaseChangeFrames();
        if (frames > mChangePhaseFrame && mIsSynced) {
            al::startAction(this, "ChangePhase02");
            al::setActionFrame(this, frames - mChangePhaseFrame);
        }
    }

    if (updateFreeze(true)) {
        return;
    }

    if (frames == mChangePhaseFrame) {
        al::startAction(this, "ChangePhase02");
    }

    if (frames >= mChangePhaseFrame + 600) {
        al::setNerve(this, &NrvSuperBowserShellGrowOverTime2);
    }

    updateAmbientSpin();
    updateCoGBaseHeightY(!mIsSynced);
    mIsSynced = false;
}

/**
 * @brief Step 2: the shell waits, then starts changing to step 3.
 */
void SuperBowserShell::exeGrowOverTime2() {
    s32 frames = calculatePhaseFrames(2);
    if (al::isFirstStep(this)) {
        if (rc::isPlayerInCloudBonus(this)) {
            al::setNerve(this, &NrvSuperBowserShellGrowOverTime2);
            return;
        }

        al::showModelIfHide(this);
        al::startAction(this, "WaitPhase02");
        al::tryStartSeWithParam(this, "RaiseOverTime", 1.0f);
        recalculatePhaseChangeFrames();
        if (frames > mChangePhaseFrame && mIsSynced) {
            al::startAction(this, "ChangePhase03");
            al::setActionFrame(this, frames - mChangePhaseFrame);
        }
    }

    if (updateFreeze(true)) {
        return;
    }

    if (frames == mChangePhaseFrame) {
        al::startAction(this, "ChangePhase03");
    }

    if (frames >= mChangePhaseFrame + 600) {
        al::setNerve(this, &NrvSuperBowserShellGrowOverTime3);
    }

    updateAmbientSpin();
    updateCoGBaseHeightY(!mIsSynced);
    mIsSynced = false;
}

/**
 * @brief Step 3: the shell waits, then starts changing to step 4.
 */
void SuperBowserShell::exeGrowOverTime3() {
    s32 frames = calculatePhaseFrames(3);
    if (al::isFirstStep(this)) {
        if (rc::isPlayerInCloudBonus(this)) {
            al::setNerve(this, &NrvSuperBowserShellGrowOverTime3);
            return;
        }

        al::showModelIfHide(this);
        al::startAction(this, "WaitPhase03");
        al::tryStartSeWithParam(this, "RaiseOverTime", 2.0f);
        recalculatePhaseChangeFrames();
        if (frames > mChangePhaseFrame && mIsSynced) {
            al::startAction(this, "ChangePhase04");
            al::setActionFrame(this, frames - mChangePhaseFrame);
        }
    }

    if (updateFreeze(true)) {
        return;
    }

    if (frames == mChangePhaseFrame) {
        al::startAction(this, "ChangePhase04");
    }

    if (frames >= mChangePhaseFrame + 600) {
        al::setNerve(this, &NrvSuperBowserShellGrowOverTime4);
    }

    updateAmbientSpin();
    updateCoGBaseHeightY(!mIsSynced);
    mIsSynced = false;
}

/**
 * @brief Step 4: the shell glows and spins faster, and starts the rain shortly before the
 * disaster.
 */
void SuperBowserShell::exeGrowOverTime4() {
    if (al::isFirstStep(this)) {
        if (rc::isPlayerInCloudBonus(this)) {
            al::setNerve(this, &NrvSuperBowserShellGrowOverTime4);
            return;
        }

        al::showModelIfHide(this);
        al::startAction(this, "StartEmission");
        mIsEmissionStarted = true;
        mForeshadowSpinSpeedCurrent = mForeshadowSpinSpeed;
    }

    if (mRumbleTimer > 0 && --mRumbleTimer == 0) {
        alPadRumbleFunction::stopPadRumbleLoop(this, "SuperShell", al::getTransPtr(this), -1);
    }

    if (updateFreeze(true)) {
        return;
    }

    if (al::isActionPlaying(this, "StartEmission") && al::isActionEnd(this)) {
        al::startAction(this, "WaitPhase04");
        al::tryStartSeWithParam(this, "RaiseOverTime", 3.0f);
    }

    if (calculatePhaseFrames(4) >= mStep4Frames - mRainFrames) {
        DisasterModeController* controller = DisasterModeController::tryGetController(this);
        if (controller != nullptr && !controller->isDisasterForeshadow()) {
            controller->triggerAnticipationSwitch();
            controller->startRain();
            alPadRumbleFunction::startPadRumbleLoopNo3D(this, "SuperShell", al::getTransPtr(this),
                                                        -1, false);
            mRumbleTimer = 60;
        }
    }

    updateForeshadowSpin();
    updateCoGBaseHeightY(!mIsSynced);
    mIsSynced = false;
}

/**
 * @brief Speeds the spin up at the start of step 4, keeps it up (floating the shell) and slows
 * it down again before the disaster.
 */
void SuperBowserShell::updateForeshadowSpin() {
    s32 frames = calculatePhaseFrames(4);
    f32 maxSpeed = frames < mStep4Frames - mRainFrames ? mForeshadowSpinSpeed :
                                                          mForeshadowSpinSpeedRain;
    if (maxSpeed > mForeshadowSpinSpeedCurrent) {
        mForeshadowSpinSpeedCurrent =
            sead::Mathf::min(mForeshadowSpinSpeedCurrent + 0.03f, maxSpeed);
    }

    if (frames <= mForeshadowSpinAccelFrames) {
        f32 rate = al::easeIn(static_cast<f32>(frames) /
                              static_cast<f32>(mForeshadowSpinAccelFrames));
        rotateByDegrees(al::lerpValue(rate, mAmbientSpinSpeed, mForeshadowSpinSpeedCurrent));
        return;
    }

    s32 decelStartFrame = mStep4Frames - mForeshadowSpinDecelFrames;
    if (frames < decelStartFrame) {
        rotateByDegrees(mForeshadowSpinSpeedCurrent);
        if (mFloatFrames - mFloatFrame < decelStartFrame - frames) {
            updateFloat();
        }

        return;
    }

    f32 rate = sead::Mathf::clamp(static_cast<f32>(frames - mForeshadowSpinDecelFrames) /
                                      static_cast<f32>(mForeshadowSpinDecelFrames),
                                  0.0f, 1.0f);
    rotateByDegrees((1.0f - al::easeOut(rate)) * mForeshadowSpinSpeedCurrent);
}

/**
 * @brief Spins the shell up in reverse before launching Fury Bowser.
 */
void SuperBowserShell::exeSpin() {
    if (al::isFirstStep(this)) {
        alPadRumbleFunction::startPadRumbleLoopNo3D(this, "SuperShell", al::getTransPtr(this), -1,
                                                    false);
        al::startAction(this, "Launch");
    }

    if (!mIsEmissionStarted) {
        al::startMclAnim(this, "StartEmission");
        mIsEmissionStarted = true;
    }

    if (al::isStep(this, mRevEndFrame)) {
        mPreLaunchCallback(mPreLaunchCallbackArg);
        alPadRumbleFunction::stopPadRumbleLoop(this, "SuperShell", al::getTransPtr(this), -1);
        return;
    }

    if (al::isLessEqualStep(this, mRevSpinMaxSpeedFrame)) {
        f32 rate = static_cast<f32>(al::getNerveStep(this)) /
                   static_cast<f32>(mRevSpinMaxSpeedFrame);
        rotateByDegrees(al::lerpValue(powf(rate, mRevSpinAccel), mForeshadowSpinSpeedCurrent,
                                      mRevSpinSpeed));
    } else if (al::isLessEqualStep(this, mRevSpinFrames)) {
        f32 rate = static_cast<f32>(al::getNerveStep(this) - mRevSpinMaxSpeedFrame) /
                   static_cast<f32>(mRevSpinFrames - mRevSpinMaxSpeedFrame);
        rotateByDegrees(mRevSpinSpeed * powf(1.0f - rate, mRevSpinAccel));
    }
}

/**
 * @brief Turns the shell around its CoG joint, keeping the angle in [0, 360).
 * @param degrees Angle to add.
 */
void SuperBowserShell::rotateByDegrees(f32 degrees) {
    mCoGRotation += degrees;
    while (mCoGRotation >= 360.0f) {
        mCoGRotation += -360.0f;
    }

    while (mCoGRotation < 0.0f) {
        mCoGRotation += 360.0f;
    }
}

/**
 * @brief Shakes the camera for the launch frames, then runs the launch callback.
 */
void SuperBowserShell::exeLaunch() {
    if (al::isFirstStep(this)) {
        alCameraFunction::requestCameraShakeLoop(this, "強");
        mLaunchTimer = 0;
    }

    if (mLaunchTimer++ > mLaunchFrames) {
        mLaunchCallback(mLaunchCallbackArg);
        al::setNerve(this, &NrvSuperBowserShellNoOp);
    }
}

/**
 * @brief Does nothing (the shell is idle or hidden).
 */
void SuperBowserShell::exeNoOp() {}

/**
 * @brief Raises the shell early (first visit), floating it until step 4 is reached.
 */
void SuperBowserShell::exeEarlyRise() {
    if (al::isFirstStep(this)) {
        if (rc::isPlayerInCloudBonus(this)) {
            al::setNerve(this, &NrvSuperBowserShellEarlyRise);
            return;
        }

        al::showModelIfHide(this);
        al::startAction(this, "WaitPhase00First");
        mIsEarlyRise = true;
        updateCoGBaseHeightY(false);
    }

    DisasterModeController* controller = DisasterModeController::tryGetController(this);
    if (controller != nullptr && controller->isBlackSunFloating()) {
        updateFloat();
    } else {
        updateCoGBaseHeightY(!mIsSynced);
    }

    updateAmbientSpin();
    if (mStep0Frames == -1 || mStepFrames == -1 || mStep4Frames == -1) {
        recalculatePhaseChangeFrames();
    }

    controller = DisasterModeController::tryGetController(this);
    if (controller != nullptr &&
        controller->getDisasterFrames() >= mStep0Frames + mStepFrames * 3) {
        al::setNerve(this, &NrvSuperBowserShellGrowOverTime4);
    }

    mIsSynced = false;
}

/**
 * @brief Bobs the shell up and down around its CoG height.
 */
void SuperBowserShell::updateFloat() {
    f32 rate = static_cast<f32>(mFloatFrame) / static_cast<f32>(mFloatFrames);
    f32 angle = sead::Mathf::deg2rad(rate * 360.0f);
    f32 floatRate = 0.5f - sead::Mathf::cos(angle) * 0.5f;
    mFloatOffsetY = mFloatHeight * floatRate;
    updateCoGJointPosition();
    mFloatFrame = mFloatFrame + 1 >= mFloatFrames ? 0 : mFloatFrame + 1;
}

/**
 * @brief Hides the shell immediately, killing its effects and rumbles.
 */
void SuperBowserShell::forceHide() {
    al::hideModelIfShow(this);
    getEffectKeeper()->tryKillEmitterAndParticleAll();
    alPadRumbleFunction::stopPadRumbleLoop(this, "SuperShellEnd", al::getTransPtr(this), -1);
    alPadRumbleFunction::stopPadRumbleLoop(this, "SuperShell", al::getTransPtr(this), -1);
    al::setQuat(this, sead::Quatf::unit);
    al::setNerve(this, &NrvSuperBowserShellNoOp);
}

/**
 * @brief Starts spinning up before the launch.
 * @param pCallback Callback run when the spin ends.
 * @param pArg Callback argument.
 */
void SuperBowserShell::preLaunch(LaunchCallback pCallback, void* pArg) {
    mPreLaunchCallback = pCallback;
    mPreLaunchCallbackArg = pArg;
    al::setNerve(this, &NrvSuperBowserShellSpin);
}

/**
 * @brief Launches Fury Bowser out of the shell.
 * @param pCallback Callback run once the launch frames elapsed.
 * @param pArg Callback argument.
 */
void SuperBowserShell::launch(LaunchCallback pCallback, void* pArg) {
    mLaunchCallback = pCallback;
    mLaunchCallbackArg = pArg;
    al::setNerve(this, &NrvSuperBowserShellLaunch);
}

/**
 * @brief Resets the shell below the lake once a disaster ended.
 */
void SuperBowserShell::returnFromDisaster() {
    if (al::isNerve(this, &NrvSuperBowserShellEarlyRise)) {
        return;
    }

    mIsEarlyRise = false;
    mIsEmissionStarted = false;
    mCoGBaseHeightY = mCoGBaseHeightYStart;
    mFloatFrame = 0;
    mFloatOffsetY = 0.0f;
    mCoGRotation = 0.0f;
    updateCoGJointPosition();
    if (mIsEarlyRiseNext) {
        mIsEarlyRiseNext = false;
        al::setNerve(this, &NrvSuperBowserShellEarlyRise);
    } else {
        al::setNerve(this, &NrvSuperBowserShellGrowOverTime0);
    }
}

/**
 * @brief Applies the CoG height and float offset to the CoG joint.
 */
void SuperBowserShell::updateCoGJointPosition() {
    mCoGJointPos = sead::Vector3f::ey * (mCoGBaseHeightY + mFloatOffsetY);
}

/**
 * @brief Sets the rotation of the CoG joint, keeping the angle in [0, 360).
 * @param degrees New angle.
 */
void SuperBowserShell::setCoGJointRotation(f32 degrees) {
    mCoGRotation = degrees;
    while (mCoGRotation >= 360.0f) {
        mCoGRotation += -360.0f;
    }

    while (mCoGRotation < 0.0f) {
        mCoGRotation += 360.0f;
    }
}

/**
 * @brief Stops the rumble and all sounds of the shell.
 */
void SuperBowserShell::stopAllSe() {
    alPadRumbleFunction::stopPadRumbleLoop(this, "SuperShell", al::getTransPtr(this), -1);
    al::stopAllSeFromUser(this, 0);
}

/**
 * @brief Sets how many frames of rain precede the disaster.
 * @param frames Rain frames.
 */
void SuperBowserShell::setRainFrames(s32 frames) {
    mRainFrames = frames;
}

/**
 * @brief Gets how many frames of rain precede the disaster.
 * @return The rain frames.
 */
s32 SuperBowserShell::getRainFrames() {
    return mRainFrames;
}

/**
 * @brief Sets the translation of the CoG joint.
 * @param pos New translation.
 */
void SuperBowserShell::setCoGJointPosition(sead::Vector3f pos) {
    mCoGJointPos = pos;
}

/**
 * @brief Recomputes the step timings and switches to the step matching the disaster timer.
 */
void SuperBowserShell::syncToDisasterTimer() {
    recalculatePhaseChangeFrames();
    mIsSynced = true;
    if (mIsEarlyRise) {
        return;
    }

    switch (calculatePhase()) {
    case 0:
        al::setNerve(this, &NrvSuperBowserShellGrowOverTime0);
        break;
    case 1:
        al::setNerve(this, &NrvSuperBowserShellGrowOverTime1);
        break;
    case 2:
        al::setNerve(this, &NrvSuperBowserShellGrowOverTime2);
        break;
    case 3:
        al::setNerve(this, &NrvSuperBowserShellGrowOverTime3);
        break;
    case 4:
        al::setNerve(this, &NrvSuperBowserShellGrowOverTime4);
        break;
    default:
        break;
    }
}

/**
 * @brief Finds the rising step matching the disaster timer.
 * @return The step index (0 to 4), or -1 if unavailable.
 */
s32 SuperBowserShell::calculatePhase() const {
    DisasterModeController* controller = DisasterModeController::tryGetController(this);
    if (controller == nullptr) {
        return -1;
    }

    s32 frames = controller->getDisasterFrames();
    if (frames < mStep0Frames) {
        return 0;
    }

    if (frames < mStep0Frames + mStepFrames) {
        return 1;
    }

    if (frames < mStep0Frames + mStepFrames * 2) {
        return 2;
    }

    return frames < mStep0Frames + mStepFrames * 3 ? 3 : 4;
}

/**
 * @brief Jumps the disaster timer to the start of a rising step (debug).
 * @param phase Step index to jump to.
 * @param isToChangePhase Whether to jump to the step's change animation instead.
 * @param isToAppear Whether to jump to the appearance (if not jumping to the change animation).
 */
void SuperBowserShell::skipTo(s32 phase, bool isToChangePhase, bool isToAppear) {
    DisasterModeController* controller = DisasterModeController::tryGetController(this);
    if (controller == nullptr) {
        return;
    }

    controller->clearTimeJumpFlags();
    recalculatePhaseChangeFrames();

    s32 phaseStart;
    if (phase == 0) {
        phaseStart = 0;
    } else if (phase == 1) {
        phaseStart = mStep0Frames;
    } else if (phase == 2) {
        phaseStart = mStep0Frames + mStepFrames;
    } else if (phase == 3) {
        phaseStart = mStep0Frames + mStepFrames * 2;
    } else {
        phaseStart = mStep0Frames + mStepFrames * 3;
    }

    s32 offset;
    if (isToChangePhase) {
        offset = phase == 0 ? mChangePhase01Frame : mChangePhaseFrame;
    } else {
        offset = isToAppear ? mAppearFrame : 0;
    }

    s32 frames = phaseStart + offset;
    controller->setDisasterFramesOffset(controller->calcProsperityStartAdditionalFramesMax());
    controller->setDisasterFrames(frames);
    syncToDisasterTimer();
}

/**
 * @brief Jumps the disaster timer to the start of the next rising step (debug).
 */
void SuperBowserShell::skipToNextStepStep() {
    s32 phase = 5;
    if (al::isNerve(this, &NrvSuperBowserShellGrowOverTime0)) {
        phase = 1;
    } else if (al::isNerve(this, &NrvSuperBowserShellGrowOverTime1)) {
        phase = 2;
    } else if (al::isNerve(this, &NrvSuperBowserShellGrowOverTime2)) {
        phase = 3;
    } else if (al::isNerve(this, &NrvSuperBowserShellGrowOverTime3)) {
        phase = 4;
    } else if (al::isNerve(this, &NrvSuperBowserShellGrowOverTime4)) {
        phase = 5;
    }

    skipTo(phase, false, false);
}

/**
 * @brief Gets the frames left over after all steps reached their maximum length.
 * @return The leftover frames.
 */
s32 SuperBowserShell::getLeftoverFrames() {
    return mLeftoverFrames;
}

/**
 * @brief Gets the frames to wait before the steps start.
 * @return The wait frames.
 */
s32 SuperBowserShell::getWaitSkipFrames() {
    return mWaitSkipFrames;
}

/**
 * @brief Gets the length of step 4.
 * @return The step 4 frames.
 */
s32 SuperBowserShell::getStep4Frames() {
    return mStep4Frames;
}

/**
 * @brief Prints the current rising step on screen when the debug display is enabled.
 */
void SuperBowserShell::tryDrawStateDebug() {
    if (!mIsDrawDebugState) {
        return;
    }

    f32 width = static_cast<u32>(al::getDebugMenuDisplayWidth());
    f32 height = static_cast<u32>(al::getDebugMenuDisplayHeight());
    sead::Viewport viewport(0.0f, 0.0f, width, height);
    sead::TextWriter::setupGraphics(al::GameFrameworkNx::getDrawContext());
    sead::TextWriter writer(al::GameFrameworkNx::getDrawContext(), &viewport);
    writer.setCursorFromTopLeft(sead::Vector2f(20.0f, 160.0f));

    DisasterModeController* controller = DisasterModeController::tryGetController(this);
    if (controller == nullptr) {
        return;
    }

    if (controller->isDisasterMode()) {
        writer.printf("BLACK SUN STEP: DISASTER");
        return;
    }

    if (al::isNerve(this, &NrvSuperBowserShellEarlyRise)) {
        writer.printf("BLACK SUN STEP: EARLY RISE");
        return;
    }

    s32 phase;
    s32 stepFrames;
    if (al::isNerve(this, &NrvSuperBowserShellGrowOverTime0)) {
        phase = 0;
        stepFrames = mStep0Frames;
    } else if (al::isNerve(this, &NrvSuperBowserShellGrowOverTime1)) {
        phase = 1;
        stepFrames = mStepFrames;
    } else if (al::isNerve(this, &NrvSuperBowserShellGrowOverTime2)) {
        phase = 2;
        stepFrames = mStepFrames;
    } else if (al::isNerve(this, &NrvSuperBowserShellGrowOverTime3)) {
        phase = 3;
        stepFrames = mStepFrames;
    } else if (al::isNerve(this, &NrvSuperBowserShellGrowOverTime4)) {
        phase = 4;
        stepFrames = mStep4Frames;
    } else {
        writer.printf("BLACK SUN STEP: TRANSITIONING");
        return;
    }

    writer.printf("BLACK SUN STEP: %d (%d / %d)", phase, calculatePhaseFrames(phase), stepFrames);
}

/**
 * @brief Toggles the on-screen step display.
 */
void SuperBowserShell::toggleDrawDebugState() {
    mIsDrawDebugState = !mIsDrawDebugState;
}

/**
 * @brief Enables or disables the shell's effects.
 * @param isOn Whether the effects are calculated and drawn.
 */
void SuperBowserShell::setEffectsOn(bool isOn) {
    getEffectKeeper()->forceSetStopCalcAndDraw(!isOn);
}
