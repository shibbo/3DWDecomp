#pragma once
namespace al { class HitSensor; class SensorMsg; }
class RouteDokanEntrance;
class IUseRouteDokan {
public:
    virtual bool isBindStart(al::HitSensor*) = 0;
    virtual bool startBind(RouteDokanEntrance*, const al::SensorMsg*, al::HitSensor*, al::HitSensor*) = 0;
    virtual bool cancelBind(al::HitSensor*) = 0;
    virtual bool damagePuppet(al::HitSensor*) = 0;
    virtual bool isEnableActorRouteDokanMove() const { return true; }
};
