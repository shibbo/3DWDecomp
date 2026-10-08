#include "Enemy/BowserSpike.hpp"

#include <math/seadMatrix.h>

#include "Layout/GuideGameWindow.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Math/MatrixUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/PostProcessing/RadialBlurDirector.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "MapObj/BallStateThrow.hpp"
#include "MapObj/ExplosionComboCounterHolder.hpp"
#include "MapObj/ItemStateGigaPlayerHold.hpp"
#include "MapObj/ItemStatePlayerHoldParam.hpp"
#include "MapObj/SePlayObj.hpp"
#include "Player/Normal/PlayerActor.hpp"
#include "Player/Normal/WaterUtil.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Collision/CollisionUtil.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"

namespace {
NERVE_ACTION_IMPL(BowserSpike, Prepare)
NERVE_ACTION_IMPL(BowserSpike, Launch)
NERVE_ACTION_IMPL(BowserSpike, Fly)
NERVE_ACTION_IMPL(BowserSpike, Land)
NERVE_ACTION_IMPL(BowserSpike, Wait)
NERVE_ACTION_IMPL(BowserSpike, Pulse)
NERVE_ACTION_IMPL(BowserSpike, Die)
NERVE_ACTION_IMPL(BowserSpike, Throw)
NERVE_ACTION_IMPL(BowserSpike, Hold)

// Each nerve registers itself to the collector constructed just before them.
alNerveFunction::NerveActionCollector sNerveActionCollector;
BowserSpikeNrvPrepare NrvBowserSpikePrepare;
BowserSpikeNrvLaunch NrvBowserSpikeLaunch;
BowserSpikeNrvFly NrvBowserSpikeFly;
BowserSpikeNrvLand NrvBowserSpikeLand;
BowserSpikeNrvWait NrvBowserSpikeWait;
BowserSpikeNrvPulse NrvBowserSpikePulse;
BowserSpikeNrvDie NrvBowserSpikeDie;
BowserSpikeNrvThrow NrvBowserSpikeThrow;
BowserSpikeNrvHold NrvBowserSpikeHold;

/// Flight of the spike after the player throws it.
BallStateThrowParam sThrowParam(600.0f, 450.0f, 10.0f, 16.0f, 16.0f, 0.996f, 0.996f, 1.25f, 0,
                                true, false);
/// Hold offsets of the spike for each player size and pose.
ItemStatePlayerHoldParam sPlayerHoldParam(
    sead::Vector3f(10.0f, 0.0f, 40.0f), sead::Vector3f(5.0f, 0.0f, 35.0f),
    sead::Vector3f(5.0f, 0.0f, 25.0f), sead::Vector3f(25.0f, 0.0f, 45.0f),
    sead::Vector3f(5.0f, 0.0f, 25.0f), sead::Vector3f(30.0f, 0.0f, 45.0f),
    sead::Vector3f(30.0f, 0.0f, 45.0f), sead::Vector3f(30.0f, 0.0f, 35.0f),
    sead::Vector3f(35.0f, 0.0f, 45.0f), sead::Vector3f(40.0f, 0.0f, 30.0f),
    sead::Vector3f(0.0f, 0.0f, 30.0f));

/// Flight used when the spike is created without a parameter.
const BowserSpikeParam sDefaultParam = {60, 125.0f, 200.0f, 0, 0.85f};
}  // namespace

/**
 * @brief Constructs a spike.
 * @param pName Actor name.
 * @param pParam Flight tuning, or nullptr to use the default one.
 */
BowserSpike::BowserSpike(const char* pName, const BowserSpikeParam* pParam)
    : al::LiveActor(pName), mParam(pParam) {
    if (pParam == nullptr) {
        mParam = &sDefaultParam;
    }
}

/**
 * @brief Initializes the model, the landing warning sound object and the throw and hold states.
 * @param rInfo Actor init info.
 */
