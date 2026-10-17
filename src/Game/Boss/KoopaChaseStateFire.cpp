#include "Boss/KoopaChaseStateFire.hpp"
#include "Boss/KoopaChase.hpp"
#include "Boss/KoopaChaseKoopa.hpp"
#include "Boss/KoopaChaseFunction.hpp"
#include "MapObj/KoopaFireBall.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include <math/seadGeometry.h>
namespace {
NERVE_DECL(KoopaChaseStateFire, Wait);
NERVE_DECL(KoopaChaseStateFire, FireStart);
NERVE_DECL(KoopaChaseStateFire, FireEnd);
NERVES_MAKE_NOSTRUCT(KoopaChaseStateFire, Wait, FireStart, FireEnd)
/** @brief Finds the forward ray intersection with the ground plane. */
void calcGroundIntersection(sead::Vector3f* pOut, const sead::Vector3f& origin,
                            const sead::Vector3f& direction, const sead::Vector3f& normal,
                            const sead::Vector3f& ground) {
    sead::Plane3f plane(normal, ground);
    sead::Ray<sead::Vector3f> ray(origin, direction);
    float t = 0.0f;
    if (sead::Geometry::calcIntersectionRayToPlane(ray, plane, &t) == 1)
        *pOut = ray.getPos() + ray.getDir() * t;
}
}
/** @brief Creates the six fireballs used during a chase attack. @param pHost Chase actor. @param rInfo Initialization data. @param isProvocation Use the taunt target and shallow firing angle. */
KoopaChaseStateFire::KoopaChaseStateFire(KoopaChase* pHost, const al::ActorInitInfo& rInfo, bool isProvocation)
    : al::HostStateBase<KoopaChase>("クッパチェイスState[火を吐く]", pHost) {
    mFireBalls.setBuffer(6, mFireBallStorage);
    mDirection.set(sead::Vector3f::ez);
    mFireNum = 3;
    mFireCount = 0;
    mIsProvocation = isProvocation;
    mKillOnCollide = false;
    initNerve(&NrvKoopaChaseStateFireWait, 0);
    for (int i = 0; i < mFireBalls.capacity(); i++) {
        auto* fire = new KoopaFireBall("クッパファイアボール", false, getHost());
        fire->setExtinguishOnSlope(true);
        fire->setActionName("ShotKoopaChase");
        al::initCreateActorNoPlacementInfo(fire, rInfo);
        mFireBalls.pushBack(fire);
    }
}
/** @brief Destroys the attack state. */
KoopaChaseStateFire::~KoopaChaseStateFire() = default;
/** @brief Resets the shot count and starts waiting. */
void KoopaChaseStateFire::appear() {
    al::NerveStateBase::appear();
    mFireCount = 0;
    al::setNerve(this, &NrvKoopaChaseStateFireWait);
}
/** @brief Sets the number of shots in the attack. @param fireNum Shot count. */
void KoopaChaseStateFire::setFireNum(int fireNum) { mFireNum = fireNum; }
/** @brief Gets the tongue joint position. @param pOut Receives the world position. */
void KoopaChaseStateFire::calcMouthPos(sead::Vector3f* pOut) const {
    al::calcJointPos(pOut, KoopaChaseFunction::getKoopa(getHost()), "Tongue1");
}
/** @brief Aims at the target player. @param pOut Receives the normalized shot direction. @return Whether the ground impact is over 400 units from the chase center line. */
bool KoopaChaseStateFire::calcFireDir(sead::Vector3f* pOut) const {
    auto* target = mIsProvocation ? KoopaChaseFunction::getLookAtTargetPlayer(getHost()) : KoopaChaseFunction::getTargetPlayer(getHost());
    if (!target) return false;
    sead::Vector3f mouth;
    calcMouthPos(&mouth);
    sead::Vector3f dir = al::getTrans(target) + sead::Vector3f::ey * 100.0f - mouth;
    dir.y = 0.0f;
    al::normalize(&dir);
    sead::Vector3f axis = dir.cross(al::getGravity(getHost()));
    al::rotateVectorDegree(&dir, dir, axis, mIsProvocation ? 15.0f : 70.0f);
    pOut->set(dir);
    al::normalize(pOut);
    sead::Vector3f intersection = sead::Vector3f::zero;
    sead::Vector3f up;
    al::calcUpDir(&up, getHost());
    sead::Vector3f origin = mouth + (mIsProvocation ? mDirection * 150.0f : sead::Vector3f::zero);
    calcGroundIntersection(&intersection, origin, *pOut, up, al::getTrans(getHost()));
    sead::Vector3f front;
    al::calcFrontDir(&front, getHost());
    sead::Vector3f linePos = al::getTrans(getHost());
    sead::Line<sead::Vector3f> line(linePos, front);
    return sead::Mathf::sqrt(sead::Geometry::calcSquaredDistancePointToLine(intersection, line, nullptr)) > 400.0f;
}
/** @brief Finds an inactive fireball, falling back to the first. @return Fireball to reuse, or null for an empty pool. */
KoopaFireBall* KoopaChaseStateFire::getFireBall() const {
    for (auto it = mFireBalls.begin(); it != mFireBalls.end(); ++it)
        if (al::isDead(&*it)) return &*it;
    return mFireBalls.front();
}
/** @brief Advances to the first shot. */
void KoopaChaseStateFire::exeWait() {
    if (al::isGreaterEqualStep(this, 0)) al::setNerve(this, &NrvKoopaChaseStateFireFireStart);
}
/** @brief Aims during the windup, then launches the fireball. */
void KoopaChaseStateFire::exeFireStart() {
    if (al::isFirstStep(this)) {
        al::startAction(KoopaChaseFunction::getKoopa(getHost()), "FireStart");
        auto* target = mIsProvocation ? KoopaChaseFunction::getLookAtTargetPlayer(getHost()) : KoopaChaseFunction::getTargetPlayer(getHost());
        if (!target) { kill(); return; }
        mKillOnCollide = calcFireDir(&mDirection);
    }
    if (al::isActionEnd(KoopaChaseFunction::getKoopa(getHost()))) {
        sead::Vector3f mouth;
        calcMouthPos(&mouth);
        sead::Vector3f origin = mouth + (mIsProvocation ? mDirection * 150.0f : sead::Vector3f::zero);
        sead::Vector3f target = mouth + mDirection * 500.0f;
        KoopaFireBall* fire = getFireBall();
        fire->setKillOnCollide(mKillOnCollide);
        fire->appearAttack(origin, target, 12.0f);
        al::startAction(KoopaChaseFunction::getKoopa(getHost()), "FireEnd");
        al::setNerve(this, &NrvKoopaChaseStateFireFireEnd);
    }
}
/** @brief Starts another shot or finishes the attack. */
void KoopaChaseStateFire::exeFireEnd() {
    if (al::isActionEnd(KoopaChaseFunction::getKoopa(getHost()))) {
        ++mFireCount;
        if (mFireNum > mFireCount) {
            calcFireDir(&mDirection);
            al::setNerve(this, &NrvKoopaChaseStateFireFireStart);
        } else kill();
    }
}
