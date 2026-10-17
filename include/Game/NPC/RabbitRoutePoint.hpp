#pragma once

#include "NPC/IUseRabbitRoutePoint.hpp"
#include <container/seadPtrArray.h>

namespace al { struct PlacementInfo; }

class RabbitRoutePoint : public IUseRabbitRoutePoint {
public:
    RabbitRoutePoint(const al::PlacementInfo& rInfo, RabbitRoutePoint* pPreviousPoint,
                     sead::PtrArray<RabbitRoutePoint>* pPoints);
    void addNextPoint(RabbitRoutePoint* pPoint);
    bool isActionJump() const override;
    const sead::Vector3f& getPos() const override;
    int getNextPointNum() const override;
    const IUseRabbitRoutePoint* getNextPoint(int index) const override;
    int getNextUniquePointNum() const override;
    const IUseRabbitRoutePoint* getNextUniquePoint(int index) const override;
    bool isPointOnLand() const override;

private:
    sead::Vector3f mPosition;
    int mAction;
    sead::FixedPtrArray<RabbitRoutePoint, 16> mNextPoints;
    sead::FixedPtrArray<RabbitRoutePoint, 8> mNextUniquePoints;
    bool mIsOnLand;
};
