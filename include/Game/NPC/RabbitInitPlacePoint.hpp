#pragma once

#include "IUseRabbitRoutePoint.hpp"

namespace al { class PlacementInfo; }

class RabbitInitPlacePoint : public IUseRabbitRoutePoint {
public:
    RabbitInitPlacePoint(const al::PlacementInfo& rInfo, const IUseRabbitRoutePoint* pNextPoint);

    bool isRoutePoint() const override;
    bool isActionJump() const override;
    const sead::Vector3f& getPos() const override;
    int getNextPointNum() const override;
    const IUseRabbitRoutePoint* getNextPoint(int index) const override;
    int getNextUniquePointNum() const override;
    const IUseRabbitRoutePoint* getNextUniquePoint(int index) const override;

private:
    sead::Vector3f mPosition;
    const IUseRabbitRoutePoint* mNextPoint;
    int mAction;
};
