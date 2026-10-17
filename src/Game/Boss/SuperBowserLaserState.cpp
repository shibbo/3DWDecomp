#include "Boss/SuperBowserLaserState.hpp"

#include <cmath>
#include <math/seadMathCalcCommon.h>
#include <math/seadQuat.h>

#include "Enemy/SuperBowser.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Connector/MtxConnector.hpp"
#include "Library/Draw/GraphicsSystemInfo.hpp"
#include "Library/Effect/EffectSystem.hpp"
#include "Library/Light/DirectionalLightKeeper.hpp"
#include "Library/Light/LppBase.hpp"
#include "Library/Light/PrePassLightFunction.hpp"
#include "Library/LiveActor/Util/ActorSceneInfo.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Yaml/ByamlIter.hpp"
#include "MapObj/DisasterModeController.hpp"
#include "MapObj/EchoEmitterHolder.hpp"
#include "MapObj/Fury/GigaBell.hpp"
#include "MapObj/Fury/GigaBellManager.hpp"
#include "MapObj/SePlayObj.hpp"
#include "Player/Normal/PlayerActor.hpp"
#include "Project/Base/StringOpUtil.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Collision/CollisionPartsFilterBase.hpp"
#include "Project/Collision/CollisionUtil.hpp"
#include "Project/Effect/Core/EffectKeeper.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Project/Effect/Effect.hpp"
#include "Project/Effect/EffectInfo.hpp"
#include "Project/Rail/LinearCurve.hpp"
#include "Scene/SceneObjID.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"

namespace {
NERVE_DECL(SuperBowserLaserState, Charge)
NERVE_DECL(SuperBowserLaserState, End)
NERVE_DECL(SuperBowserLaserState, Shoot)
NERVE_DECL(SuperBowserLaserState, Begin)

NERVES_MAKE_NOSTRUCT(SuperBowserLaserState, Charge, End, Shoot, Begin)

/** @brief Length of the rays cast to find what the beam hits. */
constexpr f32 cRayLength = 50000.0f;
/** @brief Number of rays on the outer ring of the beam. */
constexpr s32 cOuterRayNum = 8;
/** @brief Index of the first ray on the outer ring of the beam. */
constexpr s32 cOuterRayStart = 5;
/** @brief Frames the scene lighting takes to fade to and from the laser colors. */
constexpr s32 cLightFadeFrames = 30;

/**
 * @brief Get the current parameters of the directional light the laser recolors.
 * @param pActor Actor of the scene.
 * @return The light parameters.
 */
inline al::DirLightParam& getDirLightParam(const al::LiveActor* pActor) {
    al::DirectionalLightKeeper* keeper =
        pActor->getSceneInfo()->graphicsSystemInfo->getDirectionalLightKeeper();
    return keeper->getCurrentParam();
}

/**
 * @brief Get the Giga Bell manager of the scene, if any.
 * @param pHolder Object with access to the scene objects.
 * @return The manager, or nullptr.
 */
inline GigaBellManager* tryGetGigaBellManager(const al::IUseSceneObjHolder* pHolder) {
    return al::tryGetSceneObj<GigaBellManager>(pHolder, SceneObjID_GigaBellManager);
}

/**
 * @brief Make every component of a vector positive.
 * @param pVec The vector.
 */
inline void absVec(sead::Vector3f* pVec) {
    pVec->x = sead::Mathf::abs(pVec->x);
    pVec->y = sead::Mathf::abs(pVec->y);
    pVec->z = sead::Mathf::abs(pVec->z);
}
}  // namespace

/**
 * @brief Constructs the laser state, reads the laser variants and sets up the laser light,
 * effects and sound actors.
 * @param pHost Fury Bowser.
 * @param rInfo Actor init info.
 */
SuperBowserLaserState::SuperBowserLaserState(SuperBowser* pHost, const al::ActorInitInfo& rInfo)
    : al::HostStateBase<SuperBowser>("DisasterLaser", pHost) {
    initNerve(&NrvSuperBowserLaserStateCharge, 0);
    mParam.init(pHost, rInfo);

    if (pHost->isLastPhase3Bowser()) {
        const char* fileName = "InitAttackSuperBowserLastPhase3";
        mParamBigRamp.initWithName(pHost, rInfo, fileName, "DisasterLaserPlessieChaseBigRamp");
        mParamFinalAttack.initWithName(pHost, rInfo, fileName,
                                       "DisasterLaserPlessieChaseFinalAttack");
        mParamLv4.initWithName(pHost, rInfo, fileName, "DisasterLaserPlessieChaseLv4");
        mParamV2.initWithName(pHost, rInfo, fileName, "DisasterLaserPlessieChaseV2");
        mParamBigRampV2.initWithName(pHost, rInfo, fileName,
                                     "DisasterLaserPlessieChaseBigRampV2");
        mParamFinalAttackV2.initWithName(pHost, rInfo, fileName,
                                         "DisasterLaserPlessieChaseFinalAttackV2");
        mParamLv4V2.initWithName(pHost, rInfo, fileName, "DisasterLaserPlessieChaseLv4V2");
    }

    mChargeFrames = mCurrentParam->chargeFrames;
    mSceneObjHolder = rInfo.getActorSceneInfo().sceneObjHolder;
    al::invalidateHitSensor(getHost(), "LaserMiddle");
    al::invalidateHitSensor(getHost(), "LaserMiddle2");
    al::killPrePassLight(getHost(), "Laser", -1);
    mLaserLight = static_cast<al::LppLine*>(al::getPrePassLineLight(getHost(), "Laser"));
    mLaserLight->mMtxConnector->init(&mLaserLightMtx, sead::Matrix34f::ident);
    mEffectSystem = rInfo.getEffectSystemInfo()->getEffectSystem();
    al::setEffectFollowMtxPtr(getHost(), "Laser", &mLaserMtx);
    al::setEffectFollowMtxPtr(getHost(), "LaserEnd", &mLaserMtx);
    al::setEffectFollowMtxPtr(getHost(), "LaserCap", &mLaserCapMtx);
    al::setEffectFollowMtxPtr(getHost(), "LaserWaterHit", &mHitEffectMtx);
    al::setEffectFollowMtxPtr(getHost(), "LaserLandHit", &mHitEffectMtx);
    al::setEffectFollowMtxPtr(getHost(), "KoopaSuperPlayerEmbers", &mEmbersMtx);
    mIndicator.init(rInfo);

    mSeTip = new SePlayObj("SuperBowserLaserTipSoundActor");
    mSeTip->initWithAudioKeeper(rInfo, "FinalKoopa");
    al::updatePoseMtx(mSeTip, pHost->getBaseMtx());
    al::invalidateClipping(mSeTip);

    mSeNear = new SePlayObj("SuperBowserLaserNearPlayerSoundActor");
    mSeNear->initWithAudioKeeper(rInfo, "FinalKoopa");
    al::updatePoseMtx(mSeNear, pHost->getBaseMtx());
    al::invalidateClipping(mSeNear);
}

/**
 * @brief Aim function that follows the player: the aim point accelerates towards the laser
 * target once the beam has grown and Fury Bowser's sight is not locked.
 * @param rOrigin Origin of the beam (unused).
 */
void SuperBowserLaserState::updateAimTrack(const sead::Vector3f& rOrigin) {
    calcLaserTarget(mLaserTarget);

    if (mShootFrame != 0 && mShootFrame <= getFollowFramesPre()) {
        return;
    }

    if (!mIsAimLocked) {
        mAimVelocity *= mCurrentParam->followDamp;
        mAimVelocity += calcLaserAccel();

        if (getHost()->isLastPhase3Bowser() && mCurrentParam == &mParam) {
            bool isV2 = SingleModeDataFunction::isDarkBowserV2Available(
                GameDataHolderAccessor(getHost()));
            f32 speedMax = isV2 ? mAimSpeedMaxV2 : mAimSpeedMax;

            if (mAimVelocity.length() > speedMax * 0.5f) {
                mAimVelocity.normalize();
                mAimVelocity *= speedMax * 0.5f;
            }
        }
    }

    mTarget += mAimVelocity;
}

/**
 * @brief Reads the laser parameters from the init file named after Fury Bowser's suffix.
 * @param pBowser Fury Bowser.
 * @param rInfo Actor init info.
 */
void SuperBowserLaserStateParam::init(const SuperBowser* pBowser, const al::ActorInitInfo& rInfo) {
    const char* suffix = pBowser->getInitFileSuffix();
    const char* fileName =
        suffix != nullptr ? al::createConcatString("InitAttack", suffix) : "InitAttack";
    initWithName(pBowser, rInfo, fileName, "DisasterLaser");
}

/**
 * @brief Reads the laser parameters, first from the common init file, then from the given one.
 * @param pBowser Fury Bowser.
 * @param rInfo Actor init info.
 * @param pFileName Name of the init file with the variant's parameters.
 * @param pKey Key of the variant in that file.
 */
