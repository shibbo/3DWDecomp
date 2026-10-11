#include "MapObj/KoopaFireBall.hpp"

#include <cmath>
#include <math/seadQuat.h>

#include "Enemy/SuperBowserRainFireballState.hpp"
#include "MapObj/EchoEmitterHolder.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/DemoUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Collision/CollisionPartsKeeperUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Shadow/Common/ShadowUtil.hpp"
#include "Library/Shadow/ShadowKeeper.hpp"
#include "Library/Shadow/ShadowMaskBase.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Collision/CollisionParts.hpp"
#include "Project/Collision/CollisionPartsFilterBase.hpp"
#include "Project/Collision/CollisionUtil.hpp"
#include "Project/Effect/Core/EffectKeeper.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Project/Effect/Effect.hpp"
#include "Project/OceanWave/OceanWaveUserInfo.hpp"

namespace {
NERVE_DECL(KoopaFireBall, Wait)
NERVE_DECL(KoopaFireBall, Move)
NERVE_DECL(KoopaFireBall, SpawnCheck)
NERVE_DECL(KoopaFireBall, LandOnWater)
NERVE_DECL(KoopaFireBall, LandStart)
NERVE_DECL(KoopaFireBall, Land)
NERVE_DECL(KoopaFireBall, LandEnd)
NERVE_DECL(KoopaFireBall, Extinguish)

NERVES_MAKE_NOSTRUCT(KoopaFireBall, Wait, Move, SpawnCheck, LandOnWater, LandStart, Land,
                     LandEnd, Extinguish)

/// Cosine of the steepest slope (about 15 degrees) a fireball can land on in the stage version.
constexpr f32 cLandSlopeCosStrict = 0.966f;
/// Cosine of the steepest slope (60 degrees) a fireball can land on.
constexpr f32 cLandSlopeCos = 0.49999997f;
}  // namespace

/**
 * @brief Construct a fireball.
 * @param pName The actor name.
 * @param isGiant Whether to use the big fireball model.
 * @param pHost The actor shooting the fireball, whose collision is ignored (may be null).
 */
KoopaFireBall::KoopaFireBall(const char* pName, bool isGiant, al::LiveActor* pHost)
    : al::LiveActor(pName), mIsGiant(isGiant), mHost(pHost) {}

/**
 * @brief Initialize the fireball, picking the model for the current game mode and size.
 * @param rInfo The actor init info.
 */
void KoopaFireBall::init(const al::ActorInitInfo& rInfo) {
    mIsSingleMode = rInfo.getActorSceneInfo().isSingleMode;

    const char* archiveName;
    if (mIsSingleMode) {
        archiveName = mIsGiant ? "KoopaSuperFireBallBig" : "KoopaSuperFireBall";
    } else {
        archiveName = mIsGiant ? "KoopaFireBallBig" : "KoopaFireBall";
    }

    al::initActorWithArchiveName(this, rInfo, archiveName, nullptr);
    al::initNerve(this, &NrvKoopaFireBallWait, 0);

    if (mIsGiant) {
        f32 scale = al::getScale(this).y;
        al::setSensorRadius(this, "Attack", scale * al::getSensorRadius(this, "Attack"));
        al::setSensorRadius(this, "Land", scale * al::getSensorRadius(this, "Land"));
        al::setColliderRadius(this, scale * al::getColliderRadius(this));
    }

    mAttackSensorRadius = al::getSensorRadius(this, "Attack");
    mLandSensorRadius = al::getSensorRadius(this, "Land");
    mEchoRadius = mLandSensorRadius * 2.5f;

    if (mHost != nullptr) {
        mCollisionFilter = new al::CollisionPartsFilterActor(mHost);
        al::setColliderFilterCollisionParts(this, mCollisionFilter);
    }

    al::invalidateClipping(this);
    makeActorDead();

    if (mIsSingleMode) {
        if (mIsGiant) {
            al::hideModel(this);
        }

        sead::Vector3f shadowSize;
        al::calcShadowMaskSize(&shadowSize, this, "Body");
        mShadowSize = shadowSize.x;
    }
}

/**
 * @brief Extinguish the fireball when Fury Bowser pushes it while it is flying.
 * @param pMsg The received message.
 * @param pOther The sending sensor.
 * @param pSelf The receiving sensor.
 * @return True if the message was handled.
 */
