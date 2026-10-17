#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include <container/seadPtrArray.h>
class TestKinokoKuribo;
class BindPuppeteer;
class TestKinokoHead : public al::LiveActor {
public:
    explicit TestKinokoHead(const char*);
    ~TestKinokoHead() override;
    void init(const al::ActorInitInfo&) override;
    void makeActorAppeared() override;
    void makeActorDead() override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    void exeItem();
    void exeWornAnim();
    void exeWorn();
private:
    sead::PtrArray<TestKinokoKuribo> mKuribos;
    const sead::Matrix34f* mHeadMtx = nullptr;
    const sead::Matrix34f* mBodyMtx = nullptr;
    al::HitSensor* mWearer = nullptr;
    BindPuppeteer* mPuppeteer;
};
static_assert(sizeof(TestKinokoHead) == 0x178);
