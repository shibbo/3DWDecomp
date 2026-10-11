#include "Enemy/EnemyStateBlowDown.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Nerve/NerveSetup.hpp"

namespace {
NERVE_DECL(EnemyStateBlowDown, Down)
NERVES_MAKE_NOSTRUCT(EnemyStateBlowDown, Down)
}

/** @brief Creates standard knockback parameters, optionally for swimming.
 * @param isSwim Whether to use the swimming animation and reduced speeds.
 */
EnemyStateBlowDownParam::EnemyStateBlowDownParam(bool isSwim)
    : mAction("BlowDown"), mSpeed(10.3f), mJumpSpeed(28.2f), mFriction(0.995f),
      mGravity(1.1f), mStep(20) {
    if (isSwim) {
        mAction = "SwimBlowDown";
        mSpeed = 8.0f;
        mJumpSpeed = 17.0f;
        mFriction = 0.98f;
        mGravity = 0.4f;
    }
}

/** @brief Creates standard knockback parameters with a custom animation.
 * @param pAction Animation name.
 */
EnemyStateBlowDownParam::EnemyStateBlowDownParam(const char* pAction)
    : mAction(pAction), mSpeed(10.3f), mJumpSpeed(28.2f), mFriction(0.995f),
      mGravity(1.1f), mStep(20) {}

/** @brief Creates custom knockback parameters.
 * @param pAction Animation name.
 * @param speed Horizontal launch speed.
 * @param jumpSpeed Vertical launch speed.
 * @param friction Velocity multiplier per step.
 * @param gravity Downward acceleration.
 * @param step Minimum step before a wall collision ends knockback.
 */
EnemyStateBlowDownParam::EnemyStateBlowDownParam(const char* pAction, float speed,
        float jumpSpeed, float friction, float gravity, int step)
    : mAction(pAction), mSpeed(speed), mJumpSpeed(jumpSpeed), mFriction(friction),
      mGravity(gravity), mStep(step) {}

/** @brief Constructs a named knockback state.
 * @param pName State name.
 * @param pHost Actor moved by the state.
 * @param pParam Knockback settings.
 */
EnemyStateBlowDown::EnemyStateBlowDown(const char* pName, al::LiveActor* pHost,
        const EnemyStateBlowDownParam* pParam)
    : al::ActorStateBase(pName, pHost), mParam(pParam) {
    initNerve(&NrvEnemyStateBlowDownDown, 0);
}

/** @brief Constructs a knockback state with the default name.
 * @param pHost Actor moved by the state.
 * @param pParam Knockback settings.
 */
EnemyStateBlowDown::EnemyStateBlowDown(al::LiveActor* pHost,
        const EnemyStateBlowDownParam* pParam)
    : EnemyStateBlowDown("吹き飛び状態", pHost, pParam) {}

/** @brief Starts knockback and faces opposite the launch direction. */
void EnemyStateBlowDown::appear() {
    al::ActorStateBase::appear();
    al::setNerve(this, &NrvEnemyStateBlowDownDown);
    sead::Vector3f dir = mBlowDir;
    al::setVelocity(mHostActor, dir);
    dir.y = 0.0f;
    dir.normalize();
    al::faceToDirection(mHostActor, -dir);
    al::invalidateClipping(mHostActor);
}

/** @brief Marks the knockback state finished. */
void EnemyStateBlowDown::kill() { mIsDead = true; }

/** @brief Launches the host away from another actor.
 * @param pActor Actor supplying the launch origin.
 */
void EnemyStateBlowDown::setBlowDir(const al::LiveActor* pActor) {
    sead::Vector3f dir = al::getTrans(mHostActor);
    dir -= al::getTrans(pActor);
    dir.y = 0.0f;
    float speed = mParam->mSpeed;
    if (dir.x == 0.0f && dir.z == 0.0f) {
        dir.set(0.0f, 0.0f, speed);
    } else {
        float length = dir.length();
        if (length > 0.0f) {
            dir *= 1.0f / length;
        }
        dir *= speed;
    }
    dir.y = mParam->mJumpSpeed;
    setBlowDir(dir);
}

/** @brief Assigns a launch velocity and faces opposite its horizontal direction.
 * @param rDir Launch velocity.
 */
void EnemyStateBlowDown::setBlowDir(const sead::Vector3f& rDir) {
    mBlowDir = rDir;
    al::setVelocity(mHostActor, mBlowDir);
    sead::Vector3f dir = -rDir;
    dir.y = 0.0f;
    dir.normalize();
    al::faceToDirection(mHostActor, dir);
}

/** @brief Launches the host away from an attacking sensor.
 * @param pOther Attacking sensor.
 * @param pSelf Host sensor.
 */
void EnemyStateBlowDown::setBlowDir(const al::HitSensor* pOther, const al::HitSensor* pSelf) {
    sead::Vector3f dir = al::getSensorPos(pSelf);
    dir -= al::getSensorPos(pOther);
    dir.y = 0.0f;
    float speed = mParam->mSpeed;
    if (dir.x == 0.0f && dir.z == 0.0f) {
        dir.set(0.0f, 0.0f, speed);
    } else {
        float length = dir.length();
        if (length > 0.0f) {
            dir *= 1.0f / length;
        }
        dir *= speed;
    }
    dir.y = mParam->mJumpSpeed;
    setBlowDir(dir);
}

/** @brief Applies configured horizontal and vertical launch speeds.
 * @param rDir Direction away from the launch origin.
 */
void EnemyStateBlowDown::setBlowDirScale(const sead::Vector3f& rDir) {
    sead::Vector3f dir(rDir.x, 0.0f, rDir.z);
    float speed = mParam->mSpeed;
    if (dir.x == 0.0f && dir.z == 0.0f) {
        dir.set(0.0f, 0.0f, speed);
    } else {
        float length = dir.length();
        if (length > 0.0f) {
            float rate = 1.0f / length;
            dir.set(rate * dir.x, rate * dir.y, rate * dir.z);
        }
        dir *= speed;
    }
    dir.y = mParam->mJumpSpeed;
    setBlowDir(dir);
}

/** @brief Replaces the active knockback settings.
 * @param pParam New knockback settings.
 */
void EnemyStateBlowDown::setBlowDownParam(const EnemyStateBlowDownParam* pParam) {
    mParam = pParam;
}

/** @brief Applies gravity and drag until the animation or a wall impact ends knockback. */
void EnemyStateBlowDown::exeDown() {
    if (al::isFirstStep(this)) {
        al::onCollide(mHostActor);
        if (mParam->mAction) {
            al::startAction(mHostActor, mParam->mAction);
        }
    }
    if (al::isOnGround(mHostActor, 0, 0.0f)) {
        al::setVelocityZero(mHostActor);
    } else {
        al::addVelocityToGravity(mHostActor, mParam->mGravity);
        al::scaleVelocity(mHostActor, mParam->mFriction);
    }
    if (al::isActionEnd(mHostActor) || (al::isExistActorCollider(mHostActor) &&
        al::isGreaterStep(this, mParam->mStep) && al::isCollidedWall(mHostActor))) {
        if (!mIsKeepClippingInvalid) {
            al::validateClipping(mHostActor);
        }
        kill();
    }
}
