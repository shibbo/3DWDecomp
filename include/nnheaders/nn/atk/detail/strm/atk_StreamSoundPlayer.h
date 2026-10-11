#pragma once

#include <attributes.h>
#include <nn/os/os_Mutex.h>

// The loader stores its player; declare the driver-side player first so that member uses it.
namespace nn::atk::detail::driver {
class StreamSoundPlayer;
}  // namespace nn::atk::detail::driver

#include <nn/atk/atk_BasicSoundPlayer.h>
#include <nn/atk/atk_DecodeAdpcm.h>
#include <nn/atk/atk_LoaderManager.h>
#include <nn/atk/atk_SoundStartable.h>
#include <nn/atk/atk_SoundThread.h>
#include <nn/atk/atk_StreamSoundPrefetchFileReader.h>
#include <nn/atk/atk_StreamTrack.h>
#include <nn/atk/atk_WaveBuffer.h>
#include <nn/atk/detail/strm/atk_StreamSoundLoader.h>

namespace nn::atk {
struct StreamSoundDataInfo;
}  // namespace nn::atk

namespace nn::atk::detail {
/** @brief Description of one block of stream data the task thread loaded. */
struct LoadDataParam {
    static const int ChannelCountMax = 16;

    /** @brief Creates a parameter describing an empty block. */
    LoadDataParam()
        : blockIndex(0), loadSamples(0), startSamplePosition(0), offsetSamples(0), loadSize(0),
          isAdpcmContextAvailable(false), loopCount(0), isLastBlock(false),
          isStartOffsetOverRegion(false) {}

    u32 blockIndex;
    size_t loadSamples;
    s64 startSamplePosition;
    size_t offsetSamples;
    size_t loadSize;
    bool isAdpcmContextAvailable;
    AdpcmContextNotAligned adpcmContext[ChannelCountMax];
    int loopCount;
    bool isLastBlock;
    bool isStartOffsetOverRegion;
};
}  // namespace nn::atk::detail

namespace nn::atk::detail::driver {
class StreamBufferPool;

typedef LoaderManager<StreamSoundLoader> StreamSoundLoaderManager;

/** @brief Driver-side player that streams a sound's blocks into voices. */
class StreamSoundPlayer : public BasicSoundPlayer, public SoundThread::PlayerCallback {
public:
    static const int ChannelCountMax = 16;
    static const int TrackCountMax = 8;
    static const int BufferBlockCountMax = StreamChannel::BufferBlockCountMax;
    static const int FilePathMax = 639;

    /** @brief Unit of a start offset. */
    enum StartOffsetType {
        StartOffsetType_Sample,
        StartOffsetType_MilliSeconds,
    };

    /** @brief Sound archive parameters of the stream, fixed when the sound is set up. */
    struct SetupArg {
        StreamBufferPool* pBufferPool;
        int allocChannelCount;
        u16 allocTrackFlag;
        u8 fileType;
        bool loopFlag;
        bool isLoopFlagEnabled;
        StreamTrackDataInfo trackInfos[TrackCountMax];
        size_t loopStart;
        size_t loopEnd;
        float pitch;
        u8 mainSend;
        u8 fxSend[AuxBus_Count];
        DecodeMode decodeMode;
        bool isPreDecodeEnabled;
    };

    /** @brief Start parameters shared by normal and prefetched playback. */
    struct PrepareBaseArg {
        /** @brief Creates parameters that start at the beginning without delay. */
        PrepareBaseArg()
            : startOffsetType(StartOffsetType_Sample), startOffset(0), delayTime(0), delayCount(0),
              updateType(UpdateType_AudioFrame), regionCallback(nullptr),
              regionCallbackArg(nullptr), filePath() {}

        StartOffsetType startOffsetType;
        s64 startOffset;
        u32 delayTime;
        int delayCount;
        UpdateType updateType;
        StreamRegionCallback regionCallback;
        void* regionCallbackArg;
        char filePath[FilePathMax];
    };