bool KoopaFireBall::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                               al::HitSensor* pSelf) {
    if (!mIsSingleMode) {
        return false;
    }

    if (!al::isMsgBowserPush(pMsg)) {
        return false;
    }

    if (!al::isNerve(this, &NrvKoopaFireBallMove)) {
        return false;
    }

    if (!al::isSensorEnemyAttack(pSelf)) {
        return false;
    }

    extinguish();
    return true;
}

/**
 * @brief Start the extinguish animation.
 */
void KoopaFireBall::extinguish() {
    al::setNerve(this, &NrvKoopaFireBallExtinguish);
}

/**
 * @brief Burn whatever the fireball touches.
 * @param pSelf The attacking sensor.
 * @param pOther The touched sensor.
 */
void KoopaFireBall::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (!al::isSensorEnemyAttack(pSelf)) {
        return;
    }

    if (al::isSensorPlayer(pOther)) {
        if (mIsSingleMode && rc::isReallyPlayerActor(pOther)) {
            if (rc::isPlayerInWater(pOther) && !rc::isPlayerInWaterSurface(pOther)) {
                return;
            }

            if (rc::isPlayerInRouteDokanOrDokan(al::getSensorHost(pOther))) {
                return;
            }
        }

        if (al::isNerve(this, &NrvKoopaFireBallMove)) {
            if (al::sendMsgEnemyAttackFire(pOther, pSelf)) {
                if (mIsSingleMode) {
                    extinguish();
                } else {
                    kill();
                }
            }

            return;
        }

        if (rc::isPlayerOnGround(pOther)) {
            al::sendMsgEnemyAttackFire(pOther, pSelf);
        }

        return;
    }

    if (al::isSensorRide(pOther)) {
        bool isMove = al::isNerve(this, &NrvKoopaFireBallMove);
        bool isSent = al::sendMsgEnemyAttackFire(pOther, pSelf);
        if (isMove && isSent) {
            if (mIsSingleMode) {
                extinguish();
            } else {
                kill();
            }
        }

        return;
    }

    if (mIsSingleMode && al::isSensorMapObj(pOther) && al::isSensorHostName(pOther, "BallNeko")) {
        extinguish();
    } else if (mIsSingleMode && al::isSensorKickKoura(pOther)) {
        al::sendMsgKouraDestroy(pOther, pSelf);
    } else {
        al::sendMsgEnemyAttackFire(pOther, pSelf);
    }
}

/**
 * @brief Kill the fireball, releasing its slot in the fireball rain. A fireball sinking in water
 * stays alive until the sinking animation is over.
 */
void KoopaFireBall::kill() {
    if (mIsSingleMode) {
        if (mIsLandingOnWater) {
            return;
        }

        if (mRainState != nullptr) {
            mRainState->getOutOfLine(mFireballID);
        }

        al::hideModelIfShow(this);
        al::tryKillEmitterAndParticleAll(this);
        getEffectKeeper()->deleteEffectAll();
    }

    al::LiveActor::kill();
}

/**
 * @brief Shoot the fireball towards a target.
 * @param rPos The start position.
 * @param rTarget The position to fly towards.
 * @param speed The flying speed.
 */
void KoopaFireBall::appearAttack(const sead::Vector3f& rPos, const sead::Vector3f& rTarget,
                                 f32 speed) {
    sead::Vector3f dir = rTarget - rPos;
    if (al::normalizeOrZero(&dir)) {
        dir = sead::Vector3f::ey;
    }

    mMoveVelocity = dir * speed;
    al::makeQuatFrontNoSupport(al::getQuatPtr(this), dir);
    al::setTrans(this, rPos);
    al::setVelocity(this, mMoveVelocity);
    al::setNerve(this, &NrvKoopaFireBallMove);
    al::onCollide(this);
    al::showShadow(this);
    al::invalidateHitSensor(this, "Land");
    al::validateHitSensor(this, "Attack");
    appear();
}

/**
 * @brief Drop the fireball from the sky as part of Fury Bowser's fireball rain.
 * @param rPos The start position.
 */