void BowserSpike::init(const al::ActorInitInfo& rInfo) {
    al::initNerveAction(this, "Prepare", &sNerveActionCollector, 2);
    al::initActorWithArchiveName(this, rInfo, "BowserSpike", nullptr);
    al::invalidateClipping(this);
    makeActorDead();
    al::setEffectFollowPosPtr(this, "LandWarning", &mLandPos);

    mLandWarningSe = new SePlayObj("BowserSpikeLandWarningSoundActor");
    mLandWarningSe->initWithAudioKeeper(rInfo, "BowserSpike");
    al::updatePoseMtx(mLandWarningSe, getBaseMtx());
    al::invalidateClipping(mLandWarningSe);

    mStateThrow = new BallStateThrow(this, &sThrowParam);
    mStateHold = new ItemStateGigaPlayerHold(this, &sPlayerHoldParam, false, false);
    al::initNerveState(this, mStateThrow, &NrvBowserSpikeThrow, "Throw");
    al::initNerveState(this, mStateHold, &NrvBowserSpikeHold, "Hold");
    mStateHold->initColliderControl();
    mStateHold->setFlag23(true);

    sead::Vector3f maskSize = sead::Vector3f::ones;
    al::calcShadowMaskSize(&maskSize, this, "体影");
    mShadowMaskSize = maskSize.x;
    al::setScale(this, sead::Vector3f::ones * 0.3f);
}

/** @brief Appears small and harmless, waiting to be launched. */
void BowserSpike::appear() {
    al::LiveActor::appear();
    al::startNerveAction(this, "Prepare");
    al::setVelocity(this, sead::Vector3f::zero);
    al::validateHitSensors(this);
    al::invalidateHitSensor(this, "Explosion");
    al::setScale(this, sead::Vector3f::ones * 0.3f);
    mIsExplode = true;
    turnOnCollider();
    al::hideShadow(this);
}

/** @brief Enables the collider and disables the collision parts. */
void BowserSpike::turnOnCollider() {
    al::onCollide(this);
    al::invalidateCollisionParts(this);
}

/** @brief Shows the pick up guide while the player can carry the spike. */
void BowserSpike::control() {
    al::LiveActor::control();

    bool isGuideUser = rc::isCurrentGuideGameWindowUser(this);
    if (!isGuideUser) {
        mIsShowGuide = false;
    }

    if (al::isNerve(this, &NrvBowserSpikeWait)) {
        if (mIsPlayerCanCarry) {
            auto* player = static_cast<PlayerActor*>(al::tryFindNearestPlayerActor(this));
            if (player != nullptr && player->getHoldingSensor() == nullptr && !mIsShowGuide) {
                if (al::isPadTypeJoySingle(al::getMainControllerPort())) {
                    mIsShowGuide = rc::appearGuideGameWindowWithPriority(
                        this, "SingleMode_GuideMessage", "PickupSpikeGuide_SingleJoycons",
                        GuideMessagePriority(3), -1, 0.0f);
                } else {
                    mIsShowGuide = rc::appearGuideGameWindowWithPriority(
                        this, "SingleMode_GuideMessage", "PickupSpikeGuide_DualJoycons",
                        GuideMessagePriority(3), -1, 0.0f);
                }
            }
        } else if (mIsShowGuide) {
            rc::disappearGuideGameWindow(this);
            mIsShowGuide = false;
        }
    } else if (isGuideUser && rc::isGuideGameWindowActive(this)) {
        rc::disappearGuideGameWindow(this);
        mIsShowGuide = false;
    }

    mIsPlayerCanCarry = false;
}

/** @brief Kills the spike, releasing it from the player and hiding the guide. */
void BowserSpike::kill() {
    if (mHolderSensor != nullptr) {
        rc::requestPlayerRelease(mHolderSensor);
    }

    mHolderSensor = nullptr;
    al::tryDeleteEffect(this, "LandWarning");
    mIsInWater = false;
    al::setScale(this, sead::Vector3f::ones * 0.3f);
    al::hideShadow(this);

    if (mIsShowGuide) {
        rc::disappearGuideGameWindow(this);
        mIsShowGuide = false;
    }

    al::LiveActor::kill();
}

/**
 * @brief Hits the player while falling, enemies while thrown or held, and everything around
 * when exploding.
 * @param pSelf Own sensor.
 * @param pOther Touched sensor.
 */
