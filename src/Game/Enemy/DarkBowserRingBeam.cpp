#include "Enemy/DarkBowserRingBeam.hpp"
#include "MapObj/SePlayObj.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Math/MatrixUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Util/PlayerUtil.hpp"

namespace {
NERVE_DECL(DarkBowserRingBeam, Deactive)
NERVE_DECL(DarkBowserRingBeam, Active)
NERVE_DECL(DarkBowserRingBeam, FadeOut)
NERVES_MAKE_NOSTRUCT(DarkBowserRingBeam, Deactive, Active, FadeOut)
}

/** @brief Creates default ring speed, lifetime and effect settings. */
DarkBowserRingBeamParam::DarkBowserRingBeamParam()
    : mSpeed(10.0f), mLifeTime(200), mEffectName("Emit") {}

/** @brief Creates ring settings. @param speed Expansion speed. @param lifeTime Active frames.
 * @param pEffectName Action and effect name, or nullptr for Emit. */
DarkBowserRingBeamParam::DarkBowserRingBeamParam(f32 speed, s32 lifeTime, const char* pEffectName)
    : mSpeed(speed), mLifeTime(lifeTime), mEffectName(pEffectName ? pEffectName : "Emit") {}

/** @brief Creates a reusable Fury Bowser shock wave. @param pName Actor name. @param pHost Firing actor. */
DarkBowserRingBeam::DarkBowserRingBeam(const char* pName, al::LiveActor* pHost)
    : al::LiveActor(pName), mHost(pHost) {}

/** @brief Initializes the ring, shadow model and moving sound source. @param rInfo Scene information. */
void DarkBowserRingBeam::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "DarkBowserRingBeam", nullptr);
    al::initNerve(this, &NrvDarkBowserRingBeamDeactive, 0);
    al::invalidateClipping(this);
    makeActorAppeared();
    mShadow = new al::LiveActor("DarkBowserRingBeamShadow");
    al::initActorWithArchiveNameNoPlacementInfo(mShadow, rInfo, "DarkBowserRingBeamShadow", nullptr);
    mShadow->makeActorAppeared();
    al::invalidateClipping(mShadow);
    mSoundActor = new SePlayObj("DarkBowserRingBeamSoundActor");
    mSoundActor->initWithAudioKeeper(rInfo, "DarkBowserRingBeam");
    al::updatePoseMtx(mSoundActor, mHost->getBaseMtx());
    al::invalidateClipping(mSoundActor);
}

/** @brief Stops every sound belonging to the ring. */
inline void DarkBowserRingBeam::stopSoundEffects() {
    al::tryStopSe(this, "PgBeamStart");
    al::tryStopSe(this, "PgBeamEnd");
    al::tryStopSe(mSoundActor, "PgBeamNearLv");
    al::tryStopSe(mSoundActor, "PgBeamEndNear");
}

/** @brief Removes the ring, shadow, sounds and emission effect. */
void DarkBowserRingBeam::kill() {
    stopSoundEffects();
    al::tryDeleteEffect(this, mParam->mEffectName);
    mShadow->kill();
    al::LiveActor::kill();
}

/** @brief Assigns shared ring settings. @param pParam Expansion settings. */
void DarkBowserRingBeam::setRingBeamParam(const DarkBowserRingBeamParam* pParam) { mParam = pParam; }

/** @brief Checks the expanding ring against players and sends giant attacks to objects.
 * @param pSelf Ring sensor. @param pOther Contacted sensor. */
void DarkBowserRingBeam::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isSensorEnemyAttack(pSelf) && al::isSensorPlayer(pOther)) {
        if (!isActive()) {
            return;
        }
        sead::Vector3f pos = al::getSensorPos(pOther);
        f32 radius = al::getSensorRadius(pOther);
        sead::Vector3f offset = pos - al::getSensorPos(pSelf);
        f32 distance = offset.squaredLength();
        radius += 0.0f;
        f32 inner = sead::Mathf::max(mRadius + radius * -0.5f, 0.0f);
        f32 outer = radius + mRadius;
        if (distance <= outer * outer && inner * inner <= distance) {
            sead::Quatf inverse;
            inverse.setInverse(al::getQuat(this));
            offset.rotate(inverse);
            if (offset.y >= -radius && offset.y <= radius) {
                al::sendMsgGigaEnemyAttack(pOther, pSelf);
            }
        }
    }
    if (al::isSensorEnemy(pOther) || al::isSensorMapObj(pOther)) {
        al::sendMsgGigaEnemyAttack(pOther, pSelf);
    }
}

/** @brief Reports whether the wave has started damaging players. @return Active after its first frame. */
bool DarkBowserRingBeam::isActive() const {
    return al::isNerve(this, &NrvDarkBowserRingBeamActive) && al::isGreaterEqualStep(this, 1);
}

/** @brief Acknowledges the player-disregard message. @param pMsg Incoming message.
 * @param pOther Sender sensor. @param pSelf Receiver sensor. @return Whether it is a disregard message. */
bool DarkBowserRingBeam::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                                   al::HitSensor* pSelf) {
    return al::isMsgPlayerDisregard(pMsg);
}

