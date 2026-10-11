#include <nn/atk/detail/strm/atk_StreamSoundPlayer.h>

#include <algorithm>
#include <cstring>
#include <nn/atk/atk_MultiVoiceManager.h>
#include <nn/atk/atk_OutputAdditionalParam.h>
#include <nn/atk/atk_SoundArchivePlayer.h>
#include <nn/atk/atk_SoundSystem.h>
#include <nn/atk/atk_StreamBufferPool.h>
#include <nn/atk/atkfnd_ScopedLock.h>
#include <nn/util.h>

namespace nn::atk::detail::driver {
namespace {
// Largest stream block a buffer block can hold, and the decode margin kept after each block.
const size_t BlockSizeMax = 0x2000;
const size_t BufferBlockMargin = 0x900;

// A stream needs at least two buffer blocks: one playing while the next one loads.
const int BufferBlockCountMin = 2;

// Duration of one sound-thread frame, which delay times are counted in.
const u32 SoundFrameMsec = 5;

// Highest pitch the Opus decoder can keep up with.
const float OpusPitchMax = 4.0f;

// Loops shorter than this were extended by the converter, so the original count is derived.
const s64 OriginalLoopLengthMin = 1152;

// Priority of the voices of a stream; streams are never dropped for other voices.
const int VoicePriority = 255;

// Task priorities of data loads requested before and after playback starts.
const int LoadPriorityPrepare = 1;
const int LoadPriorityPlay = 2;

// Biquad filter type of the player that defers to the track's own filter.
const u8 BiquadFilterTypeInherit = 255;

// Output line that defers to the player's base output line.
const int OutputLineInherit = -1;

// Number of bus mix volumes of one wave channel.
const int BusMixVolumeCount = 24;

// Sample position reported while the stream is inactive.
const s64 InvalidSamplePosition = -1;

/**
 * @brief Converts a duration to a sample count.
 * @param msec Duration in milliseconds.
 * @param sampleRate Sample rate of the stream.
 * @return Sample count, clamped to 32 bits.
 */
ALWAYS_INLINE size_t MsecToSamples(s64 msec, int sampleRate) {
    u64 samples = static_cast<u64>(msec * sampleRate) / 1000;
    return std::min<u64>(samples, 0xffffffff);
}

/**
 * @brief Converts a stored 0-127 level to a mixing ratio.
 * @param value Level in [0, 127].
 * @return Ratio in [0, 1].
 */
ALWAYS_INLINE float LevelToRatio(u8 value) {
    return static_cast<float>(value) / 127.0f;
}

/**
 * @brief Gives a loader the loop settings of the sound archive.
 * @param pLoader Loader to configure.
 * @param loopFlag Whether the archive overrides the loop of the file.
 * @param isLoopFlagEnabled Whether the archive loop override applies.
 * @param loopStart Loop start in samples.
 * @param loopEnd Loop end in samples.
 */
ALWAYS_INLINE void SetLoaderLoopInfo(StreamSoundLoader* pLoader, bool loopFlag,
                                     bool isLoopFlagEnabled, size_t loopStart, size_t loopEnd) {
    pLoader->mLoopFlag = loopFlag;
    pLoader->mIsLoopFlagEnabled = isLoopFlagEnabled;
    pLoader->mLoopStart = loopStart;
    pLoader->mLoopEnd = loopEnd;
}

/**
 * @brief Gives a loader the archive item it loads.
 * @param pLoader Loader to configure.
 * @param pFilesHook Hook that may redirect the file, or nullptr.
 * @param itemLabel Label of the item.
 */
ALWAYS_INLINE void SetLoaderItemInfo(StreamSoundLoader* pLoader, SoundArchiveFilesHook* pFilesHook,
                                     const char* itemLabel) {
    pLoader->mFilesHook = pFilesHook;
    pLoader->mItemLabel = itemLabel;
}

/**
 * @brief Gives a loader stream data that is already in memory.
 * @param pLoader Loader to configure.
 * @param pData Stream file in memory, or nullptr to read the file path.
 * @param size Size of the stream file.
 */
ALWAYS_INLINE void SetLoaderExternalData(StreamSoundLoader* pLoader, const void* pData,
                                         size_t size) {
    pLoader->mFileAddress = pData;
    pLoader->mFileSize = size;
}

/**
 * @brief Gives a loader a buffer to cache file reads in.
 * @param pLoader Loader to configure.
 * @param pBuffer Cache buffer, or nullptr.
 * @param size Size of the cache buffer.
 */
ALWAYS_INLINE void SetLoaderCacheBuffer(StreamSoundLoader* pLoader, void* pBuffer, size_t size) {
    pLoader->mCacheBuffer = pBuffer;
    pLoader->mCacheSize = size;
}

/**
 * @brief Detaches the previous file of a loader and sets how it reacts to open failures.
 * @param pLoader Loader to configure.
 * @param isAbortOnOpenFailure Whether a failed open stops the sound system.
 */
ALWAYS_INLINE void SetLoaderOpenOption(StreamSoundLoader* pLoader, bool isAbortOnOpenFailure) {
    pLoader->mFileStream = nullptr;
    pLoader->mIsAbortOnOpenFailure = isAbortOnOpenFailure;
}

/**
 * @brief Checks whether every channel finished playing a buffer block.
 * @param pChannels Channels of the stream.
 * @param channelCount Number of channels.
 * @param rBlockIndex Buffer block to check.
 * @return Whether the block is done on every channel.
 */
ALWAYS_INLINE bool IsBufferBlockPlayed(const StreamChannel* pChannels, int channelCount,
                                       const u32& rBlockIndex) {
    for (int channelNo = 0; channelNo < channelCount; channelNo++) {
        if (pChannels[channelNo].mWaveBuffer[rBlockIndex].status != WaveBuffer::Status_Done) {
            return false;
        }
    }

    return true;
}
}  // namespace

u16 StreamSoundPlayer::g_AssignNumberCount;

/** @brief Creates an idle player with no loader, channels or tracks attached. */
StreamSoundPlayer::StreamSoundPlayer()
    : mSetupFlag(false), mIsRegisterPlayerCallback(false), mLoaderManager(nullptr),
      mLoader(nullptr), mPrefetchData(nullptr), mChannelCount(0), mTrackCount(0),
      mIsPrepareDone(false), mIsPreparePrefetchPending(false), mMutex(true) {}

/** @brief Stops playback and releases every resource the player holds. */
StreamSoundPlayer::~StreamSoundPlayer() {
    Finalize();
}

/**
 * @brief Resets the player for a new sound and takes a loader from the pool.
 * @param pReceiver Output the voices of the stream render to.
 */
void StreamSoundPlayer::Initialize(OutputReceiver* pReceiver) {
    fnd::ScopedMutexLock lock(mMutex);

    BasicSoundPlayer::Initialize(pReceiver);
    UpdatePlaySamplePosition();

    mIsPrepareDone = false;
    mIsPreparePrefetchPending = false;
    mLoopCounter = 0;
    mOriginalLoopCounter = 0;
    mPrefetchOffset = 0;
    mIsPrefetchRevisionCheckEnabled = false;
    mPrefetchRevisionValue = 0;
    mDelayCount = 0;
    mUseDelayCount = false;
    mIsPreparedPrefetch = false;
    mPauseStatus = false;
    mLoadWaitFlag = false;
    mPlayFinishFlag = false;
    mSetupFlag = false;
    mIsPrepared = false;
    mIsTaskCancelled = false;
    mPlayPosition = PlayPosition();

    if (TryAllocLoader()) {
        mLoader->Initialize();
    }

    mItemData.pitch = 1.0f;
    mItemData.mainSend = 1.0f;
    for (int i = 0; i < AuxBus_Count; i++) {
        mItemData.fxSend[i] = 0.0f;
    }

    for (int trackNo = 0; trackNo < TrackCountMax; trackNo++) {
        StreamTrack& rTrack = mTracks[trackNo];
        UpdateTrackActiveFlag(trackNo, false);
        rTrack.mVolume = 1.0f;
        rTrack.mOutputLine = OutputLineInherit;
        rTrack.mTvParam.volume = 1.0f;
        rTrack.mTvParam.mixMode = MixMode_Pan;
        rTrack.mTvParam.pan = 0.0f;
        rTrack.mTvParam.span = 0.0f;
        rTrack.mTvParam.mainSend = 0.0f;
        for (int bus = 0; bus < AuxBus_Count; bus++) {
            rTrack.mTvParam.fxSend[bus] = 0.0f;
        }
    }

    for (int channelNo = 0; channelNo < ChannelCountMax; channelNo++) {
        StreamChannel& rChannel = mChannels[channelNo];
        rChannel.mBufferAddress = nullptr;
        rChannel.mVoice = nullptr;
        for (int blockNo = 0; blockNo < BufferBlockCountMax; blockNo++) {
            rChannel.mWaveBuffer[blockNo].Initialize();
        }
    }

    for (int blockNo = 0; blockNo < BufferBlockCountMax; blockNo++) {
        BlockInfo& rInfo = mBlockInfo[blockNo];
        rInfo.startSamplePosition = 0;
        rInfo.loadSamples = 0;
        rInfo.loopCount = 0;
    }
}

/** @brief Resets the reported play position when the stream is inactive or still loading. */
void StreamSoundPlayer::UpdatePlaySamplePosition() {
    PlayPosition& rPosition = mPlayPosition;
    if (!mActiveFlag || !mTracks[0].mActiveFlag) {
        rPosition.samplePosition = InvalidSamplePosition;
        rPosition.originalSamplePosition = InvalidSamplePosition;
    } else if (!mIsPrepared) {
        rPosition.samplePosition = 0;
        rPosition.originalSamplePosition = 0;
    }
}

/**
 * @brief Takes a loader from the pool unless the player already holds one.
 * @return Whether the player holds a loader.
 */
bool StreamSoundPlayer::TryAllocLoader() {
    if (mLoader != nullptr) {
        return true;
    }

    if (mLoaderManager == nullptr) {
        return false;
    }

    StreamSoundLoader* pLoader = mLoaderManager->Alloc();
    if (pLoader == nullptr) {
        return false;
    }

    mLoader = pLoader;
    return true;
}

/**
 * @brief Enables or disables a track.
 * @param trackNo Track number, in [0, TrackCountMax).
 * @param isActive Whether the track plays.
 */
void StreamSoundPlayer::UpdateTrackActiveFlag(int trackNo, bool isActive) {
    mTracks[trackNo].mActiveFlag = isActive;
    if (trackNo == 0) {
        UpdatePlaySamplePosition();
    }
}

/** @brief Stops playback and returns the buffers, voices and loader of the stream. */
void StreamSoundPlayer::Finalize() {
    fnd::ScopedMutexLock lock(mMutex);

    FinishPlayer();

    if (mSetupFlag) {
        mIsTaskCancelled = true;
        FreeStreamBuffers();
        FreeVoices();

        if (mLoader != nullptr) {
            mLoader->Finalize();
        }

        FreeLoader();
        mBufferPool = nullptr;
        BasicSoundPlayer::Finalize();
        SetActiveFlag(false);
        mSetupFlag = false;
    } else {
        BasicSoundPlayer::Finalize();
    }
}

/** @brief Cancels pending loads, stops the voices and leaves the sound thread. */
void StreamSoundPlayer::FinishPlayer() {
    if (mLoader != nullptr) {
        mLoader->CancelRequest();
    }

    for (int channelNo = 0; channelNo < mChannelCount; channelNo++) {
        MultiVoice* pVoice = mChannels[channelNo].mVoice;
        if (pVoice != nullptr) {
            pVoice->Stop();
        }
    }

    if (mIsRegisterPlayerCallback) {
        SoundThread::GetInstance().UnregisterPlayerCallback(this);
        mIsRegisterPlayerCallback = false;
    }

    if (mStartedFlag) {
        mStartedFlag = false;
    }
}

/** @brief Returns the buffer of every channel to the buffer pool. */
void StreamSoundPlayer::FreeStreamBuffers() {
    for (int channelNo = 0; channelNo < mChannelCount; channelNo++) {
        StreamChannel& rChannel = mChannels[channelNo];
        if (rChannel.mBufferAddress != nullptr) {
            mBufferPool->Free(rChannel.mBufferAddress);
            rChannel.mBufferAddress = nullptr;
        }
    }
}

/** @brief Releases the voice of every channel. */
void StreamSoundPlayer::FreeVoices() {
    for (int channelNo = 0; channelNo < mChannelCount; channelNo++) {
        StreamChannel& rChannel = mChannels[channelNo];
        if (rChannel.mVoice != nullptr) {
            rChannel.mVoice->Free();
            rChannel.mVoice = nullptr;
        }
    }
}

/** @brief Returns the loader to the pool. */
void StreamSoundPlayer::FreeLoader() {
    if (mLoader == nullptr || mLoaderManager == nullptr) {
        return;
    }

    mLoaderManager->Free(mLoader);
    mLoader = nullptr;
}

/**
 * @brief Applies the archive parameters of the sound, or keeps them until a loader is free.
 * @param rArg Archive parameters of the stream.
 */
void StreamSoundPlayer::Setup(const SetupArg& rArg) {
    fnd::ScopedMutexLock lock(mMutex);

    if (mLoader == nullptr) {
        mSetupArg = rArg;
        return;
    }

    mFileType = rArg.fileType;
    mIsPreDecodeEnabled = rArg.isPreDecodeEnabled;
    mDecodeMode = rArg.decodeMode;
    mLoopFlag = rArg.loopFlag;
    mIsLoopFlagEnabled = rArg.isLoopFlagEnabled;
    mLoopStart = rArg.loopStart;
    mLoopEnd = rArg.loopEnd;
    mAssignNumber = g_AssignNumberCount++;
    mLoader->mAssignNumber = mAssignNumber;
    mItemData.Set(rArg);

    if (!SetupTrack(rArg)) {
        return;
    }

    mBufferPool = rArg.pBufferPool;
    mSetupFlag = true;
}

/**
 * @brief Converts the archive sends and pitch of the sound.
 * @param rArg Archive parameters of the stream.
 */
void StreamSoundPlayer::ItemData::Set(const SetupArg& rArg) {
    pitch = rArg.pitch;
    mainSend = LevelToRatio(rArg.mainSend) - 1.0f;
    for (int bus = 0; bus < AuxBus_Count; bus++) {
        fxSend[bus] = LevelToRatio(rArg.fxSend[bus]);
    }
}

/**
 * @brief Enables the tracks the sound plays and copies their file parameters.
 * @param rArg Archive parameters of the stream.
 * @return Whether at least one track plays; the player is finalized otherwise.
 */
bool StreamSoundPlayer::SetupTrack(const SetupArg& rArg) {
    u32 trackBitFlag = rArg.allocTrackFlag;
    int trackNo = 0;
    while (trackBitFlag != 0) {
        if ((trackBitFlag & 1) != 0) {
            if (trackNo >= TrackCountMax) {
                break;
            }

            UpdateTrackActiveFlag(trackNo, true);
        }

        trackBitFlag >>= 1;
        trackNo++;
    }

    mTrackCount = std::min(trackNo, TrackCountMax);
    if (mTrackCount == 0) {
        Finalize();
        return false;
    }

    for (int i = 0; i < TrackCountMax; i++) {
        mStreamDataInfo.trackInfo[i] = rArg.trackInfos[i];
    }

    return true;
}

/**
 * @brief Starts loading the stream header, or keeps the request until a loader is free.
 * @param rArg Location of the stream data and start parameters.
 */
void StreamSoundPlayer::Prepare(const PrepareArg& rArg) {
    if (!mIsRegisterPlayerCallback) {
        SoundThread::GetInstance().RegisterPlayerCallback(this);
        mIsRegisterPlayerCallback = true;
    }

    if (mLoader == nullptr) {
        mPrepareArg = rArg;
        mIsPrepareDone = false;
        return;
    }

    if (!mSetupFlag) {
        return;
    }

    if (!mIsPreparedPrefetch) {
        SetPrepareBaseArg(rArg);
    }

    mIsStoppedByLoadingDelay = false;
    mIsLoadingDelay = false;
    mIsPrepareDone = true;
    RequestLoadHeader(rArg);
}

/**
 * @brief Applies the start parameters and hands the stream description to the loader.
 * @param rArg Start parameters.
 */
void StreamSoundPlayer::SetPrepareBaseArg(const PrepareBaseArg& rArg) {
    if (rArg.delayCount == 0) {
        mDelayCount = rArg.delayTime / SoundFrameMsec;
        mUseDelayCount = mDelayCount > 0;
    }

    mStartOffsetType = rArg.startOffsetType;
    mStartOffset = rArg.startOffset;
    mUpdateType = rArg.updateType;
    mLoader->mRegionManager.SetRegionCallback(rArg.regionCallback, rArg.regionCallbackArg);
    mLoader->SetPlayer(this);
    mLoader->SetStreamDataInfo(&mStreamDataInfo);
    mLoader->SetFileType(static_cast<StreamFileType>(mFileType));
    mLoader->SetDecodeMode(mDecodeMode);
    mLoader->SetPreDecodeEnabled(mIsPreDecodeEnabled);
    SetActiveFlag(true);
}

/**
 * @brief Points the loader at the stream file and queues the header load.
 * @param rArg Location of the stream data.
 */
void StreamSoundPlayer::RequestLoadHeader(const PrepareArg& rArg) {
    SetLoaderLoopInfo(mLoader, mLoopFlag, mIsLoopFlagEnabled, mLoopStart, mLoopEnd);
    std::strncpy(mLoader->mFilePath, rArg.filePath, FilePathMax);
    SetLoaderItemInfo(mLoader, rArg.pFilesHook, rArg.itemLabel);
    SetLoaderExternalData(mLoader, rArg.pExternalData, rArg.externalDataSize);
    SetLoaderCacheBuffer(mLoader, rArg.pCacheBuffer, rArg.cacheSize);
    bool isAbortOnOpenFailure = SoundSystem::g_IsStreamOpenFailureHalt;
    SetLoaderOpenOption(mLoader, isAbortOnOpenFailure);
    mLoader->RequestLoadHeader();
}

/**
 * @brief Starts playback from prefetch data, or keeps the request until a loader is free.
 * @param rArg Prefetch data and start parameters.
 */
void StreamSoundPlayer::PreparePrefetch(const PreparePrefetchArg& rArg) {
    fnd::ScopedMutexLock lock(mMutex);

    if (mLoader == nullptr) {
        mPreparePrefetchArg = rArg;
        mIsPreparePrefetchPending = true;
        return;
    }

    if (!mSetupFlag) {
        return;
    }

    mPrefetchData = rArg.strmPrefetchFile;
    StreamSoundPrefetchFileReader reader;
    reader.Initialize(mPrefetchData);

    if (!ReadPrefetchFile(reader)) {
        return;
    }

    SetPrepareBaseArg(rArg);

    if (reader.IsIncludeRegionInfo()) {
        mStreamDataInfo.isRegionIndexCheckAvailable = reader.IsRegionIndexCheckAvailable();
        if (mLoader == nullptr) {
            return;
        }

        // The prefetch reader implements the region reader interface with the same layout.
        IRegionInfoReadable* pRegionReader = reinterpret_cast<IRegionInfoReadable*>(&reader);
        if (!mLoader->mRegionManager.InitializeRegion(pRegionReader, &mStreamDataInfo)) {
            return;
        }

        if (!mLoader->mRegionManager.IsInFirstRegion()) {
            SetActiveFlag(false);
            return;
        }
    }

    if (!ApplyStreamDataInfo(mStreamDataInfo)) {
        return;
    }

    if (!SetupPlayer()) {
        return;
    }

    if (!AllocVoices()) {
        FreeStreamBuffers();
        return;
    }

    mIsPreparedPrefetch = true;
    LoadPrefetchBlocks(reader);
}

/**
 * @brief Reads the stream parameters stored in the prefetch data.
 * @param rReader Reader of the prefetch data.
 * @return Whether the prefetch data holds sample data.
 */
bool StreamSoundPlayer::ReadPrefetchFile(StreamSoundPrefetchFileReader& rReader) {
    StreamSoundPrefetchFileReader::PrefetchDataInfo dataInfo;
    if (!rReader.ReadPrefetchDataInfo(&dataInfo, 0)) {
        return false;
    }

    mPrefetchDataInfo.startFrame = dataInfo.startFrame;
    mPrefetchDataInfo.sampleBytes = dataInfo.sampleBytes;
    mPrefetchDataInfo.samples = dataInfo.samples;

    StreamSoundFile::StreamSoundInfo soundInfo;
    rReader.ReadStreamSoundInfo(&soundInfo);
    mStreamDataInfo.channelCount = rReader.GetChannelCount();
    mStreamDataInfo.SetStreamSoundInfo(soundInfo, rReader.IsCrc32CheckAvailable());

    if (rReader.IsCrc32CheckAvailable()) {
        mIsPrefetchRevisionCheckEnabled = true;
        mPrefetchRevisionValue = soundInfo.crc32;
    } else {
        mIsPrefetchRevisionCheckEnabled = false;
        mPrefetchRevisionValue = 0;
    }

    mChannelCount = std::min(mStreamDataInfo.channelCount, ChannelCountMax);
    return true;
}

/**
 * @brief Checks the start offset against the stream and applies its track parameters.
 * @param rInfo Stream parameters.
 * @return Whether playback can start; the player is stopped otherwise.
 */
bool StreamSoundPlayer::ApplyStreamDataInfo(const StreamDataInfoDetail& rInfo) {
    if (!IsValidStartOffset(rInfo)) {
        mFinishFlag = true;
        Stop();
        return false;
    }

    ApplyTrackDataInfo(rInfo);
    return true;
}

/**
 * @brief Splits the pool buffer of each channel into buffer blocks.
 * @return Whether at least two blocks fit.
 */
bool StreamSoundPlayer::SetupPlayer() {
    size_t blockSize = mStreamDataInfo.blockSize;
    if (blockSize > BlockSizeMax) {
        return false;
    }

    mBufferBlockCount = mBufferPool->GetBlockSize() / (blockSize + BufferBlockMargin);
    if (mBufferBlockCount < BufferBlockCountMin) {
        return false;
    }

    if (mBufferBlockCount > BufferBlockCountMax) {
        mBufferBlockCount = BufferBlockCountMax;
    }

    mLoadingBufferBlockIndex = 0;
    mPlayingBufferBlockIndex = 0;
    mLastPlayFinishBufferBlockIndex = 0;
    return true;
}

/**
 * @brief Allocates a voice for every channel.
 * @return Whether every channel got a voice; none is kept otherwise.
 */
bool StreamSoundPlayer::AllocVoices() {
    for (int channelNo = 0; channelNo < mChannelCount; channelNo++) {
        StreamChannel& rChannel = mChannels[channelNo];
        MultiVoice* pVoice = MultiVoiceManager::GetInstance().AllocVoice(1, VoicePriority,
                                                                         VoiceCallbackFunc,
                                                                         &rChannel);
        if (pVoice == nullptr) {
            for (int i = 0; i < channelNo; i++) {
                StreamChannel& rAllocated = mChannels[i];
                if (rAllocated.mVoice != nullptr) {
                    rAllocated.mVoice->Free();
                    rAllocated.mVoice = nullptr;
                }
            }

            return false;
        }

        rChannel.mVoice = pVoice;
    }

    return true;
}

/**
 * @brief Queues the prefetched blocks into the buffer blocks.
 * @param rReader Reader of the prefetch data.
 * @return Whether the ADPCM parameters of every block could be read.
 */
bool StreamSoundPlayer::LoadPrefetchBlocks(StreamSoundPrefetchFileReader& rReader) {
    PrefetchIndexInfo indexInfo;
    indexInfo.Initialize(mStreamDataInfo);

    s64 startSamplePosition = 0;
    for (u32 blockNo = 0; static_cast<int>(blockNo) < mBufferBlockCount; blockNo++) {
        PrefetchLoadDataParam param;
        param.startSamplePosition = startSamplePosition;

        u32 loopBlockIndex = 0;
        if (blockNo > indexInfo.lastBlockIndex) {
            loopBlockIndex = (blockNo - indexInfo.lastBlockIndex) % indexInfo.loopBlockCount;
        }

        if (indexInfo.lastBlockIndex == 0 || blockNo == indexInfo.lastBlockIndex ||
            (blockNo > indexInfo.lastBlockIndex && loopBlockIndex == 0)) {
            PreparePrefetchOnLastBlock(&param, indexInfo);
        } else if (mStreamDataInfo.isLoop && blockNo > indexInfo.lastBlockIndex) {
            if (loopBlockIndex == 1) {
                if (!PreparePrefetchOnLoopStartBlock(&param, indexInfo, rReader)) {
                    return false;
                }

                startSamplePosition = 0;
            } else {
                PreparePrefetchOnLoopBlock(&param, indexInfo, loopBlockIndex);
            }
        } else if (!PreparePrefetchOnNormalBlock(&param, blockNo, rReader)) {
            return false;
        }

        param.blockIndex = mLoadingBufferBlockIndex;
        param.offsetSamples = 0;
        LoadStreamData(true, param, mAssignNumber, true, param.prefetchBlockIndex,
                       param.prefetchBlockBytes);

        u32 nextIndex = mLoadingBufferBlockIndex + 1;
        mLoadingBufferBlockIndex =
            nextIndex >= static_cast<u32>(mBufferBlockCount) ? 0 : nextIndex;

        if (param.isLastBlock) {
            break;
        }

        startSamplePosition += param.loadSamples;
    }

    return true;
}

/** @brief Starts playback unless it is delayed or already running. */
void StreamSoundPlayer::Start() {
    if (mUseDelayCount || mStartedFlag) {
        return;
    }

    StartPlayer();
}

/** @brief Configures and starts the voices of every active track. */
void StreamSoundPlayer::StartPlayer() {
    for (int trackNo = 0; trackNo < mTrackCount; trackNo++) {
        StreamTrack& rTrack = mTracks[trackNo];
        if (!rTrack.mActiveFlag) {
            continue;
        }

        for (int i = 0; i < rTrack.mTrackInfo.channelCount; i++) {
            StreamChannel* pChannel = rTrack.mChannels[i];
            if (pChannel == nullptr) {
                continue;
            }

            MultiVoice* pVoice = pChannel->mVoice;
            if (pVoice == nullptr) {
                continue;
            }

            pVoice->SetSampleFormat(mStreamDataInfo.sampleFormat);
            pVoice->SetSampleRate(mStreamDataInfo.sampleRate);
            pVoice->SetUpdateType(mUpdateType);
            pVoice->SetOutputReceiver(mOutputReceiver);
            pVoice->Start();
        }
    }

    UpdatePauseStatus();
    mStartedFlag = true;
}

/** @brief Stops playback. */
void StreamSoundPlayer::Stop() {
    FinishPlayer();
}

/**
 * @brief Pauses or resumes playback.
 * @param isPause Whether to pause.
 */
void StreamSoundPlayer::Pause(bool isPause) {
    mPauseFlag = isPause;
    if (isPause) {
        mLoadWaitFlag = true;
    }

    UpdatePauseStatus();
}

/** @brief Pauses the voices while the player is paused or waiting for data. */
void StreamSoundPlayer::UpdatePauseStatus() {
    bool isPause = mPauseFlag | mLoadWaitFlag;
    if (isPause == mPauseStatus) {
        return;
    }

    for (int channelNo = 0; channelNo < mChannelCount; channelNo++) {
        MultiVoice* pVoice = mChannels[channelNo].mVoice;
        if (pVoice != nullptr) {
            pVoice->Pause(isPause);
        }
    }

    mPauseStatus = isPause;
}

/**
 * @brief Checks whether playback ran out of data while the loader is still busy.
 * @return Whether playback is waiting for data.
 */
bool StreamSoundPlayer::IsLoadingDelayState() const {
    if (!mIsPrepared) {
        return false;
    }

    if (!mLoader->IsBusy()) {
        return false;
    }

    return IsBufferEmpty();
}

/**
 * @brief Checks whether no buffer block is queued or playing.
 * @return Whether every buffer block is free or done.
 */
bool StreamSoundPlayer::IsBufferEmpty() const {
    for (int blockNo = 0; blockNo < mBufferBlockCount; blockNo++) {
        WaveBuffer::Status status = mChannels[0].mWaveBuffer[blockNo].status;
        if (status == WaveBuffer::Status_Wait || status == WaveBuffer::Status_Play) {
            return false;
        }
    }

    return true;
}

/**
 * @brief Gets the loop and format parameters of the stream.
 * @param pInfo Receives the parameters.
 * @return Whether the stream header was loaded.
 */
bool StreamSoundPlayer::ReadStreamSoundDataInfo(StreamSoundDataInfo* pInfo) const {
    fnd::ScopedMutexLock lock(mMutex);

    if (!mIsPrepared && !mIsPreparedPrefetch) {
        return false;
    }

    pInfo->loopFlag = mStreamDataInfo.isLoop;
    pInfo->sampleRate = mStreamDataInfo.sampleRate;
    pInfo->loopStart = mStreamDataInfo.originalLoopStart;
    pInfo->loopEnd = mStreamDataInfo.originalLoopEnd;
    pInfo->compatibleLoopStart = mStreamDataInfo.loopStart;
    pInfo->compatibleLoopEnd = mStreamDataInfo.sampleCount;
    pInfo->channelCount = std::min(mStreamDataInfo.channelCount, ChannelCountMax);
    return true;
}

/**
 * @brief Gets the play position.
 * @param isOriginal Whether to map the position back onto the original, unextended loop.
 * @return Position in samples, or -1 when inactive.
 */
s64 StreamSoundPlayer::GetPlaySamplePosition(bool isOriginal) const {
    return isOriginal ? mPlayPosition.originalSamplePosition : mPlayPosition.samplePosition;
}

/**
 * @brief Gets how much of the buffer is filled with data not played yet.
 * @return Fill ratio in percent.
 */
float StreamSoundPlayer::GetFilledBufferPercentage() const {
    fnd::ScopedMutexLock lock(mMutex);

    float percentage = 0.0f;
    if (!mActiveFlag || !mTracks[0].mActiveFlag) {
        return percentage;
    }

    if (mPlayFinishFlag) {
        return 1.0f;
    }

    if (mIsPrepared) {
        size_t filledSamples = 0;
        size_t totalSamples = 0;
        for (int blockNo = 0; blockNo < mBufferBlockCount; blockNo++) {
            const WaveBuffer& rBuffer = mChannels[0].mWaveBuffer[blockNo];
            size_t samples = rBuffer.sampleLength;
            if (rBuffer.status == WaveBuffer::Status_Play) {
                filledSamples += samples - mChannels[0].mVoice->GetCurrentPlayingSample();
            } else if (rBuffer.status == WaveBuffer::Status_Wait) {
                filledSamples += samples;
            } else {
                samples = mStreamDataInfo.blockSampleCount;
            }

            totalSamples += samples;
        }

        percentage = static_cast<float>(filledSamples) * 100.0f / static_cast<float>(totalSamples);
    } else if (mBufferBlockCount != 0) {
        percentage = static_cast<float>(mBufferBlockCount - mPrepareCounter) * 100.0f /
                     static_cast<float>(mBufferBlockCount);
    }

    return percentage;
}

/**
 * @brief Counts the buffer blocks of every channel in a given state.
 * @param status State to count.
 * @return Number of buffer blocks.
 */
int StreamSoundPlayer::GetBufferBlockCount(WaveBuffer::Status status) const {
    int count = 0;
    for (int channelNo = 0; channelNo < mChannelCount; channelNo++) {
        for (int blockNo = 0; blockNo < mBufferBlockCount; blockNo++) {
            if (mChannels[channelNo].mWaveBuffer[blockNo].status == status) {
                count++;
            }
        }
    }

    return count;
}

/** @brief Gets the number of buffer blocks of every channel. @return Number of buffer blocks. */
int StreamSoundPlayer::GetTotalBufferBlockCount() const {
    return mChannelCount * mBufferBlockCount;
}

/**
 * @brief Handles the header loaded by the task thread and queues the first data loads.
 * @param result Whether the header was read.
 * @param ppAdpcmParam DSP ADPCM coefficients of every channel; entries may be null.
 * @param assignNumber Assign number of the request, to drop replies to an old sound.
 * @return Whether playback can continue.
 */
bool StreamSoundPlayer::LoadHeader(bool result, audio::AdpcmParameter** ppAdpcmParam,
                                   u16 assignNumber) {
    if (!mSetupFlag || mAssignNumber != assignNumber) {
        return false;
    }

    if (!result) {
        mFinishFlag = true;
        Stop();
        return false;
    }

    mChannelCount = std::min(mStreamDataInfo.channelCount, ChannelCountMax);

    if (mIsPreparedPrefetch) {
        for (int channelNo = 0; channelNo < mChannelCount; channelNo++) {
            if (mChannels[channelNo].mVoice == nullptr) {
                return false;
            }
        }

        if (!CheckPrefetchRevision(mStreamDataInfo)) {
            return false;
        }
    }

    if (!AllocStreamBuffers()) {
        Finalize();
        mIsFinalizedForCannotAllocateResource = true;
        return false;
    }

    if (!ApplyStreamDataInfo(mStreamDataInfo)) {
        return false;
    }

    if (mIsPreparedPrefetch) {
        SetPrepared(true);
    } else {
        if (!SetupPlayer()) {
            return false;
        }

        if (!AllocVoices()) {
            FreeStreamBuffers();
            return false;
        }

        mPrepareCounter = 0;
        for (int blockNo = 0; blockNo < mBufferBlockCount; blockNo++) {
            UpdateLoadingBlockIndex();
            mPrepareCounter++;
        }
    }

    for (int channelNo = 0; channelNo < mChannelCount; channelNo++) {
        if (ppAdpcmParam[channelNo] != nullptr) {
            mChannels[channelNo].mVoice->SetAdpcmParam(0, *ppAdpcmParam[channelNo]);
        }
    }

    return true;
}

/**
 * @brief Checks that the prefetch data was made from the same stream file.
 * @param rInfo Parameters of the stream file.
 * @return Whether the checksums agree or cannot be compared.
 */
bool StreamSoundPlayer::CheckPrefetchRevision(const StreamDataInfoDetail& rInfo) const {
    if (!mIsPrefetchRevisionCheckEnabled) {
        return true;
    }

    if (!rInfo.isCrc32CheckAvailable) {
        return true;
    }

    return rInfo.crc32 == mPrefetchRevisionValue;
}

/**
 * @brief Takes a buffer from the pool for every channel.
 * @return Whether every channel got a buffer; none is kept otherwise.
 */
bool StreamSoundPlayer::AllocStreamBuffers() {
    for (int channelNo = 0; channelNo < mChannelCount; channelNo++) {
        void* pBuffer = mBufferPool->Alloc();
        if (pBuffer == nullptr) {
            for (int i = 0; i < channelNo; i++) {
                mBufferPool->Free(mChannels[i].mBufferAddress);
                mChannels[i].mBufferAddress = nullptr;
            }

            return false;
        }

        mChannels[channelNo].mBufferAddress = pBuffer;
        mChannels[channelNo].mUpdateType = mUpdateType;
    }

    return true;
}

/** @brief Requests the data of the next buffer block from the loader. */
void StreamSoundPlayer::UpdateLoadingBlockIndex() {
    void* pBuffers[ChannelCountMax];
    for (int channelNo = 0; channelNo < mChannelCount; channelNo++) {
        pBuffers[channelNo] =
            static_cast<u8*>(mChannels[channelNo].mBufferAddress) +
            (mStreamDataInfo.blockSize + BufferBlockMargin) * mLoadingBufferBlockIndex;
    }

    size_t startOffsetSamples = 0;
    if (mStartOffsetType == StartOffsetType_Sample) {
        startOffsetSamples = mStartOffset;
    } else if (mStartOffsetType == StartOffsetType_MilliSeconds) {
        startOffsetSamples = MsecToSamples(mStartOffset, mStreamDataInfo.sampleRate);
    }

    size_t prefetchOffset = mPrefetchOffset;
    mStartOffset = 0;
    mPrefetchOffset = 0;
    mLoader->RequestLoadData(pBuffers, mLoadingBufferBlockIndex, startOffsetSamples,
                             prefetchOffset, mStartedFlag ? LoadPriorityPlay : LoadPriorityPrepare);

    u32 nextIndex = mLoadingBufferBlockIndex + 1;
    mLoadingBufferBlockIndex = nextIndex >= static_cast<u32>(mBufferBlockCount) ? 0 : nextIndex;
}

/**
 * @brief Marks whether the first buffer blocks are loaded.
 * @param isPrepared Whether playback can start.
 */
void StreamSoundPlayer::SetPrepared(bool isPrepared) {
    mIsPrepared = isPrepared;
    UpdatePlaySamplePosition();
}

/**
 * @brief Queues a block the task thread loaded from the stream file.
 * @param result Whether the block was read.
 * @param rParam Description of the block.
 * @param assignNumber Assign number of the request, to drop replies to an old sound.
 * @return Whether playback can continue.
 */
bool StreamSoundPlayer::LoadStreamData(bool result, const LoadDataParam& rParam,
                                       u16 assignNumber) {
    return LoadStreamData(result, rParam, assignNumber, false, 0, 0);
}

/**
 * @brief Queues a loaded block on the voice of every channel.
 * @param result Whether the block was read.
 * @param rParam Description of the block.
 * @param assignNumber Assign number of the request, to drop replies to an old sound.
 * @param usePrefetchData Whether the samples are played straight from the prefetch data.
 * @param prefetchBlockIndex Index of the block in the prefetch data.
 * @param prefetchBlockBytes Size of the block of one channel in the prefetch data.
 * @return Whether playback can continue.
 */
bool StreamSoundPlayer::LoadStreamData(bool result, const LoadDataParam& rParam,
                                       u16 assignNumber, bool usePrefetchData,
                                       u32 prefetchBlockIndex, size_t prefetchBlockBytes) {
    if (!mSetupFlag) {
        return false;
    }

    if (!result) {
        mFinishFlag = true;
        Stop();
        return false;
    }

    if (mAssignNumber != assignNumber) {
        return false;
    }

    if (!mLoadWaitFlag && IsStoppedByLoadingDelay()) {
        mIsStoppedByLoadingDelay = true;
        mIsLoadingDelay = true;
        mLoadWaitFlag = true;
        UpdatePauseStatus();
    }

    if (rParam.loadSamples != 0) {
        const size_t* pBlockSize =
            usePrefetchData ? &mStreamDataInfo.blockSize : &rParam.loadSize;
        for (int channelNo = 0; channelNo < mChannelCount; channelNo++) {
            StreamChannel& rChannel = mChannels[channelNo];

            const void* pData;
            if (usePrefetchData) {
                size_t blockOffset = static_cast<s64>(mChannelCount) * prefetchBlockIndex *
                                     mStreamDataInfo.blockSize;
                pData = static_cast<const u8*>(mPrefetchDataInfo.samples) + blockOffset +
                        channelNo * prefetchBlockBytes;
            } else {
                pData = static_cast<const u8*>(rChannel.mBufferAddress) +
                        (mStreamDataInfo.blockSize + BufferBlockMargin) * rParam.blockIndex;
            }

            WaveBuffer& rBuffer = rChannel.mWaveBuffer[rParam.blockIndex];
            AdpcmContext* pContext = nullptr;
            if (rParam.isAdpcmContextAvailable) {
                pContext = &rChannel.mAdpcmContext[rParam.blockIndex].context;
                std::memcpy(pContext, &rParam.adpcmContext[channelNo],
                            sizeof(AdpcmContextNotAligned));
            }

            rBuffer.Initialize();
            rBuffer.bufferAddress = pData;
            rBuffer.bufferSize = *pBlockSize;
            rBuffer.sampleLength = rParam.loadSamples;
            rBuffer.sampleOffset = rParam.offsetSamples;
            rBuffer.pAdpcmContext = pContext;
            rChannel.AppendWaveBuffer(&rBuffer, rParam.isLastBlock);
        }

        BlockInfo& rInfo = mBlockInfo[rParam.blockIndex];
        rInfo.startSamplePosition = rParam.startSamplePosition;
        rInfo.loadSamples = rParam.loadSamples;
        rInfo.loopCount = rParam.loopCount;
    }

    if (rParam.isLastBlock) {
        mPlayFinishFlag = true;
    }

    if (mIsPrepared || usePrefetchData) {
        return true;
    }

    mPrepareCounter--;
    if (mPrepareCounter == 0 || mPlayFinishFlag) {
        SetPrepared(true);
    }

    return true;
}

/**
 * @brief Checks whether the voices stopped because every queued block was played.
 * @return Whether playback stalled waiting for data.
 */
bool StreamSoundPlayer::IsStoppedByLoadingDelay() const {
    if (!mIsPrepared) {
        return false;
    }

    bool isStopped = false;
    for (int blockNo = 0; blockNo < mBufferBlockCount; blockNo++) {
        WaveBuffer::Status status = mChannels[0].mWaveBuffer[blockNo].status;
        if (status == WaveBuffer::Status_Free) {
            continue;
        }

        if (status != WaveBuffer::Status_Done) {
            return false;
        }

        isStopped = true;
    }

    return isStopped;
}

/**
 * @brief Forgets a channel's voice the renderer dropped or finished.
 * @param pVoice Voice the callback is for.
 * @param status Why the callback was invoked.
 * @param pArg Channel that owns the voice.
 */
void StreamSoundPlayer::VoiceCallbackFunc(MultiVoice* pVoice,
                                          MultiVoice::VoiceCallbackStatus status, void* pArg) {
    StreamChannel* pChannel = static_cast<StreamChannel*>(pArg);

    switch (status) {
    case MultiVoice::VoiceCallbackStatus_FinishWave:
    case MultiVoice::VoiceCallbackStatus_Cancel:
        pVoice->Free();
        pChannel->mVoice = nullptr;
        break;
    case MultiVoice::VoiceCallbackStatus_DropVoice:
    case MultiVoice::VoiceCallbackStatus_DropDsp:
        pChannel->mVoice = nullptr;
        break;
    default:
        break;
    }
}

/** @brief Advances the stream by one sound-thread frame. */
void StreamSoundPlayer::Update() {
    fnd::ScopedMutexLock lock(mMutex);

    if (!TryAllocLoader()) {
        return;
    }

    if (!mIsPrepareDone) {
        mLoader->Initialize();
        Setup(mSetupArg);
        if (mIsPreparePrefetchPending) {
            PreparePrefetch(mPreparePrefetchArg);
            mIsPreparePrefetchPending = false;
        }

        Prepare(mPrepareArg);
    }

    if (mLoader == nullptr) {
        return;
    }

    mLoader->Update();

    if (mDelayCount > 0) {
        mDelayCount--;
        return;
    }

    if (mUseDelayCount && mIsPrepared && !mStartedFlag) {
        StartPlayer();
    }

    UpdateBuffer();

    if (mStartedFlag) {
        for (int channelNo = 0; channelNo < mChannelCount; channelNo++) {
            if (mChannels[channelNo].mVoice == nullptr) {
                Stop();
                mFinishFlag = true;
                return;
            }
        }

        for (int trackNo = 0; trackNo < mTrackCount; trackNo++) {
            UpdateVoiceParams(&mTracks[trackNo]);
        }
    }

    if (mLoadWaitFlag && !mLoader->IsBusy() && !CheckDiskDriveError()) {
        mLoadWaitFlag = false;
        mIsLoadingDelay = false;
        UpdatePauseStatus();
    }

    if (mIsStoppedByLoadingDelay) {
        mIsStoppedByLoadingDelay = false;
    }
}

/** @brief Recycles played buffer blocks and updates the play position. */
void StreamSoundPlayer::UpdateBuffer() {
    if (!mStartedFlag || !mTracks[0].mActiveFlag) {
        return;
    }

    if (mIsPreparedPrefetch && !mIsPrepared) {
        return;
    }

    if (CheckDiskDriveError()) {
        mLoadWaitFlag = true;
        UpdatePauseStatus();
    }

    while (IsBufferBlockPlayed(mChannels, mChannelCount, mPlayingBufferBlockIndex)) {
        for (int channelNo = 0; channelNo < mChannelCount; channelNo++) {
            mChannels[channelNo].mWaveBuffer[mPlayingBufferBlockIndex].Initialize();
        }

        mLoopCounter += mBlockInfo[mPlayingBufferBlockIndex].loopCount;
        UpdateLoadingBlockIndex();

        mLastPlayFinishBufferBlockIndex = mPlayingBufferBlockIndex;
        u32 nextIndex = mPlayingBufferBlockIndex + 1;
        mPlayingBufferBlockIndex = nextIndex >= static_cast<u32>(mBufferBlockCount) ? 0 : nextIndex;
    }

    if (!mLoadWaitFlag && (IsLoadingDelayState() || mIsLoadingDelay)) {
        mIsStoppedByLoadingDelay = true;
        mIsLoadingDelay = true;
        mLoadWaitFlag = true;
        UpdatePauseStatus();
    }

    bool isPlayFinished = IsBufferEmpty() && mPlayFinishFlag;

    s64 position;
    if ((mIsLoadingDelay && IsLoadingDelayState()) || isPlayFinished) {
        int lastIndex = static_cast<int>(mPlayingBufferBlockIndex);
        if (lastIndex <= 0) {
            lastIndex = mBufferBlockCount;
        }

        const BlockInfo& rInfo = mBlockInfo[lastIndex - 1];
        position = rInfo.startSamplePosition + rInfo.loadSamples;
    } else {
        MultiVoice* pVoice = mChannels[0].mVoice;
        size_t playingSample = pVoice != nullptr ? pVoice->GetCurrentPlayingSample() : 0;
        position = mBlockInfo[mPlayingBufferBlockIndex].startSamplePosition + playingSample;
    }

    s64 originalPosition = position;
    int loopCount = 0;
    if (mStreamDataInfo.isLoop) {
        loopCount = GetOriginalLoopCount(position, mStreamDataInfo);
        originalPosition = GetOriginalPlaySamplePosition(position, mStreamDataInfo);
    }

    mOriginalLoopCounter = loopCount;
    UpdatePlaySamplePosition(position, originalPosition);
}

/**
 * @brief Recomputes the mixing parameters of the voices of a track.
 * @param pTrack Track to update.
 */
void StreamSoundPlayer::UpdateVoiceParams(StreamTrack* pTrack) {
    if (!pTrack->mActiveFlag) {
        return;
    }

    TrackData trackData;
    trackData.Set(pTrack);

    float volume = trackData.volume * mBaseVolume * pTrack->mVolume;

    float pitch = mBasePitch * mItemData.pitch;
    if (mFileType == StreamFileType_Opus && pitch > OpusPitchMax) {
        pitch = OpusPitchMax;
    }

    float lpfFreq = trackData.lpfFreq + mBaseLpfFreq;

    s8 biquadType = pTrack->mTrackInfo.biquadType;
    float biquadValue = trackData.biquadValue;
    if (mBaseBiquadType != BiquadFilterTypeInherit) {
        biquadType = mBaseBiquadType;
        biquadValue = mBaseBiquadValue;
    }

    u32 outputLine = pTrack->mOutputLine;
    if (pTrack->mOutputLine == OutputLineInherit) {
        outputLine = mBaseOutputLine;
    }

    OutputParam tvParam = mTvParam;
    SetOutputParam(&tvParam, pTrack->mTvParam, trackData);

    for (int i = 0; i < pTrack->mTrackInfo.channelCount; i++) {
        MultiVoice* pVoice = pTrack->mChannels[i]->mVoice;
        if (pVoice == nullptr) {
            continue;
        }

        pVoice->SetVolume(volume);
        pVoice->SetPitch(pitch);
        pVoice->SetLpfFreq(lpfFreq);
        pVoice->SetBiquadFilter(biquadType, biquadValue);
        pVoice->SetOutputLine(outputLine);
        pVoice->SetPanCurve(static_cast<PanCurve>(mPanCurve));
        pVoice->SetPanMode(static_cast<PanMode>(mPanMode));

        if (pTrack->mTrackInfo.channelCount == 1) {
            pVoice->SetVoiceMode(MultiVoice::VoiceMode_Mono);
            pVoice->SetTvParam(tvParam);
            if (mTvAdditionalParam != nullptr) {
                pVoice->SetTvAdditionalParam(*mTvAdditionalParam);
            }
        } else if (pTrack->mTrackInfo.channelCount == 2) {
            switch (i) {
            case 0:
                pVoice->SetVoiceMode(MultiVoice::VoiceMode_StereoLeft);
                break;
            case 1:
                pVoice->SetVoiceMode(MultiVoice::VoiceMode_StereoRight);
                break;
            default:
                NN_UNEXPECTED_DEFAULT;
            }

            ApplyTvOutputParamForMultiChannel(tvParam, mTvAdditionalParam, pVoice, i,
                                              static_cast<MixMode>(tvParam.mixMode));
        }
    }
}

/**
 * @brief Checks whether stream loads are held back by a disk drive error.
 * @return Whether loads are on hold.
 */
bool StreamSoundPlayer::CheckDiskDriveError() const {
    return SoundSystem::g_IsStreamLoadWait;
}

/**
 * @brief Converts the file parameters of a track to mixing values.
 * @param pTrack Track to read.
 */
void StreamSoundPlayer::TrackData::Set(const StreamTrack* pTrack) {
    const StreamTrack::TrackInfo& rInfo = pTrack->mTrackInfo;

    volume = LevelToRatio(rInfo.volume);
    lpfFreq = static_cast<float>(rInfo.lpfFreq) / 64.0f;
    biquadType = static_cast<s8>(rInfo.biquadType);
    biquadValue = LevelToRatio(rInfo.biquadValue);

    int panValue = rInfo.pan < 2 ? rInfo.pan - 63 : rInfo.pan - 64;
    pan = static_cast<float>(panValue) / 63.0f;

    if (rInfo.span <= 63) {
        span = static_cast<float>(rInfo.span) / 63.0f;
    } else {
        span = static_cast<float>(rInfo.span + 1) / 64.0f;
    }

    mainSend = LevelToRatio(rInfo.mainSend) - 1.0f;
    for (int bus = 0; bus < AuxBus_Count; bus++) {
        fxSend[bus] = LevelToRatio(rInfo.fxSend[bus]);
    }
}

/**
 * @brief Combines output parameters with a track's parameters and the archive sends.
 * @param pOutParam Parameters to combine into.
 * @param rParam Output parameters of the track.
 * @param rTrackData File parameters of the track.
 */
void StreamSoundPlayer::SetOutputParam(OutputParam* pOutParam, const OutputParam& rParam,
                                       const TrackData& rTrackData) {
    pOutParam->volume = rParam.volume * pOutParam->volume;
    for (int ch = 0; ch < OutputParam::WaveChannelMax; ch++) {
        for (int i = 0; i < ChannelIndex_Count; i++) {
            pOutParam->mixParameter[ch].ch[i] =
                rParam.mixParameter[ch].ch[i] * pOutParam->mixParameter[ch].ch[i];
        }
    }

    pOutParam->pan += rTrackData.pan + rParam.pan;
    pOutParam->span += rTrackData.span + rParam.span;

    pOutParam->mainSend = rParam.mainSend + pOutParam->mainSend;
    for (int bus = 0; bus < AuxBus_Count; bus++) {
        pOutParam->fxSend[bus] = rParam.fxSend[bus] + pOutParam->fxSend[bus];
    }

    pOutParam->mainSend += rTrackData.mainSend + mItemData.mainSend;
    for (int bus = 0; bus < AuxBus_Count; bus++) {
        pOutParam->fxSend[bus] += rTrackData.fxSend[bus] + mItemData.fxSend[bus];
    }
}

/**
 * @brief Applies output parameters to one voice of a stereo track.
 * @param rParam Output parameters of the track.
 * @param pAdditionalParam Additional sends and bus mix volumes, or nullptr.
 * @param pVoice Voice playing the channel.
 * @param channelIndex Channel of the track the voice plays.
 * @param mixMode How the channel is mixed.
 */
void StreamSoundPlayer::ApplyTvOutputParamForMultiChannel(
    const OutputParam& rParam, const OutputAdditionalParam* pAdditionalParam, MultiVoice* pVoice,
    int channelIndex, MixMode mixMode) {
    OutputParam tvParam = rParam;

    if (pAdditionalParam == nullptr) {
        MixSettingForOutputParam(&tvParam, nullptr, channelIndex, mixMode);
        pVoice->SetTvParam(tvParam);
        return;
    }

    if (pAdditionalParam->GetBusMixVolumePacketAddr() != nullptr) {
        OutputBusMixVolume busMixVolume = pAdditionalParam->GetBusMixVolume();
        MixSettingForOutputParam(&tvParam, &busMixVolume, channelIndex, mixMode);
        pVoice->SetTvParam(tvParam);
        pVoice->SetTvAdditionalParam(pAdditionalParam->GetAdditionalSendAddr(),
                                     pAdditionalParam->GetBusMixVolumePacketAddr(),
                                     &busMixVolume,
                                     pAdditionalParam->GetVolumeThroughModePacketAddr());
    } else {
        MixSettingForOutputParam(&tvParam, nullptr, channelIndex, mixMode);
        pVoice->SetTvParam(tvParam);
        pVoice->SetTvAdditionalParam(pAdditionalParam->GetAdditionalSendAddr(),
                                     pAdditionalParam->GetBusMixVolumePacketAddr(), nullptr,
                                     pAdditionalParam->GetVolumeThroughModePacketAddr());
    }
}

/**
 * @brief Routes the right channel of a mix-parameter stereo track through the left channel slot.
 * @param pParam Output parameters of the voice.
 * @param pBusMixVolume Bus mix volumes of the voice, or nullptr.
 * @param channelIndex Channel of the track the voice plays.
 * @param mixMode How the channel is mixed.
 */
void StreamSoundPlayer::MixSettingForOutputParam(OutputParam* pParam,
                                                 OutputBusMixVolume* pBusMixVolume,
                                                 int channelIndex, MixMode mixMode) {
    if (channelIndex != 1 || mixMode != MixMode_MixParameter) {
        return;
    }

    for (int i = 0; i < ChannelIndex_Count; i++) {
        pParam->mixParameter[0].ch[i] = pParam->mixParameter[1].ch[i];
        pParam->mixParameter[1].ch[i] = 0.0f;
    }

    if (pBusMixVolume == nullptr) {
        return;
    }

    for (int i = 0; i < BusMixVolumeCount; i++) {
        pBusMixVolume->volumes[0][i] = pBusMixVolume->volumes[1][i];
        pBusMixVolume->volumes[1][i] = 0.0f;
    }
}

/**
 * @brief Gets how often the original, unextended loop was played.
 * @param position Play position in the converted stream.
 * @param rInfo Stream parameters.
 * @return Loop count.
 */
int StreamSoundPlayer::GetOriginalLoopCount(long position,
                                            const StreamDataInfoDetail& rInfo) const {
    s64 loopEnd = rInfo.originalLoopEnd;
    if (loopEnd >= position) {
        return 0;
    }

    s64 loopLength = loopEnd - static_cast<s64>(rInfo.originalLoopStart);
    if (loopLength < OriginalLoopLengthMin) {
        return (position - static_cast<s64>(rInfo.originalLoopStart)) / loopLength;
    }

    return 1;
}

/**
 * @brief Maps a play position onto the original, unextended loop.
 * @param position Play position in the converted stream.
 * @param rInfo Stream parameters.
 * @return Position in the original stream.
 */
s64 StreamSoundPlayer::GetOriginalPlaySamplePosition(long position,
                                                     const StreamDataInfoDetail& rInfo) const {
    s64 loopEnd = rInfo.originalLoopEnd;
    if (loopEnd >= position) {
        return position;
    }

    s64 loopStart = rInfo.originalLoopStart;
    s64 loopLength = loopEnd - loopStart;
    s64 offset;
    if (loopLength < OriginalLoopLengthMin) {
        offset = (position - loopStart) % loopLength;
    } else {
        offset = position - loopEnd;
    }

    return offset + loopStart;
}

/**
 * @brief Records the play position, unless the stream is inactive or still loading.
 * @param position Play position in samples.
 * @param originalPosition Play position mapped onto the original loop.
 */
void StreamSoundPlayer::UpdatePlaySamplePosition(long position, long originalPosition) {
    if (!mActiveFlag || !mTracks[0].mActiveFlag) {
        mPlayPosition.originalSamplePosition = InvalidSamplePosition;
        mPlayPosition.samplePosition = InvalidSamplePosition;
    } else if (!mIsPrepared) {
        mPlayPosition.originalSamplePosition = 0;
        mPlayPosition.samplePosition = 0;
    } else {
        mPlayPosition.originalSamplePosition = originalPosition;
        mPlayPosition.samplePosition = position;
    }
}

/**
 * @brief Checks that the start offset lies inside a non-looping stream.
 * @param rInfo Stream parameters.
 * @return Whether playback can start at the offset.
 */
bool StreamSoundPlayer::IsValidStartOffset(const StreamDataInfoDetail& rInfo) {
    if (!rInfo.isLoop && GetStartOffsetSamples(rInfo) >= rInfo.sampleCount) {
        return false;
    }

    return true;
}

/**
 * @brief Copies the file parameters of every track and links the tracks to their channels.
 * @param rInfo Stream parameters.
 */
void StreamSoundPlayer::ApplyTrackDataInfo(const StreamDataInfoDetail& rInfo) {
    for (int trackNo = 0; trackNo < mTrackCount; trackNo++) {
        StreamTrack& rTrack = mTracks[trackNo];
        const StreamTrackDataInfo& rTrackInfo = rInfo.trackInfo[trackNo];

        rTrack.mTrackInfo.volume = rTrackInfo.volume;
        rTrack.mTrackInfo.pan = rTrackInfo.pan;
        rTrack.mTrackInfo.span = rTrackInfo.span;
        rTrack.mTrackInfo.mainSend = rTrackInfo.mainSend;
        for (int bus = 0; bus < AuxBus_Count; bus++) {
            rTrack.mTrackInfo.fxSend[bus] = rTrackInfo.fxSend[bus];
        }

        rTrack.mTrackInfo.lpfFreq = rTrackInfo.lpfFreq;
        rTrack.mTrackInfo.biquadType = rTrackInfo.biquadType;
        rTrack.mTrackInfo.biquadValue = rTrackInfo.biquadValue;
        rTrack.mTrackInfo.flags = rTrackInfo.flags;
        rTrack.mTrackInfo.channelCount = rTrackInfo.channelCount;

        for (int i = 0; i < rTrackInfo.channelCount; i++) {
            rTrack.mChannels[i] = &mChannels[rTrackInfo.globalChannelIndex[i]];
        }
    }
}

/**
 * @brief Converts the start offset to samples.
 * @param rInfo Stream parameters.
 * @return Start offset in samples.
 */
size_t StreamSoundPlayer::GetStartOffsetSamples(const StreamDataInfoDetail& rInfo) {
    switch (mStartOffsetType) {
    case StartOffsetType_Sample:
        return mStartOffset;
    case StartOffsetType_MilliSeconds:
        return MsecToSamples(mStartOffset, rInfo.sampleRate);
    default:
        return 0;
    }
}

/**
 * @brief Locates the last block and the loop inside the stream blocks.
 * @param rInfo Stream parameters.
 */
void StreamSoundPlayer::PrefetchIndexInfo::Initialize(const StreamDataInfoDetail& rInfo) {
    lastBlockIndex = (rInfo.sampleCount - 1) / rInfo.blockSampleCount;
    loopStartInBlock = rInfo.loopStart % rInfo.blockSampleCount;
    loopStartBlockIndex = (rInfo.loopStart - loopStartInBlock) / rInfo.blockSampleCount;
    loopBlockCount = lastBlockIndex + 1 - loopStartBlockIndex;
}

/**
 * @brief Describes the last block of the stream.
 * @param pParam Receives the description.
 * @param rIndexInfo Block positions of the stream.
 */
void StreamSoundPlayer::PreparePrefetchOnLastBlock(PrefetchLoadDataParam* pParam,
                                                   const PrefetchIndexInfo& rIndexInfo) {
    pParam->loadSamples = mStreamDataInfo.lastBlockSampleCount;
    pParam->prefetchBlockBytes = mStreamDataInfo.lastBlockSize;
    pParam->prefetchBlockIndex = rIndexInfo.lastBlockIndex;

    if (mStreamDataInfo.isLoop) {
        pParam->loopCount++;
        mPrefetchOffset = mStreamDataInfo.loopStart;
    } else {
        pParam->isLastBlock = true;
        mPrefetchOffset = 0;
    }
}

/**
 * @brief Describes the block the loop starts in.
 * @param pParam Receives the description.
 * @param rIndexInfo Block positions of the stream.
 * @param rReader Reader of the prefetch data.
 * @return Whether the ADPCM loop parameters could be read.
 */
bool StreamSoundPlayer::PreparePrefetchOnLoopStartBlock(PrefetchLoadDataParam* pParam,
                                                        const PrefetchIndexInfo& rIndexInfo,
                                                        StreamSoundPrefetchFileReader& rReader) {
    pParam->loadSamples = mStreamDataInfo.blockSampleCount;
    pParam->prefetchBlockBytes = mStreamDataInfo.blockSize;
    pParam->prefetchBlockIndex = rIndexInfo.loopStartBlockIndex;
    pParam->startSamplePosition = rIndexInfo.loopStartInBlock;
    mPrefetchOffset = mStreamDataInfo.blockSampleCount * (rIndexInfo.loopStartBlockIndex + 1);

    if (mStreamDataInfo.sampleFormat == SampleFormat_DspAdpcm) {
        if (!SetAdpcmLoopInfo(rReader, mStreamDataInfo, mPrefetchAdpcmParam,
                              pParam->adpcmContext)) {
            return false;
        }

        pParam->isAdpcmContextAvailable = true;
    }

    return true;
}

/**
 * @brief Describes a block inside the loop.
 * @param pParam Receives the description.
 * @param rIndexInfo Block positions of the stream.
 * @param loopBlockIndex Position of the block in the loop, counted from 1.
 */
void StreamSoundPlayer::PreparePrefetchOnLoopBlock(PrefetchLoadDataParam* pParam,
                                                   const PrefetchIndexInfo& rIndexInfo,
                                                   u32 loopBlockIndex) {
    pParam->loadSamples = mStreamDataInfo.blockSampleCount;
    pParam->prefetchBlockBytes = mStreamDataInfo.blockSize;
    pParam->prefetchBlockIndex = loopBlockIndex + rIndexInfo.loopStartBlockIndex - 1;
    mPrefetchOffset += mStreamDataInfo.blockSampleCount;
}

/**
 * @brief Describes a block before the end of the stream.
 * @param pParam Receives the description.
 * @param blockIndex Index of the block in the stream.
 * @param rReader Reader of the prefetch data.
 * @return Whether the ADPCM parameters of the first block could be read.
 */
bool StreamSoundPlayer::PreparePrefetchOnNormalBlock(PrefetchLoadDataParam* pParam,
                                                     u32 blockIndex,
                                                     StreamSoundPrefetchFileReader& rReader) {
    pParam->loadSamples = mStreamDataInfo.blockSampleCount;
    pParam->prefetchBlockBytes = mStreamDataInfo.blockSize;
    pParam->prefetchBlockIndex = blockIndex;
    mPrefetchOffset += mStreamDataInfo.blockSampleCount;

    if (mStreamDataInfo.sampleFormat == SampleFormat_DspAdpcm &&
        pParam->prefetchBlockIndex == 0) {
        if (!SetAdpcmInfo(rReader, mStreamDataInfo, mPrefetchAdpcmParam, pParam->adpcmContext)) {
            return false;
        }

        pParam->isAdpcmContextAvailable = true;
    }

    return true;
}

/**
 * @brief Reads the ADPCM coefficients and loop decoder state of every channel.
 * @param rReader Reader of the prefetch data.
 * @param rInfo Stream parameters.
 * @param pParam Receives the coefficients of every channel.
 * @param pContext Receives the loop decoder state of every channel.
 * @return Whether every channel could be read.
 */
NOINLINE bool StreamSoundPlayer::SetAdpcmLoopInfo(StreamSoundPrefetchFileReader& rReader,
                                         const StreamDataInfoDetail& rInfo,
                                         audio::AdpcmParameter* pParam,
                                         AdpcmContextNotAligned* pContext) {
    for (int channelNo = 0; channelNo < rInfo.channelCount; channelNo++) {
        DspAdpcmParam adpcmParam;
        DspAdpcmLoopParam loopParam;
        if (!rReader.ReadDspAdpcmChannelInfo(&adpcmParam, &loopParam, channelNo)) {
            return false;
        }

        pParam[channelNo] = adpcmParam.parameter;
        mChannels[channelNo].mVoice->SetAdpcmParam(0, pParam[channelNo]);
        pContext[channelNo].predictorScale = loopParam.context.predictorScale;
        pContext[channelNo].previousSample = loopParam.context.previousSample;
        pContext[channelNo].previousSample2 = loopParam.context.previousSample2;
    }

    return true;
}

/**
 * @brief Reads the ADPCM coefficients and initial decoder state of every channel.
 * @param rReader Reader of the prefetch data.
 * @param rInfo Stream parameters.
 * @param pParam Receives the coefficients of every channel.
 * @param pContext Receives the initial decoder state of every channel.
 * @return Whether every channel could be read.
 */
NOINLINE bool StreamSoundPlayer::SetAdpcmInfo(StreamSoundPrefetchFileReader& rReader,
                                     const StreamDataInfoDetail& rInfo,
                                     audio::AdpcmParameter* pParam,
                                     AdpcmContextNotAligned* pContext) {
    for (int channelNo = 0; channelNo < rInfo.channelCount; channelNo++) {
        DspAdpcmParam adpcmParam;
        DspAdpcmLoopParam loopParam;
        if (!rReader.ReadDspAdpcmChannelInfo(&adpcmParam, &loopParam, channelNo)) {
            return false;
        }

        pParam[channelNo] = adpcmParam.parameter;
        mChannels[channelNo].mVoice->SetAdpcmParam(0, pParam[channelNo]);
        pContext[channelNo].predictorScale = adpcmParam.context.predictorScale;
        pContext[channelNo].previousSample = adpcmParam.context.previousSample;
        pContext[channelNo].previousSample2 = adpcmParam.context.previousSample2;
    }

    return true;
}

/**
 * @brief Marks the player as holding live data.
 * @param isActive Whether the player is active.
 */
void StreamSoundPlayer::SetActiveFlag(bool isActive) {
    BasicSoundPlayer::SetActiveFlag(isActive);
    UpdatePlaySamplePosition();
}

/**
 * @brief Sets the volume of the selected tracks.
 * @param trackBitFlag Bit n selects track n.
 * @param volume Volume ratio.
 */
void StreamSoundPlayer::SetTrackVolume(u32 trackBitFlag, float volume) {
    for (int trackNo = 0; trackNo < mTrackCount; trackNo++) {
        if (trackBitFlag == 0) {
            break;
        }

        if ((trackBitFlag & 1) != 0) {
            mTracks[trackNo].mVolume = volume;
        }

        trackBitFlag >>= 1;
    }
}

/**
 * @brief Overrides the file volume of the selected tracks.
 * @param trackBitFlag Bit n selects track n.
 * @param volume Volume level in [0, 127].
 */
void StreamSoundPlayer::SetTrackInitialVolume(u32 trackBitFlag, u32 volume) {
    for (int trackNo = 0; trackNo < mTrackCount; trackNo++) {
        if (trackBitFlag == 0) {
            break;
        }

        if ((trackBitFlag & 1) != 0) {
            mTracks[trackNo].mTrackInfo.volume = volume;
        }

        trackBitFlag >>= 1;
    }
}

/**
 * @brief Sets the output line of the selected tracks.
 * @param trackBitFlag Bit n selects track n.
 * @param outputLine Output line bit mask.
 */
void StreamSoundPlayer::SetTrackOutputLine(u32 trackBitFlag, u32 outputLine) {
    for (int trackNo = 0; trackNo < mTrackCount; trackNo++) {
        if (trackBitFlag == 0) {
            break;
        }

        if ((trackBitFlag & 1) != 0) {
            mTracks[trackNo].mOutputLine = outputLine;
        }

        trackBitFlag >>= 1;
    }
}

/**
 * @brief Makes the selected tracks use the player's output line again.
 * @param trackBitFlag Bit n selects track n.
 */
void StreamSoundPlayer::ResetTrackOutputLine(u32 trackBitFlag) {
    for (int trackNo = 0; trackNo < mTrackCount; trackNo++) {
        if (trackBitFlag == 0) {
            break;
        }

        if ((trackBitFlag & 1) != 0) {
            mTracks[trackNo].mOutputLine = OutputLineInherit;
        }

        trackBitFlag >>= 1;
    }
}

/**
 * @brief Sets the TV volume of the selected tracks.
 * @param trackBitFlag Bit n selects track n.
 * @param volume Volume ratio.
 */
void StreamSoundPlayer::SetTrackTvVolume(u32 trackBitFlag, float volume) {
    for (int trackNo = 0; trackNo < mTrackCount; trackNo++) {
        if (trackBitFlag == 0) {
            break;
        }

        if ((trackBitFlag & 1) != 0) {
            mTracks[trackNo].mTvParam.volume = volume;
        }

        trackBitFlag >>= 1;
    }
}

/**
 * @brief Sets the TV mix parameters of one channel of the selected tracks.
 * @param trackBitFlag Bit n selects track n.
 * @param srcChannel Channel of the track.
 * @param rParam Mix volumes of every output channel.
 */
void StreamSoundPlayer::SetTrackChannelTvMixParameter(u32 trackBitFlag, u32 srcChannel,
                                                      const MixParameter& rParam) {
    for (int trackNo = 0; trackNo < mTrackCount; trackNo++) {
        if (trackBitFlag == 0) {
            break;
        }

        if ((trackBitFlag & 1) != 0) {
            MixParameter& rMix = mTracks[trackNo].mTvParam.mixParameter[srcChannel];
            for (int i = 0; i < ChannelIndex_Count; i++) {
                rMix.ch[i] = rParam.ch[i];
            }
        }

        trackBitFlag >>= 1;
    }
}

/**
 * @brief Sets the TV pan of the selected tracks.
 * @param trackBitFlag Bit n selects track n.
 * @param pan Pan, in [-1, 1].
 */
void StreamSoundPlayer::SetTrackTvPan(u32 trackBitFlag, float pan) {
    for (int trackNo = 0; trackNo < mTrackCount; trackNo++) {
        if (trackBitFlag == 0) {
            break;
        }

        if ((trackBitFlag & 1) != 0) {
            mTracks[trackNo].mTvParam.pan = pan;
        }

        trackBitFlag >>= 1;
    }
}

/**
 * @brief Sets the TV surround pan of the selected tracks.
 * @param trackBitFlag Bit n selects track n.
 * @param span Surround pan, in [0, 2].
 */
void StreamSoundPlayer::SetTrackTvSurroundPan(u32 trackBitFlag, float span) {
    for (int trackNo = 0; trackNo < mTrackCount; trackNo++) {
        if (trackBitFlag == 0) {
            break;
        }

        if ((trackBitFlag & 1) != 0) {
            mTracks[trackNo].mTvParam.span = span;
        }

        trackBitFlag >>= 1;
    }
}

/**
 * @brief Sets the TV main send of the selected tracks.
 * @param trackBitFlag Bit n selects track n.
 * @param send Send level offset.
 */
void StreamSoundPlayer::SetTrackTvMainSend(u32 trackBitFlag, float send) {
    for (int trackNo = 0; trackNo < mTrackCount; trackNo++) {
        if (trackBitFlag == 0) {
            break;
        }

        if ((trackBitFlag & 1) != 0) {
            mTracks[trackNo].mTvParam.mainSend = send;
        }

        trackBitFlag >>= 1;
    }
}

/**
 * @brief Sets a TV effect send of the selected tracks.
 * @param trackBitFlag Bit n selects track n.
 * @param bus Effect bus.
 * @param send Send level offset.
 */
void StreamSoundPlayer::SetTrackTvFxSend(u32 trackBitFlag, AuxBus bus, float send) {
    for (int trackNo = 0; trackNo < mTrackCount; trackNo++) {
        if (trackBitFlag == 0) {
            break;
        }

        if ((trackBitFlag & 1) != 0) {
            mTracks[trackNo].mTvParam.fxSend[bus] = send;
        }

        trackBitFlag >>= 1;
    }
}

/**
 * @brief Gets a track.
 * @param trackNo Track number.
 * @return The track, or nullptr when the number is out of range.
 */
StreamTrack* StreamSoundPlayer::GetPlayerTrack(int trackNo) {
    if (static_cast<u32>(trackNo) >= TrackCountMax) {
        return nullptr;
    }

    return &mTracks[trackNo];
}

/**
 * @brief Gets a track.
 * @param trackNo Track number.
 * @return The track, or nullptr when the number is out of range.
 */
const StreamTrack* StreamSoundPlayer::GetPlayerTrack(int trackNo) const {
    if (static_cast<u32>(trackNo) >= TrackCountMax) {
        return nullptr;
    }

    return &mTracks[trackNo];
}
}  // namespace nn::atk::detail::driver
