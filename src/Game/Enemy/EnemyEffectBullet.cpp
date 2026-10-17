#include "Enemy/EnemyEffectBullet.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Factory/Factory.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Math/MatrixUtil.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "System/GameDataFunction.hpp"
#include "Util/PlayerUtil.hpp"

/** @brief Creates an effect projectile. @param pName Actor name. @param pInfo Shared projectile settings. */
EnemyEffectBullet::EnemyEffectBullet(const char* pName, const EnemyEffectBulletInfo* pInfo)
    : al::LiveActor(pName), mInfo(pInfo) {}

/** @brief Initializes the projectile from its archive. @param rInfo Scene information. */
void EnemyEffectBullet::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, mInfo->mArchiveName, mInfo->mSuffix);
    makeActorDead();
}

/** @brief Starts the projectile's effects and resets its lifetime. */
void EnemyEffectBullet::appear() {
    al::LiveActor::appear();
    al::resetPosition(this, false);
    mEffectMtx.makeRT({0.0f, 0.0f, 0.0f}, al::getTrans(this));
    al::emitEffect(this, mInfo->mAppearEffect, nullptr);
    mLifeStep = 0;
    mTurnDegree = 0.0f;
}

/** @brief Stops the projectile and emits its disappearance effects. */
void EnemyEffectBullet::kill() {
    if (mInfo->mVanishSe) { al::startSe(this, mInfo->mVanishSe, nullptr); }
    al::LiveActor::kill();
    al::emitEffect(this, mInfo->mDisappearEffect, nullptr);
}

/** @brief Updates lifetime, homing velocity, and the effect pose. */
void EnemyEffectBullet::control() {
    if (mLifeStep == 0) { al::emitEffect(this, mInfo->mBodyEffect, nullptr); }
    if (mInfo->mLifeFrame <= mLifeStep || al::isCollided(this)) {
        kill();
        return;
    }
    if (mInfo->mFlySe) { al::holdSe(this, mInfo->mFlySe, nullptr); }
    ++mLifeStep;
    if (mInfo->mIsHoming && mInfo->mHomingStartFrame <= mLifeStep) {
        if (mLifeStep % 60 == 0 || !mTarget || rc::isPlayerDeadOrBubble(mTarget)) {
            mTarget = rc::findNearestActivePlayerActor(this);
        }
        if (mTarget) {
            sead::Vector3f front = al::getVelocity(this);
            sead::Vector3f targetDir = al::getTrans(mTarget) - al::getTrans(this);
            al::normalizeOrDirZ(&front);
            al::normalizeOrDirZ(&targetDir);
            f32 angle = al::calcAngleOnPlaneDegree(front, targetDir, sead::Vector3f::ey);
            mTurnDegree = al::converge(mTurnDegree, angle, mInfo->mTurnAcceleration);
            mTurnDegree = sead::Mathf::clamp(mTurnDegree, -mInfo->mMaxTurnDegree, mInfo->mMaxTurnDegree);
            al::rotateVectorDegreeY(&front, mTurnDegree);
            al::setVelocityToDirection(this, front, al::getVelocity(this).length());
        }
    }
    sead::Vector3f front = al::getVelocity(this);
    front.y = 0.0f;
    al::normalizeOrDirZ(&front);
    al::makeMtxFrontUpPos(&mEffectMtx, front, sead::Vector3f::ey, al::getTrans(this));
}

/** @brief Sends projectile damage and disappears on an accepted hit.
 * @param pSelf Projectile sensor. @param pOther Receiving sensor.
 */
void EnemyEffectBullet::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (mInfo->mIsFire && al::sendMsgEnemyAttackFire(pOther, pSelf)) {
        kill();
        return;
    }
    if ((al::isSensorPlayer(pOther) || al::isSensorKoopaJr(pOther)) &&
        al::sendMsgEnemyAttackForCrossoverSensor(pOther, pSelf)) {
        kill();
        return;
    }
    if (GameDataFunction::isSingleMode(GameDataHolderAccessor(this)) &&
        al::isEqualString(mInfo->mArchiveName, "KameckMagicBall")) {
        if (al::sendMsgEnemyAttack(pOther, pSelf) ||
            ((al::isSensorMapObj(pOther) || al::isSensorKickKoura(pOther) || al::isSensorNpc(pOther)) &&
             al::sendMsgEnemyAttackBoomerang(pOther, pSelf))) {
            kill();
        }
    }
}

/** @brief Handles attacks that destroy the projectile.
 * @param pMsg Incoming message. @param pOther Sender sensor. @param pSelf Receiving sensor.
 * @return Whether the attack was handled.
 */
bool EnemyEffectBullet::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) {
    if (al::isMsgPlayerInvincibleAttack(pMsg) || (mInfo->mIsFire && al::isMsgPlayerFireBallAttack(pMsg))) {
        kill();
        return true;
    }
    return false;
}

/** @brief Launches the projectile. @param rDir Launch direction. @param speed Launch speed. */
void EnemyEffectBullet::shot(const sead::Vector3f& rDir, f32 speed) {
    al::faceToDirection(this, rDir);
    al::setVelocityToDirection(this, rDir, speed);
    appear();
}

/** @brief Initializes shared projectile settings.
 * @param pArchiveName Model archive. @param lifeFrame Lifetime in frames. @param pBodyEffect Traveling effect.
 * @param pAppearEffect Appearance effect. @param pDisappearEffect Disappearance effect.
 * @param pFlySe Traveling sound. @param pVanishSe Disappearance sound. @param pSuffix Archive variant.
 * @param isFire Whether to use fire damage. @param isHoming Whether to track players.
 * @param homingStartFrame First homing frame. @param maxTurnDegree Maximum turning angle per frame.
 * @param turnAcceleration Change in the turning angle per frame.
 */
EnemyEffectBulletInfo::EnemyEffectBulletInfo(const char* pArchiveName, s32 lifeFrame, const char* pBodyEffect,
    const char* pAppearEffect, const char* pDisappearEffect, const char* pFlySe, const char* pVanishSe,
    const char* pSuffix, bool isFire, bool isHoming, s32 homingStartFrame, f32 maxTurnDegree, f32 turnAcceleration)
    : mArchiveName(pArchiveName), mSuffix(pSuffix), mLifeFrame(lifeFrame), mBodyEffect(pBodyEffect),
      mAppearEffect(pAppearEffect), mDisappearEffect(pDisappearEffect), mFlySe(pFlySe), mVanishSe(pVanishSe),
      mIsFire(isFire), mIsHoming(isHoming), mHomingStartFrame(homingStartFrame),
      mMaxTurnDegree(maxTurnDegree), mTurnAcceleration(turnAcceleration) {}