void SuperBowserLaserStateParam::initWithName(const SuperBowser* pBowser,
                                              const al::ActorInitInfo& rInfo,
                                              const char* pFileName, const char* pKey) {
    al::ByamlIter fileIter;
    al::ByamlIter iter;

    if (al::tryGetActorInitFileIter(&fileIter, pBowser, "InitAttackSuperBowserCommon", nullptr)) {
        fileIter.tryGetIterByKey(&iter, "DisasterLaser");
        iter.tryGetFloatByKey(&verticalAimOffset, "Vertical Aim Offset");
        iter.tryGetFloatByKey(&verticalAimOffsetPlessie, "Vertical Aim Offset Plessie");
        iter.tryGetFloatByKey(&penetrationOffsetMin, "Penetration Offset Min");
        iter.tryGetFloatByKey(&penetrationOffsetMax, "Penetration Offset Max");
        iter.tryGetFloatByKey(&penetrationDegMin, "Penetration Deg Min");
        iter.tryGetFloatByKey(&penetrationDegMax, "Penetration Deg Max");
        iter.tryGetFloatByKey(&damageSensorOffset, "Damage Sensor Offset");
        iter.tryGetFloatByKey(&laserColor.x, "Laser Color R");
        iter.tryGetFloatByKey(&laserColor.y, "Laser Color G");
        iter.tryGetFloatByKey(&laserColor.z, "Laser Color B");
        iter.tryGetFloatByKey(&effectScale, "Effect Scale");
        iter.tryGetFloatByKey(&laserRadiusInner, "Laser Radius Inner");
        iter.tryGetFloatByKey(&laserRadiusOuter, "Laser Radius Outer");
        iter.tryGetFloatByKey(&laserRadiusSensor, "Laser Radius Sensor");
        iter.tryGetFloatByKey(&laserDisasterBlockRadius, "Laser Disaster Block Radius");
        iter.tryGetFloatByKey(&laserRadiusSensorPlayerOnly, "Laser Radius Sensor Player Only");
        iter.tryGetFloatByKey(&laserRadiusInnerVerticalOffset,
                              "Laser Radius Inner Vertical Offset");
        iter.tryGetFloatByKey(&laserAngleMin, "Laser Angle Min");
        iter.tryGetFloatByKey(&laserAngleMax, "Laser Angle Max");
        iter.tryGetFloatByKey(&targetVelocityOffsetScale, "Target Velocity Offset Scale");
        iter.tryGetFloatByKey(&targetVelocityOffsetScalePlessieChaseV2,
                              "Target Velocity Offset Scale Plessie Chase V2");
        iter.tryGetFloatByKey(&targetVelocityOffsetRate, "Target Velocity Offset Rate");
        iter.tryGetIntByKey(&laserEarlyStartFrame, "Laser Early Start Frame");
        iter.tryGetFloatByKey(&laserOuterRayAngleThreshold, "Laser Outer Ray Angle Threshold");
        iter.tryGetFloatByKey(&laserSlopeThresholdAngle, "Laser Slope Threshold Angle");
        iter.tryGetIntByKey(&laserVelocityLockFrame, "Laser Velocity Lock Frame");
        iter.tryGetIntByKey(&laserVelocityLockFramePlessie, "Laser Velocity Lock Frame Plessie");
        iter.tryGetIntByKey(&laserVelocityLockFramePlessieChase,
                            "Laser Velocity Lock Frame Plessie Chase");
        iter.tryGetIntByKey(&laserVelocityLockFramePlessieChaseV2,
                            "Laser Velocity Lock Frame Plessie Chase V2");
        iter.tryGetFloatByKey(&jointPitchBeam, "Joint Pitch Beam");
        iter.tryGetFloatByKey(&jointPitchFace, "Joint Pitch Face");
        iter.tryGetFloatByKey(&jointPitchNeck, "Joint Pitch Neck");
        iter.tryGetFloatByKey(&jointPitchSpine, "Joint Pitch Spine");
        iter.tryGetFloatByKey(&jointPitchHip, "Joint Pitch Hip");
        iter.tryGetFloatByKey(&jointConstraintBeam, "Joint Constraint Beam");
        iter.tryGetFloatByKey(&jointConstraintFace, "Joint Constraint Face");
        iter.tryGetFloatByKey(&jointConstraintNeck, "Joint Constraint Neck");
        iter.tryGetFloatByKey(&jointConstraintSpine, "Joint Constraint Spine");
        iter.tryGetFloatByKey(&jointConstraintHip, "Joint Constraint Hip");
        iter.tryGetIntByKey(&beamEndFrames, "Beam End Frames");
        iter.tryGetIntByKey(&beamEndFramesOfDamage, "Beam End Frames Of Damage");
        iter.tryGetFloatByKey(&beamEndAcceleration, "Beam End Acceleration");
        iter.tryGetFloatByKey(&laserLightSourceYOffset, "Laser Light Source Y Offset");
        iter.tryGetFloatByKey(&playerLineCheckStartPosYOffset,
                              "Player Line Check Start Pos Y Offset");
        iter.tryGetFloatByKey(&playerLineCheckStartPosYOffsetBeforeJump,
                              "Player Line Check Start Pos Y Offset Before Jump");
    }

    if (al::tryGetActorInitFileIter(&fileIter, pBowser, pFileName, nullptr)) {
        fileIter.tryGetIterByKey(&iter, pKey);
        iter.tryGetIntByKey(&laserCount, "Laser Count");
        iter.tryGetFloatByKey(&chargeFrames, "Charge Frames");
        iter.tryGetFloatByKey(&chargeFramesSecondary, "Charge Frames Secondary");
        iter.tryGetIntByKey(&chargeFramesUntilTargetLock, "Charge Frames Until Target Lock");
        iter.tryGetIntByKey(&chargeFramesUntilTargetLockLaserBeginChase,
                            "Charge Frames Until Target Lock Laser Begin Chase");
        iter.tryGetIntByKey(&extendFrames, "Extend Frames");
        iter.tryGetFloatByKey(&followAccelTangent, "Follow Acceleration Tangent");
        iter.tryGetFloatByKey(&followAccelTangentPlessieChaseV2Lv3,
                              "Follow Acceleration Tangent Plessie Chase V2 Lv 3");
        iter.tryGetFloatByKey(&followAccelCross, "Follow Acceleration Cross");
        iter.tryGetFloatByKey(&followAccelCrossPlessieChaseLv2,
                              "Follow Acceleration Cross Plessie Chase Lv 2");
        iter.tryGetFloatByKey(&followAccelCrossPlessieChaseLv3,
                              "Follow Acceleration Cross Plessie Chase Lv 3");
        iter.tryGetFloatByKey(&followAccelNormal, "Follow Acceleration Normal");
        iter.tryGetFloatByKey(&followDamp, "Follow Damp");
        iter.tryGetIntByKey(&followFramesPre, "Follow Frames Pre");
        iter.tryGetIntByKey(&followFramesPrePlessieChaseLv2,
                            "Follow Frames Pre Plessie Chase Lv 2");
        iter.tryGetIntByKey(&followFramesPrePlessieChaseLv3,
                            "Follow Frames Pre Plessie Chase Lv 3");
        iter.tryGetIntByKey(&followFrames, "Follow Frames");
        iter.tryGetFloatByKey(&followOffsetNormal, "Follow Offset Normal (In global up direction)");
        iter.tryGetFloatByKey(&followOffsetTangent,
                              "Follow Offset Tangent (In direction of Bowser)");
        iter.tryGetFloatByKey(
            &followOffsetTangentPlessieChaseV2Lv3,
            "Follow Offset Tangent Plessie Chase V2 Lv 3 (In direction of Bowser)");
        iter.tryGetFloatByKey(&followOffsetCross,
                              "Follow Offset Cross (Perpendicular to Bowser)");
        iter.tryGetFloatByKey(&followOffsetCrossPlessie,
                              "Follow Offset Cross Plessie (Perpendicular to Bowser)");
        iter.tryGetFloatByKey(&followOffsetCrossPlessieV2Lv2,
                              "Follow Offset Cross Plessie V2 Lv 2 (Perpendicular to Bowser)");
        iter.tryGetFloatByKey(&followOffsetCrossPlessieV2Lv3,
                              "Follow Offset Cross Plessie V2 Lv 3 (Perpendicular to Bowser)");
        iter.tryGetBoolByKey(&isFollowOffsetCrossFlip, "Follow Offset Cross Flip");
        iter.tryGetBoolByKey(&isFollowOffsetCrossLeftToRight, "Follow Offset Cross Left to Right");
        iter.tryGetBoolByKey(&isFollowOffsetCrossLeftToRightLv2,
                             "Follow Offset Cross Left to Right Lv 2");
        iter.tryGetBoolByKey(&isSweepEnable, "Sweep Enable");
        iter.tryGetBoolByKey(&isSweepLeftThenRight, "Sweep Left Then Right");
        iter.tryGetIntByKey(&preSweepFrames, "Pre-Sweep Frames");
        iter.tryGetFloatByKey(&sweepFrames, "Sweep Frames");
        iter.tryGetIntByKey(&postSweepFrames, "Post-Sweep Frames");
        iter.tryGetFloatByKey(&sweepOffsetStart, "Sweep Offset Start");
        iter.tryGetFloatByKey(&sweepOffsetEnd, "Sweep Offset End");
    }
}

/**
 * @brief Starts the attack: picks the laser variant matching the Plessie chase and starts
 * charging.
 */
void SuperBowserLaserState::appear() {
    NerveStateBase::appear();
    mCurrentParam = &mParam;

    GigaBellManager* manager = tryGetGigaBellManager(this);
    if (manager != nullptr && getHost()->isLastPhase3Bowser()) {
        if (SingleModeDataFunction::isDarkBowserV2Available(GameDataHolderAccessor(getHost()))) {
            mCurrentParam = &mParamV2;

            if (manager->isPlessieChaseLv(4)) {
                mCurrentParam = &mParamLv4V2;
            } else if (getHost()->isPlessieChaseBigRamp()) {
                if (manager->isPlessieChaseLv(3)) {
                    mCurrentParam = &mParamFinalAttackV2;
                } else {
                    mCurrentParam = &mParamBigRampV2;
                }
            }
        } else if (manager->isPlessieChaseLv(4)) {
            mCurrentParam = &mParamLv4;
        } else if (getHost()->isPlessieChaseBigRamp()) {
            if (manager->isPlessieChaseLv(3)) {
                mCurrentParam = &mParamFinalAttack;
            } else {
                mCurrentParam = &mParamBigRamp;
            }
        }
    }

    setAimFunction();
    mLaserNum = 0;
    mIsShooting = false;
    mPlayer = static_cast<PlayerActor*>(al::findNearestPlayerActor(getHost()));
    al::setNerve(this, &NrvSuperBowserLaserStateCharge);
    getHost()->setGuideBalloonVisible(true);
}

/**
 * @brief Picks how the beam is aimed: sweeping across the target or following it.
 */
void SuperBowserLaserState::setAimFunction() {
    mAimFunc = mIsSweep || mCurrentParam->isSweepEnable ? &SuperBowserLaserState::updateAimSweep :
                                                          &SuperBowserLaserState::updateAimTrack;
}

/**
 * @brief Ends the attack at once, stopping every sound, effect, light and sensor of the laser.
 */
void SuperBowserLaserState::forceKill() {
    kill();
    al::tryStopSe(getHost(), "Warning");
    al::tryStopSe(getHost(), "PgLaserBeamStart");
    al::tryStopSe(getHost(), "PgLaserBeamEnd");
    al::tryStopSe(mSeNear, "PgLaserBeamEndNear");
    al::tryDeleteEmitterAndParticleAll(getHost());
    disableLaserSensor();

    if (mIsLightSettingsRecorded) {
        mIsLightSettingsRecorded = false;
        laserLightSettingsRestore();
    }

    if (al::isActivePrePassLight(getHost(), "Laser")) {
        al::killPrePassLight(getHost(), "Laser", -1);
    }

    mIsShooting = false;
    getHost()->setGuideBalloonVisible(false);
    mIndicator.forceKill();
}

/**
 * @brief Disables every damage sensor of the beam.
 */
void SuperBowserLaserState::disableLaserSensor() {
    al::invalidateHitSensor(getHost(), "Laser");
    al::invalidateHitSensor(getHost(), "Laser2");
    al::invalidateHitSensor(getHost(), "Laser3");
    al::invalidateHitSensor(getHost(), "Laser4");
    al::invalidateHitSensor(getHost(), "Laser5");
    al::invalidateHitSensor(getHost(), "Laser6");
    al::invalidateHitSensor(getHost(), "Laser7");
    al::invalidateHitSensor(getHost(), "Laser8");
    al::invalidateHitSensor(getHost(), "Laser9");
    al::invalidateHitSensor(getHost(), "LaserPlayerOnly");
    al::invalidateHitSensor(getHost(), "LaserMiddle");
    al::invalidateHitSensor(getHost(), "LaserMiddle2");
}

/**
 * @brief Puts back the directional light recorded before charging, unless a disaster sets
 * its own.
 */
