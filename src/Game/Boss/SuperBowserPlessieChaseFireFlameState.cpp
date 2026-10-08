#include "Boss/SuperBowserPlessieChaseFireFlameState.hpp"

#include <cmath>
#include <math/seadMatrix.h>

#include "Boss/SuperBowserLaserState.hpp"
#include "Enemy/SuperBowser.hpp"
#include "Enemy/SuperBowserFireFlamePlessieChase.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/LiveActor/Util/ActorSceneInfo.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Math/MatrixUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Yaml/ByamlIter.hpp"
#include "MapObj/SePlayObj.hpp"
#include "Player/Normal/PlayerActor.hpp"
#include "Project/Rail/LinearCurve.hpp"
#include "Raidon/RaidonSurf.hpp"
#include "Scene/SceneObjID.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"

namespace {
NERVE_DECL(SuperBowserPlessieChaseFireFlameState, Wait)
NERVE_DECL(SuperBowserPlessieChaseFireFlameState, SweepBegin)
NERVE_DECL(SuperBowserPlessieChaseFireFlameState, SweepShoot)
NERVE_DECL(SuperBowserPlessieChaseFireFlameState, SweepDelay)
NERVE_DECL(SuperBowserPlessieChaseFireFlameState, RingShoot)
NERVE_DECL(SuperBowserPlessieChaseFireFlameState, RingEnd)
NERVE_DECL(SuperBowserPlessieChaseFireFlameState, RingBegin)

NERVES_MAKE_NOSTRUCT(SuperBowserPlessieChaseFireFlameState, Wait, SweepBegin, SweepShoot,
                     SweepDelay, RingShoot, RingEnd, RingBegin)

/** @brief Distance of the point Fury Bowser's head aims at along the shot direction. */
constexpr f32 cAimDistance = 10000.0f;

/** @brief Name of the init file holding the fireball parameters. */
constexpr const char* cInitFileName = "InitAttackSuperBowserLastPhase3";
}  // namespace

/**
 * @brief Constructs the fireball state, creates the fireball pool, reads the parameters and
 * creates the sound objects of the ring and sweep attacks.
 * @param pHost Fury Bowser.
 * @param rInfo Actor init info.
 */
SuperBowserPlessieChaseFireFlameState::SuperBowserPlessieChaseFireFlameState(
    SuperBowser* pHost, const al::ActorInitInfo& rInfo)
    : al::HostStateBase<SuperBowser>("PlessieChaseFireFlameState", pHost) {
    initNerve(&NrvSuperBowserPlessieChaseFireFlameStateWait, 0);
    mSceneObjHolder = rInfo.getActorSceneInfo().sceneObjHolder;
    createFireFlamePool(rInfo);
    initFromYaml();

    mRingSe = new SePlayObj("PlessieChaseFireFlameSoundActor");
    mRingSe->initAttachedUpdatePose(rInfo);
    al::updatePoseMtx(mRingSe, pHost->getBaseMtx());
    al::invalidateClipping(mRingSe);

    mSweepSe = new SePlayObj("PlessieChaseFireFlameSweepSe");
    mSweepSe->initAttachedUpdatePose(rInfo);
    al::updatePoseMtx(mSweepSe, pHost->getBaseMtx());
    al::invalidateClipping(mSweepSe);
}

/**
 * @brief Creates the fireballs of the pool.
 * @param rInfo Actor init info.
 */
void SuperBowserPlessieChaseFireFlameState::createFireFlamePool(const al::ActorInitInfo& rInfo) {
    for (s32 i = 0; i < mFlames.capacity(); i++) {
        auto* flame = new SuperBowserFireFlamePlessieChase("SuperBowserFireFlamePlessieChase", this);
        al::initCreateActorNoPlacementInfo(flame, rInfo);
        mFlames.pushBack(flame);
    }
}

