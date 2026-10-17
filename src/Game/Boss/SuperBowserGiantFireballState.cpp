#include "Enemy/SuperBowserGiantFireballState.hpp"
#include "Enemy/SuperBowser.hpp"
#include "MapObj/KoopaFireBallGiant.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Yaml/ByamlIter.hpp"
namespace {
NERVE_DECL(SuperBowserGiantFireballState, Wait);
NERVE_DECL(SuperBowserGiantFireballState, ShootForever);
NERVES_MAKE_NOSTRUCT(SuperBowserGiantFireballState, Wait, ShootForever)
}
SuperBowserGiantFireballState::Param sParamAimOnce = {90, 2500.0f, 20.0f, 2500.0f, 60.0f, 150.0f, 350.0f, 450.0f, 50, 0.0f, 60.0f, 25, 0, 60, 60, 30, 12, 60, 1.1f, 0.7f, 150.0f, 20.0f, 2500.0f};
SuperBowserGiantFireballState::Param sParamContinuousTracking = {90, 2500.0f, 20.0f, 2500.0f, 60.0f, 150.0f, 350.0f, 450.0f, 50, 5.0f, 60.0f, 25, 0, 60, 60, 30, 12, 60, 1.1f, 0.7f, 150.0f, 20.0f, 2500.0f};
SuperBowserGiantFireballState::Param sParamVelocitySnapshot = {90, 2500.0f, 20.0f, 2500.0f, 60.0f, 150.0f, 350.0f, 450.0f, 50, 5.0f, 60.0f, 25, 30, 60, 60, 30, 12, 60, 1.1f, 0.7f, 150.0f, 20.0f, 2500.0f};