void SuperBowserLaserState::laserLightSettingsRestore() {
    DisasterModeController* controller = DisasterModeController::tryGetController(getHost());
    if (controller != nullptr && !controller->isDisasterNerve()) {
        return;
    }

    al::DirLightParam& light = getDirLightParam(getHost());
    light.getDirection()->setDirection(mSavedLightDir);
    light.getColor() = sead::Color4f(mSavedLightColor.x, mSavedLightColor.y, mSavedLightColor.z,
                                     sead::Color4f::cElementMax);
}

/**
 * @brief Charges the laser: tracks the player, fades the lighting to the laser color and
 * begins shooting once charged.
 */
void SuperBowserLaserState::exeCharge() {
    if (mPlayer == nullptr) {
        mPlayer = static_cast<PlayerActor*>(al::findNearestPlayerActor(getHost()));
        if (mPlayer == nullptr) {
            al::setNerve(this, &NrvSuperBowserLaserStateEnd);
            return;
        }
    }

    if (al::isFirstStep(this)) {
        startCharge();
        if (mIsEnableLaserLightEffects) {
            mIndicator.fadeIn(getHost(), mPlayer, 30);
        }
    }

    if (!getHost()->isLastPhase3Bowser()) {
        s32 step = sead::Mathi::min(al::getNerveStep(getHost()), cLightFadeFrames);
        f32 rate = step / (f32)cLightFadeFrames;
        f32 prevRate = sead::Mathi::max(step - 1, 0) / (f32)cLightFadeFrames;

        if (rate < 0.5f) {
            laserLightSettingsSetColor(mSavedLightColor * (1.0f - (rate + rate)));
        } else if (rate > 0.5f && prevRate <= 0.5f) {
            laserLightSettingsSetDirection(mChargeLightDir);
        } else {
            laserLightSettingsSetColor(mParam.laserColor * ((rate - 0.5f) * 2.0f));
        }
    }

    updateTargetData();

    if (getHost()->isLastPhase3Bowser()) {
        calcLaserTarget(mLaserTarget);
        updateChargeLook(mLaserTarget);
    } else {
        if (al::isLessEqualStep(this, getChargeFramesUntilTargetLock())) {
            calcLaserTarget(mLaserTarget);
            updateChargeLook(mLaserTarget);

            if (al::isStep(this, getChargeFramesUntilTargetLock())) {
                lockTarget();
            }
        } else {
            updateChargeLook(mTarget);
        }

        SuperBowser* host = getHost();
        host->updateTurning(host->getActionName("Charge"), al::getNerveStep(this));
    }

    updateEmbers();

    if (al::isGreaterEqualStep(this, mChargeFrames)) {
        beginNow();
    }
}

/**
 * @brief Starts charging: records the lighting, plays the charge action, sound and effects.
 */
void SuperBowserLaserState::startCharge() {
    mIsTargetLocked = false;
    mIsAimLocked = false;
    mVelocityOffset = sead::Vector3f::zero;
    mPlayerPrevTrans = al::getTrans(mPlayer);
    mLineCheckStart =
        getLaserOrigin() + sead::Vector3f::ey * mParam.playerLineCheckStartPosYOffset;
    laserLightSettingsRecord();

    sead::Vector3f lightPos =
        getLaserOrigin() + sead::Vector3f::ey * mParam.laserLightSourceYOffset;
    mChargeLightDir = al::getTrans(mPlayer) - lightPos;
    al::hideShadowDepth(getHost());
    mMouthSensor = al::getHitSensor(getHost(), "MouthLaser");

    if (mIsSweep) {
        mChargeFrames = mSweepChargeFrames;
    } else if (mLaserNum != 0) {
        mChargeFrames = mCurrentParam->chargeFramesSecondary;
    } else {
        mChargeFrames = mCurrentParam->chargeFrames;
    }

    if (mChargeFrames == 0) {
        return;
    }

    const char* actionName;
    if (getHost()->isLastPhase3Bowser() && mLaserNum >= 1) {
        bool isFinalAttack =
            mCurrentParam == &mParamFinalAttackV2 || mCurrentParam == &mParamFinalAttack;
        actionName = isFinalAttack ? "ChaseChargeAndBegin2ndFinalAttack" : "ChaseChargeAndBegin2nd";
    } else if (getHost()->isLastPhase3Bowser() &&
               (mCurrentParam == &mParamBigRamp || mCurrentParam == &mParamBigRampV2)) {
        actionName = "ChaseChargeLong";
    } else {
        actionName = getHost()->getActionName("Charge");
    }

    al::tryStartAction(getHost(), actionName);
    al::tryStartSe(getHost(), getChargeSeName());
    al::tryEmitEffect(getHost(), getChargeEffectName(), nullptr);
    al::emitEffect(getHost(), "KoopaSuperPlayerEmbers", nullptr);
}

/**
 * @brief Sets the color of the directional light, if the laser may change the lighting.
 * @param rColor New color.
 */
void SuperBowserLaserState::laserLightSettingsSetColor(const sead::Vector3f& rColor) {
    if (!mIsEnableLaserLightEffects) {
        return;
    }

    al::DirLightParam& light = getDirLightParam(getHost());
    light.getColor() = sead::Color4f(rColor.x, rColor.y, rColor.z, 1.0f);
}

/**
 * @brief Sets the direction of the directional light, if the laser may change the lighting.
 * @param rDir New direction.
 */
void SuperBowserLaserState::laserLightSettingsSetDirection(const sead::Vector3f& rDir) {
    if (!mIsEnableLaserLightEffects) {
        return;
    }

    getDirLightParam(getHost()).getDirection()->setDirection(rDir);
}

/**
 * @brief Updates the player's velocity from their last position.
 */
void SuperBowserLaserState::updateTargetData() {
    mPlayerVelocity = al::getTrans(mPlayer) - mPlayerPrevTrans;
    mPlayerPrevTrans = al::getTrans(mPlayer);
}

/**
 * @brief Computes where the laser should aim: the forced target, a point of the forced sweep,
 * or the player offset by their velocity and the follow offsets.
 * @param rTarget Computed target.
 */
void SuperBowserLaserState::calcLaserTarget(sead::Vector3f& rTarget) {
    if (mIsForceTarget) {
        if (mIsSweep) {
            f32 frame = mShootFrame;
            s32 frames = getFollowFramesPre() + mCurrentParam->followFrames;
            al::lerpVec(&rTarget, mForceTarget, mForceSweepEnd, frame / frames);
        } else {
            rTarget = mForceTarget;
        }

        return;
    }

    f32 radius = al::getSensorRadius(al::getHitSensor(mPlayer, "Body"));
    f32 aimOffset =
        mPlayer->isRaidonExist() ? mParam.verticalAimOffsetPlessie : mParam.verticalAimOffset;
    rTarget = al::getTrans(mPlayer);
    rTarget += sead::Vector3f::ey * (radius + radius + aimOffset);

    if (mIsTargetLocked) {
        return;
    }

    rTarget += calcTargetVelocityOffset();

    sead::Vector3f dir = al::getTrans(getHost()) - rTarget;
    dir.normalize();
    sead::Vector3f side;
    side.setCross(dir, sead::Vector3f::ey);
    side.normalize();

    rTarget += sead::Vector3f::ey * mCurrentParam->followOffsetNormal;

    f32 tangentOffset = mCurrentParam->followOffsetTangent;
    if (getHost()->isLastPhase3Bowser()) {
        GigaBellManager* manager = tryGetGigaBellManager(this);
        if (mCurrentParam == &mParamV2 && manager->isPlessieChaseLv(3)) {
            tangentOffset = mCurrentParam->followOffsetTangentPlessieChaseV2Lv3;
        }
    }

    rTarget += dir * tangentOffset;

    bool isRaidon = mPlayer->isRaidonExist();
    f32 crossOffset =
        isRaidon ? mCurrentParam->followOffsetCrossPlessie : mCurrentParam->followOffsetCross;
    s32 crossSign = mCurrentParam->isFollowOffsetCrossLeftToRight ? -1 : 1;

    if (getHost()->isLastPhase3Bowser()) {
        GigaBellManager* manager = tryGetGigaBellManager(this);
        if (manager != nullptr && manager->getPlessieChaseHitCount() == 1) {
            crossSign = mCurrentParam->isFollowOffsetCrossLeftToRightLv2 ? -1 : 1;
        }

        if (isRaidon && mCurrentParam == &mParamV2) {
            if (manager->isPlessieChaseLv(2)) {
                crossOffset = mCurrentParam->followOffsetCrossPlessieV2Lv2;
            }

            if (manager->isPlessieChaseLv(3)) {
                crossOffset = mCurrentParam->followOffsetCrossPlessieV2Lv3;
            }
        }
    }

    f32 sign = mCrossSign * crossSign;
    if (getHost()->getBowserType() == 0) {
        sign = -1.0f;
    }

    rTarget += side * crossOffset * sign;
}

/**
 * @brief Turns Fury Bowser's head and spine towards the charge target.
 * @param target Point to look at.
 */
void SuperBowserLaserState::updateChargeLook(sead::Vector3f target) {
    if (!al::isFirstStep(this)) {
        getHost()->updateJointAim(target);
        return;
    }

    SuperBowser::BowserJointState state;
    state.mRate = 1.0f;
    state.mTarget = target;
    state.mConstraintBeam = mParam.jointConstraintBeam;
    state.mConstraintFace = mParam.jointConstraintFace;
    state.mConstraintNeck = mParam.jointConstraintNeck;
    state.mConstraintSpine = mParam.jointConstraintSpine;
    state.mConstraintHip = mParam.jointConstraintHip;
    state.mPitchBeam = mParam.jointPitchBeam;
    state.mPitchFace = mParam.jointPitchFace;
    state.mPitchNeck = mParam.jointPitchNeck;
    state.mPitchSpine = mParam.jointPitchSpine;
    state.mPitchHip = mParam.jointPitchHip;
    s32 frames = sead::Mathi::min(mJointAimFrames, mChargeFrames);

    if (getHost()->isLastPhase3Bowser()) {
        getHost()->startJointAim(state, frames, 3, 30);
    } else {
        getHost()->startJointAim(state, frames, 3, 120);
    }
}

/**
 * @brief Get the charge frames after which the target stops following the player.
 * @return The frames.
 */
u32 SuperBowserLaserState::getChargeFramesUntilTargetLock() {
    u32 lockFrames = mCurrentParam->chargeFramesUntilTargetLock;
    return lockFrames > mChargeFrames ? mChargeFrames : lockFrames;
}

/**
 * @brief Locks the target of the laser: the beam starts from the mouth towards it.
 */
void SuperBowserLaserState::lockTarget() {
    if (mPlayer == nullptr) {
        mPlayer = static_cast<PlayerActor*>(al::findNearestPlayerActor(getHost()));
        if (mPlayer == nullptr) {
            return;
        }
    }

    mLaserOrigin = getLaserOrigin();
    mLaserDir = mLaserTarget - mLaserOrigin;
    mLaserDir.normalize();
    calcLaserBasis();

    if (mCurrentParam->isSweepEnable || mIsSweep) {
        calcSweepPositions();
    } else {
        mTarget = mLaserTarget;
        if (!mIsForceTarget && mCurrentParam->isFollowOffsetCrossFlip) {
            mCrossSign = -mCrossSign;
        }
    }

    mIsTargetLocked = true;
}