/** @brief Reads the fireball parameters from the init file. */
void SuperBowserPlessieChaseFireFlameState::initFromYaml() {
    al::ByamlIter fileIter;
    al::ByamlIter iter;

    if (al::tryGetActorInitFileIter(&fileIter, getHost(), cInitFileName, nullptr)) {
        fileIter.tryGetIterByKey(&iter, "PlessieChaseFireFlames");
        iter.tryGetFloatByKey(&mParam.flameEffectScale, "FlameEffectScale");
        iter.tryGetFloatByKey(&mParam.flameSpeed, "FlameSpeed");
        iter.tryGetFloatByKey(&mParam.shootOriginOffset, "ShootOriginOffset");
        iter.tryGetFloatByKey(&mParam.disappearOffset, "DisappearOffset");
        iter.tryGetFloatByKey(&mParam.disappearOffsetSweep, "DisappearOffsetSweep");
        iter.tryGetIntByKey(&mParam.sweepShootDelayFrames, "SweepShootDelayFrames");
        iter.tryGetFloatByKey(&mParam.sweepFirstShotTarget.x, "SweepFirstShotTargetX");
        iter.tryGetFloatByKey(&mParam.sweepFirstShotTarget.y, "SweepFirstShotTargetY");
        iter.tryGetFloatByKey(&mParam.sweepFirstShotTarget.z, "SweepFirstShotTargetZ");
        iter.tryGetIntByKey(&mParam.ringShootInterval, "RingShootInterval");
        iter.tryGetIntByKey(&mParam.ringFrames, "RingFrames");
        iter.tryGetIntByKey(&mParam.ringFramesV2First, "RingFramesV2First");
        iter.tryGetFloatByKey(&mParam.ringShootTargetYOffset, "RingShootTargetYOffset");
        iter.tryGetFloatByKey(&mParam.ringShootTargetYOffsetTierJump,
                              "RingShootTargetYOffsetTierJump");
        iter.tryGetFloatByKey(&mParam.ringRotationOffsetV2First, "RingRotationOffsetV2First");
        iter.tryGetFloatByKey(&mParam.ringPlayerLead, "RingPlayerLead");
        iter.tryGetFloatByKey(&mParam.ringPlayerLeadTierJump, "RingPlayerLeadTierJump");
        iter.tryGetFloatByKey(&mParam.ringScale, "RingScale");
        iter.tryGetFloatByKey(&mParam.ringScaleOffsetRange, "RingScaleOffsetRange");
        iter.tryGetFloatByKey(&mParam.ringSoundObjectSpeed, "RingSoundObjectSpeed");
    }

    initSweepParam(&mSweepParamV1, "PlessieChaseFireFlamesSweepV1");
    initSweepParam(&mSweepParamV2, "PlessieChaseFireFlamesSweepV2");
    initSweepParam(&mSweepParamLv4BigRamp, "PlessieChaseFireFlamesSweepLv4BigRamp");
}

/**
 * @brief Get the scene object holder.
 * @return The holder.
 */
al::SceneObjHolder* SuperBowserPlessieChaseFireFlameState::getSceneObjHolder() const {
    return mSceneObjHolder;
}

/** @brief Starts a series of fireball sweeps, picking the sweep variant of the chase. */
void SuperBowserPlessieChaseFireFlameState::appear() {
    al::NerveStateBase::appear();

    if (getHost()->isPlessieChaseBigRamp() && getHost()->isPlessieChaseLv4() &&
        !getHost()->isPlessieChaseTierJump()) {
        mSweepParam = &mSweepParamLv4BigRamp;
    } else if (SingleModeDataFunction::isDarkBowserV2Available(
                   GameDataHolderAccessor(getHost()))) {
        mSweepParam = &mSweepParamV2;
    } else {
        mSweepParam = &mSweepParamV1;
    }

    mIsSweepReverse = false;
    mSweepCount = 0;
    mIsOneShot = false;
    al::setNerve(this, &NrvSuperBowserPlessieChaseFireFlameStateSweepBegin);
}

/** @brief Ends the attack immediately. */
void SuperBowserPlessieChaseFireFlameState::forceKill() {
    al::setNerve(this, &NrvSuperBowserPlessieChaseFireFlameStateWait);
    kill();
}

/** @brief Waits while the attack is inactive. */
void SuperBowserPlessieChaseFireFlameState::exeWait() {}

