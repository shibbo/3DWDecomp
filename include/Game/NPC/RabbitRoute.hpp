#pragma once

#include <container/seadPtrArray.h>
#include "NPC/RabbitRoutePoint.hpp"
#include <math/seadVector.h>

namespace al { struct PlacementInfo; }
class RabbitRoutePoint;

class RabbitRoute {
public:
    /** @brief Gets the initial route point. @return the initial route point. */
    const IUseRabbitRoutePoint* getStartPoint() const { return mStartPoint; }
    explicit RabbitRoute(const al::PlacementInfo& rInfo);
    void calcNearestPosOnRoute(sead::Vector3f* pPosition, const sead::Vector3f& rPosition) const;
    void calcFarthestPosOnRoute(sead::Vector3f* pPosition, const sead::Vector3f& rPosition) const;

private:
    friend class RabbitRouteRider;
    sead::PtrArray<RabbitRoutePoint> mPoints;
    RabbitRoutePoint* mStartPoint;
};
