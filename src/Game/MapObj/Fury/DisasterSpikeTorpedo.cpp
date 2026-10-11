#include "MapObj/Fury/DisasterSpikeTorpedo.hpp"

#include <math/seadMathCalcCommon.h>
#include <cmath>

#include "AreaObj/TorpedoSpikeRestrictionArea.hpp"
#include "Enemy/SuperBowser.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "MapObj/DashPanel.hpp"
#include "MapObj/Fury/DisasterSpikeDirector.hpp"
#include "MapObj/Fury/GigaBellManager.hpp"
#include "Project/AreaObj/AreaObjGroup.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/AreaObjUtil.hpp"

namespace {
NERVE_ACTION_IMPL(DisasterSpikeTorpedo, Wait)
NERVE_ACTION_IMPL(DisasterSpikeTorpedo, Appear)
NERVE_ACTION_IMPL(DisasterSpikeTorpedo, Fall)
NERVE_ACTION_IMPL(DisasterSpikeTorpedo, Shoot)
NERVE_ACTION_IMPL(DisasterSpikeTorpedo, Sink)
NERVE_ACTION_IMPL(DisasterSpikeTorpedo, Crumble)
NERVE_ACTION_IMPL(DisasterSpikeTorpedo, CrumbleLaser)

NERVE_ACTIONS_MAKE_STRUCT(DisasterSpikeTorpedo, Wait, Appear, Fall, Shoot, Sink, Crumble,
                          CrumbleLaser)

/// Scene object id of the GigaBellManager.
constexpr s32 cSceneObjIdGigaBellManager = 50;
}  // namespace

/**
 * @brief Construct a torpedo spike.
 * @param pName Name of the actor.
 */
DisasterSpikeTorpedo::DisasterSpikeTorpedo(const char* pName) : al::LiveActor(pName) {}

/**
 * @brief Initialize the torpedo spike's nerves and model, then hide it until it is shot.
 * @param rInfo The actor init info.
 */
void DisasterSpikeTorpedo::init(const al::ActorInitInfo& rInfo) {
    al::initNerveAction(this, "Wait", &NrvDisasterSpikeTorpedo.collector, 0);
    al::initActorWithArchiveName(this, rInfo, "DisasterSpikeTorpedo", nullptr);
    makeActorDead();
}

/**
 * @brief Set the director that spawns this torpedo spike.
 * @param pDirector The disaster spike director.
 */
void DisasterSpikeTorpedo::setDisasterSpikeDirector(DisasterSpikeDirector* pDirector) {
    mDirector = pDirector;
}

/**
 * @brief Reset the torpedo spike's state and load the appear/shoot timings for the current
 * Plessie chase level.
 */
void DisasterSpikeTorpedo::appearSetup() {
    if (mGigaBellManager == nullptr) {
        mGigaBellManager = al::getSceneObj<GigaBellManager>(this, cSceneObjIdGigaBellManager);
    }

    al::showModelIfHide(this);
    al::invalidateClipping(this);
    al::invalidateCollisionParts(this);
    al::setQuat(this, sead::Quatf::unit);

    mIsFirst = false;
    mID = -1;
    mShootDir = sead::Vector3f::ez;
    mAppearStartPos = sead::Vector3f::zero;
    mShootStartPos = sead::Vector3f::zero;
    mShootStartQuat = sead::Quatf::unit;
    mSinkStartPos = sead::Vector3f::zero;
    mSinkFrames = 120;
    mDashPanel = nullptr;

    mAppearFrames = mDirector->getTorpedoParam().mAppearFrames;
    if (mGigaBellManager->isPlessieChaseLv(4)) {
        mAppearFrames = mDirector->getTorpedoParam().mAppearFrames4;
    } else if (mGigaBellManager->getPlessieChaseHitCount() == 2) {
        mAppearFrames = mDirector->getTorpedoParam().mAppearFrames3;
    }

    mShootFrames = mDirector->getTorpedoParam().mShootFrames;
    if (SingleModeDataFunction::isDarkBowserV2Available(this) &&
        mGigaBellManager->isPlessieChaseLv(2)) {
        mShootFrames = mDirector->getTorpedoParamV2().mShootFrames2;
    } else if (mGigaBellManager->getPlessieChaseHitCount() >= 2) {
        mShootFrames = mDirector->getTorpedoParam().mShootFrames3;
    }

    bool isBigRamp = mBowser->isPlessieChaseBigRamp();
    mMinDistanceFromBowser = 0.0f;
    if (SingleModeDataFunction::isDarkBowserV2Available(this) &&
        mGigaBellManager->isPlessieChaseLv(2)) {
        mMinDistanceFromBowser = isBigRamp ?
                                     mDirector->getTorpedoParamV2().mMinDistanceFromBowserBigRamp2 :
                                     mDirector->getTorpedoParamV2().mMinDistanceFromBowser2;
    } else if (mGigaBellManager->getPlessieChaseHitCount() >= 2) {
        mMinDistanceFromBowser = isBigRamp ?
                                     mDirector->getTorpedoParam().mMinDistanceFromBowserBigRamp3 :
                                     mDirector->getTorpedoParam().mMinDistanceFromBowser3;
    } else {
        mMinDistanceFromBowser = isBigRamp ?
                                     mDirector->getTorpedoParam().mMinDistanceFromBowserBigRamp :
                                     mDirector->getTorpedoParam().mMinDistanceFromBowser;
    }
}