/**
 * @brief Orients the charge embers from the beam joint towards the player.
 */
void SuperBowserLaserState::updateEmbers() {
    sead::Quatf quat = sead::Quatf::unit;
    const sead::Matrix34f* beamMtx = al::getJointMtxPtr(getHost(), "Beam01");
    sead::Vector3f beamPos(beamMtx->m[0][3], beamMtx->m[1][3], beamMtx->m[2][3]);
    al::makeQuatFrontUp(&quat, beamPos - al::getTrans(mPlayer), mUp);
    mEmbersMtx.makeQT(quat, al::getTrans(mPlayer));
}

/**
 * @brief Stops charging and begins the laser.
 */
void SuperBowserLaserState::beginNow() {
    al::tryDeleteEffect(getHost(), "KoopaSuperPlayerEmbers");
    al::tryDeleteEffect(getHost(), getChargeEffectName());
    al::tryStopSe(getHost(), "Warning");
    al::setNerve(this, &NrvSuperBowserLaserStateBegin);
}

/**
 * @brief Begins the laser: plays the begin action and starts shooting from the early start
 * frame on.
 */
void SuperBowserLaserState::exeBegin() {
    bool isChase2nd = al::isActionPlaying(getHost(), "ChaseChargeAndBegin2nd") ||
                      al::isActionPlaying(getHost(), "ChaseChargeAndBegin2ndFinalAttack");

    if (al::isFirstStep(this)) {
        mShootFrame = 0;

        if (!isChase2nd) {
            SuperBowser* host = getHost();
            al::tryStartAction(host, host->getActionName("LaserBegin"));
        }

        if (!mIsTargetLocked && !getHost()->isLastPhase3Bowser()) {
            lockTarget();
        }

        if (mIsEnableLaserLightEffects) {
            al::appearPrePassLight(getHost(), "Laser", -1);
        }
    }

    if (al::isGreaterEqualStep(this, mParam.laserEarlyStartFrame)) {
        updateLaser();
    } else if (getHost()->isLastPhase3Bowser() &&
               al::isLessEqualStep(this,
                                   mCurrentParam->chargeFramesUntilTargetLockLaserBeginChase)) {
        updateTargetData();
        calcLaserTarget(mLaserTarget);
        getHost()->updateJointAim(mLaserTarget);

        if (al::isStep(this, mCurrentParam->chargeFramesUntilTargetLockLaserBeginChase)) {
            lockTarget();
        }
    } else {
        getHost()->updateJointAim(mTarget);
    }

    SuperBowser* host = getHost();
    bool isBegin = al::isActionPlaying(host, host->getActionName("LaserBegin"));
    if ((isChase2nd || isBegin) && al::isActionEnd(getHost())) {
        al::setNerve(this, &NrvSuperBowserLaserStateShoot);
    }
}

/**
 * @brief Updates the beam while it is shot: casts it, aims it, damages and plays its effects
 * until the follow frames are over.
 */
void SuperBowserLaserState::updateLaser() {
    if (mPlayer == nullptr) {
        return;
    }

    updateTargetData();
    mLaserOrigin = getLaserOrigin();

    if (mShootFrame == 0) {
        mIsShooting = true;
        effectsLaserStart();
        enableLaserSensor();
        mIsLaserEnding = false;
        mAimVelocity = sead::Vector3f::zero;
        mIsForceTargetKeepCross = false;
        mLaserLength = 1.0f;
        mLaserGrowCounter = 0;
        mExtendFrame = 0;
        mExtendStartLength = 1.0f;
        mWaterSurfaceY = al::getTrans(getHost()).y - getHost()->getBaseOffset();
        mEndDelay = 0;
        initRayInfo();

        if (getHost()->isLastPhase3Bowser()) {
            calcRayLeadIndex();
        }
    }

    s32 lockFrame = mPlayer->isRaidonExist() ? mParam.laserVelocityLockFramePlessie :
                                               mParam.laserVelocityLockFrame;
    if (getHost()->isLastPhase3Bowser()) {
        if (SingleModeDataFunction::isDarkBowserV2Available(GameDataHolderAccessor(getHost()))) {
            lockFrame = mParam.laserVelocityLockFramePlessieChaseV2;
        } else {
            lockFrame = mParam.laserVelocityLockFramePlessieChase;
        }
    }

    if (mShootFrame > lockFrame) {
        mIsAimLocked = true;
    } else if (!getHost()->isLastPhase3Bowser()) {
        calcRayLeadIndex();
    }

    if (mEndDelay > 0) {
        mEndDelay--;
        if (mEndDelay == 0) {
            al::setNerve(this, &NrvSuperBowserLaserStateEnd);
            return;
        }
    }

    mLaserEnd = mLaserOrigin;

    if (al::isNerve(this, &NrvSuperBowserLaserStateShoot) && !getHost()->isLastPhase3Bowser()) {
        getHost()->updateTurning(nullptr, 0);
    }

    updateAimTrack(mLaserOrigin);
    (this->*mAimFunc)(mLaserOrigin);
    limitLaserAngle();
    mLaserDir = mTarget - mLaserOrigin;
    mLaserDir.normalize();
    calcLaserBasis();
    calcHitPosition();
    updateDamagePosition();
    getHost()->updateJointAim(mTarget);
    effectsLaserUpdate();

    if (mClosestRayIndex >= 0 && al::isNearZero(mRays[mClosestRayIndex].normal.y, 0.001f)) {
        al::validateHitSensor(getHost(), "LaserPlayerOnly");
    } else {
        al::invalidateHitSensor(getHost(), "LaserPlayerOnly");
    }

    if (mShootFrame < getFollowFramesPre() + mCurrentParam->followFrames) {
        updateMidBeamSensors();
        updateLaserHitEffectMtx();
        mShootFrame++;
        return;
    }

    if (mCurrentParam->isSweepLeftThenRight) {
        mSweepSign = -mSweepSign;
    }

    al::setNerve(this, &NrvSuperBowserLaserStateEnd);
}

/**
 * @brief Shoots the laser.
 */
void SuperBowserLaserState::exeShoot() {
    if (al::isFirstStep(this)) {
        getHost()->setGuideBalloonVisible(false);
        SuperBowser* host = getHost();
        al::tryStartAction(host, host->getActionName("Laser"));
    }

    updateLaser();
}

/**
 * @brief Ends the laser: shrinks the beam, fades the lighting back and either charges the
 * next laser or ends the attack.
 */
void SuperBowserLaserState::exeEnd() {
    bool isSkipEndAnim = shouldSkipEndAnim();

    if (al::isFirstStep(this)) {
        if (!isSkipEndAnim) {
            SuperBowser* host = getHost();
            al::tryStartAction(host, host->getActionName("LaserEnd"));
        }

        startShrink();
    }

    if (!getHost()->isLastPhase3Bowser()) {
        SuperBowser* host = getHost();
        al::getActionFrameMax(host, al::getActionName(host));
        s32 step = sead::Mathi::min(al::getNerveStep(this), cLightFadeFrames);
        f32 rate = step / (f32)cLightFadeFrames;
        f32 prevRate = sead::Mathi::max(step - 1, 0) / (f32)cLightFadeFrames;

        if (rate < 0.5f) {
            laserLightSettingsSetColor(mParam.laserColor * (1.0f - (rate + rate)));
        } else if (rate > 0.5f && prevRate <= 0.5f) {
            laserLightSettingsSetDirection(mSavedLightDir);
        } else {
            laserLightSettingsSetColor(mSavedLightColor * ((rate - 0.5f) * 2.0f));
        }
    }

    if (!isSkipEndAnim) {
        SuperBowser* host = getHost();
        if (!al::isActionPlaying(host, host->getActionName("LaserEnd")) ||
            !al::isActionEnd(getHost())) {
            return;
        }
    }

    mLaserNum++;
    if (mLaserNum < mCurrentParam->laserCount && !getHost()->tryCancelNextLaser()) {
        al::setNerve(this, &NrvSuperBowserLaserStateCharge);
        return;
    }

    laserLightSettingsRestore();
    al::showShadowDepth(getHost());
    kill();
}

/**
 * @brief Check whether the end action is skipped, which happens between the lasers of the
 * Plessie chase.
 * @return True if the end action is skipped.
 */
bool SuperBowserLaserState::shouldSkipEndAnim() const {
    if (getHost()->isLastPhase3Bowser()) {
        if (mLaserNum + 1 < mCurrentParam->laserCount) {
            return true;
        }

        if (getHost()->isPlessieChaseBigRamp()) {
            return true;
        }

        if (getHost()->isPlessieChaseLv4()) {
            return true;
        }
    }

    return false;
}

/**
 * @brief Starts shrinking the beam: stops its light, effects, sounds and sensors.
 */
void SuperBowserLaserState::startShrink() {
    al::killPrePassLight(getHost(), "Laser", -1);
    effectsLaserStop();
    disableLaserSensor();
    mIndicator.fadeOut();

    if (mIsShooting) {
        startLaserEndEffect();
        mIsShooting = false;
    }
}

/**
 * @brief Check whether the laser is charging.
 * @return True if charging.
 */
bool SuperBowserLaserState::isCharging() const {
    return al::isNerve(this, &NrvSuperBowserLaserStateCharge);
}

/**
 * @brief Check whether the beam is being shot.
 * @return True if shooting.
 */
bool SuperBowserLaserState::isShooting() const {
    return mIsShooting;
}

/**
 * @brief Check whether the laser is ending.
 * @return True if ending.
 */
bool SuperBowserLaserState::isEnding() const {
    return al::isNerve(this, &NrvSuperBowserLaserStateEnd);
}

/**
 * @brief Forces the laser to aim at a point instead of the player.
 * @param rTarget Point to aim at.
 * @param isKeepCross Whether the follow offsets keep being applied.
 */
void SuperBowserLaserState::forceTarget(const sead::Vector3f& rTarget, bool isKeepCross) {
    mIsForceTarget = !isKeepCross;
    mIsForceTargetKeepCross = isKeepCross;
    mForceTarget = rTarget;
}

/**
 * @brief Forces the laser to sweep from one point to another.
 * @param rStart Start of the sweep.
 * @param rEnd End of the sweep.
 */
void SuperBowserLaserState::forceSweepTarget(const sead::Vector3f& rStart,
                                             const sead::Vector3f& rEnd) {
    mIsForceTarget = true;
    mForceTarget = rStart;
    mForceSweepEnd = rEnd;
    mIsSweep = true;
}

/**
 * @brief Stops forcing the target of the laser.
 */
void SuperBowserLaserState::forceTargetDisable() {
    mIsForceTarget = false;
    mIsSweep = false;
}

/**
 * @brief Get the charge effect matching the kind of Fury Bowser.
 * @return Name of the effect.
 */
