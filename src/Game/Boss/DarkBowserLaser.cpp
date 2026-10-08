#include "Boss/DarkBowserLaser.hpp"

#include <math/seadMathCalcCommon.h>

#include "Boss/DarkBowser.hpp"
#include "Boss/DarkBowserJump.hpp"
#include "Boss/DarkBowserUtil.hpp"
#include "Library/Draw/GraphicsSystemInfo.hpp"
#include "Library/Light/DirectionalLightKeeper.hpp"
#include "Library/Light/PrePassLightFunction.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorClippingUtil.hpp"
#include "Library/LiveActor/Util/ActorModelUtil.hpp"
#include "Library/LiveActor/Util/ActorMovementUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorSceneInfo.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "MapObj/SePlayObj.hpp"
#include "Project/Collision/CollisionPartsTriangle.hpp"
#include "Project/Collision/CollisionUtil.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"

namespace {
NERVE_DECL(DarkBowserLaser, Begin)
NERVE_DECL(DarkBowserLaser, Jump)
NERVE_DECL(DarkBowserLaser, Shoot)
NERVE_DECL(DarkBowserLaser, Charge)
NERVE_DECL(DarkBowserLaser, End)

// The jump nerve hosts a nerve state, so the nerves are mutable globals (merged into one block).
DarkBowserLaserNrvBegin NrvDarkBowserLaserBegin;
DarkBowserLaserNrvJump NrvDarkBowserLaserJump;
DarkBowserLaserNrvShoot NrvDarkBowserLaserShoot;
DarkBowserLaserNrvCharge NrvDarkBowserLaserCharge;
DarkBowserLaserNrvEnd NrvDarkBowserLaserEnd;

/** @brief Tuning of the laser attack for one attack level. */
struct LaserLevelParam {
    f32 _0;                // 0x00
    f32 _4;                // 0x04
    f32 sweepFrames;       // 0x08
    f32 hitRadius;         // 0x0C
    f32 _10;               // 0x10
    s32 sweepDelayFrames;  // 0x14
    s32 endFrames;         // 0x18
    s32 _1c;               // 0x1C
    s32 _20;               // 0x20
    s32 _24;               // 0x24
    s32 shotNum;           // 0x28
    s32 _2c;               // 0x2C
};
static_assert(sizeof(LaserLevelParam) == 0x30);

constexpr s32 cLevelNum = 5;

const LaserLevelParam cLevelParams[cLevelNum] = {
    {180.0f, 300.0f, 270.0f, 300.0f, 25000.0f, 30, 30, 0, 0, 0, 1, 0},
    {180.0f, 300.0f, 270.0f, 300.0f, 25000.0f, 30, 30, 0, 0, 0, 1, 0},
    {180.0f, 300.0f, 270.0f, 300.0f, 25000.0f, 30, 30, 0, 0, 0, 1, 0},
    {180.0f, 300.0f, 270.0f, 300.0f, 25000.0f, 30, 30, 0, 0, 0, 1, 0},
    {180.0f, 300.0f, 225.0f, 300.0f, 25000.0f, 15, 15, 0, 0, 0, 3, 0},
};

/** @brief Frames the scene lighting takes to fade to black and then to the new color. */
constexpr s32 cLightFadeHalfFrames = 15;
/** @brief Frames between entering the shoot nerve and the beam appearing. */
constexpr s32 cShootDelayFrames = 30;
/** @brief How far the laser reaches, for the hit checks. */
constexpr f32 cLaserLength = 250000.0f;
/** @brief Light color while the laser is shot. */
const sead::Vector3f cLaserLightColor = {12.35f, 2.86f, 0.0f};

/**
 * @brief Scores a landing spot for the jump before the laser: spots far from the player (up to
 * a point) are preferred so the laser has room to sweep.
 * @param rHostTrans Position of Fury Bowser (unused).
 * @param rTargetTrans Position of the player.
 * @param rSpotTrans Position of the candidate spot.
 * @return Score of the spot, in [0, 1].
 */
f32 calcJumpSpotScore(const sead::Vector3f& rHostTrans, const sead::Vector3f& rTargetTrans,
                      const sead::Vector3f& rSpotTrans) {
    f32 dx = rTargetTrans.x - rSpotTrans.x;
    f32 dz = rTargetTrans.z - rSpotTrans.z;
    f32 dist = sead::Mathf::sqrt(dx * dx + dz * dz);
    if (dist > 0.1f) {
        f32 rate = dist / 50000.0f;
        if (rate > 1.0f) {
            return sead::Mathf::clamp(2.0f - rate, 0.0f, 1.0f);
        }

        return rate;
    }

    return 0.0f;
}

/**
 * @brief Gets the directional light of the scene, which the laser recolors.
 * @param pActor Actor of the scene.
 * @return The current light parameters.
 */
inline al::DirLightParam* getDirLightParam(const al::LiveActor* pActor) {
    return &pActor->getSceneInfo()->graphicsSystemInfo->getDirectionalLightKeeper()
                ->getCurrentParam();
}

/**
 * @brief Gets the position of Fury Bowser's mouth, where the laser starts.
 * @param pHost Fury Bowser.
 * @return The mouth position.
 */
inline const sead::Vector3f& getMouthPos(const DarkBowser* pHost) {
    return al::getSensorPos(al::getHitSensor(pHost, "MouthLaser"));
}
}  // namespace

