#pragma once
#include "Project/Block/BlockRailParts.hpp"
#include <math/seadQuat.h>
namespace al { class BlockRailRider; }
class TestKitaharaRouteBazooka;
class TestKitaharaRouteBazookaEntrance : public al::BlockRailParts {
public:
    TestKitaharaRouteBazookaEntrance(const char*, const char*);
    ~TestKitaharaRouteBazookaEntrance() override;
    void init(const al::ActorInitInfo&) override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    void setInitQT(const sead::Quatf&, const sead::Vector3f&);
    void setHost(TestKitaharaRouteBazooka*);
    void startRouteDokanRider(al::BlockRailRider*);
    void exeWait();
    void exeOut();
private:
    const char* mModelName;
    TestKitaharaRouteBazooka* mHost = nullptr;
    sead::Quatf mInitialRotation = sead::Quatf::unit;
    sead::Vector3f mInitialPosition = sead::Vector3f::zero;
};
static_assert(sizeof(TestKitaharaRouteBazookaEntrance) == 0x1b0);