void BowserSpike::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isSensorEnemyBody(pSelf)) {
        return;
    }

    if (al::isNerve(this, &NrvBowserSpikeFly)) {
        if (al::isSensorPlayer(pOther)) {
            if (al::isMclAnimPlaying(this, "SpikeOff") &&
                al::getMclAnimFrame(this) > al::getMclAnimFrameMax(this) * 0.5f) {
                return;
            }

            if (al::isMclAnimPlaying(this, "SpikeOn") &&
                al::getMclAnimFrame(this) < al::getMclAnimFrameMax(this) * 0.5f) {
                return;
            }

            setupMaterialSettings();
            al::startNerveAction(this, "Die");
        }

        if (!al::isEqualString(al::getSensorHost(pOther)->getName(), "DarkBowser") &&
            al::sendMsgGigaEnemyAttack(pOther, pSelf)) {
            setupMaterialSettings();
            al::startNerveAction(this, "Die");
            mIsExplode = false;
        }

        return;
    }

    if (al::isNerve(this, &NrvBowserSpikeThrow) && al::isSensorEnemyAttack(pSelf) &&
        al::sendMsgBallAttack(pOther, pSelf, nullptr)) {
        setupMaterialSettings();
        al::startNerveAction(this, "Die");
    }

    if (al::isNerve(this, &NrvBowserSpikeHold) &&
        al::isSensorHostName(pOther, "DarkBowser") && mHolderSensor != nullptr) {
        sead::Vector3f front = rc::getPlayerFront(mHolderSensor);
        sead::Vector3f dir = al::getSensorPos(pOther) - al::getSensorPos(pSelf);
        if (front.dot(dir) > 0.0f && al::sendMsgBallAttack(pOther, pSelf, nullptr)) {
            setupMaterialSettings();
            al::startNerveAction(this, "Die");
            return;
        }
    }

    if (mIsExplode && al::isNerve(this, &NrvBowserSpikeDie) &&
        al::isSensorEnemyAttack(pSelf)) {
        if (al::isSensorPlayer(pOther)) {
            al::sendMsgExplosion(pOther, pSelf, nullptr);
        } else {
            al::sendMsgExplosion(pOther, pSelf, rc::getExplosionComboCounter(this));
        }
    }
}

/** @brief Updates the effect and sound material from the ground the spike lies on. */
void BowserSpike::setupMaterialSettings() {
    if (!al::isCollidedGround(this)) {
        al::tryUpdateEffectMaterialCode(this, "NoCode");
        al::tryUpdateSeMaterialCode(this, "NoCode");
        return;
    }

    if (al::isEqualString(al::getCollidedFloorMaterialCodeName(this), "Ink")) {
        return;
    }

    if (rc::isInWaterArea(this, al::getCollidedGroundPos(this))) {
        mIsInWater = true;
        al::tryUpdateEffectMaterialCode(this, "Water");
        al::tryUpdateSeMaterialCode(this, "Water");
    }
}

/**
 * @brief Handles attacks, pick up and release by the player.
 * @param pMsg Received message.
 * @param pOther Sender sensor.
 * @param pSelf Own sensor.
 * @return Whether the message was handled.
 */