/**
 * @brief Constructs the laser state, its jump state, sound actors and grabs the scene light.
 * @param pHost Fury Bowser.
 * @param rInfo Actor init info.
 */
DarkBowserLaser::DarkBowserLaser(DarkBowser* pHost, const al::ActorInitInfo& rInfo)
    : al::NerveStateBase("Laser"), mHost(pHost), mTarget(nullptr), mJump(nullptr), mLevel(0),
      mShotCount(0), mSweepSign(1), mTargetOffset(sead::Vector3f::zero),
      mPrevAimPos(sead::Vector3f::zero), mAimPos(sead::Vector3f::zero),
      mSensorPos(sead::Vector3f::zero), _6c(false), mBeamMtx(nullptr),
      mLaserParam{sead::Matrix34f::ident,
                  sead::Vector3f::ex,
                  sead::Vector3f::ey,
                  sead::Vector3f::ez,
                  sead::Vector3f::zero,
                  sead::Vector3f::zero,
                  false,
                  0,
                  nullptr,
                  nullptr},
      mLight(nullptr), mIsLaserLight(false), mSavedLightDir(-sead::Vector3f::ey),
      mSavedLightColor(sead::Color4f::cRed), mLightColor(sead::Color4f::cRed),
      mLightFadeFrame(0) {
    initNerve(&NrvDarkBowserLaserBegin, 1);
    mJump = new DarkBowserJump(pHost, rInfo);
    al::initNerveState(this, mJump, &NrvDarkBowserLaserJump, "Jump");
    al::setHitSensorPosPtr(mHost, "Laser", &mSensorPos);
    mBeamMtx = al::getJointMtxPtr(mHost, "Beam01");
    al::setEffectFollowMtxPtr(mHost, "Laser", mBeamMtx);
    al::setEffectFollowMtxPtr(mHost, "LaserEnd", mBeamMtx);
    al::killPrePassLight(mHost, "Laser", -1);

    auto* seTip = new SePlayObj("SuperBowserLaserTipSoundActor");
    mLaserParam.mBeamActor = seTip;
    seTip->initAttachedUpdatePose(rInfo, "DarkBowser");
    al::updatePoseMtx(mLaserParam.mBeamActor, pHost->getBaseMtx());
    al::invalidateClipping(mLaserParam.mBeamActor);

    auto* seNear = new SePlayObj("SuperBowserLaserNearPlayerSoundActor");
    mLaserParam.mNearActor = seNear;
    seNear->initAttachedUpdatePose(rInfo, "DarkBowser");
    al::updatePoseMtx(mLaserParam.mNearActor, pHost->getBaseMtx());
    al::invalidateClipping(mLaserParam.mNearActor);

    if (pHost->getPhase() == 1) {
        mJump->setIgnoreTarget();
    }

    mJump->setSpotScoreFunc(calcJumpSpotScore);
    mLight = getDirLightParam(mHost);
    mSavedLightColor = mLight->getColor();
    mSavedLightDir = mLight->getDirection()->getDirection();
}

/**
 * @brief Starts the attack with the jump.
 */