    /** @brief Where the stream data is read from. */
    struct PrepareArg : PrepareBaseArg {
        /** @brief Creates parameters naming no file. */
        PrepareArg()
            : pExternalData(nullptr), externalDataSize(0), pFilesHook(nullptr),
              itemLabel(nullptr), pCacheBuffer(nullptr), cacheSize(0) {}

        const void* pExternalData;
        size_t externalDataSize;
        SoundArchiveFilesHook* pFilesHook;
        const char* itemLabel;
        void* pCacheBuffer;
        size_t cacheSize;
    };

    /** @brief Prefetch data the first blocks are played from. */
    struct PreparePrefetchArg : PrepareBaseArg {
        /** @brief Creates parameters naming no prefetch data. */
        PreparePrefetchArg()
            : pExternalData(nullptr), externalDataSize(0), pFilesHook(nullptr),
              itemLabel(nullptr), strmPrefetchFile(nullptr) {}

        const void* pExternalData;
        size_t externalDataSize;
        SoundArchiveFilesHook* pFilesHook;
        const char* itemLabel;
        const void* strmPrefetchFile;
    };

    /** @brief Sound archive item parameters applied on top of every track. */
    struct ItemData {
        void Set(const SetupArg& rArg);

        float pitch;
        float mainSend;
        float fxSend[AuxBus_Count];
    };

    /** @brief A track's file parameters converted to mixing values. */
    struct TrackData {
        void Set(const StreamTrack* pTrack);

        float volume;
        float lpfFreq;
        int biquadType;
        float biquadValue;
        float pan;
        float span;
        float mainSend;
        float fxSend[AuxBus_Count];
    };

    /** @brief Position of the loop inside the prefetched blocks. */
    struct PrefetchIndexInfo {
        void Initialize(const StreamDataInfoDetail& rInfo);

        u32 lastBlockIndex;
        size_t loopStartInBlock;
        u32 loopStartBlockIndex;
        u32 loopBlockCount;
    };

    /** @brief Load description of one prefetched block. */
    struct PrefetchLoadDataParam : LoadDataParam {
        /** @brief Creates a parameter describing the first prefetched block. */
        PrefetchLoadDataParam() : prefetchBlockIndex(0), prefetchBlockBytes(0) {}

        u32 prefetchBlockIndex;
        size_t prefetchBlockBytes;
    };

    /** @brief Play position as reported to the sound. */
    struct PlayPosition {
        s64 samplePosition;
        s64 originalSamplePosition;
    };

    /** @brief Position of the samples held in one buffer block. */
    struct BlockInfo {
        s64 startSamplePosition;
        size_t loadSamples;
        int loopCount;
    };

    static u16 g_AssignNumberCount;

