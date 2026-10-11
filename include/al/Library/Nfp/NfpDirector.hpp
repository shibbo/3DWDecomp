#pragma once

#include <basis/seadTypes.h>
#include <container/seadBuffer.h>
#include <container/seadPtrArray.h>
#include <nn/nfp/nfp_types.h>
#include <nn/os.h>
#include <nn/types.h>
#include <prim/seadEnum.h>
#include <thread/seadCriticalSection.h>

namespace sead {
class DelegateThread;
class Thread;
}  // namespace sead

namespace al {
class NfpControllerThread;
struct NfpInfo;

SEAD_ENUM(NfpApplicationCommand, Initialize, ListDevices, Start, DelayStart, Stop, Finalize, None)

enum NfpDirectorState : s32 {
    NfpDirectorState_None = 0,
    NfpDirectorState_Initializing = 1,
    NfpDirectorState_Initialized = 2,
    NfpDirectorState_ListingDevices = 3,
    NfpDirectorState_Started = 4,
    NfpDirectorState_Stopped = 5,
};

struct NfpDeviceInfo {
    nn::nfp::DeviceHandle handle;
    u32 npadId;
};

struct NfpErrorInfo {
    bool isError = false;
    nn::Result result;
};

class NfpDirector {
public:
    using CommandBuffer = sead::Buffer<NfpApplicationCommand>;
    using DeviceInfoBuffer = sead::Buffer<NfpDeviceInfo>;
    using ControllerThreadArray = sead::PtrArray<NfpControllerThread>;
    using NfpInfoArray = sead::PtrArray<NfpInfo>;

    static constexpr s32 cCommandBufferSize = 5;
    static constexpr s32 cDeviceNumMax = 9;

    NfpDirector(s32 maxDeviceNum);
    virtual ~NfpDirector();

    void executeCommand(sead::Thread* pThread, s64 message);
    void initialize();
    void enqueueCommand(NfpApplicationCommand command);
    void finalize();
    virtual void update();
    void stop(bool isClearRestart);
    void resetError();
    void listDevices();
    void start();
    void clearCommand();
    bool hadError();
    void setRequestNpadId(s32 npadId);
    virtual void showError(const nn::Result& rResult);
    void executeCommandInitialize();
    void executeCommandFinalize();
    void executeCommandListDevices();
    void executeCommandStart();
    void executeCommandStop();
    void executeCommandDelayStart();
    void startDelayed(s32 delayCount);

    /**
     * @brief Get the npad id the next scan is requested for.
     * @return The requested npad id.
     */
    s32 getRequestNpadId() const { return mRequestNpadId; }

    /**
     * @brief Find the device bound to an npad.
     * @param npadId Npad id of the device.
     * @return Index of the device, or -1 if no device is bound to the npad.
     */
    s32 findDeviceIndex(u32 npadId) const {
        for (s32 i = 0; i < mDeviceInfos.size(); i++) {
            if (mDeviceInfos(i).npadId == npadId) {
                return i;
            }
        }

        return -1;
    }

    /**
     * @brief Get the scan result of a device.
     * @param index Index of the device.
     * @return The scan result, or nullptr for an invalid index.
     */
    NfpInfo* getNfpInfo(s32 index) const { return mNfpInfos[index]; }

private:
    void setState(NfpDirectorState state) {
        mCriticalSection.lock();
        mState = state;
        mCriticalSection.unlock();
    }

    s32 mState = NfpDirectorState_None;
    CommandBuffer mCommands;
    CommandBuffer mExecCommands;
    s32 mCommandNum = 0;
    bool mIsRetryListDevices = true;
    bool mIsRestart = false;
    sead::DelegateThread* mThread = nullptr;
    sead::CriticalSection mCriticalSection;
    DeviceInfoBuffer mDeviceInfos;
    s32 mDeviceNum = 0;
    s32 mMaxDeviceNum;
    s32 mRequestNpadId = -1;
    ControllerThreadArray mControllerThreads;
    sead::Buffer<bool> mIsActiveDevice;
    NfpInfoArray mNfpInfos;
    s32 mActiveDeviceNum = 0;
    NfpErrorInfo mErrorInfo;
    NfpErrorInfo mUpdateErrorInfo;
    nn::os::SystemEventType mAvailabilityChangeEvent;
    s32 mDelayStartCount = -1;
    bool mHadError = false;
};

static_assert(sizeof(NfpDirector) == 0x120);
}  // namespace al