/**
 * @brief Make the torpedo spike rise out of Fury Bowser's shell as part of a volley.
 * @param pBowser Fury Bowser shooting the spike.
 * @param index Index of the spike in the volley.
 * @param isFirst Whether this is the first volley, whose spikes turn towards the player.
 */
void DisasterSpikeTorpedo::appear(SuperBowser* pBowser, s32 index, bool isFirst) {
    mBowser = pBowser;
    appearSetup();
    mID = index;
    mIsFirst = isFirst;

    if (mBowser->getCurrentSpawnInfo() != nullptr) {
        mBowserPos = mBowser->getCurrentSpawnInfo()->mTrans;
    }

    calcAppearStartPos();
    al::setTrans(this, mAppearStartPos);
    al::startNerveAction(this, "Appear");
    al::LiveActor::appear();
}

/**
 * @brief Compute the position the spike rises from, relative to Fury Bowser.
 */
void DisasterSpikeTorpedo::calcAppearStartPos() {
    sead::Vector3f offset = mDirector->getTorpedoParam().mAppearOffset;
    sead::Quatf quat = sead::Quatf::unit;
    al::makeQuatFrontUp(&quat, mBowser->getCurrentSpawnInfo()->mFrontDir, sead::Vector3f::ey);
    al::rotateVectorQuat(&offset, quat);
    mAppearStartPos = mBowser->getCurrentSpawnInfo()->mTrans + offset;
}

/**
 * @brief Set up the dash panel riding on the spike (does nothing in the shipped game).
 */
void DisasterSpikeTorpedo::setUpDashPanel() {}

/**
 * @brief Make the torpedo spike rise out of Fury Bowser's shell and land at a given position.
 * @param pBowser Fury Bowser shooting the spike.
 * @param pos Position the spike lands at.
 */
void DisasterSpikeTorpedo::appearReckless(SuperBowser* pBowser, sead::Vector3f pos) {
    mBowser = pBowser;
    appearSetup();
    mID = -1;
    mIsFirst = false;

    if (mBowser->getCurrentSpawnInfo() != nullptr) {
        mBowserPos = mBowser->getCurrentSpawnInfo()->mTrans;
    }

    calcAppearStartPos();
    al::setTrans(this, mAppearStartPos);
    mShootStartPos = pos - sead::Vector3f::ey * 500.0f;

    if (pBowser->getCurrentSpawnInfo() != nullptr) {
        mShootDir = pBowser->getCurrentSpawnInfo()->mFrontDir;
    }

    al::makeQuatFrontUp(&mShootStartQuat, sead::Vector3f::ey, -mShootDir);
    al::startNerveAction(this, "Appear");
    al::LiveActor::appear();
}

/**
 * @brief Kill the spike and the dash panel riding on it.
 */
void DisasterSpikeTorpedo::kill() {
    al::LiveActor::kill();

    if (mDashPanel != nullptr && al::isAlive(mDashPanel)) {
        mDashPanel->kill();
    }
}

/**
 * @brief Damage the player (or Plessie) touching the spike, or crumble on other torpedo spikes.
 * @param pSelf The spike's sensor.
 * @param pOther The touched sensor.
 */
