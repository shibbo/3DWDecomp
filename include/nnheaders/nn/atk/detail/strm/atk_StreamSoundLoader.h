#pragma once

#include <attributes.h>
#include <nn/atk/atk_DriverCommand.h>
#include <nn/atk/atk_Global.h>
#include <nn/atk/atk_InstancePool.h>
#include <nn/atk/atk_RegionManager.h>
#include <nn/atk/atk_StreamSoundFileLoader.h>
#include <nn/atk/atk_Task.h>
#include <nn/atk/atk_TaskProfileReader.h>
#include <nn/atk/atkfnd_FileStream.h>
#include <nn/util/util_IntrusiveList.h>

namespace nn::atk {
namespace detail {
class SoundArchiveFilesHook;

/** @brief Container format of a stream sound file. */
enum StreamFileType {
    StreamFileType_Bfstm,
    StreamFileType_Opus,
};

/** @brief Where compressed stream data is decoded. */
enum DecodeMode {
    DecodeMode_Invalid = -1,
    DecodeMode_Default,
    DecodeMode_Cpu,
    DecodeMode_Accelerator,
};

/** @brief Hands out decoders for one stream file type and decode mode. */
class IStreamDataDecoderManager {
public:
    virtual ~IStreamDataDecoderManager() {}
    virtual IStreamDataDecoder* AllocDecoder() = 0;
    virtual void FreeDecoder(IStreamDataDecoder* pDecoder) = 0;
    virtual StreamFileType GetStreamFileType() = 0;
    virtual DecodeMode GetDecodeMode() = 0;

    util::IntrusiveListNode mNode;
};

/** @brief Mixing parameters of one stream track. */
struct StreamTrackDataInfo {
    static const int ChannelCountMax = 2;

    u8 volume;
    u8 pan;
    u8 span;
    u8 flags;
    u8 mainSend;
    u8 fxSend[3];
    u8 lpfFreq;
    u8 biquadType;
    u8 biquadValue;
    u8 channelCount;
    u8 globalChannelIndex[ChannelCountMax];
};
static_assert(sizeof(StreamTrackDataInfo) == 0xe, "StreamTrackDataInfo size");

/** @brief Stream parameters the loader reports to the stream sound player. */
struct StreamDataInfoDetail {
    static const int TrackCountMax = 8;

    void SetStreamSoundInfo(const StreamSoundFile::StreamSoundInfo& rInfo,
                            bool isCrc32CheckAvailable);

    SampleFormat sampleFormat;
    int sampleRate;
    bool isLoop;
    size_t loopStart;
    size_t sampleCount;
    size_t originalLoopStart;
    size_t originalLoopEnd;
    bool isCrc32CheckAvailable;
    bool isRegionIndexCheckAvailable;
    u32 crc32;
    size_t blockSampleCount;
    size_t blockSize;
    size_t lastBlockSize;
    size_t lastBlockSampleCount;
    size_t preSkipSampleCount;
    int channelCount;
    u8 _64[4];
    StreamTrackDataInfo trackInfo[TrackCountMax];
    int regionCount;
};
static_assert(sizeof(StreamDataInfoDetail) == 0xe0, "StreamDataInfoDetail size");

namespace driver {
/** @brief Reads stream sound files on the task thread and feeds their blocks to a player. */
class StreamSoundLoader {
public:
    static const int ChannelCountMax = 16;
    static const int FilePathMax = 639;
    static const size_t LoadBufferSize = 0x5200;
    static const size_t FileStreamBufferSize = 0x200;
    static const size_t DataLoadTaskPoolBufferSize = 0x1f00;
    static const size_t MinimumLoadSampleCount = 1152;
    static const size_t AdpcmSamplesPerFrame = 14;

    /** @brief Byte and sample extents of the block being loaded. */
    struct BlockInfo {
        size_t size;
        s64 samples;
        size_t startOffsetSamples;
        size_t startOffsetSamplesAlign;
        size_t startOffsetByte;
        size_t copyByte;
    };

    /** @brief Decoder parameters of one DSP ADPCM channel. */
    struct AdpcmInfo {
        ALIGNED(64) audio::AdpcmParameter param;
        ALIGNED(64) AdpcmContext beginContext;
        ALIGNED(64) AdpcmContext loopContext;
    };

    /** @brief Task that opens the file and reads its header. */
    class StreamHeaderLoadTask : public Task {
    public:
        ~StreamHeaderLoadTask() override;
        void Execute(TaskProfileLogger& rLogger) override;

        StreamSoundLoader* mLoader;
    };

    /** @brief Task that closes the file. */
    class StreamCloseTask : public Task {
    public:
        ~StreamCloseTask() override;
        void Execute(TaskProfileLogger& rLogger) override;

