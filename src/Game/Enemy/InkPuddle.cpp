#include "Enemy/InkPuddle.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
namespace {
NERVE_ACTION_IMPL(InkPuddle, Start)
NERVE_ACTION_IMPL(InkPuddle, Wait)
NERVE_ACTION_IMPL(InkPuddle, Ignite)
NERVE_ACTION_IMPL(InkPuddle, QuickEnd)
NERVE_ACTION_IMPL(InkPuddle, End)
NERVE_ACTIONS_MAKE_STRUCT(InkPuddle, Start, Wait, Ignite, QuickEnd, End)
}
/** @brief Creates an ink puddle. @param pName Actor name. */
InkPuddle::InkPuddle(const char* pName) : al::LiveActor(pName) {}
/** @brief Initializes the hazard and its lifetime mode. @param rInfo Placement information. */
void InkPuddle::init(const al::ActorInitInfo& rInfo) {
    al::initNerveAction(this, "Start", &NrvInkPuddle.collector, 0);
    al::initActorWithArchiveName(this, rInfo, "InkPuddle", nullptr);
    al::tryGetArg(&mInfinite, rInfo, "Infinite");
    al::invalidateClipping(this);
    mRadius = al::getSensorRadius(this, "Body");
    if (mInfinite) {
        makeActorAppeared();
        al::startNerveAction(this, "Wait");
        al::setSensorRadius(this, "Body", al::getScaleX(this) * mRadius);
    } else makeActorDead();
}
/** @brief Limits player movement and applies ink or fire damage. @param pSelf Own sensor. @param pOther Contact sensor. */
void InkPuddle::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (mInfinite) return;
    bool burning = al::isNerve(this, NrvInkPuddle.Ignite.data()) && al::isGreaterEqualStep(this, 15);
    if (al::isSensorPlayer(pOther) && al::isSensorName(pOther, "Foot")) {
        f32 radius = al::getSensorRadius(pSelf);
        sead::Vector3f offset = al::getSensorPos(pOther) - al::getSensorPos(pSelf);
        offset.y -= al::getSensorRadius(pOther);
        if (!(offset.y < 0.0f)) return;
        if (!(offset.squaredLength() < radius * radius)) return;
        sead::Vector3f velocity = rc::getPlayerVelocity(pOther);
        f32 maxSpeed = rc::isPlayerDash(pOther) ? 100.0f : 80.0f;
        sead::Vector3f horizontal(velocity.x, 0.0f, velocity.z);
        if (horizontal.x * horizontal.x + horizontal.z * horizontal.z > maxSpeed * maxSpeed && !al::normalizeOrZero(&horizontal)) horizontal *= maxSpeed;
        f32 vertical = al::clamp(velocity.y, -240.0f, 120.0f);
        rc::setPlayerVelocity(al::getSensorHost(pOther), horizontal + sead::Vector3f(0.0f, vertical, 0.0f));
        if (burning) al::sendMsgGigaEnemyAttack(pOther, pSelf);
        rc::sendMsgInkTouch(pOther, pSelf);
    } else if (burning) al::sendMsgEnemyAttackFire(pOther, pSelf);
}
/** @brief Allows players to disregard finite puddles. @param pMsg Message. @param pOther Sender. @param pSelf Own sensor. @return Whether handled. */
bool InkPuddle::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) { return !mInfinite && al::isMsgPlayerDisregard(pMsg); }
/** @brief Starts growth or restores an infinite puddle. */
void InkPuddle::appear() {
    al::LiveActor::appear();
    if (mInfinite) al::startNerveAction(this, "Wait");
    else { al::startNerveAction(this, "Start"); al::setScaleAll(this, 0.1f); }
}
/** @brief Removes the puddle. */
void InkPuddle::kill() { al::LiveActor::kill(); }
/** @brief Starts a randomly sized puddle beside a direction. @param rDirection Lateral scatter reference. */
void InkPuddle::start(const sead::Vector3f& rDirection) {
    appear();
    al::rotateQuatYDirRandomDegree(this);
    mScaleFactor = al::getRandom(-0.2f, 0.2f) + 1.0f;
    sead::Vector3f offset = rDirection;
    al::rotateVectorDegreeY(&offset, 90.0f);
    offset *= al::getRandom(-800.0f, 800.0f);
    al::resetPosition(this, al::getTrans(this) + offset, false);
}
/** @brief Begins the accelerated fade. */
void InkPuddle::quickEnd() { if (al::isAlive(this)) al::startNerveAction(this, "QuickEnd"); }
/** @brief Expands the puddle over thirty frames. */
void InkPuddle::exeStart() {
    f32 scale = (static_cast<f32>(al::getNerveStep(this)) / 30.0f * 0.9f + 0.1f) * 20.0f;
    scale = mScaleFactor * scale;
    al::setScaleAll(this, scale);
    al::setSensorRadius(this, "Body", mRadius * scale);
    if (al::isGreaterEqualStep(this, 30)) al::startNerveAction(this, "Wait");
}
/** @brief Keeps finite puddles alive for twenty seconds. */
void InkPuddle::exeWait() { if (!mInfinite && al::isGreaterEqualStep(this, 1200)) al::startNerveAction(this, "End"); }
/** @brief Burns for ten seconds. */
void InkPuddle::exeIgnite() { if (al::isGreaterEqualStep(this, 600)) al::startNerveAction(this, "End"); }
/** @brief Shrinks away in ten frames. */
void InkPuddle::exeQuickEnd() {
    f32 scale = ((static_cast<f32>(al::getNerveStep(this)) / -10.0f + 1.0f) * 0.99f + 0.01f) * 20.0f;
    al::setScaleAll(this, scale);
    al::setSensorRadius(this, "Body", mRadius * scale);
    if (al::isGreaterEqualStep(this, 10)) kill();
}
/** @brief Shrinks away in ninety frames. */
void InkPuddle::exeEnd() {
    f32 scale = ((static_cast<f32>(al::getNerveStep(this)) / -90.0f + 1.0f) * 0.99f + 0.01f) * 20.0f;
    al::setScaleAll(this, scale);
    al::setSensorRadius(this, "Body", mRadius * scale);
    if (al::isGreaterEqualStep(this, 90)) kill();
}
