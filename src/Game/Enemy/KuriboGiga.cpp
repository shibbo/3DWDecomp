#include "Enemy/KuriboGiga.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Collision/Collider.hpp"
#include "Library/HitSensor/HitSensorKeeper.hpp"
#include "Library/Math/MathUtil.hpp"
#include <cstdlib>

/** @brief Creates an unsplit Goomba. @param pName Actor name. */
KuriboGiga::KuriboGiga(const char* pName) : Kuribo(pName) {}

/** @brief Scales the root Goomba and creates its descendants. @param rInfo Placement information. */
void KuriboGiga::init(const al::ActorInitInfo& rInfo) {
    Kuribo::init(rInfo);
    s32 iterations;
    al::tryGetArg(&iterations, rInfo, "GigaIterations");
    s32 size = 1 << std::abs(iterations);
    f32 scale;
    if (size < 1)
        scale = 1.0f;
    else
        scale = size < 16 ? size : 16;
    al::setScaleAll(this, scale);
    for (s32 i = 0; i < mHitSensorKeeper->getSensorNum(); ++i) {
        al::setSensorRadius(this, i, scale * al::getSensorRadius(al::getHitSensor(this, i)));
        al::setSensorFollowPosOffset(this, i, scale * al::getSensorFollowPosOffset(this, i));
    }
    mCollider->mRadius = scale * mCollider->mRadius;
    f32 offsetY = scale * mCollider->getOffsetY();
    mCollider->setOffsetY(offsetY);
    auto* pFirst = new KuriboGiga("KuriboGiga");
    pFirst->initSpecial(rInfo, scale * 0.5f);
    pFirst->makeActorDead();
    mFirstChild = pFirst;
    auto* pSecond = new KuriboGiga("KuriboGiga");
    pSecond->initSpecial(rInfo, scale * 0.5f);
    pSecond->makeActorDead();
    mSecondChild = pSecond;
}

/** @brief Initializes one descendant and recursively creates smaller children. @param rInfo Placement information. @param scale Relative size. */
void KuriboGiga::initSpecial(const al::ActorInitInfo& rInfo, f32 scale) {
    Kuribo::init(rInfo);
    al::setScaleAll(this, scale);
    for (s32 i = 0; i < mHitSensorKeeper->getSensorNum(); ++i) {
        al::setSensorRadius(this, i, al::getSensorRadius(al::getHitSensor(this, i)) * scale);
        al::setSensorFollowPosOffset(this, i, al::getSensorFollowPosOffset(this, i) * scale);
    }
    mCollider->mRadius *= scale;
    f32 offsetY = mCollider->getOffsetY() * scale;
    mCollider->setOffsetY(offsetY);
    if (scale < 1.0f)
        return;
    auto* pFirst = new KuriboGiga("KuriboGiga");
    pFirst->initSpecial(rInfo, scale * 0.5f);
    pFirst->makeActorDead();
    mFirstChild = pFirst;
    auto* pSecond = new KuriboGiga("KuriboGiga");
    pSecond->initSpecial(rInfo, scale * 0.5f);
    pSecond->makeActorDead();
    mSecondChild = pSecond;
}

/** @brief Releases two children when the parent is defeated. @param pMsg Attack message. @param pOther Sender. @param pSelf Own sensor. @return Base Goomba's response. */
bool KuriboGiga::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) {
    bool result = Kuribo::receiveMsg(pMsg, pOther, pSelf);
    if ((al::isDead(this) || isDown()) && mFirstChild) {
        f32 radius = mCollider->getRadius() * 0.5f;
        sead::Vector3f up = (radius * sead::Vector3f::ey) * 0.5f;
        sead::Vector3f direction = sead::Vector3f::ez;
        al::rotateVectorDegreeY(&direction, al::getRandomDegree());
        al::faceToTarget(mFirstChild, al::getSensorPos(pOther));
        al::resetPosition(mFirstChild, up + (al::getTrans(this) + radius * direction), false);
        mFirstChild->makeActorAppeared();
        al::faceToTarget(mSecondChild, al::getSensorPos(pOther));
        al::resetPosition(mSecondChild, up + (al::getTrans(this) - radius * direction), false);
        mSecondChild->makeActorAppeared();
        if (!al::isDead(this))
            kill();
    }
    return result;
}
