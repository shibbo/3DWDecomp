#include "MapObj/KoopaFireBallGiant.hpp"

#include <cmath>
#include <gfx/seadCamera.h>
#include <math/seadQuat.h>

#include "AreaObj/FireBallSafeArea.hpp"
#include "Boss/DarkBowserUtil.hpp"
#include "Enemy/SuperBowser.hpp"
#include "Enemy/SuperBowserGiantFireballState.hpp"
#include "Enemy/SuperBowserRainFireballState.hpp"
#include "MapObj/EchoEmitterHolder.hpp"
#include "Player/Normal/PlayerActor.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/DemoUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Collision/CollisionPartsKeeperUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Shadow/Common/ShadowUtil.hpp"
#include "Library/Shadow/ShadowKeeper.hpp"
#include "Library/Shadow/ShadowMaskCylinder.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Camera/Info/CameraViewInfo.hpp"
#include "Project/Camera/Info/SceneCameraInfo.hpp"
#include "Project/Collision/CollisionParts.hpp"
#include "Project/Collision/CollisionPartsFilterBase.hpp"
#include "Project/Collision/CollisionUtil.hpp"
#include "Project/Effect/Core/EffectKeeper.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Project/Effect/Effect.hpp"
#include "Project/OceanWave/OceanWaveUserInfo.hpp"

namespace {
NERVE_DECL(KoopaFireBallGiant, Wait)
NERVE_DECL(KoopaFireBallGiant, Move)
NERVE_DECL(KoopaFireBallGiant, Arc)
NERVE_DECL(KoopaFireBallGiant, LandStart)
NERVE_DECL(KoopaFireBallGiant, LandEnd)
NERVE_DECL(KoopaFireBallGiant, Land)
NERVE_DECL(KoopaFireBallGiant, LandOnWater)
NERVE_DECL(KoopaFireBallGiant, KillWait)
NERVE_DECL(KoopaFireBallGiant, KillLanding)

NERVES_MAKE_NOSTRUCT(KoopaFireBallGiant, Wait, Move, Arc, LandStart, LandEnd, Land, LandOnWater,
                     KillWait, KillLanding)

/// Cosine of the steepest slope (60 degrees) a giant fireball can land on.
constexpr f32 cLandSlopeCos = 0.49999997f;
}  // namespace

/**
 * @brief Construct a giant fireball.
 * @param pName The actor name.
 * @param pBowser Fury Bowser, who shoots the fireball.
 */
KoopaFireBallGiant::KoopaFireBallGiant(const char* pName, SuperBowser* pBowser)
    : al::LiveActor(pName), mBowser(pBowser) {}

/**
 * @brief Initialize the fireball, scaling its sensors and collider by the placement scale.
 * @param rInfo The actor init info.
 */
void KoopaFireBallGiant::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "KoopaSuperFireBallBig", nullptr);
    al::initNerve(this, &NrvKoopaFireBallGiantWait, 0);

    f32 scale = al::getScale(this).y;
    al::setSensorRadius(this, "Attack", scale * al::getSensorRadius(this, "Attack"));
    al::setSensorRadius(this, "Land", scale * al::getSensorRadius(this, "Land"));
    al::setSensorRadius(this, "NPCDisasterAvoid",
                        scale * al::getSensorRadius(this, "NPCDisasterAvoid"));
    al::setColliderRadius(this, scale * al::getColliderRadius(this));

    mAttackSensorRadius = al::getSensorRadius(this, "Attack");
    mLandSensorRadius = al::getSensorRadius(this, "Land");
    mEchoRadius = mLandSensorRadius * 2.5f;

    sead::Vector3f shadowSize;
    al::calcShadowMaskSize(&shadowSize, this, "Body");
    al::invalidateClipping(this);
    makeActorDead();
    al::hideModel(this);
}

/**
 * @brief Extinguish the fireball when Fury Bowser pushes it while it is falling.
 * @param pMsg The received message.
 * @param pOther The sending sensor.
 * @param pSelf The receiving sensor.
 * @return True if the message was handled.
 */
