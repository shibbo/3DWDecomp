#pragma once

#include <math/seadVector.h>

/**
 * @brief Interface for a point a rabbit can run along (a route point or its initial place point).
 * @note Only what reconstructed code needs is declared so far.
 */
class IUseRabbitRoutePoint {
public:
    virtual bool isRoutePoint() const;
    virtual bool isActionJump() const = 0;
    virtual const sead::Vector3f& getPos() const = 0;
    virtual s32 getNextPointNum() const = 0;
    virtual const IUseRabbitRoutePoint* getNextPoint(s32 index) const = 0;
    virtual s32 getNextUniquePointNum() const = 0;
    virtual const IUseRabbitRoutePoint* getNextUniquePoint(s32 index) const = 0;
    virtual bool isPointOnLand() const;
};