const char* SuperBowserLaserState::getChargeEffectName() const {
    switch (getHost()->getBowserType()) {
    case 3:
        return "LaserChargeMid";
    case 5:
    case 7:
    case 8:
    case 10:
        return "LaserChargeFast";
    default:
        return "LaserCharge";
    }
}

/**
 * @brief Sets the color of the beam effects.
 * @param color New color.
 */
void SuperBowserLaserState::setBeamColor(sead::Color4f color) {
    al::setEffectParticleColor(getHost(), "Laser", color);
    al::setEffectParticleColor(getHost(), "LaserCap", color);
    mEffectSystem->calcParticle(reinterpret_cast<u64>(getHost()->getEffectKeeper()));
}

/**
 * @brief Damages the player with the beam unless a wall stands between the mouth and them.
 * @param pSelf Sensor of the beam.
 * @param pOther Sensor of the player.
 * @return True unless a wall blocks the beam.
 */
bool SuperBowserLaserState::tryDamagePlayer(al::HitSensor* pSelf, al::HitSensor* pOther) {
    sead::Vector3f headPos =
        al::getSensorPos(al::getHitSensor(al::tryFindNearestPlayerActor(getHost()), "Head"));
    al::CollisionPartsFilterSpecialPurpose filter("BowserLaser");

    if (alCollisionUtil::tryGetStrikeArrowCollisionSensor(
            getHost(), mLineCheckStart, headPos - mLineCheckStart, &filter, nullptr) != nullptr) {
        return false;
    }

    if (al::sendMsgLaserAttack(pSelf, pOther)) {
        if (!getHost()->isLastPhase3Bowser() ||
            !SingleModeDataFunction::isDarkBowserV2Available(GameDataHolderAccessor(getHost())) ||
            getHost()->isPlessieChaseBigRamp()) {
            mLaserNum = mCurrentParam->laserCount;
        }

        getHost()->notifyPlayerHit();

        if (mEndDelay == 0 && mShootFrame >= mEndDelayMinFrame) {
            mEndDelay = mEndDelayFrames;
        }
    }

    return true;
}

/**
 * @brief Check whether the line from the mouth to the player's head is free.
 * @param isUpdateStart Whether the line starts again from the mouth, as before a jump.
 * @return True if a wall blocks the line.
 */
bool SuperBowserLaserState::checkPlayerLineSegment(bool isUpdateStart) {
    sead::Vector3f headPos =
        al::getSensorPos(al::getHitSensor(al::tryFindNearestPlayerActor(getHost()), "Head"));

    if (isUpdateStart) {
        mLineCheckStart = getLaserOrigin() +
                          sead::Vector3f::ey * mParam.playerLineCheckStartPosYOffsetBeforeJump;
    }

    al::CollisionPartsFilterSpecialPurpose filter("BowserLaser");
    al::HitSensor* sensor = alCollisionUtil::tryGetStrikeArrowCollisionSensor(
        getHost(), mLineCheckStart, headPos - mLineCheckStart, &filter, nullptr);
    if (sensor == nullptr) {
        return false;
    }

    if (isUpdateStart &&
        al::isEqualString(al::getSensorHost(sensor)->getName(), "BlockHardLaserOnly")) {
        return false;
    }

    return true;
}

/**
 * @brief Ends the laser after a short delay.
 */
void SuperBowserLaserState::endAfterDelay() {
    mEndDelay = mEndDelayFrames;
}

/**
 * @brief Updates the beam while it shrinks after being shot: its end keeps flying away while
 * it thins out, damaging until it is gone.
 */
void SuperBowserLaserState::tryUpdateLaserEndEffect() {
    if (!mIsLaserEnding) {
        return;
    }

    if (mEndFrame == 0) {
        al::validateHitSensor(getHost(), "Laser");
        al::validateHitSensor(getHost(), "Laser2");
        al::validateHitSensor(getHost(), "Laser3");
        al::validateHitSensor(getHost(), "Laser4");
        al::validateHitSensor(getHost(), "Laser5");
        al::validateHitSensor(getHost(), "Laser6");
        al::validateHitSensor(getHost(), "Laser7");
        al::validateHitSensor(getHost(), "Laser8");
        al::validateHitSensor(getHost(), "Laser9");
    }

    f32 rate = al::easeInOut(1.0f - (f32)mEndFrame / (f32)mParam.beamEndFrames);
    mLaserOrigin = getLaserOrigin();
    mLaserEnd += 2.0f * mEndVelocity;

    if (mParam.beamEndAcceleration != 0.0f) {
        sead::Vector3f accel = mEndVelocity;
        accel.normalize();
        mEndVelocity += accel * mParam.beamEndAcceleration;
    }

    f32 length = (mLaserEnd - mLaserOrigin).length() / 100.0f;
    f32 scale = rate * mParam.effectScale;
    al::setEffectParticleScale(getHost(), "LaserCap", sead::Vector3f::ones * scale);
    setLaserBeamEffectScale(sead::Vector3f(length, scale, scale));
    sead::Vector3f endScale = sead::Vector3f::ones;
    endScale.z = length;
    al::setEffectEmitterScale(getHost(), "LaserEnd", endScale);

    sead::Vector3f dir = mLaserEnd - mLaserOrigin;
    dir.normalize();
    sead::Quatf quat = sead::Quatf::unit;
    al::makeQuatSideUp(&quat, dir, sead::Vector3f::ey);
    mLaserMtx.makeQT(quat, mLaserOrigin);
    mLaserCapMtx.makeQT(quat, mLaserEnd);

    if (mEndFrame < mParam.beamEndFramesOfDamage) {
        f32 radius = rate * mParam.laserRadiusSensor;
        al::setSensorRadius(getHost(), "Laser", radius);
        al::setSensorRadius(getHost(), "Laser2", radius);
        al::setSensorRadius(getHost(), "Laser3", radius);
        al::setSensorRadius(getHost(), "Laser4", radius);
        al::setSensorRadius(getHost(), "Laser5", radius);
        al::setSensorRadius(getHost(), "Laser6", radius);
        al::setSensorRadius(getHost(), "Laser7", radius);
        al::setSensorRadius(getHost(), "Laser8", radius);
        al::setSensorRadius(getHost(), "Laser9", radius);

        mSensorPos[0] =
            mLaserEnd - dir * mParam.laserRadiusSensor + dir * mParam.damageSensorOffset;
        for (s32 i = 1; i < cSensorNum; i++) {
            mSensorPos[i] = mSensorPos[i - 1] - dir * (mParam.laserRadiusSensor * 2.0f);
        }
    }

    if (mEndFrame == mParam.beamEndFramesOfDamage && !mIsShooting) {
        al::invalidateHitSensor(getHost(), "Laser");
        al::invalidateHitSensor(getHost(), "Laser2");
        al::invalidateHitSensor(getHost(), "Laser3");
        al::invalidateHitSensor(getHost(), "Laser4");
        al::invalidateHitSensor(getHost(), "Laser5");
        al::invalidateHitSensor(getHost(), "Laser6");
        al::invalidateHitSensor(getHost(), "Laser7");
        al::invalidateHitSensor(getHost(), "Laser8");
        al::invalidateHitSensor(getHost(), "Laser9");
    }

    if (mEndFrame >= mParam.beamEndFrames) {
        al::tryDeleteEffect(getHost(), "Laser");
        al::tryDeleteEffect(getHost(), "LaserCap");
        mIsLaserEnding = false;

        if (mIsLightSettingsRecorded) {
            mIsLightSettingsRecorded = false;
        }
    }

    mEndFrame++;
}

/**
 * @brief Get the origin of the beam, Fury Bowser's mouth.
 * @return The origin.
 */
sead::Vector3f SuperBowserLaserState::getLaserOrigin() const {
    return al::getSensorPos(al::getHitSensor(getHost(), "MouthLaser"));
}

/**
 * @brief Computes the scale of the beam effect: its length and its thickness.
 * @return The scale.
 */
sead::Vector3f SuperBowserLaserState::calcLaserEffectScale() {
    f32 length = (mLaserEnd - mLaserOrigin).length() / 100.0f;
    return {length, mParam.effectScale, mParam.effectScale};
}

/**
 * @brief Scales the beam effect.
 * @param scale Length (x) and thickness (y, z) of the beam.
 */
void SuperBowserLaserState::setLaserBeamEffectScale(sead::Vector3f scale) {
    al::Effect* effect = getHost()->getEffectKeeper()->findEffect("Laser");
    const_cast<al::EffectInfo*>(effect->getEffectInfo())->mParam.mScale = scale.x;
    scale.set(1.0f, scale.y / scale.x, scale.z / scale.x);
    al::setEffectParticleScale(getHost(), "Laser", scale);
    al::setEffectEmitterScale(getHost(), "Laser", scale);
}

/**
 * @brief Records the directional light, to restore it after the laser.
 */
void SuperBowserLaserState::laserLightSettingsRecord() {
    const al::DirLightParam& light = getDirLightParam(getHost());
    mSavedLightDir = light.getDirection()->getDirection();
    const sead::Color4f& color = light.getColor();
    mSavedLightColor.set(color.r, color.g, color.b);
    mIsLightSettingsRecorded = true;
}

/**
 * @brief Sets the direction and color of the directional light.
 * @param rDir New direction.
 * @param rColor New color.
 */
void SuperBowserLaserState::laserLightSettingsSet(const sead::Vector3f& rDir,
                                                  const sead::Vector3f& rColor) {
    al::DirLightParam& light = getDirLightParam(getHost());
    light.getDirection()->setDirection(rDir);
    light.getColor() = sead::Color4f(rColor.x, rColor.y, rColor.z, 1.0f);
}

/**
 * @brief Computes the start and end of the sweep around the laser target.
 */
void SuperBowserLaserState::calcSweepPositions() {
    if (mIsSweep) {
        mSweepStart = mLaserTarget + mSide * (f32)mSweepOffsetStart;
        mSweepEnd = mLaserTarget - mSide * (f32)mSweepOffsetEnd;
    } else {
        mSweepStart = mLaserTarget + mSide * mCurrentParam->sweepOffsetStart * mSweepSign;
        mSweepEnd = mLaserTarget - mSide * mCurrentParam->sweepOffsetEnd * mSweepSign;
    }
}

/**
 * @brief Keeps the aim point within the maximum angle from Fury Bowser's face.
 */
void SuperBowserLaserState::limitLaserAngle() {
    sead::Vector3f toTarget = mTarget - mLaserOrigin;
    f32 distance = toTarget.length();
    toTarget.normalize();

    const sead::Matrix34f* faceMtx = al::getJointMtxPtr(getHost(), "Face_P");
    sead::Vector3f facePos(faceMtx->m[0][3], faceMtx->m[1][3], faceMtx->m[2][3]);
    const sead::Matrix34f* beamMtx = al::getJointMtxPtr(getHost(), "Beam01");
    sead::Vector3f faceDir =
        sead::Vector3f(beamMtx->m[0][3], beamMtx->m[1][3], beamMtx->m[2][3]) - facePos;
    faceDir.normalize();

    if (al::calcAngleDegree(toTarget, faceDir) > mParam.laserAngleMax) {
        sead::Vector3f axis;
        axis.setCross(faceDir, toTarget);

        if (!al::isNearZero(axis, 0.001f)) {
            axis.normalize();
            al::rotateVectorDegree(&toTarget, faceDir, axis, mParam.laserAngleMax);
            mTarget = mLaserOrigin + toTarget * distance;
        }
    }
}

