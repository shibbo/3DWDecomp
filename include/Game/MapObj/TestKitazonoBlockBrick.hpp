#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class TestKitazonoBlockBrickBreak;
class TestKitazonoBlockBrick : public al::LiveActor {
public:
    TestKitazonoBlockBrick(const char* name);
    ~TestKitazonoBlockBrick() override;
    void init(const al::ActorInitInfo&) override;
    void control() override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    void exeWait();
    void exeBreak();
    void exeReaction();
    void exeMove();
    void exeStop();
private:
    void* _148 = nullptr;
    int mHitCount = 0;
    sead::Vector3f mInitialPos = sead::Vector3f(0.0f, 0.0f, 0.0f);
    TestKitazonoBlockBrickBreak* mBreakModel = nullptr;
};
static_assert(sizeof(TestKitazonoBlockBrick) == 0x168);
