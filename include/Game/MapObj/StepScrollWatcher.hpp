#pragma once
#include "Library/LiveActor/LiveActor.hpp"
namespace al { class FixMapParts; }
class StepScrollParts;
class StepScrollWatcher : public al::LiveActor {
public:
    explicit StepScrollWatcher(const char*);
    ~StepScrollWatcher() override;
    void init(const al::ActorInitInfo&) override;
    void control() override;
    void kill() override;
    void exeWait();
    void exeAccel();
private:
    int mOneTimeCount = 0;
    StepScrollParts** mOneTimeParts = nullptr;
    int mLoopCount = 0;
    StepScrollParts** mLoopParts = nullptr;
    int mWheelCount = 0;
    al::FixMapParts** mWheels = nullptr;
    sead::Vector3f mDirection = {0.0f, 0.0f, 0.0f};
    float mSpeed = 0.0f;
    float mInitialSpeed = 0.0f;
    float mMaxSpeed = 0.0f;
    int mAccelTiming = 0;
    sead::Vector3f mBoundary = {0.0f, 0.0f, 0.0f};
    sead::Vector3f mUnknown198 = {0.0f, 0.0f, 0.0f};
    int mLoopIndex = 0;
    float mLoopDistance;
    float mLoopStepWidth = 0.0f;
    float* mWheelCircumferences = nullptr;
    float* mWheelFrames = nullptr;
    bool mUseCameraShake = false;
    int mShakeStep = 0;
};
static_assert(sizeof(StepScrollWatcher) == 0x1c8);