/** @brief Plays the wind-up of a sweep while turning Fury Bowser's head to its start. */
void SuperBowserPlessieChaseFireFlameState::exeSweepBegin() {
    if (al::isFirstStep(this)) {
        al::startAction(getHost(), "ChaseFireballBegin");
    } else if (al::isActionEnd(getHost())) {
        al::setNerve(this, &NrvSuperBowserPlessieChaseFireFlameStateSweepShoot);
    }

    calcSweepShootInfo();
    const sead::Vector3f& dir = mIsSweepReverse ? mSweepEndDir : mSweepStartDir;
    sead::Vector3f target = dir * cAimDistance + mShootOrigin;

    if (al::isFirstStep(this)) {
        startJointAim(target, al::getActionFrameMax(getHost(), "ChaseFireballBegin"));
    } else {
        getHost()->updateJointAim(target);
    }

    mSweepFlames.clear();
}

/**
 * @brief Computes the origin of the sweep and the directions it starts and ends at, around the
 * fixed target, the first shot target or the player.
 */
void SuperBowserPlessieChaseFireFlameState::calcSweepShootInfo() {
    mShootOrigin = al::getSensorPos(al::getHitSensor(getHost(), "MouthLaser"));

    sead::Vector3f target;
    if (mSweepCount == 0 && !mIsOneShot) {
        target = al::getTrans(getHost()) + mParam.sweepFirstShotTarget;
    } else if (mSweepParam->isUseFixedTarget) {
        target = al::getTrans(getHost()) + mSweepParam->fixedTarget;
    } else {
        target = al::getTrans(al::findNearestPlayerActor(getHost())) +
                 sead::Vector3f::ey * mSweepParam->shootTargetYOffset;
    }

    sead::Vector3f dir = target - mShootOrigin;
    dir.normalize();
    mShootOrigin += dir * mParam.shootOriginOffset;
    mSweepStartDir = dir;
    mSweepEndDir = dir;
    al::rotateVectorDegreeY(&mSweepStartDir, mSweepParam->shootAngle * -0.5f);
    al::rotateVectorDegreeY(&mSweepEndDir, mSweepParam->shootAngle * 0.5f);
    mShootDir = mSweepStartDir;
    mSoundDir = mSweepStartDir;
}

/**
 * @brief Makes Fury Bowser turn his head and spine towards a target, with the joint limits of
 * his laser.
 * @param target Point to aim at.
 * @param frames Frames the turn takes.
 */
void SuperBowserPlessieChaseFireFlameState::startJointAim(sead::Vector3f target, s32 frames) {
    SuperBowser::BowserJointState state;
    state.mConstraintBeam = getHost()->getLaserState()->getParam()->jointConstraintBeam;
    state.mConstraintFace = getHost()->getLaserState()->getParam()->jointConstraintFace;
    state.mConstraintNeck = getHost()->getLaserState()->getParam()->jointConstraintNeck;
    state.mConstraintSpine = getHost()->getLaserState()->getParam()->jointConstraintSpine;
    state.mConstraintHip = getHost()->getLaserState()->getParam()->jointConstraintHip;
    state.mPitchBeam = getHost()->getLaserState()->getParam()->jointPitchBeam;
    state.mPitchFace = getHost()->getLaserState()->getParam()->jointPitchFace;
    state.mPitchNeck = getHost()->getLaserState()->getParam()->jointPitchNeck;
    state.mPitchSpine = getHost()->getLaserState()->getParam()->jointPitchSpine;
    state.mPitchHip = getHost()->getLaserState()->getParam()->jointPitchHip;
    state.mRate = 1.0f;
    state.mTarget = target;
    getHost()->startJointAim(state, frames, 3, 120);
}

/** @brief Sweeps the fireballs from one side to the other, shooting at a fixed interval. */
void SuperBowserPlessieChaseFireFlameState::exeSweepShoot() {
    if (al::isFirstStep(this)) {
        al::startAction(getHost(), "ChaseFireball");
        calcSweepShootInfo();
    }

    f32 rate = static_cast<f32>(al::getNerveStep(this)) /
               static_cast<f32>(mSweepParam->shootFrames);
    al::lerpVec(&mShootDir, mSweepStartDir, mSweepEndDir, mIsSweepReverse ? 1.0f - rate : rate);
    getHost()->updateJointAim(mShootDir * cAimDistance + mShootOrigin);

    if (al::getNerveStep(this) % mSweepParam->shootInterval == 0) {
        shootFireFlame(mShootOrigin, mShootDir, mParam.disappearOffsetSweep, false);
    }

    if (al::isGreaterEqualStep(this, mSweepParam->shootFrames)) {
        mIsSweepReverse = !mIsSweepReverse;
        mSweepCount++;
        al::setNerve(this, &NrvSuperBowserPlessieChaseFireFlameStateSweepDelay);
    }
}