    StreamSoundPlayer();
    ~StreamSoundPlayer() override;
    void Initialize(OutputReceiver* pReceiver) override;
    void UpdatePlaySamplePosition();
    bool TryAllocLoader();
    void UpdateTrackActiveFlag(int trackNo, bool isActive);
    void Finalize() override;
    void FinishPlayer();
    void FreeStreamBuffers();
    void FreeVoices();
    void FreeLoader();
    void Setup(const SetupArg& rArg);
    bool SetupTrack(const SetupArg& rArg);
    void Prepare(const PrepareArg& rArg);
    void SetPrepareBaseArg(const PrepareBaseArg& rArg);
    void RequestLoadHeader(const PrepareArg& rArg);
    void PreparePrefetch(const PreparePrefetchArg& rArg);
    bool ReadPrefetchFile(StreamSoundPrefetchFileReader& rReader);
    bool ApplyStreamDataInfo(const StreamDataInfoDetail& rInfo);
    bool SetupPlayer();
    bool AllocVoices();
    bool LoadPrefetchBlocks(StreamSoundPrefetchFileReader& rReader);
    void Start() override;
    void StartPlayer();
    void Stop() override;
    void Pause(bool isPause) override;
    void UpdatePauseStatus();
    bool IsLoadingDelayState() const;
    bool IsBufferEmpty() const;
    bool ReadStreamSoundDataInfo(StreamSoundDataInfo* pInfo) const;
    s64 GetPlaySamplePosition(bool isOriginal) const;
    float GetFilledBufferPercentage() const;
    int GetBufferBlockCount(WaveBuffer::Status status) const;
    int GetTotalBufferBlockCount() const;
    bool LoadHeader(bool result, audio::AdpcmParameter** ppAdpcmParam, u16 assignNumber);
    bool CheckPrefetchRevision(const StreamDataInfoDetail& rInfo) const;
    bool AllocStreamBuffers();
    void UpdateLoadingBlockIndex();
    void SetPrepared(bool isPrepared);
    bool LoadStreamData(bool result, const LoadDataParam& rParam, u16 assignNumber);
    bool LoadStreamData(bool result, const LoadDataParam& rParam, u16 assignNumber,
                        bool usePrefetchData, u32 prefetchBlockIndex, size_t prefetchBlockBytes);
    bool IsStoppedByLoadingDelay() const;
    static void VoiceCallbackFunc(MultiVoice* pVoice, MultiVoice::VoiceCallbackStatus status,
                                  void* pArg);
    void Update();
    void UpdateBuffer();
    void UpdateVoiceParams(StreamTrack* pTrack);
    bool CheckDiskDriveError() const;
    void SetOutputParam(OutputParam* pOutParam, const OutputParam& rParam,
                        const TrackData& rTrackData);
    void ApplyTvOutputParamForMultiChannel(const OutputParam& rParam,
                                           const OutputAdditionalParam* pAdditionalParam,
                                           MultiVoice* pVoice, int channelIndex, MixMode mixMode);
    void MixSettingForOutputParam(OutputParam* pParam, OutputBusMixVolume* pBusMixVolume,
                                  int channelIndex, MixMode mixMode);
    int GetOriginalLoopCount(long position, const StreamDataInfoDetail& rInfo) const;
    s64 GetOriginalPlaySamplePosition(long position, const StreamDataInfoDetail& rInfo) const;
    void UpdatePlaySamplePosition(long position, long originalPosition);
    bool IsValidStartOffset(const StreamDataInfoDetail& rInfo);
    void ApplyTrackDataInfo(const StreamDataInfoDetail& rInfo);
    size_t GetStartOffsetSamples(const StreamDataInfoDetail& rInfo);
    void PreparePrefetchOnLastBlock(PrefetchLoadDataParam* pParam,
                                    const PrefetchIndexInfo& rIndexInfo);
    bool PreparePrefetchOnLoopStartBlock(PrefetchLoadDataParam* pParam,
                                         const PrefetchIndexInfo& rIndexInfo,
                                         StreamSoundPrefetchFileReader& rReader);
    void PreparePrefetchOnLoopBlock(PrefetchLoadDataParam* pParam,
                                    const PrefetchIndexInfo& rIndexInfo, u32 loopBlockIndex);
    bool PreparePrefetchOnNormalBlock(PrefetchLoadDataParam* pParam, u32 blockIndex,
                                      StreamSoundPrefetchFileReader& rReader);
    bool SetAdpcmLoopInfo(StreamSoundPrefetchFileReader& rReader,
                          const StreamDataInfoDetail& rInfo, audio::AdpcmParameter* pParam,
                          AdpcmContextNotAligned* pContext);
    bool SetAdpcmInfo(StreamSoundPrefetchFileReader& rReader, const StreamDataInfoDetail& rInfo,
                      audio::AdpcmParameter* pParam, AdpcmContextNotAligned* pContext);
    void SetActiveFlag(bool isActive) override;
    void SetTrackVolume(u32 trackBitFlag, float volume);
    void SetTrackInitialVolume(u32 trackBitFlag, u32 volume);
    void SetTrackOutputLine(u32 trackBitFlag, u32 outputLine);
    void ResetTrackOutputLine(u32 trackBitFlag);
    void SetTrackTvVolume(u32 trackBitFlag, float volume);
    void SetTrackChannelTvMixParameter(u32 trackBitFlag, u32 srcChannel,
                                       const MixParameter& rParam);
    void SetTrackTvPan(u32 trackBitFlag, float pan);
    void SetTrackTvSurroundPan(u32 trackBitFlag, float span);
    void SetTrackTvMainSend(u32 trackBitFlag, float send);
    void SetTrackTvFxSend(u32 trackBitFlag, AuxBus bus, float send);
    StreamTrack* GetPlayerTrack(int trackNo);
    const StreamTrack* GetPlayerTrack(int trackNo) const;

