#pragma once
#include "Project/Block/BlockRailParts.hpp"
#include <math/seadQuat.h>
class IUseRouteDokan;
namespace al { class BlockRailRider; }
class RouteDokanEntrance : public al::BlockRailParts {
public:
    RouteDokanEntrance(const char*, const char*, const char*);
    ~RouteDokanEntrance() override;
    void init(const al::ActorInitInfo&) override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    void setInitQT(const sead::Quatf&, const sead::Vector3f&);
    void setHost(IUseRouteDokan*);
    void startRouteDokanRider(al::BlockRailRider*);
    void exeWait();
    void exeOut();
private:
    const char* mModelName;
    const char* mEntranceModelSuffix;
    IUseRouteDokan* mHost = nullptr;
    sead::Quatf mInitialRotation = sead::Quatf::unit;
    sead::Vector3f mInitialPosition = sead::Vector3f::zero;
    bool mDisableOutAction = false;
};
static_assert(sizeof(RouteDokanEntrance) == 0x1b8);