/**
 * @brief Shoots a fireball of the pool, if one is free, and starts the sound of the attack.
 * @param pos Position to shoot from.
 * @param dir Direction to shoot in.
 * @param disappearOffset Distance after which the fireball disappears.
 * @param isFirstShot Whether this is the first fireball of a ring.
 */
void SuperBowserPlessieChaseFireFlameState::shootFireFlame(sead::Vector3f pos, sead::Vector3f dir,
                                                           f32 disappearOffset,
                                                           bool isFirstShot) {
    SuperBowserFireFlamePlessieChase* flame = nullptr;
    for (s32 i = 0; i < mFlames.capacity(); i++) {
        if (al::isDead(mFlames[i])) {
            flame = mFlames[i];
            break;
        }
    }

    if (flame == nullptr) {
        return;
    }

    if (al::isNerve(this, &NrvSuperBowserPlessieChaseFireFlameStateSweepShoot)) {
        mSweepFlames.pushBack(flame);
    }

    flame->startAttack(pos, dir, sead::Vector3f::ey, mParam.flameEffectScale, mParam.flameSpeed,
                       disappearOffset, false, 1.0f, false);

    if (al::isNerve(this, &NrvSuperBowserPlessieChaseFireFlameStateRingShoot) && isFirstShot &&
        !mIsRingSePlaying) {
        sead::Matrix34f mtx;
        al::makeMtxFrontUpPos(&mtx, mSoundDir, sead::Vector3f::ey, mRingOrigin);
        al::updatePoseMtx(mRingSe, &mtx);
        al::setVelocity(mRingSe, mSoundDir * mParam.ringSoundObjectSpeed);
        al::startSe(mRingSe, "SuperBowserFireRingLv");
        mIsRingSePlaying = true;
    } else if (al::isNerve(this, &NrvSuperBowserPlessieChaseFireFlameStateSweepShoot) &&
               !mIsSweepSePlaying) {
        sead::Matrix34f mtx;
        al::makeMtxFrontUpPos(&mtx, mSoundDir, sead::Vector3f::ey, mRingOrigin);
        al::updatePoseMtx(mSweepSe, &mtx);
        al::startSe(mSweepSe, "SuperBowserFireLineLv");
        mIsSweepSePlaying = true;
    }
}

/** @brief Waits between two sweeps, or ends a single sweep. */
void SuperBowserPlessieChaseFireFlameState::exeSweepDelay() {
    if (al::isFirstStep(this)) {
        al::startAction(getHost(), "ChaseFireballEnd");
    } else if (al::isActionPlaying(getHost(), "ChaseFireballEnd") &&
               al::isActionEnd(getHost())) {
        if (mIsOneShot) {
            kill();
            return;
        }

        al::startAction(getHost(), "ChaseWait");
    }

    if (al::isGreaterEqualStep(this, mParam.sweepShootDelayFrames)) {
        al::setNerve(this, &NrvSuperBowserPlessieChaseFireFlameStateSweepBegin);
    }
}

/** @brief Plays the wind-up of a ring while turning Fury Bowser's head to the player. */
void SuperBowserPlessieChaseFireFlameState::exeRingBegin() {
    if (al::isFirstStep(this)) {
        al::startAction(getHost(), "ChaseFireballBegin");
    } else if (al::isActionEnd(getHost())) {
        al::setNerve(this, &NrvSuperBowserPlessieChaseFireFlameStateRingShoot);
    }

    calcRingShootInfo();
    sead::Vector3f target = mShootDir * cAimDistance + mShootOrigin;

    if (al::isFirstStep(this)) {
        startJointAim(target, al::getActionFrameMax(getHost(), "ChaseFireballBegin"));
    } else {
        getHost()->updateJointAim(target);
    }
}

/** @brief Computes the origin and the direction of a ring of fireballs. */
void SuperBowserPlessieChaseFireFlameState::calcRingShootInfo() {
    mShootOrigin = al::getSensorPos(al::getHitSensor(getHost(), "MouthLaser"));
    mRingOrigin = mShootOrigin;
    mShootDir = calcRingTargetPos() - mShootOrigin;
    mShootDir.normalize();
    mShootOrigin += mShootDir * mParam.shootOriginOffset;
    mSoundDir = mShootDir;
}

