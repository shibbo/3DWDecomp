#pragma once

#include <math/seadVector.h>

class IUseRabbitRoutePoint {
public:
    /** @brief Identifies an ordinary route point. @return True. */
    virtual bool isRoutePoint() const { return true; }
    virtual bool isActionJump() const = 0;
    virtual const sead::Vector3f& getPos() const = 0;
    virtual int getNextPointNum() const = 0;
    virtual const IUseRabbitRoutePoint* getNextPoint(int index) const = 0;
    virtual int getNextUniquePointNum() const = 0;
    virtual const IUseRabbitRoutePoint* getNextUniquePoint(int index) const = 0;

    /** @brief Supplies the default terrain classification. @return False. */
    virtual bool isPointOnLand() const { return false; }
};
