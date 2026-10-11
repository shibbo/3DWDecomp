#pragma once

#include <container/seadPtrArray.h>
#include <math/seadVector.h>

namespace al {
struct PlacementInfo;
}

class IUseRabbitRoutePoint;
class RabbitRoutePoint;

/**
 * @brief The network of route points a rabbit runs along.
 * @note Only what reconstructed code needs is declared so far.
 */
class RabbitRoute {
public:
    RabbitRoute(const al::PlacementInfo& rInfo);

    void calcNearestPosOnRoute(sead::Vector3f* pPos, const sead::Vector3f& rTarget) const;
    void calcFarthestPosOnRoute(sead::Vector3f* pPos, const sead::Vector3f& rTarget) const;

    /** @return The first point of the route. */
    const IUseRabbitRoutePoint* getStartPoint() const { return mStartPoint; }

private:
    sead::PtrArray<RabbitRoutePoint> mPoints;
    const IUseRabbitRoutePoint* mStartPoint;
};

static_assert(sizeof(RabbitRoute) == 0x18);