        StreamSoundLoader* mLoader;
    };

    /** @brief Task that loads one buffer block of every channel. */
    class StreamDataLoadTask : public Task {
    public:
        ~StreamDataLoadTask() override;
        void Execute(TaskProfileLogger& rLogger) override;

        void* mBuffer[ChannelCountMax];
        u32 mBufferBlockIndex;
        size_t mStartOffsetSamples;
        size_t mPrefetchOffsetSamples;
        StreamSoundLoader* mLoader;
        util::IntrusiveListNode mLink;
    };
    static_assert(sizeof(StreamDataLoadTask) == 0xf8, "StreamDataLoadTask size");

    using DataLoadTaskList =
        util::IntrusiveList<StreamDataLoadTask,
                            util::IntrusiveListMemberNodeTraits<StreamDataLoadTask,
                                                                &StreamDataLoadTask::mLink>>;
    using DataLoadTaskPool = InstancePool<StreamDataLoadTask>;

    StreamSoundLoader();
    ~StreamSoundLoader();

    void WaitFinalize();
    void Initialize();
    void Finalize();
    void CancelRequest();
    void RequestClose();
    static void RegisterStreamDataDecoderManager(IStreamDataDecoderManager* pManager);
    static void UnregisterStreamDataDecoderManager(IStreamDataDecoderManager* pManager);
    fnd::FileStream* detail_SetFsAccessLog(fnd::FsAccessLog* pFsAccessLog);
    size_t detail_GetCurrentPosition();
    size_t detail_GetCachePosition();
    size_t detail_GetCachedLength();
    void RequestLoadHeader();
    void RequestLoadData(void** ppBuffer, u32 bufferBlockIndex, long startOffsetSamples,
                         long prefetchOffsetSamples, int priority);
    void Update();
    void ForceFinish();
    bool IsBusy() const;
    bool IsInUse();
    fnd::FndResult Open();
    void Close();
    void LoadHeader();
    bool LoadHeader1(DriverCommandStreamSoundLoadHeader* pCommand);
    bool LoadHeaderForOpus(DriverCommandStreamSoundLoadHeader* pCommand, StreamFileType type,
                           DecodeMode mode);
    bool ReadTrackInfoFromStreamSoundFile(StreamSoundFileReader& rReader);
    bool SetAdpcmInfo(StreamSoundFileReader& rReader, int channelCount,
                      audio::AdpcmParameter** ppParam);
    void UpdateLoadingDataBlockIndex();
    IStreamDataDecoderManager* SelectStreamDataDecoderManager(StreamFileType type,
                                                              DecodeMode mode);
    void SetStreamSoundInfoForOpus(const IStreamDataDecoder::DataInfo& rInfo);
    void LoadData(void** ppBuffer, u32 bufferBlockIndex, size_t startOffsetSamples,
                  size_t prefetchOffsetSamples, TaskProfileLogger& rLogger);
    bool LoadData1(DriverCommandStreamSoundLoadData* pCommand, void** ppBuffer,
                   u32 bufferBlockIndex, size_t startOffsetSamples, size_t prefetchOffsetSamples,
                   TaskProfileLogger& rLogger);
    bool LoadDataForOpus(DriverCommandStreamSoundLoadData* pCommand, void** ppBuffer,
                         u32 bufferBlockIndex, size_t startOffsetSamples,
                         size_t prefetchOffsetSamples, TaskProfileLogger& rLogger);
    bool ApplyStartOffset(long offset, int* pLoopCount);
    void CalculateBlockInfo(BlockInfo& rBlockInfo);
    bool LoadAdpcmContextForStartOffset();
    bool LoadOneBlockDataViaCache(void** ppBuffer, const BlockInfo& rBlockInfo, long destOffset,
                                  bool isStartOffsetBlock, bool isAdpcmStartOffset);
    bool LoadOneBlockData(void** ppBuffer, const BlockInfo& rBlockInfo, long destOffset,
                          bool isStartOffsetBlock, bool isAdpcmStartOffset);
    bool MoveNextRegion(int* pLoopCount);
    bool DecodeStreamData(void** ppBuffer, IStreamDataDecoder::DecodeType type);
    void ResetDecoder();
    void UpdateLoadingDataBlockIndexForOpus(void** ppBuffer);
    bool IsLoopStartFilePos(u32 blockIndex);
    int GetLoadChannelCount(int channelIndex);
    bool LoadStreamBuffer(u8* pBuffer, const BlockInfo& rBlockInfo, u32 loadChannelCount);
    bool LoadStreamBuffer(u8* pBuffer, size_t size);
    bool SkipStreamBuffer(size_t size);
    void UpdateAdpcmInfoForStartOffset(const void* pData, int channelIndex,
                                       const BlockInfo& rBlockInfo);

