#pragma once

#include <container/seadPtrArray.h>
#include <container/seadSafeArray.h>
#include <heap/seadDisposer.h>
#include <mc/seadCoreInfo.h>
#include <nn/account.h>
#include <prim/seadSafeString.h>
#include <time/seadDateTime.h>
#include <time/seadTickTime.h>

#include <erepo/Data/AtomicBitFlag.h>
#include <erepo/Data/SaveDataInfo.h>
#include <erepo/Types.h>

namespace erepo {

class NetworkStatusObserver;
class Reporter;
class SendDataBase;
class SendThread;

class Manager {
    SEAD_SINGLETON_DISPOSER(Manager)

public:
    using ReporterArray = sead::PtrArray<Reporter>;

    SEAD_ENUM(EFlag, cInitialized, cStartupRequested, cStartupFinished, cUnused3,
              cWaitFinishSending, cWaitFinishSendingAsync, cSystemReportTag,
              cRequestImmediateTransmission, cEnableSaveLoad, cEnableLog)

    struct InitializeArg {
        sead::Heap* heap;
        size_t heapSize;
        sead::CoreIdMask threadAffinity;
        s32 threadPriority;
        s32 sendQueueSize;
        size_t threadStackSize;
        s32 threadMessageQueueSize;
        bool isSystemReportTagEnabled;
        nn::account::Uid uid;
    };

    struct StartupArg {
        u32 applicationIds[2];
        sead::SafeString name;
        sead::SafeString versions[2];
        bool isSaveLoadEnabled;
        bool isLogEnabled;
    };

    struct UpdateArg {
        UpdateArg() {}

        f32 deltaTime;
        s32 message;
    };

    Manager();

    bool initialize(const InitializeArg& rArg);
    bool startUp(const StartupArg& rArg);
    void finalize();
    void update(const UpdateArg& rArg);

    bool requestLoadData();
    void loadData(u32 value0, u32 value1, u32 value2, u32 value3, u32* pTimes0, u32* pTimes1,
                  u32* pTimes2, f32* pFloats);
    void setLoadFinish();
    bool requestSaveData();
    void saveData(u32* pValue0, u32* pValue1, u32* pValue2, u32* pValue3, u32* pTimes0,
                  u32* pTimes1, u32* pTimes2, f32* pFloats);
    void setSaveFinish();

    bool applySaveDataInfoFromGameData();
    bool applyGameDataFromSaveDataInfo();

    u64 getCurrentDateTime() const;
    const sead::DateTime& getLaunchTime() const;

    void setCurrentControllerStyle(EControllerStyle style);
    EControllerStyle getCurrentControllerStyle() const;

    bool requestWaitFinishSendingAsync(s32 timeoutMs);
    bool waitFinishSending(s32 timeoutMs);
    bool isSending() const;
    bool isSendThread() const;

    bool requestSend_(SendDataBase* pData);

    sead::Heap* getHeap() const { return mHeap; }
    bool isLoadRequested() const { return mSaveLoadState == cSaveLoadState_Load; }
    bool isSaveRequested() const { return mSaveLoadState == cSaveLoadState_Save; }
    void setUid(const nn::account::Uid& rUid) { mUid = rUid; }
    const nn::account::Uid& getUid() const { return mUid; }
    bool isFlagOn(EFlag flag) const { return mFlags.isOn(flag); }
    SaveDataInfo& getSaveDataInfo() { return mSaveDataInfo; }

private:
    struct ReporterSetting {
        struct Arg {
            u32 applicationId;
            sead::SafeString name = sead::SafeString::cEmptyString;
            sead::SafeString version = sead::SafeString::cEmptyString;
        };

        void set(const Arg&) {}
    };

    struct ReporterList {
        ReporterType type;
        ReporterArray reporters;
        ReporterSetting* setting = nullptr;
        AtomicBitFlag<ReporterType> requestFlag;
        StringId requestId;
    };

    enum SaveLoadState {
        cSaveLoadState_None,
        cSaveLoadState_Load,
        cSaveLoadState_Save,
        cSaveLoadState_Finish,
    };

    void updateBeginStartup_();
    bool waitFinishSendingAsync_(s32 timeoutMs);
    void updateFinishStartup_(const UpdateArg& rArg);
    void updateSaveLoad_();
    bool sendStartupReport_(ReporterType type);
    bool checkSendDailyReport_();
    void sendDailyReport_(ReporterType type);
    void updateLoadData_();
    void updateSaveData_();
    void resetSaveState_();

    sead::Heap* mHeap = nullptr;
    SendThread* mSendThread = nullptr;
    s32 mWaitTimeoutMs = -1;
    sead::TickTime mWaitStartTime;
    s32 mSaveLoadState = cSaveLoadState_None;
    ReporterType mSaveLoadType;
    NetworkStatusObserver* mNetworkStatusObserver = nullptr;
    SaveDataInfo mSaveDataInfo;
    EControllerStyle mControllerStyle;
    nn::account::Uid mUid = {};
    sead::SafeArray<ReporterList, 2> mReporterLists;
    AtomicBitFlag<EFlag> mFlags;
};

}  // namespace erepo