void DarkBowserLaser::appear() {
    al::NerveStateBase::appear();
    al::setNerve(this, &NrvDarkBowserLaserJump);
    mShotCount = 0;
}

/**
 * @brief Ends the attack: restores the light, releases the joints and stops every effect and
 * sound of the laser.
 */
void DarkBowserLaser::kill() {
    if (mIsLaserLight) {
        endLaserLight();
    }

    if (al::isNerve(this, &NrvDarkBowserLaserJump)) {
        mJump->kill();
    }

    mTarget = nullptr;
    mHost->jointRelease();
    al::invalidateHitSensor(mHost, "Laser");
    DarkBowserUtil::stopLaserEffects(mHost);
    al::tryDeleteEffect(mHost, "LaserCharge");
    al::tryDeleteEffect(mHost, "LaserEnd");
    al::tryStopSe(mHost, "Warning");
    al::tryStopSe(mHost, "PgLaserBeamStart");
    al::tryStopSe(mHost, "PgLaserBeamEnd");
    al::tryStopSe(mLaserParam.mNearActor, "PgLaserBeamEndNear");
    al::NerveStateBase::kill();
}

/**
 * @brief Starts fading the scene light back to its original settings.
 */
void DarkBowserLaser::endLaserLight() {
    al::addToDepthShadowDrawer(mHost);
    mLightColor = mLight->getColor();
    mIsLaserLight = false;
    mLightFadeFrame = 0;
}

/**
 * @brief Hits sensors touched by the laser beam, or the collision sensor blocking the way.
 * @param pSelf Sensor of Fury Bowser.
 * @param pOther Touched sensor.
 */
void DarkBowserLaser::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (!al::isSensorName(pSelf, "Laser") || !al::isNerve(this, &NrvDarkBowserLaserShoot) ||
        !al::isGreaterEqualStep(this, 3)) {
        return;
    }

    sead::Vector3f pos = al::getSensorPos(pOther);
    f32 radius = al::getSensorRadius(pOther);
    if (!checkLaserToSphere(pos, radius)) {
        return;
    }

    sead::Vector3f mouth = getMouthPos(mHost);
    al::Triangle triangle;
    sead::Vector3f dir = pos - mouth;
    if (alCollisionUtil::getFirstPolyOnArrow(mHost, nullptr, &triangle, mouth, dir, nullptr,
                                             nullptr)) {
        al::sendMsgLaserAttack(triangle.getSensor(), pSelf);
    } else {
        al::sendMsgLaserAttack(pOther, pSelf);
    }
}

/**
 * @brief Checks whether a sphere is touched by the laser beam or the area it swept.
 * @param rPos Center of the sphere.
 * @param radius Radius of the sphere.
 * @return Whether the sphere is hit.
 */
bool DarkBowserLaser::checkLaserToSphere(const sead::Vector3f& rPos, f32 radius) const {
    sead::Vector3f origin = getMouthPos(mHost);
    sead::Vector3f end = mLaserParam.mFront * cLaserLength + origin;
    if (al::calcDistancePointToSegment(rPos, origin, end) <
        cLevelParams[mLevel].hitRadius + radius) {
        return true;
    }

    return isInLaserFrustum(rPos, radius);
}

/**
 * @brief Jumps to a spot, then begins the attack.
 */
void DarkBowserLaser::exeJump() {
    al::updateNerveStateAndNextNerve(this, &NrvDarkBowserLaserBegin);
}

/**
 * @brief Turns to the nearest player and starts charging once facing them.
 */
void DarkBowserLaser::exeBegin() {
    if (al::isFirstStep(this)) {
        mTarget = al::findNearestPlayerActor(mHost);
        if (mTarget == nullptr) {
            kill();
            return;
        }

        al::setHitSensorPosPtr(mHost, "Laser", &mSensorPos);
        _6c = false;
    }

    sead::Vector3f front;
    sead::Vector3f dir = al::getTrans(mTarget) - al::getTrans(mHost);
    al::calcFrontDir(&front, mHost);
    al::normalizeOrDirZ(&dir);
    if (!DarkBowserUtil::isFacingWithinThreshold(front, dir, 0.925f)) {
        al::turnToTarget(mHost, mTarget, 1.0f);
        if (front.z * dir.x - dir.z * front.x > 0.0f) {
            al::tryStartActionIfNotPlaying(mHost, "TurnRight");
            return;
        }

        al::tryStartActionIfNotPlaying(mHost, "TurnLeft");
        return;
    }

    al::startAction(mHost, "Charge");
    al::tryStartSe(mHost, "Warning");
    al::tryEmitEffect(mHost, "LaserCharge", nullptr);
    beginLaserLight();
    al::setNerve(this, &NrvDarkBowserLaserCharge);
}