bool KoopaFireBallGiant::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                                    al::HitSensor* pSelf) {
    if (!al::isMsgBowserPush(pMsg)) {
        return false;
    }

    if (!al::isNerve(this, &NrvKoopaFireBallGiantMove)) {
        return false;
    }

    if (!al::isSensorEnemyAttack(pSelf)) {
        return false;
    }

    killOnLanding();
    return true;
}

/**
 * @brief Stop the fireball and start its extinguish animation.
 */
void KoopaFireBallGiant::killOnLanding() {
    al::offCollide(this);
    al::invalidateHitSensors(this);
    al::hideShadow(this);
    al::setVelocity(this, sead::Vector3f::zero);
    al::setNerve(this, &NrvKoopaFireBallGiantKillLanding);
}

/**
 * @brief Burn whatever the fireball touches.
 * @param pSelf The attacking sensor.
 * @param pOther The touched sensor.
 */
void KoopaFireBallGiant::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (!al::isSensorEnemyAttack(pSelf)) {
        return;
    }

    if (al::isSensorPlayer(pOther) && rc::isReallyPlayerActor(pOther)) {
        if (rc::isPlayerInWater(pOther) && !rc::isPlayerInWaterSurface(pOther)) {
            return;
        }

        if (rc::isPlayerInRouteDokanOrDokan(al::getSensorHost(pOther))) {
            return;
        }

        if (al::isNerve(this, &NrvKoopaFireBallGiantMove) &&
            al::sendMsgEnemyAttackFire(pOther, pSelf)) {
            killOnLanding();
            return;
        }

        if (rc::isPlayerOnGround(pOther)) {
            al::sendMsgEnemyAttackFire(pOther, pSelf);
        }
    } else if (al::isSensorRide(pOther) || al::isSensorKoopaJr(pOther)) {
        if (al::sendMsgEnemyAttackFire(pOther, pSelf)) {
            killOnLanding();
        }
    } else if (al::isSensorKickKoura(pOther)) {
        al::sendMsgKouraDestroy(pOther, pSelf);
    } else if (al::isSensorMapObj(pOther) && (al::isSensorHostName(pOther, "BallNeko") ||
                                              al::isSensorHostName(pOther, "ShadowRacer"))) {
        killOnLanding();
    } else {
        al::sendMsgEnemyAttackFire(pOther, pSelf);
    }
}

/**
 * @brief Kill the fireball and release its slot in the fireball rain.
 */
void KoopaFireBallGiant::kill() {
    al::setTrans(this, sead::Vector3f::zero);
    al::hideModelIfShow(this);
    al::tryKillEmitterAndParticleAll(this);
    al::stopAllSeFromUser(this, 0);

    if (mRainState != nullptr) {
        mRainState->getOutOfLine(mFireballID);
    }

    al::LiveActor::kill();
}

/**
 * @brief Shoot the fireball at a target.
 * @param pTarget The actor to fall on (the player).
 * @param index The index of the fireball in the current volley.
 */