void KoopaFireBall::appearSingleMode(const sead::Vector3f& rPos) {
    if (mRainState == nullptr) {
        return;
    }

    al::setTrans(this, rPos);
    al::makeQuatFrontNoSupport(al::getQuatPtr(this), -sead::Vector3f::ey);
    al::setScaleAll(this, mRainState->getParam()->mFlyScale);
    mMoveVelocity = -(sead::Vector3f::ey * mRainState->getParam()->mFallSpeed);
    al::setVelocity(this, mMoveVelocity);
    mIsKillOnLand = false;
    mConnectedHost = nullptr;
    mConnectedMtx = nullptr;
    mSpawnCheckStep = 0;
    al::offCollide(this);
    al::invalidateHitSensors(this);
    al::hideShadow(this);
    al::setShadowMaskSize(this, "Body", sead::Vector3f::zero);
    al::setNerve(this, &NrvKoopaFireBallSpawnCheck);
    appear();
}

/**
 * @brief Set the action played while flying.
 * @param pActionName The action name.
 */
void KoopaFireBall::setActionName(const char* pActionName) {
    mActionName = pActionName;
}

/**
 * @brief Register the fireball as part of a fireball rain.
 * @param pRainState The fireball rain state.
 * @param id The slot of the fireball in the rain.
 */
void KoopaFireBall::setFireballID(SuperBowserRainFireballState* pRainState, s32 id) {
    mRainState = pRainState;
    mFireballID = id;
}

/**
 * @brief Wait while dead.
 */
void KoopaFireBall::exeWait() {}

/**
 * @brief Fly until hitting collision, then land on it.
 */
void KoopaFireBall::exeMove() {
    if (al::isFirstStep(this)) {
        if (mLandType == LandType::NoLand) {
            al::offCollide(this);
        } else {
            al::onCollide(this);
        }

        al::startAction(this, mActionName);
        if (mIsSingleMode) {
            al::setSensorRadius(this, "Attack", mAttackSensorRadius);
        }
    }

    if (mIsSingleMode) {
        bool isDemo = rc::isAnyActiveButDemoCameraDemo(this);
        if (!mIsPaused && isDemo) {
            mIsPaused = true;
            al::setVelocity(this, sead::Vector3f::zero);
        } else if (mIsPaused && !isDemo) {
            mIsPaused = false;
            al::setVelocity(this, mMoveVelocity);
        }

        if (mIsPaused) {
            return;
        }

        f32 rate = sead::Mathf::clamp(al::getNerveStep(this) / 100.0f, 0.0f, 1.0f);
        f32 shadowSize = al::easeOut(rate) * mShadowSize;
        al::setShadowMaskSize(this, "Body", sead::Vector3f::ones * shadowSize);

        if (mRainState != nullptr && mRainState->canFireballDoCheck(mFireballID)) {
            if (rc::isInAreaObj(this, rc::AreaObjType::FireBallSafeArea)) {
                al::hideShadow(this);
                kill();
                return;
            }

            if (rc::isInWaterArea(this)) {
                mIsLandingOnWater = true;
                al::setNerve(this, &NrvKoopaFireBallLandOnWater);
                return;
            }
        }
    }

    if (al::isCollided(this)) {
        if (mIsSingleMode && mIsKillOnLand) {
            extinguish();
            return;
        }

        sead::Vector3f normal = sead::Vector3f::ez;
        al::calcCollidedNormalSum(this, &normal);
        if (mIsSingleMode && al::isNearZero(normal, 0.001f)) {
            normal = sead::Vector3f::ez;
        }

        al::normalize(&normal);
        if (mIsKillOnCollide) {
            kill();
            return;
        }

        if (mIsExtinguishOnSlope && normal.dot(sead::Vector3f::ey) < cLandSlopeCosStrict) {
            if (mIsSingleMode) {
                extinguish();
            } else {
                kill();
            }

            return;
        }

        if (mIsSingleMode && normal.dot(sead::Vector3f::ey) < cLandSlopeCos) {
            extinguish();
            return;
        }

        sead::Vector3f landPos = {0.0f, 0.0f, 0.0f};
        if (al::isCollidedGround(this)) {
            landPos = al::getCollidedGroundPos(this);
        } else if (al::isCollidedWall(this)) {
            landPos = al::getCollidedWallPos(this);
        } else if (al::isCollidedCeiling(this)) {
            landPos = al::getCollidedCeilingPos(this);
        }

        al::offCollide(this);
        al::hideShadow(this);
        al::validateHitSensor(this, "Land");
        al::invalidateHitSensor(this, "Attack");
        al::makeQuatFrontNoSupport(al::getQuatPtr(this), -normal);
        al::setVelocityZero(this);
        al::setTrans(this, landPos);

        if (mLandType == LandType::NoLand) {
            kill();
            return;
        }

        if (mIsSingleMode && !checkGround()) {
            return;
        }

        al::setNerve(this, &NrvKoopaFireBallLandStart);
        return;
    }

    if (al::isLessStep(this, mLifeFrame)) {
        return;
    }

    al::setNerve(this, &NrvKoopaFireBallWait);
    if (mLandType == LandType::NoLand) {
        al::StringTmp<128> reactionName("自動消滅[%s]", mActionName);
        al::startHitReaction(this, reactionName.cstr());
    }

    kill();
}