/** @brief Creates the giant-fireball pool and loads attack settings. @param pHost Fury Bowser. @param rInfo Initialization data. */
SuperBowserGiantFireballState::SuperBowserGiantFireballState(SuperBowser* pHost, const al::ActorInitInfo& rInfo)
    : HostStateBase("GiantFireball", pHost) {
    initNerve(&NrvSuperBowserGiantFireballStateWait, 0);
    createFireballPool(rInfo);
    initParam(rInfo);
}
/** @brief Destroys the attack state. */
SuperBowserGiantFireballState::~SuperBowserGiantFireballState() = default;
/** @brief Creates the reusable fireball actors. @param rInfo Initialization data. */
void SuperBowserGiantFireballState::createFireballPool(const al::ActorInitInfo& rInfo) {
    for (int i = 0; i < mFireballs.capacity(); i++) {
        auto* fireball = new KoopaFireBallGiant("KoopaFireBallGiant", getHost());
        al::initCreateActorNoPlacementInfo(fireball, rInfo);
        mFireballs.pushBack(fireball);
    }
}
/** @brief Loads giant-fireball attack settings. @param rInfo Initialization data. */
void SuperBowserGiantFireballState::initParam(const al::ActorInitInfo& rInfo) {
    al::ByamlIter file;
    al::ByamlIter iter;
    if (al::tryGetActorInitFileIter(&file, getHost(), "InitAttackSuperBowserCommon", nullptr)) {
        file.tryGetIterByKey(&iter, "GiantFireball");
        iter.tryGetIntByKey(&mParam.mArcFrame, "Arc Time");
        iter.tryGetFloatByKey(&mParam.mArcHeight, "Arc Height");
        iter.tryGetFloatByKey(&mParam.mFallSpeed, "Fall Speed");
        iter.tryGetFloatByKey(&mParam.mFallHeight, "Spawn Height");
        iter.tryGetFloatByKey(&mParam.mAngleRandomRange, "Angle Variance");
        iter.tryGetFloatByKey(&mParam.mDistanceRandomMin, "Distance Variance Min");
        iter.tryGetFloatByKey(&mParam.mDistanceRandomMax, "Distance Variance Max");
        iter.tryGetFloatByKey(&mParam.mShadowScale, "Shadow Scale");
        iter.tryGetIntByKey(&mParam.mShadowGrowTime, "Shadow Grow Time");
        iter.tryGetFloatByKey(&mParam.mMoveSpeed, "Tracking Speed");
        iter.tryGetFloatByKey(&mParam.mTrackingRate, "Tracking Lead");
        iter.tryGetIntByKey(&mParam.mHomingFrame, "Tracking Time");
        iter.tryGetIntByKey(&mParam.mAimFrame, "Snapshot Delay");
        iter.tryGetIntByKey(&mParam.mStartEndDelay, "Start and End Delay Frames");
        iter.tryGetIntByKey(&mParam.mPrimaryDelay, "In Between Delay Frames Primary");
        iter.tryGetIntByKey(&mParam.mSecondaryDelay, "In Between Delay Frames Secondary");
        iter.tryGetIntByKey(&mParam.mArcFrameStepMin, "In Between Shoot Delay Frames");
        iter.tryGetFloatByKey(&mParam.mFlyScale, "Scale Falling");
        iter.tryGetFloatByKey(&mParam.mLandScale, "Scale Landing");
        iter.tryGetFloatByKey(&mParam.mTrackingRateRaidon, "Tracking Lead Plessie");
        iter.tryGetFloatByKey(&mParam.mFallSpeedRaidon, "Fall Speed Plessie");
        iter.tryGetFloatByKey(&mParam.mFallHeightRaidon, "Spawn Height Plessie");
    }
}
/** @brief Starts a volley. @param shots Number of shots. @param secondary Selects the secondary interval. */
void SuperBowserGiantFireballState::appear(int shots, bool secondary) {
    if (isShootNowAndForever()) return;
    al::NerveStateBase::appear();
    mShotsLeft = shots;
    mShotIndex = 0;
    mParam.mArcFrameStepMax = secondary ? mParam.mSecondaryDelay : mParam.mPrimaryDelay;
    mTarget = al::tryFindNearestPlayerActor(getHost());
    if (mTarget) {
        mDelay = mParam.mStartEndDelay;
        mAngleOffset = al::getRandomDegree();
        al::setNerve(this, &NrvSuperBowserGiantFireballStateWait);
    } else kill();
}
/** @brief Checks whether continuous firing is active. @return Result of the check. */
bool SuperBowserGiantFireballState::isShootNowAndForever() {
    return al::isNerve(this, &NrvSuperBowserGiantFireballStateShootForever);
}
/** @brief Updates the nerve and reports completion or continuous firing. @return Result of the check. */
bool SuperBowserGiantFireballState::update() {
    bool result = al::NerveStateBase::update();
    return result | isShootNowAndForever();
}
/** @brief Ends the attack and removes every fireball. */
void SuperBowserGiantFireballState::forceKill() {
    kill();
    for (int i = 0; i < mFireballs.capacity(); i++) mFireballs[i]->kill();
}
/** @brief Waits between shots and ends a completed volley. */
void SuperBowserGiantFireballState::exeWait() {
    if (al::isGreaterStep(this, mDelay)) {
        if (mShotsLeft == 0) { kill(); return; }
        shoot();
        mDelay = mShotsLeft == 0 ? mParam.mStartEndDelay : mParam.mArcFrameStepMin;
        al::setNerve(this, &NrvSuperBowserGiantFireballStateWait);
    }
}
/** @brief Launches an available fireball toward the target. */
void SuperBowserGiantFireballState::shoot() {
    KoopaFireBallGiant* fireball = nullptr;
    for (int i = 0; i < mFireballs.capacity(); i++) {
        if (al::isDead(mFireballs.at(i))) { fireball = mFireballs.at(i); break; }
    }
    if (fireball) {
        --mShotsLeft;
        if (!mTarget) mTarget = al::tryFindNearestPlayerActor(getHost());
        fireball->appear(mTarget, mShotIndex++);
    }
}
/** @brief Fires repeatedly at the configured interval. */
void SuperBowserGiantFireballState::exeShootForever() {
    if (al::isGreaterStep(this, mParam.mArcFrameStepMax)) {
        shootNow();
        al::setNerve(this, &NrvSuperBowserGiantFireballStateShootForever);
    }
}
/** @brief Requests an immediate shot with a fresh shot index. */
void SuperBowserGiantFireballState::shootNow() {
    ++mShotsLeft;
    mShotIndex = 0;
    shoot();
}
/** @brief Returns the current attack settings. @return Settings shared by the fireballs. */
const SuperBowserGiantFireballState::Param* SuperBowserGiantFireballState::getParam() { return &mParam; }
/** @brief Selects the fixed-target preset. */
void SuperBowserGiantFireballState::setModeAimOnce() { mParam = sParamAimOnce; }
/** @brief Selects the continuous-tracking preset. */
void SuperBowserGiantFireballState::setModeContinuousTracking() { mParam = sParamContinuousTracking; }
/** @brief Selects the delayed velocity-snapshot preset. */
void SuperBowserGiantFireballState::setModeVelocitySnapshot() { mParam = sParamVelocitySnapshot; }
/** @brief Toggles continuous firing. */
void SuperBowserGiantFireballState::shootNowAndForever() {
    if (isShootNowAndForever()) {
        al::setNerve(this, &NrvSuperBowserGiantFireballStateWait);
        forceKill();
    } else {
        appear(1, false);
        al::setNerve(this, &NrvSuperBowserGiantFireballStateShootForever);
    }
}
/** @brief Checks whether all fireballs are inactive. @return Result of the check. */
bool SuperBowserGiantFireballState::checkFireballsDone() {
    for (int i = 0; i < mFireballs.capacity(); i++)
        if (al::isAlive(mFireballs.at(i))) return false;
    return true;
}
/** @brief Checks whether every active fireball has landed. @return Result of the check. */
bool SuperBowserGiantFireballState::checkFireballsLanded() {
    for (int i = 0; i < mFireballs.capacity(); i++)
        if (al::isAlive(mFireballs.at(i)) && !mFireballs.at(i)->hasLanded()) return false;
    return true;
}
/** @brief Assigns consecutive rain-fireball IDs. @param pState Rain controller. @param start First ID. */
void SuperBowserGiantFireballState::setGiantFireballIDs(SuperBowserRainFireballState* pState, int start) {
    for (int i = 0; i < mFireballs.capacity(); i++) mFireballs.at(i)->setFireballID(pState, start + i);
}
/** @brief Returns the remaining shot count. @return Shots left. */
int SuperBowserGiantFireballState::getShotsLeft() { return mShotsLeft; }
/** @brief Returns the randomized volley angle. @return Angle in degrees. */
float SuperBowserGiantFireballState::getAngleOffset() { return mAngleOffset; }
NERVE_IMPL(SuperBowserGiantFireballState, Wait)
NERVE_IMPL(SuperBowserGiantFireballState, ShootForever)
