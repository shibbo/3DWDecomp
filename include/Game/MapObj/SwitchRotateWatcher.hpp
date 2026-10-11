#pragma once

#include "Library/LiveActor/LiveActor.hpp"
#include <math/seadMatrix.h>
#include <math/seadQuat.h>

class SwitchRotateParts;
class SwitchRotateSwitch;

class SwitchRotateWatcher : public al::LiveActor {
public:
    SwitchRotateWatcher(const char* pName);
    virtual ~SwitchRotateWatcher();
    virtual void init(const al::ActorInitInfo& rInfo);

    void calcClippingRange();
    void exeWait();
    void exeRotate();

    SwitchRotateParts** mParts = nullptr; // 0x148
    int mPartsCount; // 0x150
    SwitchRotateSwitch** mSwitches = nullptr; // 0x158
    int mSwitchCount = -1; // 0x160
    float mClockAngle = 0.0f; // 0x164
    int mRotateAxis = 1; // 0x168
    int mRotateTime = 60; // 0x16c
    sead::Quatf mInitialQuat = sead::Quatf::unit; // 0x170
    sead::Matrix34f mMtx = sead::Matrix34f::ident; // 0x180
    int mRotateCount = 0; // 0x1b0
};