void DisasterSpikeTorpedo::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (!canDamage()) {
        return;
    }

    if (al::isSensorPlayer(pOther)) {
        damage(pSelf, pOther);
        return;
    }

    if (al::isSensorRide(pOther) && al::isSensorName(pOther, "PlayerSensor")) {
        bool isDamageSensor = al::isSensorName(pSelf, "PlayerDamage");
        if (al::getTrans(al::getSensorHost(pOther)).y <=
                al::getSensorPos(pSelf).y + al::getSensorRadius(pSelf) ||
            isDamageSensor) {
            damage(pSelf, pOther);
        }
        return;
    }

    if (al::isEqualString(al::getSensorHost(pOther)->getName(), "DisasterSpikeTorpedo") &&
        al::sendMsgDisasterSpikeAttack(pOther, pSelf)) {
        al::startNerveAction(this, "Crumble");
    }
}

/**
 * @brief Check whether the spike currently hurts what it touches.
 * @return True while shooting, or while sinking for the first few frames.
 */
bool DisasterSpikeTorpedo::canDamage() {
    if (al::isNerve(this, NrvDisasterSpikeTorpedo.Shoot.data())) {
        return true;
    }

    if (al::isNerve(this, NrvDisasterSpikeTorpedo.Sink.data())) {
        return al::getNerveStep(this) <= mDirector->getParam().mTorpedoSpikeSinkDamageFrames;
    }

    return false;
}

/**
 * @brief Damage the player and let Fury Bowser know he hit them.
 * @param pSelf The spike's sensor.
 * @param pOther The player's sensor.
 */
void DisasterSpikeTorpedo::damage(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::sendMsgEnemyAttack(pOther, pSelf) || al::sendMsgDisasterSpikeAttack(pOther, pSelf)) {
        mBowser->notifyPlayerHit();
    }
}

/**
 * @brief Crumble when attacked by an invincible player, another spike or a laser.
 * @param pMsg The received message.
 * @param pSelf The spike's sensor.
 * @param pOther The sender's sensor.
 * @return Whether the message was handled.
 */
bool DisasterSpikeTorpedo::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSelf,
                                      al::HitSensor* pOther) {
    if (!canDamage()) {
        return false;
    }

    if (al::isMsgDisasterSpikeAttack(pMsg) || al::isMsgPlayerInvincibleTouch(pMsg) ||
        al::isMsgPlayerInvincibleAttack(pMsg)) {
        al::startNerveAction(this, "Crumble");
        return true;
    }

    if (al::isMsgLaserAttack(pMsg)) {
        al::startNerveAction(this, "CrumbleLaser");
        return true;
    }

    return false;
}

/**
 * @brief Wait nerve: do nothing.
 */
void DisasterSpikeTorpedo::exeWait() {}

/**
 * @brief Appear nerve: fly in an arc from Fury Bowser's shell to the shoot start position.
 */
void DisasterSpikeTorpedo::exeAppear() {
    if (al::isFirstStep(this)) {
        al::tryStartMclAnimIfExist(this, "DisasterSpikeOn");
    }

    if (mID != -1) {
        updateShootInfo();
    }

    sead::Vector3f side;
    side.setCross(mShootDir, sead::Vector3f::ey);
    sead::Vector3f arcDir = sead::Vector3f::ey;
    f32 sideDist = side.dot(mShootStartPos - mBowserPos) / 1000.0f;
    al::lerpVec(&arcDir, sead::Vector3f::ey, side * al::sign(sideDist),
                mDirector->getTorpedoParam().mAppearArcSpread * sead::Mathf::abs(sideDist));

    sead::Vector3f prevTrans = al::getTrans(this);
    f32 rate = (f32)al::getNerveStep(this) / (f32)mAppearFrames;
    sead::Vector3f pos = sead::Vector3f::zero;
    al::lerpVec(&pos, mAppearStartPos, mShootStartPos, rate);
    f32 arcHeight = mDirector->getTorpedoParam().mAppearArcHeight;
    pos += arcDir * (arcHeight * sinf(sead::Mathf::deg2rad(rate * 180.0f)));
    al::setTrans(this, pos);

    sead::Vector3f velocity = al::getTrans(this) - prevTrans;
    if (!al::isNearZero(velocity, 0.001f)) {
        sead::Vector3f dir = velocity;
        dir.normalize();
        sead::Quatf quat = sead::Quatf::unit;
        if (al::isNear(dir, sead::Vector3f::ey, 0.001f)) {
            al::makeQuatUpNoSupport(&quat, -sead::Vector3f::ey);
        } else if (al::isNear(dir, -sead::Vector3f::ey, 0.001f)) {
            al::makeQuatUpNoSupport(&quat, sead::Vector3f::ey);
        } else {
            al::makeQuatUpFront(&quat, -dir, sead::Vector3f::ey);
        }

        al::setQuat(this, quat);
    }

    if (mDashPanel != nullptr) {
        updateDashPanel();
    }

    if (al::isGreaterEqualStep(this, mAppearFrames)) {
        if (rc::isInWaterArea(this)) {
            al::startNerveAction(this, "Shoot");
        } else {
            mFallSpeed = velocity.length();
            al::startNerveAction(this, "Fall");
        }
    } else {
        checkTorpedoSpikeRestrictionAreas(true, false);
    }
}