bool BowserSpike::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                             al::HitSensor* pSelf) {
    if (al::isMsgPlayerDisregard(pMsg)) {
        return true;
    }

    if (al::isNerve(this, &NrvBowserSpikeDie)) {
        return false;
    }

    if (rc::isMsgBombBoundKickedAttack(pMsg)) {
        if (!al::isNerve(this, &NrvBowserSpikeHold)) {
            setupMaterialSettings();
            al::startNerveAction(this, "Die");
        }

        return true;
    }

    if (al::isNerve(this, &NrvBowserSpikeWait)) {
        if (al::isMsgGigaEnemyAttack(pMsg) || al::isMsgLaserAttack(pMsg)) {
            setupMaterialSettings();
            al::startNerveAction(this, "Die");
            mIsExplode = false;
            return true;
        }

        if (al::isMsgPlayerCanCarry(pMsg)) {
            mIsPlayerCanCarry = true;
        }
    }

    if ((al::isNerve(this, &NrvBowserSpikeWait) ||
         al::isNerve(this, &NrvBowserSpikePulse)) &&
        al::isSensorPlayer(pOther) &&
        (mHolderSensor == nullptr ||
         al::getSensorHost(pOther) != al::getSensorHost(mHolderSensor)) &&
        !rc::isPlayerHoldingSomething(pOther) &&
        mStateHold->tryStartCarryFront(pMsg, pOther, false)) {
        mHolderSensor = pOther;
        al::startNerveAction(this, "Hold");
        turnOnCollider();
        al::offCollide(this);
        return true;
    }

    if (al::isNerve(this, &NrvBowserSpikeHold)) {
        sead::Quatf quat = al::getQuat(this);
        if (mStateHold->receiveMsg(pMsg, pOther, pSelf)) {
            if (al::isMsgPlayerReleaseDamage(pMsg) || al::isMsgPlayerReleaseDead(pMsg) ||
                al::isMsgHoldCancel(pMsg)) {
                mIsExplode = false;
                setupMaterialSettings();
                al::startNerveAction(this, "Die");
                al::setQuat(this, quat);
            } else if (al::isMsgPlayerRelease(pMsg) || al::isMsgWarpStart(pMsg)) {
                al::startNerveAction(this, "Throw");
                al::offCollide(this);
                mStateThrow->setThrowParam(mHolderSensor, &sThrowParam, nullptr);
            }

            mHolderSensor = nullptr;
            return true;
        }
    }

    if (al::isMsgBindEnd(pMsg)) {
        setupMaterialSettings();
        al::startNerveAction(this, "Die");
        return true;
    }

    return false;
}

/**
 * @brief Checks whether a player can pick up the spike.
 * @param pSensor Sensor of the player.
 * @return Whether the spike is not already held by that player.
 */
bool BowserSpike::isEnableHold(al::HitSensor* pSensor) const {
    if (!al::isSensorPlayer(pSensor)) {
        return false;
    }

    if (mHolderSensor == nullptr) {
        return true;
    }

    return al::getSensorHost(pSensor) != al::getSensorHost(mHolderSensor);
}

/** @brief Updates the collider, following the player while held. */
void BowserSpike::updateCollider() {
    if (mStateHold->isDead()) {
        al::LiveActor::updateCollider();
    } else {
        mStateHold->updateCollider(al::getHitSensor(this, "Body"));
    }

    if (mIsInWater) {
        al::tryUpdateEffectMaterialCode(this, "Water");
        al::tryUpdateSeMaterialCode(this, "Water");
    }
}

/** @brief Waits hidden until launched. */
void BowserSpike::exePrepare() {
    if (al::isFirstStep(this)) {
        al::offCollide(this);
        al::hideShadow(this);
    }
}

/** @brief Rises out of Bowser's back, then starts falling onto the target. */
void BowserSpike::exeLaunch() {
    if (al::isFirstStep(this)) {
        al::emitRadialBlur(this, al::getTransPtr(this), 2000.0f, 3000.0f, 20, -1);
    }

    sead::Vector3f up = sead::Vector3f::ey;
    up.rotate(al::getQuat(this));
    al::addVelocity(this, up * mParam->mLaunchSpeed);
    al::scaleVelocity(this, mParam->mVelocityScale);

    if (al::isGreaterEqualStep(this, mParam->mLaunchStep)) {
        al::startNerveAction(this, "Fly");
        if (mIsGolden) {
            al::startMclAnim(this, "GoldenSpikeOn");
        } else {
            al::startMclAnim(this, "SpikeOn");
        }
        al::setMclAnimFrame(this, 0.0f);
        al::setQuat(this, sead::Quatf(0.0f, 0.0f, 0.0f, 1.0f));
        al::setVelocity(this, sead::Vector3f::zero);
    }
}