/**
 * @brief Stops the laser after its last shot.
 */
void SuperBowserLaserState::cancel() {
    mLaserNum = mCurrentParam->laserCount;
    al::setNerve(this, &NrvSuperBowserLaserStateEnd);
}

/**
 * @brief Ends the current laser.
 */
void SuperBowserLaserState::endLaser() {
    al::setNerve(this, &NrvSuperBowserLaserStateEnd);
}

/**
 * @brief Stops the mouth and hit effects and the beam sound.
 */
void SuperBowserLaserState::effectsLaserStop() {
    al::tryDeleteEffect(getHost(), "LaserMouth");
    al::tryDeleteEffect(getHost(), "LaserWaterHit");
    al::tryDeleteEffect(getHost(), "LaserLandHit");
    al::tryStopSe(getHost(), "PgLaserBeamStart");
}

/**
 * @brief Starts the shrinking of the beam, which keeps the speed of its aim.
 */
void SuperBowserLaserState::startLaserEndEffect() {
    mIsLaserEnding = true;
    mEndFrame = 0;
    mEndVelocity = mAimVelocity;
    al::startSe(getHost(), "PgLaserBeamEnd");
    al::startSe(mSeNear, "PgLaserBeamEndNear");

    if (shouldSkipEndAnim()) {
        al::emitEffect(getHost(), "LaserEnd", nullptr);
    }
}

/**
 * @brief Check whether the beam is shrinking.
 * @return True if shrinking.
 */
bool SuperBowserLaserState::isLaserShrinking() {
    return mIsLaserEnding;
}

/**
 * @brief Sets whether the laser may change the lighting of the scene.
 * @param isEnable Whether the laser may change the lighting.
 */
void SuperBowserLaserState::enableLaserLightEffects(bool isEnable) {
    mIsEnableLaserLightEffects = isEnable;

    if (isEnable) {
        laserLightSettingsRecord();
        return;
    }

    laserLightSettingsRestore();
    mIndicator.fadeOut();
}

/**
 * @brief Check whether the laser may change the lighting of the scene.
 * @return True if it may.
 */
bool SuperBowserLaserState::isEnableLaserLightEffects() const {
    return mIsEnableLaserLightEffects;
}

/**
 * @brief Get the base parameters of the laser.
 * @return The parameters.
 */
const SuperBowserLaserStateParam* SuperBowserLaserState::getParam() const {
    return &mParam;
}

/**
 * @brief Aim function that sweeps the beam from the sweep start to the sweep end.
 * @param rOrigin Origin of the beam.
 */
void SuperBowserLaserState::updateAimSweep(const sead::Vector3f& rOrigin) {
    if (mCurrentParam->isSweepFollowTarget && mPlayer != nullptr) {
        calcLaserTarget(mLaserTarget);
        calcSweepPositions();
    }

    f32 rate = (f32)(al::getNerveStep(this) - mCurrentParam->preSweepFrames) /
               mCurrentParam->sweepFrames;
    rate = sead::Mathf::clamp(rate, 0.0f, 1.0f);
    f32 sweepRate = std::pow(rate, 3.0);
    al::lerpVec(&mTarget, mSweepStart, mSweepEnd, sweepRate);
    limitLaserAngle();
    mLaserDir = mTarget - rOrigin;
    mLaserDir.normalize();
}

/**
 * @brief Get the frames the beam waits before following the player, shorter later in the
 * Plessie chase.
 * @return The frames, including the extend frames.
 */
s32 SuperBowserLaserState::getFollowFramesPre() {
    s32 frames = mCurrentParam->followFramesPre;

    if (getHost()->isLastPhase3Bowser()) {
        GigaBellManager* manager = tryGetGigaBellManager(this);
        if (manager != nullptr) {
            if (manager->getPlessieChaseHitCount() == 1) {
                frames = mCurrentParam->followFramesPrePlessieChaseLv2;
            } else if (manager->getPlessieChaseHitCount() >= 2) {
                frames = mCurrentParam->followFramesPrePlessieChaseLv3;
            }
        }
    }

    return mParam.extendFrames + frames;
}

/**
 * @brief Computes the acceleration of the aim point towards the laser target, split along
 * the beam's side, the global up and the direction to Fury Bowser.
 * @return The acceleration.
 */
sead::Vector3f SuperBowserLaserState::calcLaserAccel() const {
    sead::Vector3f diff = mLaserTarget - mTarget;
    const sead::Vector3f& hostTrans = al::getTrans(getHost());
    sead::Vector3f tangent(hostTrans.x - mLaserTarget.x, 0.0f, hostTrans.z - mLaserTarget.z);
    tangent.normalize();

    f32 crossDiff = diff.dot(mSide);
    f32 tangentDiff = diff.dot(tangent);
    f32 accelCross = mCurrentParam->followAccelCross;
    f32 accelTangent = mCurrentParam->followAccelTangent;

    if (getHost()->isLastPhase3Bowser()) {
        GigaBellManager* manager = tryGetGigaBellManager(this);
        if (manager != nullptr) {
            if (manager->getPlessieChaseHitCount() == 1) {
                accelCross = mCurrentParam->followAccelCrossPlessieChaseLv2;
            } else if (manager->getPlessieChaseHitCount() >= 2) {
                accelCross = mCurrentParam->followAccelCrossPlessieChaseLv3;
            }

            if (mCurrentParam == &mParamV2 && manager->isPlessieChaseLv(3)) {
                accelTangent = mCurrentParam->followAccelTangentPlessieChaseV2Lv3;
            }
        }
    }

    f32 accelNormal = mCurrentParam->followAccelNormal;
    return mSide * (crossDiff * accelCross) + sead::Vector3f::ey * (diff.y * accelNormal) +
           tangent * (tangentDiff * accelTangent);
}

/**
 * @brief Computes the front, side and up axes of the beam from its direction.
 */
void SuperBowserLaserState::calcLaserBasis() {
    mFront = mLaserDir;
    mSide.setCross(sead::Vector3f::ey, mFront);
    mSide.normalize();
    mUp.setCross(mFront, mSide);
    mUp.normalize();
}

/**
 * @brief Places the starts of the rays around the mouth: a cross of inner rays and a ring of
 * outer rays.
 */
void SuperBowserLaserState::calcRayOrigins() {
    mRays[0].start = mLaserOrigin;
    mRays[1].start = mLaserOrigin + mSide * mParam.laserRadiusInner;
    mRays[2].start = mLaserOrigin - mSide * mParam.laserRadiusInner;
    mRays[3].start =
        mLaserOrigin +
        mUp * (mParam.laserRadiusInner - mParam.laserRadiusInnerVerticalOffset);
    mRays[4].start =
        mLaserOrigin -
        mUp * (mParam.laserRadiusInner - mParam.laserRadiusInnerVerticalOffset);

    for (s32 i = 0; i < cOuterRayNum; i++) {
        f32 angle = sead::Mathf::deg2rad(i * 360.0f / cOuterRayNum);
        sead::Vector3f& start = mRays[cOuterRayStart + i].start;
        start = mLaserOrigin;
        start += mSide * mParam.laserRadiusOuter * cosf(angle);
        start += mUp * mParam.laserRadiusOuter * sinf(angle);
    }

    mRays[13].start =
        mLaserOrigin + mSide * mParam.laserRadiusInner -
        mUp * (mParam.laserRadiusInner - mParam.laserRadiusInnerVerticalOffset);
    mRays[14].start =
        mLaserOrigin - mSide * mParam.laserRadiusInner -
        mUp * (mParam.laserRadiusInner - mParam.laserRadiusInnerVerticalOffset);
}

/**
 * @brief Resets every ray to a miss straight ahead.
 */
void SuperBowserLaserState::initRayInfo() {
    calcRayOrigins();

    for (s32 i = 0; i < cRayNum; i++) {
        Ray& ray = mRays[i];
        ray.isHit = false;
        ray.angle = 0.0f;
        ray.sensor = nullptr;
        ray.hitPos = ray.start + mFront * cRayLength;
        ray.normal = -mFront;
        ray.isHitWater = false;
    }
}

/**
 * @brief Casts the rays due this frame and computes where the beam ends: the closest hit of
 * the rays allowed to block it, pushed in by the penetration.
 */
void SuperBowserLaserState::calcHitPosition() {
    calcRayOrigins();

    f32 closestDist = 3.4028235e+38f;
    sead::Vector3f closestPos;
    al::CollisionPartsFilterSpecialPurpose filter("BowserLaser");

    for (s32 i = 0; i < cRayNum; i++) {
        Ray& ray = mRays[i];

        if (shouldUpdateRay(mShootFrame, i)) {
            ray.isHit = alCollisionUtil::getHitPosAndNormalAndSensorOnArrow(
                getHost(), &ray.hitPos, &ray.normal, &ray.sensor, ray.start, mFront * cRayLength,
                &filter, nullptr);

            if (!ray.isHit) {
                ray.hitPos = ray.start + mFront * cRayLength;
                ray.sensor = nullptr;
                ray.normal = -mFront;
            }

            if (ray.hitPos.y > ray.start.y) {
                ray.isHitWater = false;
            } else {
                f32 rate = std::abs(mWaterSurfaceY - ray.start.y) /
                           std::abs(ray.hitPos.y - ray.start.y);
                if (rate >= 0.0f && rate < 1.0f) {
                    ray.hitPos = ray.start + (ray.hitPos - ray.start) * rate;
                    ray.isHitWater = true;
                    ray.isHit = true;
                    ray.normal = sead::Vector3f::ey;
                    ray.sensor = nullptr;
                } else {
                    ray.isHitWater = false;
                }
            }

            ray.angle = al::calcAngleDegree(ray.normal, -mFront);
            doRaySpecialSensorChecks(i);
        }

        if (canRayBlockLaser(i)) {
            f32 dist = (ray.hitPos - ray.start).dot(mFront);
            if (dist < closestDist) {
                closestDist = dist;
                closestPos = mLaserOrigin + mFront * dist;
                mClosestRayIndex = i;
            }
        }
    }

    updateLaserLength(closestPos, closestDist);
    checkHitDisasterSpike();
    checkHitDisasterBlocks();
    updateLaserPenetration();
    mLaserEnd += mFront * mPenetration;
}

/**
 * @brief Check whether a ray is cast this frame: the lead rays every frame, the others in
 * turns of three frames.
 * @param frame Frame of the beam.
 * @param index Index of the ray.
 * @return True if the ray is cast.
 */