void KoopaFireBallGiant::appear(const al::LiveActor* pTarget, s32 index) {
    if (mParam == nullptr) {
        mParam = mBowser->getGiantFireballState()->getParam();
    }

    mTarget = static_cast<const PlayerActor*>(pTarget);
    mIndex = index;
    mTargetPrevTrans = al::getTrans(pTarget);
    mTargetVelocity = sead::Vector3f::zero;
    mMoveVelocity = sead::Vector3f::zero;
    mFallStartPos = sead::Vector3f::zero;

    al::makeQuatFrontNoSupport(al::getQuatPtr(this), -sead::Vector3f::ey);
    al::setVelocity(this, sead::Vector3f::zero);
    al::invalidateHitSensors(this);
    al::offCollide(this);
    al::hideShadow(this);
    al::setShadowIntensityUser(this, 191, "Body");

    al::ShadowMaskBase* shadowMask = getShadowKeeper()->findShadowMask("Body");
    if (shadowMask != nullptr) {
        static_cast<al::ShadowMaskCylinder*>(shadowMask)->mExpXZ = 0.0f;
    }

    al::setNerve(this, &NrvKoopaFireBallGiantArc);
    al::hideModelIfShow(this);
    mIsKillOnLand = false;
    mConnectedHost = nullptr;
    mConnectedMtx = nullptr;
    mIsInCameraView = false;
    al::setTrans(this, sead::Vector3f::zero);

    mRandomAngle =
        al::getRandom(mParam->mAngleRandomRange * -0.5f, mParam->mAngleRandomRange * 0.5f);
    mRandomDistance = al::getRandom(mParam->mDistanceRandomMin, mParam->mDistanceRandomMax);
    if (mIndex == 0 && mRandomDistance > mLandSensorRadius * 0.75f) {
        mRandomDistance = mLandSensorRadius * 0.75f;
    }

    al::LiveActor::appear();
}

/**
 * @brief Check whether the fireball has reached the ground.
 * @return True while it is landing, burning on the ground or sinking in water.
 */
bool KoopaFireBallGiant::hasLanded() {
    return al::isNerve(this, &NrvKoopaFireBallGiantLandStart) ||
           al::isNerve(this, &NrvKoopaFireBallGiantLandEnd) ||
           al::isNerve(this, &NrvKoopaFireBallGiantLand) ||
           al::isNerve(this, &NrvKoopaFireBallGiantLandOnWater);
}

/**
 * @brief Hide the fireball and kill it once the current cutscene is over.
 */
void KoopaFireBallGiant::killAfterCutscene() {
    al::hideModelIfShow(this);
    al::setNerve(this, &NrvKoopaFireBallGiantKillWait);
}

/**
 * @brief Register the fireball as part of a fireball rain.
 * @param pRainState The fireball rain state.
 * @param id The slot of the fireball in the rain.
 */
void KoopaFireBallGiant::setFireballID(SuperBowserRainFireballState* pRainState, s32 id) {
    mRainState = pRainState;
    mFireballID = id;
}

/**
 * @brief Wait while dead.
 */
void KoopaFireBallGiant::exeWait() {}

/**
 * @brief Check whether the target rides Plessie.
 * @return True if there is a target and it rides Plessie.
 */
inline bool KoopaFireBallGiant::isTargetRaidon() const {
    return mTarget != nullptr && mTarget->isRaidonExist();
}

/**
 * @brief Fly from Bowser's mouth over the target, then start falling.
 */
