#include "Boss/TentackMagmaBall.hpp"
#include "Boss/TentackBase.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
namespace {
NERVE_DECL(TentackMagmaBall, Fall);
NERVE_DECL(TentackMagmaBall, LandStart);
NERVE_DECL(TentackMagmaBall, Land);
NERVE_DECL(TentackMagmaBall, LandEnd);
NERVES_MAKE_NOSTRUCT(TentackMagmaBall, Fall, LandStart, Land, LandEnd)
}
/** @brief Creates a lava ball. @param pName Actor name. @param pHost Host boss. */
TentackMagmaBall::TentackMagmaBall(const char* pName, TentackBase* pHost) : al::LiveActor(pName), mHost(pHost) {}
/** @brief Destroys the lava ball. */
TentackMagmaBall::~TentackMagmaBall() = default;
/** @brief Loads the lava ball and leaves it inactive. @param rInfo Initialization data. */
void TentackMagmaBall::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "TentackMagmaBall", nullptr);
    al::initNerve(this, &NrvTentackMagmaBallFall, 0);
    makeActorDead();
}
/** @brief Deactivates the ball and returns its reserved fall point. */
void TentackMagmaBall::kill() {
    al::LiveActor::kill();
    if (mFallPoint) {
        mHost->returnRockAppearPointPtr(mFallPoint);
        mFallPoint = nullptr;
    }
}
/** @brief Burns players and enemies in range. @param pSelf Attack sensor. @param pOther Target sensor. */
void TentackMagmaBall::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (!al::isSensorEnemyAttack(pSelf)) return;
    if (!(al::isSensorPlayer(pOther) || al::isSensorEnemy(pOther))) return;
    if (!al::isNerve(this, &NrvTentackMagmaBallFall)) {
        float height = al::getActorTrans(pOther).y - al::getTrans(this).y;
        if ((height > 0.0f ? height : -height) > 20.0f) return;
    }
    if (al::sendMsgEnemyAttackFire(pOther, pSelf) && al::isSensorPlayer(pOther)) {
        al::startHitReaction(this, "接触で消滅");
        kill();
    }
}
/** @brief Handles shattering and player fireballs. @param pMsg Message. @param pSelf Own sensor. @param pOther Sender sensor. @return Whether handled. */
bool TentackMagmaBall::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (rc::isMsgTentackMagmaBallBreak(pMsg)) {
        al::startHitReaction(this, "接触で消滅");
        kill();
        return true;
    }
    if (al::isMsgPlayerFireBallAttack(pMsg)) {
        rc::requestHitReactionToAttacker(pMsg, pOther, pSelf);
        return true;
    }
    return false;
}
/** @brief Starts falling at a world position. @param rPosition Starting position. */
void TentackMagmaBall::startFall(const sead::Vector3f& rPosition) {
    al::resetPosition(this, rPosition, false);
    auto* quat = al::getQuatPtr(this);
    al::makeQuatFrontNoSupport(quat, -sead::Vector3f::ey);
    al::setVelocityZero(this);
    al::setSensorRadius(this, 0, 65.0f);
    al::setNerve(this, &NrvTentackMagmaBallFall);
    al::onCollide(this);
    appear();
}
/** @brief Reserves a fall point and starts above it. @param pPoint Fall point. @param height Starting height. */
void TentackMagmaBall::startFall(const sead::Vector2f* pPoint, f32 height) {
    mFallPoint = pPoint;
    startFall(sead::Vector3f(pPoint->x, height, pPoint->y));
}
/** @brief Checks whether the ball is available for reuse. @return Whether inactive. */
bool TentackMagmaBall::isDeadRock() const { return al::isDead(this); }
/** @brief Falls until impact, shattering or spreading on the ground. */
void TentackMagmaBall::exeFall() {
    if (al::isFirstStep(this)) al::startAction(this, "Shot");
    al::addVelocityToGravity(this, 2.0f);
    al::scaleVelocityY(this, 0.9f);
    if (al::isCollidedGround(this)) {
        auto* sensor = al::tryGetCollidedGroundSensor(this);
        if (sensor && rc::sendMsgTentackMagmaBallBreak(sensor, al::getHitSensor(this, 0))) {
            al::startHitReaction(this, "接触で消滅");
            kill();
            return;
        }
        al::offCollide(this);
        al::setNerve(this, &NrvTentackMagmaBallLandStart);
        al::setTrans(this, al::getCollidedGroundPos(this));
        al::setVelocityZero(this);
        sead::Vector3f normal = sead::Vector3f::ez;
        al::calcCollidedNormalSum(this, &normal);
        al::normalize(&normal);
        auto* quat = al::getQuatPtr(this);
        al::makeQuatFrontNoSupport(quat, -normal);
    }
    if (rc::isInDeathArea(this) || al::isGreaterEqualStep(this, 600)) {
        al::startHitReaction(this, "接触で消滅");
        kill();
    }
}
/** @brief Expands the attack radius during the landing animation. */
void TentackMagmaBall::exeLandStart() {
    if (al::isFirstStep(this)) al::startAction(this, "LandStart");
    float frame = al::getActionFrame(this);
    float max = al::getActionFrameMax(this, "LandStart");
    al::setSensorRadius(this, 0, al::lerpValue(al::normalize(frame, 0.0f, max), 65.0f, 180.0f));
    al::setNerveAtActionEnd(this, &NrvTentackMagmaBallLand);
}
/** @brief Keeps the landed lava active for a limited time. */
void TentackMagmaBall::exeLand() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Land");
        al::setSensorRadius(this, 0, 180.0f);
    }
    if (al::isGreaterEqualStep(this, 180)) al::setNerve(this, &NrvTentackMagmaBallLandEnd);
}
/** @brief Removes the lava after its ending animation. */
void TentackMagmaBall::exeLandEnd() {
    if (al::isFirstStep(this)) al::startAction(this, "LandEnd");
    if (al::isActionEnd(this)) kill();
}