/**
 * @brief Compute where and in which direction the spike is shot, based on its index in the volley.
 */
void DisasterSpikeTorpedo::updateShootInfo() {
    if (mID != 0) {
        mShootStartPos = mDirector->getParam().mTorpedoSpikeLockPos;
        mShootDir = mDirector->getParam().mTorpedoSpikeLockDir;
    } else {
        calcShootStartPos();
        mDirector->setTorpedoSpikeLockPosDir(mShootStartPos, mShootDir);
    }

    sead::Vector3f side = mShootDir.cross(sead::Vector3f::ey);
    f32 sideScale = getSideOffsetScale();
    mShootStartPos += side * (sideScale * mDirector->getTorpedoParam().mSpacingX);

    s32 delayFrames = mDirector->getTorpedoParam().mStartDelayFrames;
    if (mGigaBellManager->isPlessieChaseLv(4)) {
        delayFrames = mDirector->getTorpedoParam().mStartDelayFrames4;
    } else if (mGigaBellManager->getPlessieChaseHitCount() == 1) {
        delayFrames = mDirector->getTorpedoParam().mStartDelayFrames2;
    } else if (mGigaBellManager->getPlessieChaseHitCount() >= 2) {
        delayFrames = mDirector->getTorpedoParam().mStartDelayFrames3;
    }

    f32 delayDist = (f32)(mID * delayFrames);
    mShootStartPos += mShootDir * (getShootSpeed() * delayDist);

    f32 frontScale = getFrontOffsetScale();
    mShootStartPos -= mShootDir * (frontScale * mDirector->getTorpedoParam().mSpacingZ);
    calcShootStartQuat();
}

/**
 * @brief Move the dash panel riding on the spike along with it.
 */
void DisasterSpikeTorpedo::updateDashPanel() {
    sead::Vector3f down = sead::Vector3f::ez;
    al::calcUpDir(&down, this);
    down = -down;
    sead::Vector3f front = sead::Vector3f::ey;
    al::calcFrontDir(&front, this);
    sead::Quatf quat = sead::Quatf::unit;
    al::makeQuatFrontUp(&quat, down, front);
    al::setQuat(mDashPanel, quat);

    sead::Vector3f offset =
        sead::Vector3f::ey * (mDirector->getParam().mTorpedoSpikeScale * 200.0f);
    offset -= sead::Vector3f::ez * (mDirector->getParam().mTorpedoSpikeScale * 400.0f);
    al::rotateVectorQuat(&offset, quat);
    al::setTrans(mDashPanel, al::getTrans(this) + offset);
    al::setScaleAll(mDashPanel, mDirector->getParam().mTorpedoSpikeScale);
}

/**
 * @brief Make the spike sink or explode when it enters a TorpedoSpikeRestrictionArea.
 * @param isCheckSink Whether sink areas are checked.
 * @param isCheckExplode Whether explode areas are checked.
 */
