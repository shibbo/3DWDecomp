#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include <math/seadQuat.h>
namespace al { class MtxConnector; class UniformBlock; }
class TestMusaCoinChameleon : public al::LiveActor {
public:
    TestMusaCoinChameleon(const char* name);
    ~TestMusaCoinChameleon() override;
    void init(const al::ActorInitInfo& info) override;
    void initAfterPlacement() override;
    void control() override;
    void draw() const override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    void exeWait();
private:
    al::MtxConnector* mConnector = nullptr;
    sead::Quatf mBaseQuat = sead::Quatf::unit;
    al::UniformBlock* mUniformBlock;
    float mIndirectScale = 0.5f;
    float mIndirectOffset = 0.0f;
    float mAlpha = 0.0f;
};
static_assert(sizeof(TestMusaCoinChameleon) == 0x178);
