#pragma once
#include "Library/LiveActor/LiveActor.hpp"
namespace al { class CollisionPartsFilterBase; }

class Imozo : public al::LiveActor {
public:
    bool isHitBody(al::HitSensor* pOther, al::HitSensor* pSelf) const;
    bool isRideRail() const;
    bool isTerminateRail() const;
    void addRailAccel(f32 accel);
    void resetRailAccel();
    void moveOnRail();
    void rotateBody();
    void rotateBodyAir(f32 rate);
    void reverse();
    void getCollisionDir(sead::Vector3f* pDir);
    /** @brief Gets the filter used to detect obstacles along the rail. @return Collision filter. */
    const al::CollisionPartsFilterBase* getCollisionFilter() const { return mCollisionFilter; }
private:
    al::CollisionPartsFilterBase* mCollisionFilter;
};