    /** @brief Advances playback by one sound-thread frame. @param frame Elapsed frames. */
    void OnUpdateFrameSoundThread(int frame) override { Update(); }

    /**
     * @brief Advances playback when the stream runs at the audio-frame rate.
     * @param frame Elapsed frames.
     */
    void OnUpdateFrameSoundThreadWithAudioFrameFrequency(int frame) override {
        if (mUpdateType == UpdateType_AudioFrame) {
            Update();
        }
    }

    /** @brief Stops playback because the sound thread is going away. */
    void OnShutdownSoundThread() override { Stop(); }

    /**
     * @brief Sets the loader pool the player draws its loader from.
     * @param pManager Loader pool; must outlive the player's loads.
     */
    void SetLoaderManager(StreamSoundLoaderManager* pManager) { mLoaderManager = pManager; }

    /** @brief Checks whether the player stopped accepting loaded data. @return Whether it did. */
    bool IsTaskCancelled() const { return mIsTaskCancelled; }

private:
    bool mSetupFlag;
    bool mIsPrepared;
    bool mIsTaskCancelled;
    bool mIsPreparedPrefetch;
    bool mPauseStatus;
    bool mLoadWaitFlag;
    bool mPlayFinishFlag;
    bool mIsStoppedByLoadingDelay;
    bool mIsLoadingDelay;
    bool mIsRegisterPlayerCallback;
    bool mUseDelayCount;
    int mLoopCounter;
    int mOriginalLoopCounter;
    int mPrepareCounter;
    StreamSoundLoaderManager* mLoaderManager;
    StreamSoundLoader* mLoader;
    StreamBufferPool* mBufferPool;
    int mBufferBlockCount;
    u32 mLoadingBufferBlockIndex;
    u32 mPlayingBufferBlockIndex;
    u32 mLastPlayFinishBufferBlockIndex;
    StartOffsetType mStartOffsetType;
    s64 mStartOffset;
    int mDelayCount;
    u16 mAssignNumber;
    u8 mFileType;
    bool mIsPreDecodeEnabled;
    DecodeMode mDecodeMode;
    bool mLoopFlag;
    bool mIsLoopFlagEnabled;
    StreamDataInfoDetail mStreamDataInfo;
    size_t mLoopStart;
    size_t mLoopEnd;
    ItemData mItemData;
    const void* mPrefetchData;
    audio::AdpcmParameter mPrefetchAdpcmParam[ChannelCountMax];
    StreamSoundPrefetchFileReader::PrefetchDataInfo mPrefetchDataInfo;
    size_t mPrefetchOffset;
    bool mIsPrefetchRevisionCheckEnabled;
    u32 mPrefetchRevisionValue;
    int mChannelCount;
    int mTrackCount;
    u8 _470[0x10];  // Places the channels on a cache line.
    StreamChannel mChannels[ChannelCountMax];
    StreamTrack mTracks[TrackCountMax];
    UpdateType mUpdateType;
    BlockInfo mBlockInfo[BufferBlockCountMax];
    PrepareArg mPrepareArg;
    bool mIsPrepareDone;
    PreparePrefetchArg mPreparePrefetchArg;
    bool mIsPreparePrefetchPending;
    SetupArg mSetupArg;
    PlayPosition mPlayPosition;
    mutable os::Mutex mMutex;
};
}  // namespace nn::atk::detail::driver
