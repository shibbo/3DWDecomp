#pragma once

#include <controller/nin/seadNinJoyNpadDevice.h>
#include <controller/seadController.h>

namespace al {
class NpadController : public sead::Controller {
    SEAD_RTTI_OVERRIDE(NpadController, sead::Controller)

public:
    NpadController(sead::ControllerMgr* pMgr);

    bool isConnected() const override;
    bool isValidNpadId() const;
    s32 getNpadId() const;
    void setAnyControllerMode();
    void setAnyControllerMode(bool isFlag);
    void setIndexControllerMode(s32 index);
    void setIndexControllerMode(s32 index, bool isFlag);
    virtual void setIndexControllerMode_(s32 index);
    const nn::hid::SixAxisSensorHandle& getSixAxisSensorHandle(s32 index) const;
    const nn::hid::VibrationDeviceHandle& getVibrationDeviceHandle(s32 index) const;
    bool isSixAxisSensorAtRest(s32 index) const;
    void resetInput();
    bool gatherInput();
    void applyInput();
    void setNpadId(s32 npadId);
    void setIsConnected(bool isConnected);
    void setSixAxisSensorNum(s32 num);
    void setSamplingNumber(s64 samplingNumber);
    void setStyle(sead::NinJoyNpadDevice::Style style);

    sead::NinJoyNpadDevice::Style getStyle() const { return mStyle; }
    s32 getSixAxisSensorNum() const { return mSixAxisSensorNum; }
    s32 getAccelerometerWaitCount() const { return mAccelerometerWaitCount; }
    bool isWaitingConnect() const { return mIsWaitingConnect; }
    s32 getControllerModeIndex() const { return mControllerModeIndex; }
    void setUnknown184(s32 value) { _184 = value; }

private:
    void calcImpl_() override;

    s32 mControllerModeIndex = -1;
    s32 mNpadId = -1;
    sead::NinJoyNpadDevice::Style mStyle = sead::NinJoyNpadDevice::cStyle_Invalid;
    s32 _184 = 1;
    s32 mSixAxisSensorNum = 0;
    s32 _18c = 0;
    bool _190 = false;
    s32 mAccelerometerWaitCount = 0;
    s64 mSamplingNumber = 0;
    sead::NinJoyNpadDevice::Style mPrevStyle = sead::NinJoyNpadDevice::cStyle_FullKey;
    s32 _1a4 = 0;
    s32 _1a8 = 0;
    s32 _1ac = 0;
    s32 _1b0 = 0;
    s32 _1b4 = 0;
    s32 _1b8 = 0;
    s32 _1bc = 0;
    s32 _1c0 = 0;
    s32 _1c4 = 0;
    bool mIsConnected = false;
    bool mIsWaitingConnect = false;
    bool _1ca = false;
    bool _1cb = false;
    s32 _1cc = 0;
};
}  // namespace al
