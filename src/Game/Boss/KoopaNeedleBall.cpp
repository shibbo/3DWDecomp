#include "Boss/KoopaNeedleBall.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Actor/ComboCounter.hpp"
#include "Library/Collision/Collider.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
namespace {
NERVE_DECL(KoopaNeedleBall, Wait);
NERVE_DECL(KoopaNeedleBall, Break);
NERVE_DECL(KoopaNeedleBall, Fall);
NERVE_DECL(KoopaNeedleBall, Move);
NERVES_MAKE_NOSTRUCT(KoopaNeedleBall, Wait, Break, Fall, Move)
}
/** @brief Creates a rolling needle ball and combo counter. @param pName Actor name. */
KoopaNeedleBall::KoopaNeedleBall(const char* pName) : al::LiveActor(pName), mComboCounter(new al::ComboCounter) {}
/** @brief Destroys the needle ball. */
KoopaNeedleBall::~KoopaNeedleBall() = default;
/** @brief Loads the ball and starts waiting. @param rInfo Initialization data. */
void KoopaNeedleBall::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "KoopaNeedleBall", nullptr);
    al::initNerve(this, &NrvKoopaNeedleBallWait, 0);
    makeActorAppeared();
}
/** @brief Damages the contacted sensor. @param pSelf Own sensor. @param pOther Target sensor. */
void KoopaNeedleBall::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) { al::sendMsgEnemyAttack(pOther, pSelf); }
/** @brief Breaks when hit by an invincible player. @param pMsg Message. @param pSelf Own sensor. @param pOther Sender sensor. @return Whether handled. */
bool KoopaNeedleBall::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isMsgPlayerInvincibleAttack(pMsg)) {
        if (al::isNerve(this, &NrvKoopaNeedleBallBreak)) return false;
        al::setNerve(this, &NrvKoopaNeedleBallBreak);
        return true;
    }
    return false;
}
/** @brief Launches the ball forward and enables collision. */
void KoopaNeedleBall::launch() {
    sead::Vector3f front = sead::Vector3f::zero;
    al::calcQuatFront(&front, this);
    al::onCollide(this);
    al::setVelocity(this, front * 8.0f);
    al::startAction(this, "Wait");
    al::setNerve(this, &NrvKoopaNeedleBallFall);
    makeActorAppeared();
}
/** @brief Waits for a launch request. */
void KoopaNeedleBall::exeWait() {}
/** @brief Updates airborne rolling and detects landing. */
void KoopaNeedleBall::exeFall() {
    bool grounded = al::isOnGroundNoVelocity(this, 0);
    updateVelocity(grounded);
    updateRolling(grounded);
    if (grounded) al::setNerve(this, &NrvKoopaNeedleBallMove);
    else if (al::isGreaterStep(this, 300)) kill();
}
/** @brief Reflects off walls and applies gravity or slope acceleration. @param isOnGround Whether grounded. */
void KoopaNeedleBall::updateVelocity(bool isOnGround) {
    sead::Vector3f wallNormal;
    sead::Vector3f velocity;
    if (al::isCollidedWallFace(this)) {
        sead::Vector3f normal;
        static_cast<const al::SphereHitInfo&>(getCollider()->mWall).calcFixVectorNormal(&normal, nullptr);
        if (al::getVelocity(this).dot(normal) < 0.0f) {
            wallNormal.set(normal);
            al::verticalizeVec(&wallNormal, al::getGravity(this), wallNormal);
            if (!al::normalizeOrZero(&wallNormal)) {
                velocity.set(al::getVelocity(this));
                al::calcReflectionVector(&velocity, wallNormal, 1.0f, 0.0f);
                velocity *= 0.92f;
                al::setVelocity(this, velocity);
                return;
            }
        }
    }
    float damping = isOnGround ? 0.7f : 1.0f;
    const auto& gravity = al::getGravity(this);
    al::verticalizeVec(&wallNormal, gravity, al::getVelocity(this));
    velocity.set(al::getVelocity(this));
    al::parallelizeVec(&velocity, gravity, al::getVelocity(this));
    velocity = wallNormal + velocity * damping;
    al::setVelocity(this, velocity);
    if (isOnGround) {
        wallNormal = sead::Vector3f(al::getGravity(this));
        al::verticalizeVec(&wallNormal, al::getOnGroundNormal(this, 0), wallNormal);
        al::tryAddVelocityLimit(this, wallNormal, 8.0f);
    } else {
        al::scaleVelocity(this, 1.0f);
        al::addVelocityToGravity(this, 1.2f);
    }
}
/** @brief Rotates the ball according to its velocity and contact normal. @param isOnGround Whether grounded. */
void KoopaNeedleBall::updateRolling(bool isOnGround) {
    sead::Vector3f moment = sead::Vector3f::zero;
    sead::Vector3f up = -al::getGravity(this);
    if (isOnGround) up.set(al::getOnGroundNormal(this, 0));
    const auto& velocity = al::getVelocity(this);
    al::calcMomentRollBall(&moment, velocity, up, al::getColliderRadius(this));
    auto* quat = al::getQuatPtr(this);
    al::rotateQuatMoment(quat, *quat, moment);
}
/** @brief Rolls along the ground until falling or timing out. */
void KoopaNeedleBall::exeMove() {
    bool grounded = al::isOnGroundNoVelocity(this, 0);
    updateVelocity(grounded);
    updateRolling(grounded);
    if (!grounded) al::setNerve(this, &NrvKoopaNeedleBallFall);
    else if (al::isGreaterStep(this, 300)) kill();
}
/** @brief Plays the breaking animation and removes the ball. */
void KoopaNeedleBall::exeBreak() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Break");
        al::setVelocityZero(this);
    }
    if (al::isActionEnd(this)) kill();
}