void DisasterSpikeTorpedo::checkTorpedoSpikeRestrictionAreas(bool isCheckSink,
                                                             bool isCheckExplode) {
    al::AreaObjGroup* group =
        rc::tryFindAreaObjGroup(this, rc::AreaObjType::TorpedoSpikeRestrictionArea);
    if (group == nullptr) {
        return;
    }

    sead::Vector3f checkPos = calcTorpedoSpikeRestrictionAreaCheckPos();
    for (s32 i = 0; i < group->getSize(); i++) {
        auto* area = static_cast<TorpedoSpikeRestrictionArea*>(group->getAreaObj(i));
        if (!area->isInVolume(checkPos)) {
            continue;
        }

        if (area->isExplode() && isCheckExplode) {
            al::startNerveAction(this, "Crumble");
            return;
        }

        if (area->isSink() && isCheckSink) {
            mSinkFrames = 60;
            al::startNerveAction(this, "Sink");
            return;
        }
    }
}

/**
 * @brief Fall nerve: fall down until the spike reaches the water (or falls out of the world).
 */
void DisasterSpikeTorpedo::exeFall() {
    sead::Vector3f up = sead::Vector3f::ez;
    al::calcUpDir(&up, this);
    al::setTrans(this, al::getTrans(this) - up * mFallSpeed);

    bool isInWater = rc::isInWaterArea(this);
    const sead::Vector3f& trans = al::getTrans(this);
    if (isInWater) {
        mShootStartPos = trans;
        al::startNerveAction(this, "Shoot");
        return;
    }

    if (trans.y < -1000.0f) {
        al::startNerveAction(this, "Crumble");
        return;
    }

    checkTorpedoSpikeRestrictionAreas(true, false);
}

/**
 * @brief Shoot nerve: rush forward through the water, turning towards the player at first.
 */
void DisasterSpikeTorpedo::exeShoot() {
    if (al::isFirstStep(this)) {
        al::setTrans(this, mShootStartPos);
        al::setQuat(this, mShootStartQuat);
        if (mDashPanel != nullptr) {
            al::validateCollisionParts(this);
        }
    }

    if (mIsFirst && al::isLessEqualStep(this, mDirector->getTorpedoParam().mLockFrame)) {
        sead::Vector3f playerTrans = al::getTrans(al::tryFindNearestPlayerActor(this));
        const sead::Vector3f& trans = al::getTrans(this);
        sead::Vector3f toPlayer = {playerTrans.x - trans.x, 0.0f, playerTrans.z - trans.z};
        toPlayer.normalize();

        sead::Quatf quat = al::getQuat(this);
        sead::Quatf targetQuat = sead::Quatf::unit;
        al::makeQuatFrontUp(&targetQuat, sead::Vector3f::ey, -toPlayer);
        al::slerpQuat(&quat, quat, targetQuat, mDirector->getTorpedoParam().mFollowRate);
        al::setQuat(this, quat);
        enforceFollowAngleLimit();
    }

    moveForward();

    if (al::isLessEqualStep(this, 30)) {
        f32 rate = al::getNerveStep(this) / 30.0f;
        f32 startY = mShootStartPos.y;
        f32 endY = startY + 500.0f;
        al::setTransY(this, al::lerpValue(al::easeOut(rate), startY, endY));
    }

    if (al::isGreaterStep(this, 15) && al::getNerveStep(this) % 10 == 0) {
        al::emitEffect(this, "WaterImpact", &al::getTrans(this));
    }

    if (mDashPanel != nullptr) {
        updateDashPanel();
    }

    if (al::isGreaterEqualStep(this, mShootFrames)) {
        al::startNerveAction(this, "Sink");
    } else {
        checkTorpedoSpikeRestrictionAreas(true, true);
    }
}

/**
 * @brief Keep the spike from turning further than the follow angle limit away from its shoot
 * direction.
 */
void DisasterSpikeTorpedo::enforceFollowAngleLimit() {
    sead::Vector3f up = sead::Vector3f::ez;
    al::calcUpDir(&up, this);
    sead::Vector3f front = -up;

    f32 cos = sead::Mathf::clamp(mShootDir.dot(front), -1.0f, 1.0f);
    f32 sign = al::sign(mShootDir.z * front.x - mShootDir.x * front.z);
    f32 angle = sead::Mathf::abs(sign * sead::Mathf::rad2deg(acosf(cos)));
    if (angle > mDirector->getTorpedoParam().mFollowAngleLimit) {
        // the original fetches the tuning once more without using it
        mDirector->getTorpedoParam();
        sead::Vector3f dir = mShootDir;
        al::rotateVectorDegree(&dir, dir, sead::Vector3f::ey,
                               sign * mDirector->getTorpedoParam().mFollowAngleLimit);
        sead::Quatf quat = sead::Quatf::unit;
        al::makeQuatFrontUp(&quat, sead::Vector3f::ey, -dir);
        al::setQuat(this, quat);
    }
}