/** @brief Starts expansion from zero radius. */
void DarkBowserRingBeam::setActive() {
    appear();
    mRadius = 0.0f;
    al::setNerve(this, &NrvDarkBowserRingBeamActive);
}

/** @brief Starts the ring and places its shadow on the water plane.
 * @param rTrans Emission position. @param rQuat Ring orientation. */
void DarkBowserRingBeam::setActiveWithPosture(const sead::Vector3f& rTrans, const sead::Quatf& rQuat) {
    setActive();
    al::setTrans(this, rTrans);
    al::setQuat(this, rQuat);
    mShadow->appear();
    sead::Vector3f shadowPos = rTrans;
    shadowPos.y = -50.0f;
    al::setTrans(mShadow, shadowPos);
    al::setQuat(mShadow, rQuat);
    al::setScale(mShadow, al::getScale(this));
}

/** @brief Deactivates the ring. */
void DarkBowserRingBeam::setDeactive() { al::setNerve(this, &NrvDarkBowserRingBeamDeactive); }

/** @brief Reports whether the ring is fading. @return Fade state. */
bool DarkBowserRingBeam::isFadeOut() const { return al::isNerve(this, &NrvDarkBowserRingBeamFadeOut); }

/** @brief Stops ring audio and starts a harmless fade. */
void DarkBowserRingBeam::cutSoundEffects() {
    stopSoundEffects();
    mLifeStep = 0;
    al::setNerve(this, &NrvDarkBowserRingBeamFadeOut);
    al::invalidateHitSensors(this);
}

/** @brief Expands the ring and shadow and updates its sound source. */
void DarkBowserRingBeam::exeActive() {
    if (al::isFirstStep(this)) {
        if (al::isHideModel(this)) {
            al::showModel(this);
        }
        if (al::isHideModel(mShadow)) {
            al::showModel(mShadow);
        }
        al::validateHitSensors(this);
        mLifeStep = 0;
        al::startAction(this, mParam->mEffectName);
        al::startAction(mShadow, mParam->mEffectName);
        f32 rate = mParam->mSpeed / 5000.0f * 100.0f;
        al::setSklAnimFrameRate(this, rate, 0);
        al::setSklAnimFrameRate(mShadow, rate, 0);
        al::tryStartSe(this, "PgBeamStart", nullptr);
    }
    if (static_cast<u32>(mParam->mLifeTime) < ++mLifeStep) {
        mLifeStep = 0;
        al::setNerve(this, &NrvDarkBowserRingBeamFadeOut);
    } else {
        f32 radius = mLifeStep * mParam->mSpeed + 0.0f;
        mRadius = al::getScaleX(this) * radius;
        if (mFollowHost) {
            const sead::Vector3f& rTrans = al::getTrans(mHost);
            const sead::Quatf& rQuat = al::getQuat(mHost);
            al::setTrans(this, rTrans);
            al::setQuat(this, rQuat);
        }
        mSensorRadius = mRadius + mParam->mSpeed;
        al::setSensorRadius(this, "Ring", mSensorRadius);
        updateSoundEffects();
    }
}

/** @brief Positions the nearby ring sound along the direction to the nearest player. */
void DarkBowserRingBeam::updateSoundEffects() {
    const al::LiveActor* pPlayer = rc::findNearestActivePlayerActor(mHost);
    const sead::Vector3f& rTrans = al::getTrans(this);
    const sead::Vector3f& rPlayerTrans = al::getTrans(pPlayer);
    sead::Vector3f direction(rPlayerTrans.x - rTrans.x, 0.0f, rPlayerTrans.z - rTrans.z);
    direction.normalize();
    sead::Vector3f soundPos = rTrans + direction * mSensorRadius;
    sead::Matrix34f mtx;
    al::makeMtxFrontUpPos(&mtx, sead::Vector3f::ex, sead::Vector3f::ey, soundPos);
    al::updatePoseMtx(mSoundActor, &mtx);
    al::holdSe(mSoundActor, "PgBeamNearLv", nullptr);
    al::holdSe(this, "PgBeamCenterLv", nullptr);
}

/** @brief Stops expansion and plays the ending animation and sounds. */
void DarkBowserRingBeam::exeFadeOut() {
    if (al::isStep(this, 0)) {
        al::tryDeleteEffect(this, mParam->mEffectName);
    }
    if (al::isFirstStep(this)) {
        al::setSklAnimFrameRate(this, 0.0f, 0);
        al::startMclAnim(this, "FadeOut");
        al::startSe(this, "PgBeamEnd", nullptr);
        al::startSe(mSoundActor, "PgBeamEndNear", nullptr);
    }
    if (al::isMclAnimEnd(this)) {
        al::setNerve(this, &NrvDarkBowserRingBeamDeactive);
    }
}

/** @brief Hides the ring and shadow and removes the actor. */
void DarkBowserRingBeam::exeDeactive() {
    if (al::isFirstStep(this)) {
        mRadius = 0.0f;
        if (!al::isHideModel(this)) {
            al::hideModel(this);
        }
        if (!al::isHideModel(mShadow)) {
            al::hideModel(mShadow);
        }
        al::invalidateHitSensors(this);
        kill();
    }
}