/** @brief Falls down from the sky onto the landing position, showing a warning there. */
void BowserSpike::exeFly() {
    if (al::isFirstStep(this)) {
        sead::Vector3f pos = *mLaunchTarget;
        sead::Vector3f offset = sead::Vector3f::ez;
        al::rotateVectorDegreeY(&offset, mLaunchDegree);
        offset *= mLaunchDistance;
        pos.y = al::getTrans(this).y;
        pos += offset;
        al::resetPosition(this, pos, false);
        al::onCollide(this);

        pos.y = 300.0f;
        al::setShadowMaskSize(this, "体影", 100.0f, 0.0f, 100.0f);
        al::setScale(this, sead::Vector3f::ones);
        alCollisionUtil::getHitPosOnArrow(this, &mLandPos, pos, sead::Vector3f::ey * -256000.0f,
                                          nullptr, nullptr);
        WaterUtil::calcWaterSurfacePos(this, mLandPos, 1000.0f, &mLandPos);

        sead::Vector3f surfacePos = mLandPos;
        surfacePos.y = 300.0f;
        if (WaterUtil::calcWaterSurfacePos(this, surfacePos, 10000.0f, &surfacePos)) {
            mLandPos.x = surfacePos.x;
            mLandPos.y = surfacePos.y;
            mLandPos.z = surfacePos.z;
        }

        al::rotateQuatYDirRandomDegree(this);
        al::showShadow(this);
        al::emitEffect(this, "LandWarning", nullptr);
    }

    if (al::isGreaterEqualStep(this, mParam->mFallStartStep)) {
        al::addVelocity(this, -sead::Vector3f::ey * mParam->mGravity);
        al::scaleVelocity(this, mParam->mVelocityScale);
    }

    updateShadowEffects();

    if (al::isCollided(this)) {
        al::setVelocity(this, sead::Vector3f::zero);
        al::deleteEffect(this, "LandWarning");
        setupMaterialSettings();
        al::startNerveAction(this, "Land");
        al::setShadowMaskSize(this, "体影", mShadowMaskSize, 0.0f, 0.0f);
    }
}

/** @brief Grows the shadow while falling and plays the landing warning sound. */
void BowserSpike::updateShadowEffects() {
    f32 rate = al::calcNerveRate(this, mParam->mFallStartStep + 60);
    f32 size = al::lerpValue(rate, 0.1f, 1.0f) * mShadowMaskSize;
    al::setShadowMaskSize(this, "体影", size, 0.0f, size);

    sead::Matrix34f mtx;
    al::makeMtxFrontUpPos(&mtx, sead::Vector3f::ex, sead::Vector3f::ey, mLandPos);
    al::updatePoseMtx(mLandWarningSe, &mtx);
    al::holdSe(mLandWarningSe, "PgLandSign");
}

/** @brief Sticks in the ground, or dies right away outside of the spike areas. */
void BowserSpike::exeLand() {
    if (al::isFirstStep(this)) {
        if (al::isCollidedGround(this)) {
            al::getCollidedGroundPos(this);
        } else if (al::isCollidedWall(this)) {
            al::getCollidedWallPos(this);
        }

        al::emitRadialBlur(this, al::getTransPtr(this), 2000.0f, 3000.0f, 20, -1);
        al::hideShadow(this);

        if (rc::isInAreaObj(this, rc::AreaObjType::DisasterSpikeArea) &&
            !rc::isInAreaObj(this, rc::AreaObjType::OceanSpikeSafeArea)) {
            turnOnCollision();
        } else {
            setupMaterialSettings();
            al::startNerveAction(this, "Die");
            mIsExplode = false;
            return;
        }
    }

    if (al::isGreaterEqualStep(this, 60)) {
        al::startNerveAction(this, "Wait");
    }
}

/** @brief Disables the collider and enables the collision parts. */
void BowserSpike::turnOnCollision() {
    al::offCollide(this);
    al::validateCollisionParts(this);
}

/** @brief Lies on the ground, then starts pulsing. */
void BowserSpike::exeWait() {
    if (al::isFirstStep(this) && !mIsGolden) {
        al::startMclAnim(this, "SpikeOff");
    }

    if (al::isMclAnimPlaying(this, "SpikeOn") && al::isMclAnimEnd(this) && !mIsGolden) {
        al::startMclAnim(this, "SpikeOff");
    }

    if (al::isGreaterEqualStep(this, 1800)) {
        al::startNerveAction(this, "Pulse");
    }
}