/**
 * @brief Move the spike forward along its shoot direction.
 */
void DisasterSpikeTorpedo::moveForward() {
    al::setTrans(this, al::getTrans(this) + mShootDir * getShootSpeed());
}

/**
 * @brief Sink nerve: keep moving forward while sinking into the water, then die.
 */
void DisasterSpikeTorpedo::exeSink() {
    if (al::isFirstStep(this)) {
        mSinkStartPos = al::getTrans(this);
        al::tryStartMclAnimIfExist(this, "DisasterSpikeOff");
        al::setMclAnimFrame(this, 15.0f);
    }

    if (!al::isLessEqualStep(this, mSinkFrames)) {
        kill();
        return;
    }

    moveForward();
    f32 rate = (f32)al::getNerveStep(this) / (f32)mSinkFrames;
    al::setTransY(this, mSinkStartPos.y + rate * -500.0f);

    if (mDashPanel != nullptr) {
        updateDashPanel();
    }

    checkTorpedoSpikeRestrictionAreas(true, false);
}

/**
 * @brief Crumble nerve: break apart, then die.
 */
void DisasterSpikeTorpedo::exeCrumble() {
    updateCrumble();
}

/**
 * @brief Hide the spike and its dash panel when it starts crumbling and kill it once done.
 */
void DisasterSpikeTorpedo::updateCrumble() {
    if (al::isFirstStep(this)) {
        al::hideModelIfShow(this);
        al::invalidateCollisionParts(this);
        if (mDashPanel != nullptr && al::isAlive(mDashPanel)) {
            mDashPanel->kill();
        }
    }

    if (al::isGreaterEqualStep(this, 30)) {
        kill();
    }
}

/**
 * @brief CrumbleLaser nerve: break apart after being hit by a laser, then die.
 */
void DisasterSpikeTorpedo::exeCrumbleLaser() {
    updateCrumble();
}

/**
 * @brief Get the sideways offset of this spike in the volley's pattern.
 * @return The offset scale, flipped when the director flips the pattern.
 */
f32 DisasterSpikeTorpedo::getSideOffsetScale() {
    f32 scale;
    if (SingleModeDataFunction::isDarkBowserV2Available(this)) {
        if (mGigaBellManager->isPlessieChaseLv(1)) {
            scale = mDirector->getTorpedoParam().mOffsetScale[mID][0];
        } else if (mGigaBellManager->isPlessieChaseLv(2)) {
            scale = mDirector->getTorpedoParamV2().mOffsetScale2[mID][0];
        } else if (mGigaBellManager->isPlessieChaseLv(3)) {
            scale = mDirector->getTorpedoParamV2().mOffsetScale3[mID][0];
        } else if (mGigaBellManager->isPlessieChaseLv(4)) {
            scale = mDirector->getTorpedoParam().mOffsetScale4[mID][0];
        } else {
            scale = 0.0f;
        }
    } else if (mGigaBellManager->isPlessieChaseLv(4)) {
        scale = mDirector->getTorpedoParam().mOffsetScale4[mID][0];
    } else {
        scale = mDirector->getTorpedoParam().mOffsetScale[mID][0];
    }

    return scale * (mDirector->getParam().mIsFlipTorpedoSpikePattern ? -1.0f : 1.0f);
}

/**
 * @brief Get the forward offset of this spike in the volley's pattern.
 * @return The offset scale.
 */
f32 DisasterSpikeTorpedo::getFrontOffsetScale() {
    if (SingleModeDataFunction::isDarkBowserV2Available(this)) {
        if (mGigaBellManager->isPlessieChaseLv(2)) {
            return mDirector->getTorpedoParamV2().mOffsetScale2[mID][1];
        }

        if (mGigaBellManager->isPlessieChaseLv(3)) {
            return mDirector->getTorpedoParamV2().mOffsetScale3[mID][1];
        }

        if (mGigaBellManager->isPlessieChaseLv(4)) {
            return mDirector->getTorpedoParam().mOffsetScale4[mID][1];
        }

        return mDirector->getTorpedoParam().mOffsetScale[mID][1];
    }

    if (mGigaBellManager->isPlessieChaseLv(4)) {
        return mDirector->getTorpedoParam().mOffsetScale4[mID][1];
    }

    return mDirector->getTorpedoParam().mOffsetScale[mID][1];
}