void KoopaFireBallGiant::exeArc() {
    if (!mIsInCameraView) {
        updateTargetVelocity();
        updateFallStartPosition();
    }

    s32 arcFrame =
        mParam->mArcFrame + mIndex * (mParam->mArcFrameStepMax - mParam->mArcFrameStepMin);

    if (al::isFirstStep(this)) {
        al::setScaleAll(this, mParam->mFlyScale);
        mShotStartPos = al::getSensorPos(al::getHitSensor(mBowser, "MouthLaser"));
        al::startAction(this, "ShotSingleMode");
        al::showModelIfHide(this);
        al::hideShadow(this);
    }

    if (!al::isLessEqualStep(this, arcFrame)) {
        al::setNerve(this, &NrvKoopaFireBallGiantMove);
        return;
    }

    if (al::isGreaterStep(this, arcFrame - mRainState->getParam()->mCheckRingFrame)) {
        // The original fetches the parameters once more without using them.
        mRainState->getParam();
        f32 checkHeight = mRainState->getParam()->mCheckHeight;
        s32 step = al::getNerveStep(this) - arcFrame;
        f32 ringRate = (step + mRainState->getParam()->mCheckRingFrame) /
                       static_cast<f32>(mRainState->getParam()->mCheckRingFrame);
        f32 radius = mAttackSensorRadius;
        sead::Vector3f dir = sead::Vector3f::ey * -(checkHeight + radius + 1.0f);
        f32 angle = sead::Mathf::deg2rad(ringRate * 360.0f);
        sead::Vector3f offset(cosf(angle), 0.0f, sinf(angle));
        sead::Vector3f pos = offset * radius + (sead::Vector3f::ey * checkHeight + mFallStartPos);
        al::CollisionPartsFilterActor filter(this);

        if (alCollisionUtil::checkStrikeArrow(this, pos, dir, &filter, nullptr)) {
            killOnLanding();
            return;
        }
    }

    f32 rate = al::getNerveStep(this) / static_cast<f32>(arcFrame);
    sead::Vector3f pos = getArcPosition(rate);
    al::setTrans(this, pos);

    f32 prevRate = (al::getNerveStep(this) - 1) / static_cast<f32>(arcFrame);
    sead::Vector3f front = pos - getArcPosition(prevRate);
    front.normalize();
    if (!al::isNearZero(front, 0.001f)) {
        al::makeQuatFrontNoSupport(al::getQuatPtr(this), front);
    }

    if (rate <= 0.5f) {
        f32 effectScale = al::lerpValue(rate * 2, mBodyEffectScale, 1.0f);
        al::setEffectParticleScale(this, "BodySingleMode", effectScale);
    }

    if (mParam->mAimFrame >= 1 && al::isStep(this, arcFrame - mParam->mAimFrame)) {
        mMoveVelocity = mTargetVelocity;
        mMoveVelocity.normalize();
        mMoveVelocity *= mParam->mMoveSpeed;
    }

    if (rate >= 0.5f && !mIsInCameraView) {
        mIsInCameraView = isInCameraView();
    }
}

/**
 * @brief Update the velocity of the target on the horizontal plane.
 */
void KoopaFireBallGiant::updateTargetVelocity() {
    const sead::Vector3f& targetTrans = al::getTrans(mTarget);
    mTargetVelocity.set(targetTrans.x - mTargetPrevTrans.x, 0.0f,
                        targetTrans.z - mTargetPrevTrans.z);
    mTargetPrevTrans = al::getTrans(mTarget);
}

/**
 * @brief Update the position the fireball starts falling from, above the tracked target.
 */
void KoopaFireBallGiant::updateFallStartPosition() {
    f32 height = mParam->mFallHeight;
    if (isTargetRaidon()) {
        height = mParam->mFallHeightRaidon;
    }

    mFallStartPos = getTrackingPosition();
    mFallStartPos.y = height + al::getTrans(mTarget).y;
}

/**
 * @brief Calculate a position on the arc from Bowser's mouth to the fall start position.
 * @param rate The progress on the arc, from 0 to 1.
 * @return The position.
 */
sead::Vector3f KoopaFireBallGiant::getArcPosition(f32 rate) {
    return DarkBowserUtil::calculateArc(mShotStartPos, mFallStartPos, mParam->mArcHeight,
                                        al::easeOut(rate), 5, 0);
}

/**
 * @brief Check whether the fireball is near the target or in front of the camera.
 * @return True if the fireball is visible.
 */
bool KoopaFireBallGiant::isInCameraView() {
    sead::Vector3f trans = al::getTrans(this);
    bool isVisible = true;
    if (mTarget == nullptr || !((al::getTrans(mTarget) - trans).squaredLength() < 1000000.0f)) {
        sead::LookAtCamera camera = getSceneCameraInfo()->getViewAt(0)->getLookAtCam();
        sead::Vector3f cameraDir = camera.getAt() - camera.getPos();
        sead::Vector3f toFireball = trans - camera.getPos();
        cameraDir.normalize();
        toFireball.normalize();
        isVisible = cameraDir.dot(toFireball) > 0.766f;
    }

    return isVisible;
}

/**
 * @brief Fall towards the target and land.
 */