/**
 * @brief Check that there is ground under the whole fire pool, extinguishing otherwise.
 * @return True if the fireball can burn here.
 */
bool KoopaFireBall::checkGround() {
    al::CollisionPartsFilterActor filter(this);
    al::setVelocity(this, sead::Vector3f::zero);

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

    extinguish();
    return false;
}

/**
 * @brief Grow into a pool of fire on the ground.
 */
void KoopaFireBall::exeLandStart() {
    if (al::isFirstStep(this)) {
        if (mIsKoopaLastAction) {
            al::startAction(this, "LandStartKoopaLast");
        } else {
            al::startAction(this, "LandStart");
        }

        if (mIsSingleMode) {
            if (mRainState != nullptr) {
                al::setScaleAll(this, mRainState->getParam()->mLandScale);
            }

            tryConnectToGround();
        }
    }

    f32 rate = al::getActionFrame(this) / al::getActionFrameMax(this, "LandStart");
    al::setSensorRadius(this, "Land", al::lerpValue(rate, mAttackSensorRadius, mLandSensorRadius));

    if (mIsSingleMode) {
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

    if (al::isActionEnd(this)) {
        al::setSensorRadius(this, "Land", mLandSensorRadius);
        al::setNerve(this, &NrvKoopaFireBallLand);
    }
}

/**
 * @brief Attach the fireball to the collision it landed on, so it follows moving ground.
 */
void KoopaFireBall::tryConnectToGround() {
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
void KoopaFireBall::updateConnection() {
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

            extinguish();
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
        al::Effect* effect = getEffectKeeper()->findEffect("LandEnd");
        if (effect != nullptr) {
            effect->tryKillEmitterAndParticleAll();
        }

        extinguish();
    }
}

/**
 * @brief Emit an echo pulse every second while burning on an echo block.
 */
void KoopaFireBall::updateEchoPulse() {
    if (mIsOnEchoBlock && mEchoTimer++ % 60 == 0) {
        rc::emitEcho(this, al::getTrans(this), mEchoRadius, 180, false);
    }
}

/**
 * @brief Burn on the ground, shrinking the damage area at the end.
 */
void KoopaFireBall::exeLand() {
    if (al::isFirstStep(this)) {
        if (mIsKoopaLastAction) {
            al::startAction(this, "LandLoopKoopaLast");
        } else {
            al::startAction(this, "LandLoop");
        }
    }

    s32 landFrame;
    if (mLandType == LandType::VeryShort) {
        landFrame = 30;
    } else if (mLandType == LandType::Short) {
        landFrame = 120;
    } else {
        landFrame = 240;
    }

    if (mIsSingleMode) {
        updateConnection();

        if (al::isGreaterEqualStep(this, landFrame - 45)) {
            f32 rate = sead::Mathf::clamp((al::getNerveStep(this) - (landFrame - 45)) / 30.0f,
                                          0.0f, 1.0f);
            al::setSensorRadius(this, "Land", al::lerpValue(rate, mLandSensorRadius, 0.0f));
        } else if (al::isLessEqualStep(this, landFrame - 105)) {
            updateEchoPulse();
        }
    }

    if (al::isGreaterEqualStep(this, landFrame)) {
        al::setNerve(this, &NrvKoopaFireBallLandEnd);
    }
}

/**
 * @brief Die out on the ground.
 */
void KoopaFireBall::exeLandEnd() {
    if (al::isFirstStep(this)) {
        if (mIsKoopaLastAction) {
            al::startAction(this, "LandEndKoopaLast");
        } else {
            al::startAction(this, "LandEnd");
        }
    }

    if (!mIsSingleMode) {
        f32 rate = al::getActionFrame(this) / al::getActionFrameMax(this, "LandEnd");
        al::setSensorRadius(this, "Land", al::lerpValue(rate, mLandSensorRadius, 0.0f));
    }

    if (mIsSingleMode) {
        updateConnection();
    }

    if (al::isActionEnd(this)) {
        al::setSensorRadius(this, "Land", 0.0f);
        kill();
    }
}

/**
 * @brief Sink in water.
 */
void KoopaFireBall::exeLandOnWater() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "LandOnWater");
        al::hideShadow(this);
        al::invalidateHitSensors(this);
        if (mOceanWaveKeeper != nullptr) {
            al::startOceanWave(this, "WaterColumn");
        }
    }

    if (al::isStep(this, 100)) {
        mIsLandingOnWater = false;
        kill();
    }
}