/**
 * @brief Get the speed the spike is shot at for the current Plessie chase level.
 * @return The shoot speed.
 */
f32 DisasterSpikeTorpedo::getShootSpeed() {
    if (mGigaBellManager->isPlessieChaseLv(4)) {
        return mDirector->getTorpedoParam().mShootSpeed4;
    }

    if (mGigaBellManager->isPlessieChaseLv(3)) {
        return mDirector->getTorpedoParam().mShootSpeed3;
    }

    if (mGigaBellManager->isPlessieChaseLv(2)) {
        return mDirector->getTorpedoParam().mShootSpeed2;
    }

    return mDirector->getTorpedoParam().mShootSpeed;
}

/**
 * @brief Compute the shoot direction towards the player and the position the volley starts from.
 */
void DisasterSpikeTorpedo::calcShootStartPos() {
    sead::Vector3f playerTrans = al::getTrans(al::tryFindNearestPlayerActor(this));
    sead::Vector3f target = playerTrans + mDirector->getTorpedoSpikePlayerLead();
    target.y = mBowserPos.y;
    mShootDir = target - mBowserPos;
    mShootDir.normalize();

    s32 hitCount = mGigaBellManager->getPlessieChaseHitCount();
    sead::Quatf quat = sead::Quatf::unit;
    al::makeQuatFrontUp(&quat, -mShootDir, sead::Vector3f::ey);
    sead::Vector3f offset = hitCount >= 1 ? mDirector->getTorpedoParam().mBaseOffset :
                                            mDirector->getTorpedoParam().mBaseOffsetFirst;
    al::rotateVectorQuat(&offset, quat);
    mShootStartPos = target + offset - sead::Vector3f::ey * 500.0f;

    sead::Vector3f diff = mShootStartPos - mBowserPos;
    f32 distanceH = sead::Mathf::sqrt(diff.x * diff.x + diff.z * diff.z);
    if (distanceH < mMinDistanceFromBowser) {
        mShootStartPos += mShootDir * (mMinDistanceFromBowser - diff.dot(mShootDir));
    }
}

/**
 * @brief Compute the orientation the spike is shot with, turning the first volley towards the
 * player.
 */
void DisasterSpikeTorpedo::calcShootStartQuat() {
    if (mIsFirst) {
        sead::Vector3f playerTrans = al::getTrans(al::tryFindNearestPlayerActor(this));
        sead::Vector3f toPlayer = {playerTrans.x - mShootStartPos.x, 0.0f,
                                   playerTrans.z - mShootStartPos.z};
        toPlayer.normalize();

        sead::Vector3f dir = mShootDir;
        f32 cos = sead::Mathf::clamp(toPlayer.dot(dir), -1.0f, 1.0f);
        f32 sign = al::sign(toPlayer.z * dir.x - toPlayer.x * dir.z);
        f32 angle = sign * sead::Mathf::rad2deg(acosf(cos));
        al::rotateVectorDegree(&dir, dir, sead::Vector3f::ey,
                               angle * mDirector->getTorpedoParam().mFollowStartAngleScale);
        al::makeQuatFrontUp(&mShootStartQuat, sead::Vector3f::ey, -dir);
    } else {
        al::makeQuatFrontUp(&mShootStartQuat, sead::Vector3f::ey, -mShootDir);
    }

    if (SingleModeDataFunction::isDarkBowserV2Available(this) &&
        !mGigaBellManager->isPlessieChaseLv(1)) {
        al::rotateQuatXDirDegree(&mShootStartQuat, mShootStartQuat,
                                 mDirector->getTorpedoParamV2().mAngle);
    }
}

/**
 * @brief Compute the point checked against the TorpedoSpikeRestrictionAreas.
 * @return The spike's position, offset along its up axis.
 */
sead::Vector3f DisasterSpikeTorpedo::calcTorpedoSpikeRestrictionAreaCheckPos() const {
    sead::Vector3f offset = {0.0f, mDirector->getParam().mTorpedoSpikeRestrictionOffset, 0.0f};
    al::rotateVectorQuat(&offset, al::getQuat(this));
    return al::getTrans(this) + offset;
}