/** @brief Pulses faster and faster, then dies. */
void BowserSpike::exePulse() {
    if (al::isFirstStep(this)) {
        mPulseCount = 0;
        al::startMclAnim(this, "SpikeOn");
        al::setMclAnimFrameRate(this, 2.0f);
    }

    if (al::isMclAnimEnd(this)) {
        mPulseCount++;
        f32 frameRate = al::getMclAnimFrameRate(this) * 1.5f;
        al::startMclAnim(this, "SpikeOn");
        al::setMclAnimFrameRate(this, frameRate);
    }

    if (mPulseCount >= 9) {
        setupMaterialSettings();
        al::startNerveAction(this, "Die");
        mIsExplode = false;
    }
}

/** @brief Explodes (if enabled), drops an item for a golden spike and dies. */
void BowserSpike::exeDie() {
    if (al::isFirstStep(this)) {
        al::invalidateHitSensors(this);
        if (mHolderSensor != nullptr) {
            rc::requestPlayerRelease(mHolderSensor);
        }

        if (mIsExplode) {
            al::validateHitSensor(this, "Explosion");
        }

        mHolderSensor = nullptr;
        al::setVelocityZero(this);
    }

    if (al::isGreaterEqualStep(this, 5)) {
        if (mIsGolden) {
            al::appearItem(this, al::getTrans(this) + sead::Vector3f::ey * 5000.0f,
                           sead::Vector3f::ez);
        }

        kill();
    }
}

/** @brief Flies after being thrown by the player until it hits something. */
void BowserSpike::exeThrow() {
    if (al::isStep(this, 3)) {
        al::onCollide(this);
    }

    if (al::updateNerveState(this) || al::isCollided(this)) {
        if (!mStateThrow->isDead()) {
            mStateThrow->kill();
        }

        al::scaleVelocity(this, 0.1f);
        setupMaterialSettings();
        al::startNerveAction(this, "Die");
    }
}

/** @brief Carried by the player. */
void BowserSpike::exeHold() {
    if (al::isFirstStep(this)) {
        al::setVelocityZero(this);
        mIsInWater = false;
        al::tryDeleteEffect(this, "LandWarning");
    } else if (al::isStep(this, 3)) {
        al::onCollide(this);
    }

    al::updateNerveState(this);
}

/**
 * @brief Places the spike on Bowser, ready to be launched.
 * @param rPos Start position.
 * @param rQuat Start rotation.
 * @param isGolden Whether this is a golden spike dropping an item.
 */
void BowserSpike::prepareLaunch(const sead::Vector3f& rPos, const sead::Quatf& rQuat,
                                bool isGolden) {
    if (al::isNerve(this, &NrvBowserSpikeHold) ||
        al::isNerve(this, &NrvBowserSpikeThrow)) {
        return;
    }

    al::resetPosition(this, rPos, false);
    al::setQuat(this, rQuat);
    appear();
    mIsGolden = isGolden;
}

/** @brief Makes the spike die right away unless the player has it. */
void BowserSpike::quickEnd() {
    if (al::isDead(this) || al::isNerve(this, &NrvBowserSpikeHold) ||
        al::isNerve(this, &NrvBowserSpikeThrow)) {
        return;
    }

    setupMaterialSettings();
    al::startNerveAction(this, "Die");
    mIsExplode = false;
}

/**
 * @brief Launches the prepared spike towards a position around a target.
 * @param pTarget Target position.
 * @param degree Direction of the landing position around the target (Y rotation in degrees).
 * @param distance Distance of the landing position from the target.
 * @param pParam Flight tuning, or nullptr to keep the current one.
 */
void BowserSpike::launch(const sead::Vector3f* pTarget, f32 degree, f32 distance,
                         const BowserSpikeParam* pParam) {
    if (al::isNerve(this, &NrvBowserSpikePrepare)) {
        if (pParam != nullptr) {
            mParam = pParam;
        }

        al::startNerveAction(this, "Launch");
    }

    mLaunchTarget = pTarget;
    mLaunchDegree = degree;
    mLaunchDistance = distance;
}

/** @brief Kills the spike if it was prepared but not launched yet. */
void BowserSpike::delaunch() {
    if (al::isAlive(this) && al::isNerve(this, &NrvBowserSpikePrepare)) {
        kill();
    }
}
