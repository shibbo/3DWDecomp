#include "Boss/BunbunShell.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Util/ProjectMsgUtil.hpp"
namespace {
NERVE_DECL(BunbunShell, Appear);
NERVE_DECL(BunbunShell, Spin);
NERVES_MAKE_NOSTRUCT(BunbunShell, Appear, Spin)
}
/** @brief Creates the spinning shell with cleared timers. @param pName Actor name. */
BunbunShell::BunbunShell(const char* pName) : al::LiveActor(pName) {}
/** @brief Destroys the shell. */
BunbunShell::~BunbunShell() = default;
/** @brief Loads the shell and leaves it inactive. @param rInfo Initialization data. */
void BunbunShell::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, sead::SafeString("BunbunShell"), nullptr);
    al::initNerve(this, &NrvBunbunShellAppear, 0);
    makeActorDead();
}
/** @brief Resets the shell and starts its appearance animation. */
void BunbunShell::appear() {
    mSpinStep = 0;
    mTrampleCooldown = 0;
    al::invalidateClipping(this);
    al::setNerve(this, &NrvBunbunShellAppear);
    al::LiveActor::appear();
}
/** @brief Updates the stomp cooldown and vertical velocity. */
void BunbunShell::control() {
    mTrampleCooldown = mTrampleCooldown > 0 ? mTrampleCooldown - 1 : 0;
    al::addVelocityToGravity(this, 6.0f);
    al::scaleVelocityY(this, 0.8f);
}
/** @brief Pushes players and sends the shell attack. @param pSelf Shell sensor. @param pOther Contacted sensor. */
void BunbunShell::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isSensorEnemyAttack(pSelf)) {
        if (al::isSensorPlayer(pOther)) al::sendMsgPush(pOther, pSelf);
        al::sendMsgEnemyAttack(pOther, pSelf);
    }
}
/** @brief Handles stomps, reflections, and projectile hits. @param pMsg Incoming message. @param pOther Sender sensor. @param pSelf Shell sensor. @return Whether the message was handled. */
bool BunbunShell::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) {
    if ((al::isMsgPlayerTrample(pMsg) || al::isMsgPlayerObjHipDropReflectAll(pMsg) ||
         al::isMsgPlayerBodyAttackReflect(pMsg) || al::isMsgPlayerTailAttack(pMsg) ||
         al::isMsgPlayerClimbAttack(pMsg) || al::isMsgPlayerSpinAttack(pMsg)) && mTrampleCooldown == 0) {
        *al::getVelocityPtr(this) *= 0.9f;
        auto* velocity = al::getVelocityPtr(this);
        *velocity += sead::Vector3f::ey * 70.0f;
        mTrampleCooldown = 10;
        al::startSe(this, sead::SafeString("PgTrampledShell"), nullptr);
        return true;
    }
    if (al::isMsgEnemyAttack(pMsg)) {
        if (mSpinStep < 60) return false;
        reflectSensor(pOther, pSelf);
        return true;
    }
    if (al::isMsgPlayerFireBallAttack(pMsg)) {
        rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        return true;
    }
    return al::isMsgPlayerBoomerangReflect(pMsg);
}
/** @brief Reflects the shell away from another sensor. @param pOther Contacted sensor. @param pSelf Shell sensor. */
void BunbunShell::reflectSensor(al::HitSensor* pOther, al::HitSensor* pSelf) {
    sead::Vector3f difference = al::getSensorPos(pSelf) - al::getSensorPos(pOther);
    sead::Vector3f normal;
    al::verticalizeVec(&normal, sead::Vector3f::ey, difference);
    al::normalizeOrDirZ(&normal);
    al::calcReflectionVector(al::getVelocityPtr(this), normal, 1.0f, 0.0f);
    al::setFront(this, normal);
    if (!al::isActionPlaying(this, "SpinReflect")) {
        al::startHitReaction(this, "反射");
        al::startAction(this, "SpinReflect");
    }
}
/** @brief Accepts touch-assist messages. @param pMsg Incoming message. @return Whether this is touch assist. */
bool BunbunShell::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer*, al::ScreenPointTarget*) {
    return al::isMsgTouchAssist(pMsg);
}
/** @brief Plays the appearance animation before spinning. */
void BunbunShell::exeAppear() {
    if (al::isFirstStep(this)) al::startAction(this, "AppearJump");
    if (al::isActionEnd(this)) al::setNerve(this, &NrvBunbunShellSpin);
}
/** @brief Spins and reflects away from walls. */
void BunbunShell::exeSpin() {
    if (al::isFirstStep(this)) al::startAction(this, "Spin");
    ++mSpinStep;
    if (al::isActionPlaying(this, "SpinReflect") && al::isActionEnd(this)) al::startAction(this, "Spin");
    if (al::isCollidedWallVelocity(this)) {
        sead::Vector3f normal = al::getCollidedWallNormal(this);
        if (al::isReverseDirection(normal, al::getFront(this), 0.2f))
            al::turnDirectionDegree(this, &normal, al::getFront(this), 30.0f);
        else if (!al::isReverseDirection(normal, al::getFront(this), 0.8f))
            al::turnDirectionDegree(this, &normal, -al::getFront(this), 20.0f);
        al::calcReflectionVector(al::getVelocityPtr(this), normal, 1.0f, 0.0f);
        sead::Vector3f horizontal;
        al::verticalizeVec(&horizontal, sead::Vector3f::ey, al::getVelocity(this));
        al::normalizeOrDirZ(al::getFrontPtr(this), horizontal);
        if (!al::isActionPlaying(this, "SpinReflect")) al::startAction(this, "SpinReflect");
        al::startHitReaction(this, "反射");
    }
}