bool SuperBowserLaserState::shouldUpdateRay(s32 frame, s32 index) const {
    if (mLeadRayIndex[0] == index) {
        return true;
    }

    if (mLeadRayIndex[1] == index) {
        return true;
    }

    switch (frame % 3) {
    case 0:
        return index == 0 || index == 3 || index == 13;
    case 1:
        return index == 1 || index == 2 || index == 14;
    case 2:
        return index == 0 || index == 4;
    default:
        return false;
    }
}

/**
 * @brief Checks a ray against the sensors of the unlocked Giga Bells, which the beam hits.
 * @param index Index of the ray.
 */
void SuperBowserLaserState::doRaySpecialSensorChecks(s32 index) {
    GigaBellManager* manager = tryGetGigaBellManager(this);
    if (manager == nullptr) {
        return;
    }

    for (s32 i = 0; i < manager->getGigaBellCount(); i++) {
        GigaBell* bell = manager->getGigaBell(i);
        if (!bell->isUnlocked()) {
            continue;
        }

        al::HitSensor* sensor1 = al::getHitSensor(bell, "Unlocked1");
        al::HitSensor* sensor2 = al::getHitSensor(bell, "Unlocked2");

        if (sensor1 != nullptr) {
            doRayToSensorCheck(index, sensor1);
        }

        if (sensor2 != nullptr) {
            doRayToSensorCheck(index, sensor2);
        }
    }
}

/**
 * @brief Check whether a ray may stop the beam: inner rays and lead rays always, outer rays
 * only on surfaces facing the beam.
 * @param index Index of the ray.
 * @return True if the ray may stop the beam.
 */
bool SuperBowserLaserState::canRayBlockLaser(s32 index) const {
    if (index < cOuterRayStart || index >= cOuterRayStart + cOuterRayNum) {
        return true;
    }

    if (mLeadRayIndex[0] == index) {
        return true;
    }

    if (mLeadRayIndex[1] == index) {
        return true;
    }

    return mRays[index].angle < 90.0f - mParam.laserOuterRayAngleThreshold;
}

/**
 * @brief Updates the length of the beam: it grows smoothly when its end jumps away and
 * shrinks at once.
 * @param pos New end of the beam.
 * @param length New length of the beam.
 */
void SuperBowserLaserState::updateLaserLength(sead::Vector3f pos, f32 length) {
    mLaserGrowCounter = sead::Mathi::min(mLaserGrowCounter + 1, mParam.laserGrowDelayFrames);

    if (mLaserLength < length) {
        if (mLaserGrowCounter == 1) {
            mExtendFrame = 0;
            mExtendStartLength = mLaserLength;
        }

        if (mExtendFrame < mCurrentParam->extendFrames) {
            mLaserLength = al::lerpValue((f32)mExtendFrame / (f32)mCurrentParam->extendFrames,
                                         mExtendStartLength, length);
            mLaserEnd = mLaserOrigin + mFront * mLaserLength;
            mExtendFrame++;
            return;
        }
    } else {
        mLaserGrowCounter = 0;
    }

    mLaserLength = length;
    mLaserEnd = pos;
}

/**
 * @brief Damages the disaster spikes hit by the beam near its end.
 */
void SuperBowserLaserState::checkHitDisasterSpike() {
    if (mClosestRayIndex < 0) {
        return;
    }

    for (s32 i = 0; i < 8; i++) {
        const Ray& ray = mRays[i];
        if (ray.sensor == nullptr) {
            continue;
        }

        const char* name = al::getSensorHost(ray.sensor)->getName();
        if (!al::isEqualString(name, "DisasterSpike") &&
            !al::isEqualString(name, "DisasterSpikeBouncy") &&
            !al::isEqualString(name, "DisasterSpikeGold")) {
            continue;
        }

        f32 dx = ray.hitPos.x - mLaserEnd.x;
        f32 dz = ray.hitPos.z - mLaserEnd.z;
        if (dx * dx + dz * dz <= mParam.laserRadiusSensor * mParam.laserRadiusSensor) {
            al::sendMsgLaserAttack(ray.sensor, mMouthSensor);
        }
    }
}

/**
 * @brief Breaks the laser-only blocks hit by the beam near its end.
 */
void SuperBowserLaserState::checkHitDisasterBlocks() {
    if (mClosestRayIndex < 0) {
        return;
    }

    for (s32 i = 0; i < 8; i++) {
        const Ray& ray = mRays[i];
        if (ray.sensor == nullptr ||
            !al::isEqualString(al::getSensorHost(ray.sensor)->getName(), "BlockHardLaserOnly")) {
            continue;
        }

        f32 dx = ray.hitPos.x - mLaserEnd.x;
        f32 dz = ray.hitPos.z - mLaserEnd.z;
        if (dx * dx + dz * dz <=
            mParam.laserDisasterBlockRadius * mParam.laserDisasterBlockRadius) {
            al::sendMsgLaserAttack(ray.sensor, mMouthSensor);
        }
    }
}

/**
 * @brief Updates how far the beam end goes into what it hits, deeper on steep surfaces.
 */
void SuperBowserLaserState::updateLaserPenetration() {
    f32 penetration;

    if (isHitSurfaceGround(mClosestRayIndex)) {
        f32 range = mParam.penetrationDegMax - mParam.penetrationDegMin;
        f32 rate = 1.0f;

        if (range != 0.0f) {
            rate = (90.0f - mRays[mClosestRayIndex].angle - mParam.penetrationDegMin) / range;
            rate = sead::Mathf::clamp(rate, 0.0f, 1.0f);
        }

        penetration = al::lerpValue(rate, mParam.penetrationOffsetMin, mParam.penetrationOffsetMax);
    } else {
        penetration = mParam.penetrationOffsetMax;
    }

    mPenetration = al::lerpValue(mPenetrationRate, mPenetration, penetration);
}

/**
 * @brief Computes the offset of the target along the player's horizontal velocity.
 * @return The offset.
 */
sead::Vector3f SuperBowserLaserState::calcTargetVelocityOffset() {
    sead::Vector3f offset = mPlayerVelocity;
    offset.y = 0.0f;

    if (getHost()->isLastPhase3Bowser() &&
        SingleModeDataFunction::isDarkBowserV2Available(GameDataHolderAccessor(getHost()))) {
        offset *= mParam.targetVelocityOffsetScalePlessieChaseV2;
    } else {
        offset *= mParam.targetVelocityOffsetScale;
    }

    al::lerpVec(&mVelocityOffset, mVelocityOffset, offset, mParam.targetVelocityOffsetRate);
    return mVelocityOffset;
}

/**
 * @brief Starts the beam sound and effects.
 */
void SuperBowserLaserState::effectsLaserStart() {
    al::startSe(getHost(), "PgLaserBeamStart");
    al::tryEmitEffect(getHost(), "LaserMouth", nullptr);
    al::tryEmitEffect(getHost(), "Laser", nullptr);
    al::tryEmitEffect(getHost(), "LaserCap", nullptr);
}

/**
 * @brief Plays the hit effects (water or land) where the beam ends, its echo and its sound.
 */
void SuperBowserLaserState::effectsLaserHitPlay() {
    if (mShootFrame == 0) {
        mHitSeCounter = 0;
    }

    if (mLaserGrowCounter != 0 && mExtendFrame < mCurrentParam->extendFrames) {
        return;
    }

    mIsHitWater = shouldPlayWaterHitEffect();

    if (mIsHitWater) {
        if (!al::isEffectEmitting(getHost(), "LaserWaterHit")) {
            al::tryEmitEffect(getHost(), "LaserWaterHit", nullptr);
        }

        if (al::isEffectEmitting(getHost(), "LaserLandHit")) {
            al::tryDeleteEffect(getHost(), "LaserLandHit");
        }
    } else {
        if (!al::isEffectEmitting(getHost(), "LaserLandHit")) {
            al::tryEmitEffect(getHost(), "LaserLandHit", nullptr);
        }

        if (al::isEffectEmitting(getHost(), "LaserWaterHit")) {
            al::tryDeleteEffect(getHost(), "LaserWaterHit");
        }
    }

    rc::emitEcho(getHost(), mLaserEnd, mEchoRadius, mEchoFrames, false);

    if (mHitSeCounter % 30 == 0) {
        al::tryStartSe(getHost(), "LaserHitLand");
    }

    mHitSeCounter++;
}

/**
 * @brief Check whether most of the rays ending near the beam end hit water.
 * @return True if the water hit effect should be played.
 */
bool SuperBowserLaserState::shouldPlayWaterHitEffect() const {
    static const s32 cInnerRayIndices[] = {0, 1, 2, 3, 4, 13, 14};
    s32 waterNum = 0;
    u8 nearNum = 0;

    for (s32 i = 0; i < 7; i++) {
        if (isRayHitNearClosest(cInnerRayIndices[i])) {
            nearNum++;
            waterNum += mRays[cInnerRayIndices[i]].isHitWater;
        }
    }

    for (s32 i = 0; i < 2; i++) {
        if (isRayHitNearClosest(mLeadRayIndex[i])) {
            nearNum++;
            waterNum += mRays[mLeadRayIndex[i]].isHitWater;
        }
    }

    return waterNum > nearNum / 2;
}

/**
 * @brief Updates the beam effects, its light and the position of its sounds.
 */
void SuperBowserLaserState::effectsLaserUpdate() {
    mLaserCapMtx.setBase(0, mFront);
    mLaserCapMtx.setBase(1, mUp);
    mLaserCapMtx.setBase(2, -mSide);
    mLaserCapMtx.setTranslation(mLaserEnd);

    sead::Quatf quat = sead::Quatf::unit;
    al::makeQuatSideUp(&quat, mFront, mUp);
    mLaserMtx.makeQT(quat, mLaserOrigin);
    effectsLaserHitPlay();
    effectsLaserScale();

    sead::Vector3f lightStart =
        al::getTrans(getHost()) + sead::Vector3f(0.0f, -getHost()->getBaseOffset() + 150.0f, 0.0f);
    sead::Vector3f lightEnd(mTarget.x, lightStart.y, mTarget.z);
    sead::Vector3f lightDir = lightEnd - lightStart;
    f32 lightLength = lightDir.length();
    sead::Vector3f lightSide;
    lightSide.setCross(sead::Vector3f::ey, lightDir);
    sead::Vector3f lightUp;
    lightUp.setCross(lightDir, lightSide);
    lightSide.normalize();
    sead::Vector3f lightCenter = (lightEnd + lightStart) * 0.5f;
    lightUp.normalize();
    lightDir.normalize();
    mLaserLightMtx.setBase(0, lightSide);
    mLaserLightMtx.setBase(1, lightUp);
    mLaserLightMtx.setBase(2, lightDir);
    mLaserLightMtx.setTranslation(lightCenter);
    mLaserLight->mParam.mLength = lightLength;

    al::holdSe(getHost(), "PgLaserBeamLv");
    al::updatePoseMtx(mSeTip, &mLaserCapMtx);
    al::holdSe(mSeTip, "PgLaserBeamTipLv");

    sead::Vector3f playerTrans = al::getTrans(mPlayer);
    al::LinearCurve curve;
    curve.set(mLaserOrigin, mLaserEnd);
    sead::Vector3f nearPos;
    curve.calcNearestPos(&nearPos, playerTrans);
    sead::Matrix34f nearMtx;
    al::makeMtxFrontUpPos(&nearMtx, mLaserDir, sead::Vector3f::ey, nearPos);
    al::updatePoseMtx(mSeNear, &nearMtx);
    al::holdSe(mSeNear, "PgLaserBeamNearLv");
}