void KoopaFireBallGiant::exeMove() {
    updateTargetVelocity();

    if (al::isFirstStep(this)) {
        if (forceCheckKillArea(false)) {
            return;
        }

        al::setScaleAll(this, mParam->mFlyScale);
        al::setTrans(this, mFallStartPos);
        startVelocity();

        sead::Vector3f front = al::getVelocity(this);
        front.normalize();
        al::makeQuatFrontNoSupport(al::getQuatPtr(this), front);
        al::showShadow(this);
        al::invalidateHitSensor(this, "Land");
        al::validateHitSensor(this, "Attack");
        al::validateHitSensor(this, "NPCDisasterAvoid");
        al::onCollide(this);
        al::startSe(this, "PgShadow", nullptr);
    } else if (al::isGreaterEqualStep(this, 600)) {
        al::setNerve(this, &NrvKoopaFireBallGiantWait);
        kill();
        return;
    }

    updatePaused();
    if (mIsPaused) {
        return;
    }

    if (al::isLessEqualStep(this, 60) && !al::isHideShadow(this)) {
        al::ShadowMaskBase* shadowMask = getShadowKeeper()->findShadowMask("Body");
        if (shadowMask != nullptr) {
            static_cast<al::ShadowMaskCylinder*>(shadowMask)->mExpXZ =
                al::easeInOut(al::getNerveStep(this) / 60.0f);
        }
    }

    if (mParam->mAimFrame == 0 && mParam->mHomingFrame >= 1 &&
        al::isLessEqualStep(this, mParam->mHomingFrame)) {
        f32 homingRate = al::getNerveStep(this) / static_cast<f32>(mParam->mHomingFrame);
        f32 speed = al::easeOut(homingRate) * mParam->mMoveSpeed;
        sead::Vector3f target = getTrackingPosition();
        target.y = al::getTrans(this).y;

        sead::Vector3f diff = target - al::getTrans(this);
        if (diff.x * diff.x + diff.z * diff.z > speed * speed) {
            diff.normalize();
            const sead::Vector3f& trans = al::getTrans(this);
            target = speed * diff + trans;
        }

        al::setTrans(this, target);
    }

    if (tryCheckKillArea(true)) {
        return;
    }

    if (al::isCollided(this)) {
        tryToLand();
    }

    if (mTarget != nullptr && rc::isInPlayerControlOffArea(mTarget)) {
        killOnLanding();
    }
}

/**
 * @brief Kill the fireball if it is inside a fireball safe area, or sink it in water.
 * @param isCheckWater Whether to also check for water.
 * @return True if the fireball was killed or sunk.
 */
bool KoopaFireBallGiant::forceCheckKillArea(bool isCheckWater) {
    auto* safeArea = static_cast<FireBallSafeArea*>(
        rc::tryFindAreaObj(this, rc::AreaObjType::FireBallSafeArea, al::getTrans(this)));
    if (safeArea != nullptr && !safeArea->isIgnoreGiantFireballs()) {
        kill();
        return true;
    }

    if (isCheckWater && rc::isInWaterArea(this)) {
        al::setNerve(this, &NrvKoopaFireBallGiantLandOnWater);
        return true;
    }

    return false;
}

/**
 * @brief Start falling with the current horizontal velocity.
 */
void KoopaFireBallGiant::startVelocity() {
    f32 speed = mParam->mFallSpeed;
    if (isTargetRaidon()) {
        speed = mParam->mFallSpeedRaidon;
    }

    al::setVelocity(this, mMoveVelocity - sead::Vector3f::ey * speed);
}

/**
 * @brief Freeze the fireball in the air while a demo is active.
 */
void KoopaFireBallGiant::updatePaused() {
    if (!mIsPaused && rc::isAnyActiveButDemoCameraDemo(this)) {
        mIsPaused = true;
        al::setVelocity(this, sead::Vector3f::zero);
        return;
    }

    if (mIsPaused && !rc::isAnyActiveButDemoCameraDemo(this)) {
        mIsPaused = false;
        startVelocity();
    }
}

