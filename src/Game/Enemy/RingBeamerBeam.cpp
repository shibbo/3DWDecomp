#include "Enemy/RingBeamerBeam.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"

namespace {
NERVE_DECL(RingBeamerBeam, Deactive)
NERVE_DECL(RingBeamerBeam, Active)
NERVE_DECL(RingBeamerBeam, FadeOut)
NERVES_MAKE_NOSTRUCT(RingBeamerBeam, Deactive, Active, FadeOut)
}

/** @brief Creates the default expanding beam parameters. */
RingBeamerParam::RingBeamerParam() : mSpeed(10.0f), mLifeTime(200), mEmitEffectName("Emit") {}

/** @brief Creates beam parameters. @param speed Expansion speed. @param lifeTime Active frames.
 * @param pEmitEffectName Action and effect name, or nullptr for Emit. */
RingBeamerParam::RingBeamerParam(f32 speed, s32 lifeTime, const char* pEmitEffectName)
    : mSpeed(speed), mLifeTime(lifeTime), mEmitEffectName(pEmitEffectName ? pEmitEffectName : "Emit") {}

/** @brief Creates a reusable shock wave. @param pName Actor name. @param pHost Firing actor.
 * @param type Wave shape and owner kind. @param flag Whether to follow the host's pose. */
RingBeamerBeam::RingBeamerBeam(const char* pName, al::LiveActor* pHost, RingBeamerType type, bool flag)
    : al::LiveActor(pName), mHost(pHost), mType(type), mFollowHost(flag) {}

/** @brief Initializes the model and inactive state. @param rInfo Scene information. */
void RingBeamerBeam::init(const al::ActorInitInfo& rInfo) {
    const char* pArchive = "RingBeamerBeam";
    if (mType == RingBeamerType_Square) {
        pArchive = "RingBeamerBeamSquare";
    }
    if (mType == RingBeamerType_BossBunretsu) {
        pArchive = "BossBunretsuBeam";
    }
    al::initActorWithArchiveName(this, rInfo, pArchive, nullptr);
    al::initNerve(this, &NrvRingBeamerBeamDeactive, 0);
    al::invalidateClipping(this);
    makeActorAppeared();
}

/** @brief Assigns shared wave parameters. @param pParam Expansion settings. */
void RingBeamerBeam::setRingBeamerParam(const RingBeamerParam* pParam) { mParam = pParam; }

/** @brief Checks the expanding wave against player and enemy sensors.
 * @param pSelf Beam sensor. @param pOther Contacted sensor. */
void RingBeamerBeam::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    switch (mType) {
    case RingBeamerType_Circle:
    case RingBeamerType_Square:
    case RingBeamerType_BossBunretsu:
        break;
    default:
        return;
    }
    if (!al::isSensorEnemyAttack(pSelf) ||
        (!al::isSensorPlayer(pOther) && !al::isSensorEnemyBody(pOther)) || !isActive()) {
        return;
    }
    switch (mType) {
    case RingBeamerType_Circle:
    case RingBeamerType_BossBunretsu: {
        sead::Vector3f pos = al::getSensorPos(pOther);
        f32 radius = al::getSensorRadius(pOther);
        sead::Vector3f offset = pos - al::getSensorPos(pSelf);
        f32 distance = offset.squaredLength();
        if (al::isSensorEnemyBody(pOther)) {
            radius += mParam->mSpeed * 10.0f;
            f32 inner = sead::Mathf::max(mSensorRadius - radius, 0.0f);
            f32 outer = mSensorRadius + radius;
            if (!(distance <= outer * outer && inner * inner <= distance)) {
                return;
            }
            sead::Quatf inverse;
            inverse.setInverse(al::getQuat(this));
            offset.rotate(inverse);
            if (!(offset.y >= -radius && offset.y <= radius)) {
                return;
            }
            rc::sendMsgRingBeamerSign(pOther, pSelf);
        } else {
            radius += 10.0f;
            f32 inner = sead::Mathf::max(mRadius - radius, 0.0f);
            f32 outer = radius + mRadius;
            if (!(distance <= outer * outer && inner * inner <= distance)) {
                return;
            }
            sead::Quatf inverse;
            inverse.setInverse(al::getQuat(this));
            offset.rotate(inverse);
            if (!(offset.y >= -radius && offset.y <= radius)) {
                return;
            }
            al::sendMsgEnemyAttack(pOther, pSelf);
        }
        break;
    }
    case RingBeamerType_Square: {
        sead::Vector3f pos = al::getSensorPos(pOther);
        f32 radius = al::getSensorRadius(pOther);
        sead::Vector3f offset = pos - al::getSensorPos(pSelf);
        f32 distance = offset.squaredLength();
        radius += 10.0f;
        f32 inner = sead::Mathf::max(mRadius - radius, 0.0f);
        f32 outer = radius + mRadius * 1.41421354f;
        if (!(distance <= outer * outer && inner * inner <= distance)) {
            return;
        }
        sead::Quatf inverse;
        inverse.setInverse(al::getQuat(this));
        offset.rotate(inverse);
        if (!(offset.y >= -radius && offset.y <= radius)) {
            return;
        }
        f32 edgeX = mRadius - offset.x;
        if (offset.x < 0.0f) {
            edgeX = -mRadius - offset.x;
        }
        if (sead::Mathf::abs(edgeX) < radius) {
            if (!(-mRadius - radius <= offset.z && offset.z <= radius + mRadius)) {
                return;
            }
            if (al::isSensorEnemyBody(pOther)) {
                rc::sendMsgRingBeamerSign(pOther, pSelf);
            } else {
                al::sendMsgEnemyAttack(pOther, pSelf);
            }
        } else {
            f32 edgeZ = mRadius - offset.z;
            if (offset.z < 0.0f) {
                edgeZ = -mRadius - offset.z;
            }
            if (sead::Mathf::abs(edgeZ) < radius && -mRadius - radius <= offset.x &&
                offset.x <= radius + mRadius) {
                if (!al::isSensorEnemyBody(pOther)) {
                    al::sendMsgEnemyAttack(pOther, pSelf);
                }
            }
        }
        break;
    }
    default:
        break;
    }
}

