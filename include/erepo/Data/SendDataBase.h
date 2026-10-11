#pragma once

#include <basis/seadTypes.h>
#include <container/seadBuffer.h>
#include <container/seadRingBuffer.h>
#include <nn/account.h>
#include <prim/seadEnum.h>
#include <prim/seadSafeString.h>

#include <erepo/Data/AtomicBitFlag.h>
#include <erepo/Types.h>

namespace sead {
class Heap;
}

namespace erepo {

class Array;
class Struct;

class SendDataBase {
public:
    SEAD_ENUM(ESendResult, cNone, cSuccess, cFailure, cBusy, cNotInitialized)
    SEAD_ENUM(EFlag, cInitialized, cRequestSave, cSaving, cSaved)

    struct CreateArg {
        EventIdString eventId;
        s32 dataNum;
        s32 arrayNum;
        s32 structNum;
        StringId reporterId;
        bool isNothrow;
    };

    struct LogData {
        char text[0xac];
        s32 type;
    };

    SendDataBase(sead::Heap* pHeap, const EventIdString& rEventId,
                 const StringId& rReporterId, bool isNothrow);
    virtual ~SendDataBase() = default;

    virtual bool addData(const KeyString& rKey, bool value) = 0;
    virtual bool addData(const KeyString& rKey, u64 value) = 0;
    virtual bool addData(const KeyString& rKey, s64 value) = 0;
    virtual bool addData(const KeyString& rKey, u32 value) = 0;
    virtual bool addData(const KeyString& rKey, s32 value) = 0;
    virtual bool addData(const KeyString& rKey, f32 value) = 0;
    virtual bool addData(const KeyString& rKey, const char* value) = 0;
    virtual bool addData(const KeyString& rKey, const sead::SafeString& rValue) = 0;
    virtual bool addData(const KeyString& rKey, const void* pValue, size_t size) = 0;
    virtual bool addData(const KeyString& rKey, const Array& rValue) = 0;
    virtual bool addData(const KeyString& rKey, const Struct& rValue) = 0;
    virtual bool clear() = 0;

    void addLog(const KeyString& rKey, bool value);
    void addLog(const sead::SafeString& rKey, bool value);
    void addLog(const KeyString& rKey, u64 value);
    void addLog(const sead::SafeString& rKey, u64 value);
    void addLog(const KeyString& rKey, s64 value);
    void addLog(const sead::SafeString& rKey, s64 value);
    void addLog(const KeyString& rKey, u32 value);
    void addLog(const sead::SafeString& rKey, u32 value);
    void addLog(const KeyString& rKey, s32 value);
    void addLog(const sead::SafeString& rKey, s32 value);
    void addLog(const KeyString& rKey, f32 value);
    void addLog(const sead::SafeString& rKey, f32 value);
    void addLog(const KeyString& rKey, const char* value);
    void addLog(const sead::SafeString& rKey, const char* value);
    void addLog(const KeyString& rKey, const sead::SafeString& rValue);
    void addLog(const sead::SafeString& rKey, const sead::SafeString& rValue);
    void addLog(const KeyString& rKey, const void* pValue, size_t size);
    void addLog(const sead::SafeString& rKey, const void* pValue, size_t size);
    void addLog(const KeyString& rKey, const Array& rValue);
    void addLog(const sead::SafeString& rKey, const Array& rValue);
    void addLog(const KeyString& rKey, const Struct& rValue);
    void addLog(const sead::SafeString& rKey, const Struct& rValue);

    template <typename T>
    Array* CreateArray(s32 num);
    template <typename T>
    Array* CreateArray(s32 num, s32 length);
    Struct* CreateStruct(s32 memberNum);
    Struct* CreateStructWithBufferSize(s32 bufferSize);
    Array* CreateStructArray(s32 num, s32 memberNum);
    Array* CreateStructArrayWithBufferSize(s32 num, s32 bufferSize);

    bool save(ESendResult* pResult);
    bool requestSave();
    void dump();

    bool isInitialized() const { return mFlags.isOn(EFlag::cInitialized); }
    const StringId& getReporterId() const { return mReporterId; }
    bool isSaved() const { return mFlags.isOn(EFlag::cSaved); }

protected:
    virtual bool isValid_() const { return false; }
    virtual bool initializeInnerData_(sead::Heap* pHeap) { return true; }
    virtual void finalizeInnerData_() {}
    virtual bool trySetInnerDataEventId_(const EventIdString& rEventId)
    {
        return true;
    }

    virtual ESendResult saveInnerData_() { return ESendResult::cSuccess; }

    bool addSendDataNum_(s32 num);
    ESendResult saveImpl_();
    void initialize_(s32 dataNum, s32 arrayNum, s32 structNum, sead::Heap* pHeap);
    void finalize_();
    void setEventId_(const EventIdString& rEventId);
    ESendResult checkCanSave_() const;
    void clearLog_();

    template <typename T>
    static T** findEmptySlot_(sead::Buffer<T*>& rBuffer)
    {
        for (s32 i = 0; i < rBuffer.size(); i++) {
            if (!rBuffer(i)) {
                return &rBuffer(i);
            }
        }

        return nullptr;
    }

    bool mIsNothrow;
    s32 mDataNum;
    sead::FixedRingBuffer<LogData, 48> mLogs;
    nn::account::Uid mUid;
    sead::Buffer<Array*> mArrays;
    sead::Buffer<Struct*> mStructs;
    AtomicBitFlag<EFlag> mFlags;
    StringId mReporterId;
    s32 mSendDataNum;
};

}  // namespace erepo