    static u8 g_LoadBuffer[LoadBufferSize];

    /**
     * @brief Sets the player the loaded data is handed to.
     * @param pPlayer Player owning the loader.
     */
    void SetPlayer(StreamSoundPlayer* pPlayer) { mPlayer = pPlayer; }

    /**
     * @brief Sets where the stream parameters read from the header are stored.
     * @param pDataInfo Stream parameters of the player.
     */
    void SetStreamDataInfo(StreamDataInfoDetail* pDataInfo) { mDataInfo = pDataInfo; }

    /** @brief Sets the container format of the file. @param fileType Container format. */
    void SetFileType(StreamFileType fileType) { mFileType = fileType; }

    /** @brief Sets where compressed data is decoded. @param decodeMode Decode mode. */
    void SetDecodeMode(DecodeMode decodeMode) { mDecodeMode = decodeMode; }

    /** @brief Sets whether data is decoded ahead of playback. @param isEnabled Whether it is. */
    void SetPreDecodeEnabled(bool isEnabled) { mIsPreDecodeEnabled = isEnabled; }

private:
    /** @brief Checks whether file reads go through the stream cache. @return True when cached. */
    bool IsCacheEnabled() const { return mCacheBuffer != nullptr && mCacheSize != 0; }

    /** @brief Returns the decoder to its manager and forgets both. */
    void ReleaseDecoder() {
        if (mDecoderManager != nullptr) {
            if (mDecoder != nullptr) {
                mDecoderManager->FreeDecoder(mDecoder);
                mDecoder = nullptr;
            }

            mDecoderManager = nullptr;
        }
    }

    /**
     * @brief Releases every finished data load task.
     * @param isWaitAll True to wait for and release unfinished tasks too.
     */
    void ReleaseDataLoadTasks(bool isWaitAll) {
        for (auto it = mDataLoadTaskList.begin(); it != mDataLoadTaskList.end();) {
            StreamDataLoadTask& rTask = *it;
            if (!isWaitAll && rTask.GetStatus() != Task::Status_Done &&
                rTask.GetStatus() != Task::Status_Cancel) {
                break;
            }

            ++it;
            rTask.Wait();
            mDataLoadTaskList.erase(mDataLoadTaskList.iterator_to(rTask));
            rTask.~StreamDataLoadTask();
            mDataLoadTaskPool.Free(&rTask);
        }
    }

public:
    StreamSoundFileLoader mFileLoader;
    StreamSoundPlayer* mPlayer;
    fnd::FileStream* mFileStream;
    StreamDataInfoDetail* mDataInfo;
    StreamFileType mFileType;
    DecodeMode mDecodeMode;
    SoundArchiveFilesHook* mFilesHook;
    const char* mItemLabel;
    int mChannelCount;
    u16 mAssignNumber;
    bool mLoopFlag;
    bool mIsLoopFlagEnabled;
    bool mIsAbortOnOpenFailure;
    bool mIsPreDecodeEnabled;
    size_t mLoopStart;
    size_t mLoopEnd;
    char mFilePath[FilePathMax + 1];
    const void* mFileAddress;
    size_t mFileSize;
    void* mCacheBuffer;
    size_t mCacheSize;
    u32 mLoadingDataBlockIndex;
    u32 mLastBlockIndex;
    u32 mLoopStartBlockIndex;
    size_t mDataStartFilePos;
    size_t mLoopStartFilePos;
    size_t mLoopStartBlockSampleOffset;
    bool mLoopJumpFlag;
    bool mLoadFinishFlag;
    RegionManager mRegionManager;
    u8 _7c0[0x40];
    StreamHeaderLoadTask mHeaderLoadTask;
    StreamCloseTask mCloseTask;
    DataLoadTaskList mDataLoadTaskList;
    DataLoadTaskPool mDataLoadTaskPool;
    u8 mDataLoadTaskPoolBuffer[DataLoadTaskPoolBufferSize];
    SampleFormat mSampleFormat;
    AdpcmInfo mAdpcmInfo[ChannelCountMax];
    u8 mFileStreamBuffer[FileStreamBufferSize];
    IStreamDataDecoder* mDecoder;
    IStreamDataDecoderManager* mDecoderManager;
    util::IntrusiveListNode mManagerLink;
};
static_assert(sizeof(StreamSoundLoader::AdpcmInfo) == 0xc0, "AdpcmInfo size");
}  // namespace driver
}  // namespace detail
}  // namespace nn::atk