/** @brief Reports whether the beam can hit sensors. @return True after the first active frame. */
bool RingBeamerBeam::isActive() const {
    return al::isNerve(this, &NrvRingBeamerBeamActive) && al::isGreaterEqualStep(this, 1);
}

/** @brief Ignores incoming sensor messages. @param pMsg Message. @param pOther Sender.
 * @param pSelf Receiver. @return False. */
bool RingBeamerBeam::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) {
    return false;
}

/** @brief Starts a new expanding wave. */
void RingBeamerBeam::setActive() {
    appear();
    mRadius = 0.0f;
    al::setNerve(this, &NrvRingBeamerBeamActive);
}

/** @brief Starts a wave at the supplied pose. @param rTrans Position. @param rQuat Orientation. */
void RingBeamerBeam::setActiveWithPosture(const sead::Vector3f& rTrans, const sead::Quatf& rQuat) {
    setActive();
    al::setTrans(this, rTrans);
    al::setQuat(this, rQuat);
}

/** @brief Stops the wave. */
void RingBeamerBeam::setDeactive() { al::setNerve(this, &NrvRingBeamerBeamDeactive); }

/** @brief Reports whether the wave is fading away. @return Fade state. */
bool RingBeamerBeam::isFadeOut() const { return al::isNerve(this, &NrvRingBeamerBeamFadeOut); }

/** @brief Expands the wave and updates its animation and collision radius. */
void RingBeamerBeam::exeActive() {
    if (al::isFirstStep(this)) {
        if (al::isHideModel(this)) {
            al::showModel(this);
        }
        al::validateHitSensors(this);
        mLifeStep = 0;
        al::startAction(this, mParam->mEmitEffectName);
        if (mType == RingBeamerType_BossBunretsu) {
            al::startMclAnimAndSetFrameAndStop(this, "BossBunretsuColor", mColorFrame);
        }
        al::setSklAnimFrameRate(this, mParam->mSpeed / 5000.0f * 100.0f, 0);
        al::tryStartSe(this, "PgBeamStart", nullptr);
    }
    al::holdSe(this, "Beam", nullptr);
    if (static_cast<u32>(mParam->mLifeTime) < ++mLifeStep) {
        mLifeStep = 0;
        al::setNerve(this, &NrvRingBeamerBeamFadeOut);
    } else {
        mRadius = mLifeStep * mParam->mSpeed + 100.0f;
        if (mFollowHost) {
            const sead::Vector3f& rTrans = al::getTrans(mHost);
            const sead::Quatf& rQuat = al::getQuat(mHost);
            al::setTrans(this, rTrans);
            al::setQuat(this, rQuat);
        }
        switch (mType) {
        case RingBeamerType_Circle:
        case RingBeamerType_BossBunretsu:
            mSensorRadius = mRadius + mParam->mSpeed * 10.0f;
            break;
        case RingBeamerType_Square:
            mSensorRadius = mRadius * 1.41421354f;
            break;
        default:
            break;
        }
        al::setSensorRadius(this, "Ring", mSensorRadius);
    }
}

/** @brief Stops expansion and fades the wave before deactivating it. */
void RingBeamerBeam::exeFadeOut() {
    if (al::isStep(this, 0)) {
        al::tryDeleteEffect(this, mParam->mEmitEffectName);
    }
    if (al::isFirstStep(this)) {
        al::setSklAnimFrameRate(this, 0.0f, 0);
        al::startMclAnim(this, "FadeOut");
    }
    if (al::isMclAnimEnd(this)) {
        al::setNerve(this, &NrvRingBeamerBeamDeactive);
    }
}

/** @brief Hides the wave and disables its sensors. */
void RingBeamerBeam::exeDeactive() {
    if (al::isFirstStep(this)) {
        mRadius = 0.0f;
        if (!al::isHideModel(this)) {
            al::hideModel(this);
        }
        al::invalidateHitSensors(this);
        kill();
    }
}