/**
 * @brief Die out in the air.
 */
void KoopaFireBall::exeExtinguish() {
    if (al::isFirstStep(this)) {
        al::tryDeleteEffectAndParticle(this, "LandImpact");
        al::tryDeleteEffectAndParticle(this, "Land");
        al::startAction(this, "Extinguish");
        al::invalidateHitSensors(this);
        al::hideShadow(this);
        al::setVelocity(this, sead::Vector3f::zero);
    }

    if (al::isGreaterStep(this, 60)) {
        kill();
    }
}

/**
 * @brief Check around the spawn position in a ring before starting to fall, extinguishing the
 * fireball if it spawned inside collision.
 */
void KoopaFireBall::exeSpawnCheck() {
    al::CollisionPartsFilterActor filter(this);
    sead::Vector3f pos = sead::Vector3f::zero;
    sead::Vector3f dir = sead::Vector3f::ey;
    getSpawnCheckArrow(&pos, &dir);

    if (alCollisionUtil::checkStrikeArrow(this, pos, dir, &filter, nullptr)) {
        kill();
        return;
    }

    mSpawnCheckStep++;
    if (mSpawnCheckStep < mRainState->getParam()->mCheckRingFrame) {
        return;
    }

    al::ShadowMaskBase* shadowMask = getShadowKeeper()->findShadowMask("Body");
    if (shadowMask != nullptr) {
        sead::Vector3f hitPos = sead::Vector3f::zero;
        f32 dropLength;
        if (alCollisionUtil::getFirstPolyOnArrow(this, &hitPos, nullptr, al::getTrans(this),
                                                 sead::Vector3f::ey * -20000.0f, nullptr,
                                                 nullptr)) {
            dropLength = sead::Mathf::max(2000.0f, al::getTrans(this).y - hitPos.y + 50.0f);
        } else {
            dropLength = 20000.0f;
        }

        shadowMask->setDropLength(dropLength);
    }

    al::showShadow(this);
    al::setShadowMaskSize(this, "Body", sead::Vector3f::zero);
    al::onCollide(this);
    al::invalidateHitSensor(this, "Land");
    al::validateHitSensor(this, "Attack");
    al::validateHitSensor(this, "NPCDisasterAvoid");
    al::showModelIfHide(this);
    al::setNerve(this, &NrvKoopaFireBallMove);
}

/**
 * @brief Calculate the arrow checked for collision in the current spawn check step: it goes down
 * from a point turning in a ring above the fireball.
 * @param pPos The output start position of the arrow.
 * @param pDir The output arrow.
 */
void KoopaFireBall::getSpawnCheckArrow(sead::Vector3f* pPos, sead::Vector3f* pDir) const {
    f32 checkHeight = mRainState->getParam()->mCheckHeight;
    const sead::Vector3f& trans = al::getTrans(this);
    sead::Vector3f center = checkHeight * sead::Vector3f::ey + trans;
    sead::Vector3f dir = sead::Vector3f::ey * -(checkHeight + mAttackSensorRadius + 1.0f);

    f32 rate = mSpawnCheckStep / static_cast<f32>(mRainState->getParam()->mCheckRingFrame);
    f32 radius = mAttackSensorRadius;
    f32 angle = sead::Mathf::deg2rad(rate * 360.0f);
    sead::Vector3f offset(cosf(angle), 0.0f, sinf(angle));
    *pPos = center + radius * offset;
    *pDir = dir;
}