/**
 * @brief Starts fading the scene light to the laser color.
 */
void DarkBowserLaser::beginLaserLight() {
    al::removeFromDepthShadowDrawer(mHost);
    mLightColor = mLight->getColor();
    mIsLaserLight = true;
    mLightFadeFrame = 0;
}

/**
 * @brief Charges the laser while turning to the player, then starts shooting.
 */
void DarkBowserLaser::exeCharge() {
    if (al::isLessEqualStep(this, 45)) {
        al::turnToTarget(mHost, mTarget, 0.5f);
    }

    if (al::getActionFrame(mHost) >= al::getActionFrameMax(mHost, "Charge") * 0.5f) {
        calcTargets();
        al::tryStartAction(mHost, "LaserBegin");
        al::setNerve(this, &NrvDarkBowserLaserShoot);
        al::tryDeleteEffect(mHost, "LaserCharge");
        al::tryStopSe(mHost, "Warning");
    }
}

/**
 * @brief Computes the point the laser sweeps around, relative to Fury Bowser.
 */
void DarkBowserLaser::calcTargets() {
    if (mTarget == nullptr) {
        return;
    }

    sead::Vector3f diff = al::getTrans(mTarget) - al::getTrans(mHost);
    sead::Vector3f front;
    al::calcFrontDir(&front, mHost);
    f32 dist = sead::Mathf::max(8000.0f, sead::Mathf::sqrt(diff.x * diff.x + diff.z * diff.z));
    mTargetOffset = front * dist + sead::Vector3f::ey * (diff.y + 1300.0f);
}

/**
 * @brief Sweeps the laser across the target, hitting what the beam touches, and shoots again
 * or ends once the sweep is over.
 */
