#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class ClimbHandleMapParts;
class ClimbHandle : public al::LiveActor {
public:
    explicit ClimbHandle(const char* name);
    ~ClimbHandle() override;
    void init(const al::ActorInitInfo&) override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    void exeStop();
    void exeTop();
    void exeTopFirst();
    void exeTopRotate();
    void exeUp();
    void exeSkid();
    void exeDown();
    void exeKeep();
    void exeStopAtTop();
    void exeSkidBeforeDown();
    void exeBound();
private:
    ClimbHandleMapParts* mMapParts = nullptr;
    sead::Vector3f mClippingCenter = {0.0f, 0.0f, 0.0f};
    sead::Vector3f mBottom = {0.0f, 0.0f, 0.0f};
    sead::Vector3f mTop = {0.0f, 0.0f, 0.0f};
    sead::Vector3f mMoveStart = {0.0f, 0.0f, 0.0f};
    sead::Vector3f mMoveEnd = {0.0f, 0.0f, 0.0f};
    float mFallSpeed = 0.0f;
    float mRotation = 0.0f;
    float mRotationSpeed = 0.0f;
    float mRotationAccel = 0.0f;
    int mWaitTime = 0;
    float mTopRotation = 0.0f;
    float mUnused = 0.0f;
    float mCancelDistance = -1.0f;
    bool mIsStop = false;
    bool mTimerHigh = false;
    bool mIsSingleMode = false;
};
static_assert(sizeof(ClimbHandle) == 0x1b0);