/** @brief Shoots the fireballs of a ring around the direction of the player. */
void SuperBowserPlessieChaseFireFlameState::exeRingShoot() {
    if (al::isFirstStep(this)) {
        al::startAction(getHost(), "ChaseFireball");
        calcRingShootInfo();
    }

    s32 ringFrames = mParam.ringFrames;
    bool isV2First = false;
    if (SingleModeDataFunction::isDarkBowserV2Available(GameDataHolderAccessor(getHost())) &&
        getHost()->isIgnoreBellOffsetY()) {
        ringFrames = mParam.ringFramesV2First;
        isV2First = true;
    }

    if (al::isGreaterEqualStep(this, ringFrames)) {
        al::setNerve(this, &NrvSuperBowserPlessieChaseFireFlameStateRingEnd);
        return;
    }

    getHost()->updateJointAim(mShootDir * cAimDistance + mShootOrigin);

    if (al::getNerveStep(this) % mParam.ringShootInterval != 0) {
        return;
    }

    sead::Vector3f side;
    side.setCross(sead::Vector3f::ey, mShootDir);
    sead::Vector3f up;
    up.setCross(mShootDir, side);

    f32 rate = static_cast<f32>(al::getNerveStep(this)) / static_cast<f32>(ringFrames);
    sead::Vector3f ringOffset = sead::Vector3f::zero;
    f32 offset = isV2First ? mParam.ringRotationOffsetV2First : 0.0f;
    f32 angle = sead::Mathf::deg2rad(rate * 360.0f + offset);

    ringOffset += side * std::cos(angle);
    ringOffset -= up * std::sin(angle);

    sead::Vector3f dir = mShootDir + ringOffset * mParam.ringScale;
    dir.normalize();

    if (mParam.ringScaleOffsetRange > 0.0f) {
        dir += ringOffset * al::getRandom(0.0f, mParam.ringScaleOffsetRange);
        dir.normalize();
    }

    shootFireFlame(mShootOrigin, dir, mParam.disappearOffset, al::isFirstStep(this));
}

/** @brief Plays the end of the ring attack and ends the state. */
void SuperBowserPlessieChaseFireFlameState::exeRingEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(getHost(), "ChaseFireballEnd");
        kill();
    }
}

/** @brief Moves the sound of the sweep along the fireballs while it plays. */
void SuperBowserPlessieChaseFireFlameState::control() {
    if (mIsSweepSePlaying) {
        updateFlameSweepSoundActorPos();
    }
}

/**
 * @brief Moves the sound of the sweep to the point of the line of living fireballs that is the
 * nearest to the player, and stops it once no fireball is left.
 */
void SuperBowserPlessieChaseFireFlameState::updateFlameSweepSoundActorPos() {
    if (mSweepFlames.size() <= 0) {
        return;
    }

    SuperBowserFireFlamePlessieChase* first = nullptr;
    SuperBowserFireFlamePlessieChase* last = nullptr;
    for (s32 i = 0; i < mSweepFlames.size(); i++) {
        SuperBowserFireFlamePlessieChase* flame = mSweepFlames[i];
        if (flame != nullptr && al::isAlive(flame)) {
            if (first == nullptr) {
                first = flame;
            } else {
                last = flame;
            }
        }
    }

    if (first == nullptr) {
        al::tryStopSe(mSweepSe, "SuperBowserFireLineLv");
        mIsSweepSePlaying = false;
        return;
    }

    if (last == nullptr) {
        al::updatePoseMtx(mSweepSe, first->getBaseMtx());
        return;
    }

    sead::Vector3f playerPos = al::getTrans(al::findNearestPlayerActor(getHost()));
    sead::Vector3f nearest;
    al::LinearCurve curve;
    curve.set(al::getTrans(first), al::getTrans(last));
    curve.calcNearestPos(&nearest, playerPos);

    sead::Matrix34f mtx;
    al::makeMtxFrontUpPos(&mtx, sead::Vector3f::ex, sead::Vector3f::ey, nearest);
    al::updatePoseMtx(mSweepSe, &mtx);
}

