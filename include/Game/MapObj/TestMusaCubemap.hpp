#pragma once
#include "Library/LiveActor/LiveActor.hpp"
namespace al { class BreakModel; class UniformBlock; }
class TestMusaCubemap : public al::LiveActor {
public:
    TestMusaCubemap(const char* name);
    ~TestMusaCubemap() override;
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
static_assert(sizeof(TestMusaCubemap) == 0x168);