/**
 * @brief Scales the cap and beam effects.
 */
void SuperBowserLaserState::effectsLaserScale() {
    al::setEffectParticleScale(getHost(), "LaserCap", sead::Vector3f::ones * mParam.effectScale);
    setLaserBeamEffectScale(calcLaserEffectScale());
}

/**
 * @brief Enables the damage sensors of the beam and attaches them to the damage positions.
 */
void SuperBowserLaserState::enableLaserSensor() {
    al::setHitSensorPosPtr(getHost(), "Laser", &mSensorPos[0]);
    al::setHitSensorPosPtr(getHost(), "Laser2", &mSensorPos[1]);
    al::setHitSensorPosPtr(getHost(), "Laser3", &mSensorPos[2]);
    al::setHitSensorPosPtr(getHost(), "Laser4", &mSensorPos[3]);
    al::setHitSensorPosPtr(getHost(), "Laser5", &mSensorPos[4]);
    al::setHitSensorPosPtr(getHost(), "Laser6", &mSensorPos[5]);
    al::setHitSensorPosPtr(getHost(), "Laser7", &mSensorPos[6]);
    al::setHitSensorPosPtr(getHost(), "Laser8", &mSensorPos[7]);
    al::setHitSensorPosPtr(getHost(), "Laser9", &mSensorPos[8]);
    al::setHitSensorPosPtr(getHost(), "LaserPlayerOnly", &mPlayerOnlySensorPos);
    al::setSensorRadius(getHost(), "Laser", mParam.laserRadiusSensor);
    al::setSensorRadius(getHost(), "Laser2", mParam.laserRadiusSensor);
    al::setSensorRadius(getHost(), "Laser3", mParam.laserRadiusSensor);
    al::setSensorRadius(getHost(), "Laser4", mParam.laserRadiusSensor);
    al::setSensorRadius(getHost(), "Laser5", mParam.laserRadiusSensor);
    al::setSensorRadius(getHost(), "Laser6", mParam.laserRadiusSensor);
    al::setSensorRadius(getHost(), "Laser7", mParam.laserRadiusSensor);
    al::setSensorRadius(getHost(), "Laser8", mParam.laserRadiusSensor);
    al::setSensorRadius(getHost(), "Laser9", mParam.laserRadiusSensor);
    al::setSensorRadius(getHost(), "LaserMiddle", mParam.laserRadiusSensor);
    al::setSensorRadius(getHost(), "LaserMiddle2", mParam.laserRadiusSensor);
    al::setSensorRadius(getHost(), "LaserPlayerOnly", mParam.laserRadiusSensorPlayerOnly);
    al::validateHitSensor(getHost(), "Laser");
    al::validateHitSensor(getHost(), "Laser2");
    al::validateHitSensor(getHost(), "Laser3");
    al::validateHitSensor(getHost(), "Laser4");
    al::validateHitSensor(getHost(), "Laser5");
    al::validateHitSensor(getHost(), "Laser6");
    al::validateHitSensor(getHost(), "Laser7");
    al::validateHitSensor(getHost(), "Laser8");
    al::validateHitSensor(getHost(), "Laser9");
    al::validateHitSensor(getHost(), "LaserMiddle");
    al::validateHitSensor(getHost(), "LaserMiddle2");
}

/**
 * @brief Moves the position of the middle sensors along the beam, looping back to the mouth.
 */
void SuperBowserLaserState::updateMidBeamRate() {
    f32 step = (s32)mParam.laserRadiusSensor * 2;
    mMidBeamRate += step / (mSensorPos[0] - mLaserOrigin).length();
    if (mMidBeamRate > 1.0f) {
        mMidBeamRate = 0.0f;
    }
}

/**
 * @brief Places the two middle sensors along the beam, half a beam apart.
 */
void SuperBowserLaserState::updateMidBeamSensors() {
    updateMidBeamRate();

    f32 rate = mMidBeamRate + 0.5f;
    if (rate > 1.0f) {
        rate += -1.0f;
    }

    sead::Vector3f offset1 = calcMidBeamPosition(mMidBeamRate) - al::getTrans(getHost());
    sead::Vector3f offset2 = calcMidBeamPosition(rate) - al::getTrans(getHost());

    sead::Quatf invQuat;
    invQuat.setInverse(al::getQuat(getHost()));
    offset1.rotate(invQuat);
    invQuat.setInverse(al::getQuat(getHost()));
    offset2.rotate(invQuat);
    al::setSensorFollowPosOffset(getHost(), "LaserMiddle", offset1);
    al::setSensorFollowPosOffset(getHost(), "LaserMiddle2", offset2);
}

/**
 * @brief Computes a position between the mouth and the first damage sensor.
 * @param rate Position along the beam, from 0 (mouth) to 1.
 * @return The position.
 */
sead::Vector3f SuperBowserLaserState::calcMidBeamPosition(f32 rate) const {
    sead::Vector3f pos = sead::Vector3f::zero;
    al::lerpVec(&pos, mLaserOrigin, mSensorPos[0], rate);
    return pos;
}

/**
 * @brief Picks the two outer rays that lead the beam: the one towards where the aim is
 * moving and the one opposite.
 */
void SuperBowserLaserState::calcRayLeadIndex() {
    sead::Vector3f aimDiff = mLaserTarget - mTarget;
    f32 closestDist = -1.0f;

    for (s32 i = cOuterRayStart; i < cOuterRayStart + cOuterRayNum; i++) {
        f32 dist = (mRays[i].start - mLaserOrigin - aimDiff).squaredLength();
        if (closestDist == -1.0f || dist < closestDist) {
            closestDist = dist;
            mLeadRayIndex[0] = i;
        }
    }

    mLeadRayIndex[1] = ((mLeadRayIndex[0] - 1) & (cOuterRayNum - 1)) + cOuterRayStart;
}

/**
 * @brief Places the damage sensors along the beam from its end back to the mouth.
 */
void SuperBowserLaserState::updateDamagePosition() {
    mSensorPos[0] =
        mLaserEnd - mFront * mParam.laserRadiusSensor + mFront * mParam.damageSensorOffset;
    for (s32 i = 1; i < cSensorNum; i++) {
        mSensorPos[i] = mSensorPos[i - 1] - mFront * (mParam.laserRadiusSensor * 2.0f);
    }

    mPlayerOnlySensorPos = mSensorPos[0] + mFront * mPlayerOnlySensorFrontOffset +
                           mUp * mPlayerOnlySensorUpOffset;
}

/**
 * @brief Places the hit effects: on the water surface, or on the surface the beam hits.
 */
void SuperBowserLaserState::updateLaserHitEffectMtx() {
    if (mIsHitWater) {
        sead::Quatf quat = sead::Quatf::unit;
        al::makeQuatSideUp(&quat, sead::Vector3f::ey, mSide);
        sead::Vector3f pos = mFront * mParam.laserRadiusSensor + mLaserEnd;
        pos.y = mWaterSurfaceY;
        mHitEffectMtx.makeQT(quat, pos);
        return;
    }

    sead::Vector3f normal = mRays[mClosestRayIndex].normal;
    sead::Vector3f absNormal = normal;
    absVec(&absNormal);
    sead::Vector3f absSide = mSide;
    absVec(&absSide);

    if (al::isNear(absNormal, absSide, 0.1f) || al::isNearZero(normal, 0.001f) ||
        al::isNearZero(mSide, 0.001f)) {
        mHitEffectMtx.fromQuat(sead::Quatf::unit);
    } else {
        sead::Quatf quat = sead::Quatf::unit;
        al::makeQuatSideUp(&quat, normal, mSide);
        mHitEffectMtx.fromQuat(quat);
    }

    mHitEffectMtx.setTranslation(mRays[mClosestRayIndex].hitPos);
}

/**
 * @brief Get the charge sound matching the kind of Fury Bowser.
 * @return Name of the sound.
 */
const char* SuperBowserLaserState::getChargeSeName() const {
    switch (getHost()->getBowserType()) {
    case 3:
        return "PgCharge2";
    case 5:
        return "PgCharge3";
    case 8:
        return "PgCharge4";
    default:
        return "PgCharge1";
    }
}

/**
 * @brief Check whether a ray hit a surface flat enough to be ground.
 * @param index Index of the ray.
 * @return True if the surface is ground.
 */
bool SuperBowserLaserState::isHitSurfaceGround(s32 index) const {
    return mRays[index].normal.y >=
           sinf(sead::Mathf::deg2rad(90.0f - mParam.laserSlopeThresholdAngle));
}

/**
 * @brief Checks a ray against a spherical sensor, which stops it if hit closer than its
 * current hit.
 * @param index Index of the ray.
 * @param pSensor The sensor.
 */
void SuperBowserLaserState::doRayToSensorCheck(s32 index, al::HitSensor* pSensor) {
    sead::Vector3f sensorPos = al::getSensorPos(pSensor);
    f32 radius = al::getSensorRadius(pSensor);
    Ray& ray = mRays[index];

    sead::Vector3f toSensor = sensorPos - ray.start;
    sead::Vector3f nearPos = ray.start + mLaserDir * toSensor.dot(mLaserDir);
    f32 distance = (nearPos - sensorPos).length();
    if (distance > radius) {
        return;
    }

    f32 angle = acosf(distance / radius);
    sead::Vector3f normal = sead::Vector3f::zero;
    toSensor.normalize();
    sead::Vector3f axis;
    axis.setCross(toSensor, mLaserDir);

    if (al::isNearZero(axis, 0.001f)) {
        normal = -mLaserDir;
    } else {
        f32 degree = sead::Mathf::rad2deg(angle);
        axis.normalize();
        al::rotateVectorDegree(&normal, nearPos - sensorPos, axis, degree);
        normal.normalize();
    }

    sead::Vector3f hitPos = sensorPos + normal * radius;
    if ((hitPos - ray.start).dot(mFront) < (ray.hitPos - ray.start).dot(mFront)) {
        ray.isHit = true;
        ray.hitPos = hitPos;
        ray.normal = normal;
        ray.angle = al::calcAngleDegree(normal, -mFront);
        ray.sensor = pSensor;
        ray.isHitWater = false;
    }
}

/**
 * @brief Check whether a ray hit near the end of the beam.
 * @param index Index of the ray.
 * @return True if the hit is near the end.
 */
bool SuperBowserLaserState::isRayHitNearClosest(s32 index) const {
    f32 distanceSq = (mRays[index].hitPos - mLaserEnd).squaredLength();
    f32 radius = mParam.laserRadiusSensor + mParam.laserRadiusSensor;
    return distanceSq <= radius * radius;
}