/**
 * @brief Calculate where the fireball aims at: around the target, ahead of its movement.
 * @return The position.
 */
sead::Vector3f KoopaFireBallGiant::getTrackingPosition() const {
    s32 shootCount = mBowser->getGiantFireballShootCount();
    if (shootCount == 0) {
        shootCount = 1;
    }

    s32 slot = (mIndex * 2) % shootCount;
    f32 angle = sead::Mathf::deg2rad(mBowser->getGiantFireballState()->getAngleOffset() +
                                     mRandomAngle + slot * 360.0f / shootCount);
    sead::Vector3f offset = mRandomDistance * sead::Vector3f(cosf(angle), 0.0f, sinf(angle));
    f32 trackingRate = mParam->mTrackingRate;
    if (isTargetRaidon()) {
        trackingRate = mParam->mTrackingRateRaidon;
    }

    const sead::Vector3f& targetTrans = al::getTrans(mTarget);
    return offset + (trackingRate * mTargetVelocity + targetTrans);
}

/**
 * @brief Check for kill areas if the fireball rain allows it this frame.
 * @param isCheckWater Whether to also check for water.
 * @return True if the fireball was killed or sunk.
 */
bool KoopaFireBallGiant::tryCheckKillArea(bool isCheckWater) {
    if (mRainState == nullptr) {
        return false;
    }

    if (!mRainState->canFireballDoCheck(mFireballID)) {
        return false;
    }

    return forceCheckKillArea(isCheckWater);
}

/**
 * @brief Land on the collided surface, or extinguish if it is too steep.
 */
void KoopaFireBallGiant::tryToLand() {
    if (mIsKillOnLand) {
        killOnLanding();
        return;
    }

    sead::Vector3f normal = sead::Vector3f::ez;
    al::calcCollidedNormalSum(this, &normal);
    if (al::isNearZero(normal, 0.001f)) {
        normal = sead::Vector3f::ez;
    }

    al::normalize(&normal);
    f32 slopeCos = normal.dot(sead::Vector3f::ey);
    if (slopeCos < cLandSlopeCos) {
        killOnLanding();
        return;
    }

    al::offCollide(this);
    al::hideShadow(this);
    al::validateHitSensor(this, "Land");
    al::invalidateHitSensor(this, "Attack");
    al::makeQuatFrontNoSupport(al::getQuatPtr(this), -normal);
    al::setVelocityZero(this);

    sead::Vector3f landPos = {0.0f, 0.0f, 0.0f};
    if (al::isCollidedGround(this)) {
        landPos = al::getCollidedGroundPos(this);
    } else if (al::isCollidedWall(this)) {
        landPos = al::getCollidedWallPos(this);
    } else if (al::isCollidedCeiling(this)) {
        landPos = al::getCollidedCeilingPos(this);
    }

    al::setTrans(this, landPos);
    if (checkGround()) {
        al::setNerve(this, &NrvKoopaFireBallGiantLandStart);
    }
}

/**
 * @brief Grow into a pool of fire on the ground.
 */
void KoopaFireBallGiant::exeLandStart() {
    if (al::isFirstStep(this)) {
        al::setScaleAll(this, mParam->mLandScale);
        al::startAction(this, "LandStart");
        tryConnectToGround();
    } else if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvKoopaFireBallGiantLand);
        return;
    }

    f32 rate = al::getActionFrame(this) / al::getActionFrameMax(this, "LandStart");
    al::setSensorRadius(this, "Land", al::lerpValue(rate, mAttackSensorRadius, mLandSensorRadius));
    updateConnection();

    if (al::isFirstStep(this)) {
        mEchoTimer = 0;
        mIsOnEchoBlock = false;

        al::Triangle triangle;
        if (alCollisionUtil::getFirstPolyOnArrow(
                this, nullptr, &triangle, al::getTrans(this) + sead::Vector3f::ey * 50.0f,
                sead::Vector3f::ey * -300.0f, nullptr, nullptr)) {
            mIsOnEchoBlock = al::isMaterialCode("EchoBlock", triangle);
        }
    }

    updateEchoPulse();
}