void DarkBowserLaser::exeShoot() {
    if (al::isFirstStep(this) && mShotCount == 0) {
        const sead::Vector3f& targetTrans = al::getTrans(mTarget);
        const sead::Vector3f& hostTrans = al::getTrans(mHost);
        f32 dx = targetTrans.x - hostTrans.x;
        f32 dz = targetTrans.z - hostTrans.z;
        sead::Vector3f front;
        al::calcFrontDir(&front, mHost);
        mSweepSign = dx * front.z - dz * front.x < 0.0f ? -1 : 1;
        mAimPos = al::getTrans(mHost) + mTargetOffset;
        mHost->jointAim(mAimPos);
        mHost->jointSetPower(1.0f);
    }

    mPrevAimPos = mAimPos;
    f32 sweepFrames = cLevelParams[mLevel].sweepFrames;
    if (mShotCount == 0) {
        sweepFrames *= 0.5f;
    }

    f32 rate = sead::Mathf::clamp(
        (al::getNerveStep(this) - cShootDelayFrames - cLevelParams[mLevel].sweepDelayFrames) /
            sweepFrames,
        0.0f, 1.0f);
    rate = rate * rate;
    rate = rate * rate;
    if (mShotCount == 0) {
        rate = rate * 0.5f + 0.5f;
    }

    mAimPos = mTargetOffset;

    f32 degree = rate * 60.0f;
    degree = degree + degree + -60.0f;
    f32 dirSign = (mShotCount & 1) == 0 ? 1.0f : -1.0f;
    al::rotateVectorDegreeY(&mAimPos, dirSign * degree * mSweepSign);
    mAimPos = al::getTrans(mHost) + mAimPos;
    if (al::isStep(this, cShootDelayFrames)) {
        sead::Vector3f mouth = getMouthPos(mHost);
        mLaserParam.mHitPos = mAimPos;
        mLaserParam.mOrigin = mouth;
        if (mShotCount == 0) {
            al::startSe(mHost, "PgLaserBeamStart");
        }

        DarkBowserUtil::startLaserEffects(mHost, &mLaserParam);
        DarkBowserUtil::updateLaserEffects(mHost, &mLaserParam);
        al::validateHitSensor(mHost, "Laser");
        al::tryStartActionIfNotPlaying(mHost, "Laser");
    }

    if (mShotCount == 0 && al::isLessEqualStep(this, cShootDelayFrames)) {
        return;
    }

    sead::Vector3f origin = getMouthPos(mHost);
    mLaserParam.mHitPos = mAimPos;
    mLaserParam.mOrigin = origin;
    DarkBowserUtil::updateLaserEffects(mHost, &mLaserParam);
    f32 halfLength = (origin - mLaserParam.mHitPos).length() * 0.5f;
    mSensorPos = mLaserParam.mFront * halfLength + origin;
    al::setSensorRadius(mHost, "Laser", halfLength + 1000.0f);

    const LaserLevelParam& param = cLevelParams[mLevel];
    if (al::isGreaterEqualStep(this, sweepFrames + param.endFrames + param.sweepDelayFrames +
                                         cShootDelayFrames)) {
        mShotCount++;
        if (mShotCount < cLevelParams[mLevel].shotNum) {
            al::setNerve(this, &NrvDarkBowserLaserShoot);
        } else {
            al::tryStopSe(mHost, "PgLaserBeamStart");
            al::setNerve(this, &NrvDarkBowserLaserEnd);
        }
        return;
    }

    if (al::isSensorValid(al::getHitSensor(mHost, "Laser"))) {
        sead::Vector3f hitPos;
        al::HitSensor* sensor;
        sead::Vector3f arrow = mLaserParam.mFront * (halfLength + 100.0f) * 2.0f;
        if (alCollisionUtil::getFirstCollisionSensorOnArrow(mHost, &hitPos, &sensor, origin,
                                                            arrow, nullptr, nullptr)) {
            al::sendMsgLaserAttack(sensor, al::getHitSensor(mHost, "Laser"));
        }
    }

    mHost->jointAim(mAimPos);
}

/**
 * @brief Shrinks the beam away, then waits for the end animation while releasing the joints.
 */
void DarkBowserLaser::exeEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(mHost, "LaserEnd");
    }

    f32 scaleRate = 1.0f - al::calcNerveEaseOutRate(this, 15);
    DarkBowserUtil::scaleLaserEffects(mHost, mLaserParam.mHitPos, mLaserParam.mOrigin, scaleRate);
    f32 length = (mLaserParam.mHitPos - mLaserParam.mOrigin).length();
    sead::Vector3f scale = {1.0f, 1.0f, length / 100.0f};
    al::setEffectEmitterScale(mHost, "LaserEnd", scale);
    if (al::isStep(this, 15)) {
        DarkBowserUtil::stopLaserEffects(mHost);
        endLaserLight();
        mTarget = nullptr;
        al::startSe(mHost, "PgLaserBeamEnd");
        al::startSe(mLaserParam.mNearActor, "PgLaserBeamEndNear");
        al::invalidateHitSensor(mHost, "Laser");
        al::tryDeleteEffect(mHost, "Laser");
        al::tryDeleteEffect(mHost, "LaserWaterHit");
        al::tryDeleteEffect(mHost, "LaserLandHit");
    }

    if (al::isActionPlaying(mHost, "LaserEnd") && al::isActionEnd(mHost)) {
        kill();
        return;
    }

    f32 rate = sead::Mathf::clamp(
        (al::getNerveStep(this) + -20.0f) / cLevelParams[mLevel].endFrames, 0.0f, 1.0f);
    f32 power = 1.0f - al::easeIn(rate);
    mHost->jointSetPower(power);
}

/**
 * @brief Checks whether a sphere is inside the area swept by the laser since the last frame.
 * @param rPos Center of the sphere.
 * @param radius Radius of the sphere.
 * @return Whether the sphere is inside every side of the swept area.
 */
