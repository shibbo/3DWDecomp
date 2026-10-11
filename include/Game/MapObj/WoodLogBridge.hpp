#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class WoodLogBridgeParts;
class WoodLogBridge : public al::LiveActor {
public:
    explicit WoodLogBridge(const char*);
    ~WoodLogBridge() override;
    void init(const al::ActorInitInfo&) override;
    void control() override;
private:
    WoodLogBridgeParts** mParts = nullptr;
    float* mHeights;
    int mCount = 20;
};
class WoodLogBridgeParts : public al::LiveActor {
public:
    __attribute__((noinline)) explicit WoodLogBridgeParts(const char*);
    ~WoodLogBridgeParts() override;
    void init(const al::ActorInitInfo&) override;
    void control() override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    __attribute__((noinline)) void setInitPos(const sead::Vector3f&);
    bool isPlayerRide() const;
    void setMaxSink(float sink) { mMaxSink = sink; }
    void exeWait();
    void exeRide();
    void exeHipDrop();
    sead::Vector3f mInitPos = sead::Vector3f::zero;
    float mSink = 0.0f;
    float mMaxSink = 0.0f;
    bool mIsFixed = false;
};
static_assert(sizeof(WoodLogBridge) == 0x160);
static_assert(sizeof(WoodLogBridgeParts) == 0x160);