/**
 * @brief Attach the fireball to the collision it landed on, so it follows moving ground.
 */
void KoopaFireBallGiant::tryConnectToGround() {
    al::CollisionPartsFilterActor filter(this);
    sead::Vector3f down = sead::Vector3f::ey;
    al::calcQuatFront(&down, this);
    down.negate();

    f32 radius = mLandSensorRadius;
    sead::Vector3f pos = al::getTrans(this) + down * radius;
    sead::Vector3f dir = down * -(radius + 1.0f);
    al::CollisionParts* parts =
        alCollisionUtil::getStrikeArrowCollisionParts(this, nullptr, pos, dir, &filter, nullptr);
    if (parts == nullptr) {
        return;
    }

    mConnectedMtx = &parts->getBaseMtx();
    mConnectionLocalMtx.setMul(parts->getBaseInvMtx(), *getBaseMtx());
    mConnectedHost = parts->getConnectedHost();
}

/**
 * @brief Follow the connected collision, extinguishing when it disappears or tilts too much.
 */
void KoopaFireBallGiant::updateConnection() {
    if (mConnectedMtx == nullptr) {
        return;
    }

    if (mConnectedHost != nullptr) {
        bool isHostGone;
        if (al::isDead(mConnectedHost)) {
            isHostGone = true;
        } else {
            bool isHostHidden;
            if (al::isEqualString(mConnectedHost->getName(), "半アタリ床")) {
                isHostHidden = al::isActionPlaying(mConnectedHost, "Disappear");
            } else if (al::isEqualString(mConnectedHost->getName(), "DisasterSpike") ||
                       al::isEqualString(mConnectedHost->getName(), "DisasterSpikeBouncy") ||
                       al::isEqualString(mConnectedHost->getName(), "DisasterSpikeGold")) {
                isHostHidden = al::isHideModel(mConnectedHost);
            } else {
                isHostHidden = false;
            }

            bool isRouteDokan =
                al::isEqualString(mConnectedHost->getName(), "ルート土管パーツ★");
            bool isRaidonOnly =
                al::isEqualString(mConnectedHost->getName(), "TransparentWallRaidonOnly") ||
                al::isEqualString(mConnectedHost->getName(), "TransparentCubeRaidonOnlySkate") ||
                al::isEqualString(mConnectedHost->getName(), "TransparentWallRaidonOnlySkate") ||
                al::isEqualString(mConnectedHost->getName(), "TransparentSphereRaidonOnlySkate");
            isHostGone = isRouteDokan || isHostHidden || isRaidonOnly;
        }

        if (isHostGone) {
            al::setSensorRadius(this, "Land", 0.0f);
            al::Effect* effect = getEffectKeeper()->findEffect("LandEnd");
            if (effect != nullptr) {
                effect->tryKillEmitterAndParticleAll();
            }

            kill();
        }
    }

    sead::Matrix34f mtx;
    mtx.setMul(*mConnectedMtx, mConnectionLocalMtx);
    sead::Quatf quat = sead::Quatf::unit;
    mtx.toQuat(quat);
    al::setQuat(this, quat);
    al::setTrans(this, mtx.getTranslation());

    sead::Vector3f front = sead::Vector3f::ey;
    al::calcFrontDir(&front, this);
    if ((-sead::Vector3f::ey).dot(front) < cLandSlopeCos) {
        al::setSensorRadius(this, "Land", 0.0f);
        getEffectKeeper()->findEffect("LandEnd")->tryKillEmitterAndParticleAll();
        kill();
    }
}

/**
 * @brief Emit an echo pulse every second while burning on an echo block.
 */
void KoopaFireBallGiant::updateEchoPulse() {
    if (mIsOnEchoBlock && mEchoTimer++ % 60 == 0) {
        rc::emitEcho(this, al::getTrans(this), mEchoRadius, 180, false);
    }
}