bool DarkBowserLaser::isInLaserFrustum(const sead::Vector3f& rPos, f32 radius) const {
    sead::Vector3f origin = mLaserParam.mOrigin;
    sead::Vector3f prev = mPrevAimPos - origin;
    sead::Vector3f cur = mAimPos - origin;
    sead::Vector3f normal;
    normal.setCross(prev, cur);
    sead::Vector3f prevSide;
    prevSide.setCross(normal, prev);
    sead::Vector3f curSide;
    curSide.setCross(normal, cur);

    sead::Vector3f planes[4] = {normal, -prevSide, curSide, -normal};
    for (s32 i = 0; i < 4; i++) {
        if (al::normalizeOrZero(&planes[i])) {
            return false;
        }

        sead::Vector3f diff =
            rPos - origin - planes[i] * (cLevelParams[mLevel].hitRadius + radius);
        if (planes[i].dot(diff) > 0.0f) {
            return false;
        }
    }

    return true;
}

/**
 * @brief Updates the fade of the scene light, to or from the laser colors.
 */
void DarkBowserLaser::updateLaserLight() {
    if (mIsLaserLight) {
        updateLaserLightFadeIn();
    } else {
        updateLaserLightFadeOut();
    }
}

/**
 * @brief Fades the scene light to black, turns it to come from the player's side, then fades
 * it to the laser color.
 */
void DarkBowserLaser::updateLaserLightFadeIn() {
    if (mLightFadeFrame > cLightFadeHalfFrames * 2) {
        return;
    }

    sead::Vector3f color = sead::Vector3f::zero;
    sead::Vector3f from = sead::Vector3f::zero;
    sead::Vector3f to = sead::Vector3f::zero;
    f32 rate;
    if (mLightFadeFrame <= cLightFadeHalfFrames) {
        rate = mLightFadeFrame / (f32)cLightFadeHalfFrames;
        from = {mLightColor.r, mLightColor.g, mLightColor.b};
        if (mLightFadeFrame == cLightFadeHalfFrames) {
            sead::Vector3f mouth = getMouthPos(mHost);
            const sead::Vector3f& targetTrans = al::getTrans(mTarget);
            sead::Vector3f dir = {targetTrans.x - mouth.x, 0.0f, targetTrans.z - mouth.z};
            dir.normalize();
            f32 horizontal = sead::Mathf::sqrt(mSavedLightDir.x * mSavedLightDir.x +
                                               mSavedLightDir.z * mSavedLightDir.z);
            sead::Vector3f lightDir = {dir.x * horizontal, mSavedLightDir.y, dir.z * horizontal};
            mLight->getDirection()->setDirection(lightDir);
        }
    }

    if (mLightFadeFrame > cLightFadeHalfFrames) {
        rate = (mLightFadeFrame - cLightFadeHalfFrames) / (f32)cLightFadeHalfFrames;
        to = cLaserLightColor;
    }

    al::lerpVec(&color, from, to, rate);
    sead::Color4f& lightColor = mLight->getColor();
    lightColor = sead::Color4f(color.x, color.y, color.z, 1.0f);
    mLightFadeFrame++;
}

/**
 * @brief Fades the scene light to black, puts back its direction, then fades it back to its
 * original color.
 */
void DarkBowserLaser::updateLaserLightFadeOut() {
    if (mLightFadeFrame > cLightFadeHalfFrames * 2) {
        return;
    }

    sead::Vector3f color = sead::Vector3f::zero;
    sead::Vector3f from = sead::Vector3f::zero;
    sead::Vector3f to = sead::Vector3f::zero;
    f32 rate;
    if (mLightFadeFrame <= cLightFadeHalfFrames) {
        rate = mLightFadeFrame / (f32)cLightFadeHalfFrames;
        from = {mLightColor.r, mLightColor.g, mLightColor.b};
        if (mLightFadeFrame == cLightFadeHalfFrames) {
            mLight->getDirection()->setDirection(mSavedLightDir);
        }
    }

    if (mLightFadeFrame > cLightFadeHalfFrames) {
        rate = (mLightFadeFrame - cLightFadeHalfFrames) / (f32)cLightFadeHalfFrames;
        to = {mSavedLightColor.r, mSavedLightColor.g, mSavedLightColor.b};
    }

    al::lerpVec(&color, from, to, rate);
    sead::Color4f& lightColor = mLight->getColor();
    lightColor = sead::Color4f(color.x, color.y, color.z, 1.0f);
    mLightFadeFrame++;
}