/** @brief Starts a ring of fireballs. */
void SuperBowserPlessieChaseFireFlameState::startRingAttack() {
    al::setNerve(this, &NrvSuperBowserPlessieChaseFireFlameStateRingBegin);
}

/** @brief Starts a single fireball sweep. */
void SuperBowserPlessieChaseFireFlameState::startOneShotSweep() {
    appear();
    mIsOneShot = true;
}

/**
 * @brief Get the fireball parameters.
 * @return A copy of the parameters.
 */
SuperBowserPlessieChaseFireFlameParam SuperBowserPlessieChaseFireFlameState::getParam() const {
    return mParam;
}

/**
 * @brief Computes the point a ring of fireballs aims at: above the player, led by the player's
 * horizontal velocity.
 * @return The target.
 */
sead::Vector3f SuperBowserPlessieChaseFireFlameState::calcRingTargetPos() const {
    sead::Vector3f pos = al::getTrans(al::findNearestPlayerActor(getHost()));
    f32 yOffset = getHost()->isPlessieChaseTierJump() ? mParam.ringShootTargetYOffsetTierJump :
                                                        mParam.ringShootTargetYOffset;
    pos += sead::Vector3f::ey * yOffset;

    f32 lead = getHost()->isPlessieChaseTierJump() ? mParam.ringPlayerLeadTierJump :
                                                     mParam.ringPlayerLead;
    return pos + getPlayerVelocityXZ() * lead;
}

/**
 * @brief Get the horizontal velocity of the player, or of Plessie when the player rides it.
 * @return The velocity without its vertical component.
 */
sead::Vector3f SuperBowserPlessieChaseFireFlameState::getPlayerVelocityXZ() const {
    const al::LiveActor* actor = al::findNearestPlayerActor(getHost());
    if (static_cast<const PlayerActor*>(actor)->isRaidonExist()) {
        actor = al::tryGetSceneObj<RaidonSurf>(this, SceneObjID_RaidonSurf);
    }

    const sead::Vector3f& velocity = al::getVelocity(actor);
    return {velocity.x, 0.0f, velocity.z};
}

/** @brief Stops the sounds of the attack once the state ended and every fireball is gone. */
void SuperBowserPlessieChaseFireFlameState::tryUpdateSoundObjectStop() {
    if (!isDead()) {
        return;
    }

    if (!mIsRingSePlaying && !mIsSweepSePlaying) {
        return;
    }

    for (s32 i = 0; i < mFlames.capacity(); i++) {
        if (al::isAlive(mFlames[i])) {
            return;
        }
    }

    al::setVelocityZero(mRingSe);
    al::stopAllSeFromUser(mRingSe, 0);
    mIsRingSePlaying = false;
    al::stopAllSeFromUser(mSweepSe, 0);
    mIsSweepSePlaying = false;
}

/**
 * @brief Reads the parameters of a sweep variant from the init file.
 * @param pParam Parameters to fill.
 * @param pKey Key of the variant in the init file.
 */
void SuperBowserPlessieChaseFireFlameState::initSweepParam(
    SuperBowserPlessieChaseFireFlameSweepParam* pParam, const char* pKey) {
    al::ByamlIter fileIter;
    al::ByamlIter iter;

    if (al::tryGetActorInitFileIter(&fileIter, getHost(), cInitFileName, nullptr)) {
        fileIter.tryGetIterByKey(&iter, pKey);
        iter.tryGetIntByKey(&pParam->shootInterval, "SweepShootInterval");
        iter.tryGetIntByKey(&pParam->shootFrames, "SweepShootFrames");
        iter.tryGetFloatByKey(&pParam->shootAngle, "SweepShootAngle");
        iter.tryGetFloatByKey(&pParam->shootTargetYOffset, "SweepShootTargetYOffset");
        iter.tryGetBoolByKey(&pParam->isUseFixedTarget, "SweepUseFixedTarget");
        iter.tryGetFloatByKey(&pParam->fixedTarget.x, "SweepFixedTargetX");
        iter.tryGetFloatByKey(&pParam->fixedTarget.y, "SweepFixedTargetY");
        iter.tryGetFloatByKey(&pParam->fixedTarget.z, "SweepFixedTargetZ");
    }
}
