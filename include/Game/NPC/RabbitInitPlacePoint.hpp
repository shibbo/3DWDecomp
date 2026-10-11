#pragma once

#include "NPC/IUseRabbitRoutePoint.hpp"

namespace al {
struct PlacementInfo;
}

/**
 * @brief The point a rabbit waits at before it starts running along its route.
 * @note Only what reconstructed code needs is declared so far.
 */
class RabbitInitPlacePoint : public IUseRabbitRoutePoint {
public:
    RabbitInitPlacePoint(const al::PlacementInfo& rInfo, const IUseRabbitRoutePoint* pNextPoint);

    bool isRoutePoint() const override;
    bool isActionJump() const override;
    const sead::Vector3f& getPos() const override;
    s32 getNextPointNum() const override;
    const IUseRabbitRoutePoint* getNextPoint(s32 index) const override;
    s32 getNextUniquePointNum() const override;
    const IUseRabbitRoutePoint* getNextUniquePoint(s32 index) const override;

private:
    u8 _8[0x28 - 0x8];
};

static_assert(sizeof(RabbitInitPlacePoint) == 0x28);