/**
 * @brief Burn on the ground, shrinking the damage area at the end.
 */
void KoopaFireBallGiant::exeLand() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "LandLoop");
        al::setSensorRadius(this, "Land", mLandSensorRadius);
    } else if (al::isGreaterEqualStep(this, 240)) {
        al::setNerve(this, &NrvKoopaFireBallGiantLandEnd);
        return;
    }

    updateConnection();

    if (al::isGreaterEqualStep(this, 195)) {
        f32 rate = sead::Mathf::clamp((al::getNerveStep(this) - 195) / 30.0f, 0.0f, 1.0f);
        al::setSensorRadius(this, "Land", al::lerpValue(rate, mLandSensorRadius, 0.0f));
        return;
    }

    if (al::isLessEqualStep(this, 135)) {
        updateEchoPulse();
    }
}

/**
 * @brief Die out on the ground.
 */
void KoopaFireBallGiant::exeLandEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "LandEnd");
        al::setSensorRadius(this, "Land", 0.0f);
    } else if (al::isActionEnd(this)) {
        kill();
        return;
    }

    updateConnection();
}

/**
 * @brief Sink in water.
 */
void KoopaFireBallGiant::exeLandOnWater() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "LandOnWater");
        al::hideShadow(this);
        al::invalidateHitSensors(this);
        if (mOceanWaveKeeper != nullptr) {
            al::startOceanWave(this, "WaterColumn");
        }
    }

    if (al::isStep(this, 100)) {
        kill();
    }
}

/**
 * @brief Wait for the current demo to end, then die.
 */
void KoopaFireBallGiant::exeKillWait() {
    if (rc::isAnyActiveButDemoCameraDemo(this)) {
        return;
    }

    kill();
}

/**
 * @brief Extinguish, fading out the shadow.
 */
void KoopaFireBallGiant::exeKillLanding() {
    if (al::isFirstStep(this)) {
        al::tryDeleteEffectAndParticle(this, "LandImpact");
        al::tryDeleteEffectAndParticle(this, "Land");
        al::startAction(this, "Extinguish");
    }

    if (!al::isHideShadow(this)) {
        al::setShadowIntensityUser(this, (al::getNerveStep(this) / -60.0f + 1.0f) * 191.25f,
                                   "Body");
    }

    if (al::isGreaterEqualStep(this, 60)) {
        kill();
    }
}

/**
 * @brief Check that there is ground under the whole fire pool, extinguishing otherwise.
 * @return True if the fireball can burn here.
 */
bool KoopaFireBallGiant::checkGround() {
    sead::Vector3f down = sead::Vector3f::ey;
    sead::Vector3f side = sead::Vector3f::ex;
    sead::Vector3f up = sead::Vector3f::ez;
    al::calcQuatFront(&down, this);
    al::calcQuatSide(&side, this);
    al::calcQuatUp(&up, this);
    down.negate();

    f32 radius = mLandSensorRadius;
    sead::Vector3f center = al::getTrans(this) + down * radius;
    sead::Vector3f dir = down * -(radius + 1.0f);
    al::CollisionPartsFilterActor filter(this);

    sead::Vector3f pos = center + side * radius;
    bool isOnGround = alCollisionUtil::checkStrikeArrow(this, pos, dir, &filter, nullptr);

    if (isOnGround) {
        pos = center - side * radius;
        isOnGround = alCollisionUtil::checkStrikeArrow(this, pos, dir, &filter, nullptr);
    }

    if (isOnGround) {
        pos = center + up * radius;
        isOnGround = alCollisionUtil::checkStrikeArrow(this, pos, dir, &filter, nullptr);
    }

    if (isOnGround) {
        pos = center - up * radius;
        isOnGround = alCollisionUtil::checkStrikeArrow(this, pos, dir, &filter, nullptr);
    }

    if (isOnGround) {
        return true;
    }

    killOnLanding();
    return false;
}
