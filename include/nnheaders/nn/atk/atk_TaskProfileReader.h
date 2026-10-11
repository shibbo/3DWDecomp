#pragma once
#include <nn/os.h>
#include <nn/util/util_IntrusiveList.h>
#include <atomic>

namespace nn::atk {
namespace detail {
class StreamSoundPlayer;
namespace fnd {
class FileStream;
}  // namespace fnd

/** @brief Decoder that turns blocks of a compressed (Opus) stream file into PCM samples. */
class IStreamDataDecoder {
public:
    /** @brief How a block is decoded. */
    enum DecodeType {
        DecodeType_Normal,
        DecodeType_Last,
        DecodeType_Idling,
    };

    /** @brief Stream parameters read from the file header. */
    struct DataInfo {
        int channelCount;
        int sampleRate;
        int blockSampleCount;
        size_t blockSize;
        int preSkipSampleCount;
    };

    struct CacheProfile {
        s64 cachePosition;
        size_t cachedLength;
        s64 currentPosition;
        StreamSoundPlayer* player;
    };
    struct DecodeProfile {
        os::Tick decodeTicks;
        int decodedSampleCount;
        os::Tick fsAccessTicks;
        size_t fsReadSize;
    };

    virtual ~IStreamDataDecoder() {}
    virtual bool ReadHeaderInfo(DataInfo* pInfo, fnd::FileStream* pFileStream) = 0;
    virtual bool Skip(fnd::FileStream* pFileStream) = 0;
    virtual bool Decode(void** ppBuffer, fnd::FileStream* pFileStream, int channelCount,
                        DecodeType type) = 0;
    virtual void Reset() = 0;
    virtual void ResetDecodeProfile() = 0;
    virtual DecodeProfile GetDecodeProfile() const = 0;
};
}  // namespace detail
class TaskProfile {
public:
    class LoadStreamBlock {
    public:
        TimeSpan GetTotalTime() const;
        os::Tick GetBeginTick() const;
        os::Tick GetEndTick() const;
        float GetRemainingCachePercentage() const;
        size_t GetCachedLength() const;
        detail::StreamSoundPlayer* GetStreamSoundPlayer() const;
        void SetData(const os::Tick& begin, const os::Tick& end,
                     const detail::IStreamDataDecoder::CacheProfile& cache);
    private:
        os::Tick mBegin, mEnd;
        s64 mCachePosition;
        size_t mCachedLength;
        s64 mCurrentPosition;
        detail::StreamSoundPlayer* mPlayer;
    };
    class LoadOpusStreamBlock {
    public:
        TimeSpan GetTotalTime() const;
        os::Tick GetBeginTick() const;
        os::Tick GetEndTick() const;
        float GetRemainingCachePercentage() const;
        size_t GetCachedLength() const;
        TimeSpan GetDecodeTime() const;
        int GetDecodedSampleCount() const;
        TimeSpan GetFsAccessTime() const;
        size_t GetFsReadSize() const;
        detail::StreamSoundPlayer* GetStreamSoundPlayer() const;
        void SetData(const os::Tick& begin, const os::Tick& end,
                     const detail::IStreamDataDecoder::DecodeProfile& decode,
                     const detail::IStreamDataDecoder::CacheProfile& cache);
    private:
        os::Tick mBegin, mEnd;
        s64 mCachePosition;
        size_t mCachedLength;
        s64 mCurrentPosition;
        os::Tick mDecodeTicks, mFsAccessTicks;
        size_t mFsReadSize;
        int mDecodedSampleCount;
        detail::StreamSoundPlayer* mPlayer;
    };

    /** @brief Kind of task a record describes. */
    enum TaskProfileType {
        TaskProfileType_LoadStreamBlock,
        TaskProfileType_LoadOpusStreamBlock,
    };

    /**
     * @brief Sets the kind of task the record describes.
     * @param type Record kind; selects the active union member.
     */
    void SetType(TaskProfileType type) { mType = type; }

    /** @brief Gets the stream block load record. @return Record data. */
    LoadStreamBlock& GetLoadStreamBlock() { return mStream; }

    /** @brief Gets the Opus stream block load record. @return Record data. */
    LoadOpusStreamBlock& GetLoadOpusStreamBlock() { return mOpus; }

private:
    TaskProfileType mType;
    union {
        LoadStreamBlock mStream;
        LoadOpusStreamBlock mOpus;
    };
};
class TaskProfileLogger;
template <typename T> class AtkProfileReader {
    friend class TaskProfileLogger;
private:
    util::IntrusiveListNode mNode;
    u8 mReserved10[8];
    T* mRecords;
    int mCapacity;
    int mWriteIndex;
    int mReadIndex;
    std::atomic<int> mCount;
};
class TaskProfileLogger {
public:
    TaskProfileLogger();
    void Record(const TaskProfile& profile);
    void RegisterReader(AtkProfileReader<TaskProfile>& reader);
    void UnregisterReader(const AtkProfileReader<TaskProfile>& reader);
    void SetProfilingEnabled(bool enabled);
    void Finalize();

    /** @brief Checks whether task records are collected. @return True when profiling. */
    bool IsProfilingEnabled() const { return mEnabled; }

private:
    using Reader = AtkProfileReader<TaskProfile>;
    using ReaderList = util::IntrusiveList<Reader, util::IntrusiveListMemberNodeTraits<Reader, &Reader::mNode>>;
    ReaderList mReaders;
    os::Mutex mMutex;
    bool mEnabled;
};
static_assert(sizeof(TaskProfile::LoadStreamBlock) == 0x30, "Stream task profile size");
static_assert(sizeof(TaskProfile::LoadOpusStreamBlock) == 0x50, "Opus task profile size");
static_assert(sizeof(TaskProfile) == 0x58, "Task profile record size");
static_assert(sizeof(AtkProfileReader<TaskProfile>) == 0x30, "Profile reader size");
static_assert(sizeof(TaskProfileLogger) == 0x38, "Task profile logger size");
}
