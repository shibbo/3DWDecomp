#pragma once
#include "Library/LiveActor/LiveActor.hpp"
namespace al { class BreakModel; class UniformBlock; }
class TestMusaBlockChameleon : public al::LiveActor {
public:
    TestMusaBlockChameleon(const char* name);
    ~TestMusaBlockChameleon() override;
    void init(const al::ActorInitInfo& info) override;
    void control() override;
    void draw() const override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    void exeWait();
    void exeBreak();
    void exeMove();
    void exeStop();
private:
    al::BreakModel* mBreakModel = nullptr;
    al::UniformBlock* mUniformBlock;
    float mIndirectScale = 0.1f;
    float mIndirectOffset = 0.0f;
    float mAlpha = 0.0f;
};
static_assert(sizeof(TestMusaBlockChameleon) == 0x168);
