#include <nn/atk/detail/strm/atk_StreamSoundLoader.h>

#include <cstring>
#include <new>
#include <nn/atk/atk_HardwareManager.h>
#include <nn/atk/atk_MemoryFileStream.h>
#include <nn/atk/atk_SoundArchiveFilesHook.h>
#include <nn/atk/atk_StreamSoundPlayer.h>
#include <nn/atk/atk_StreamTrack.h>
#include <nn/atk/atk_TaskManager.h>
#include <nn/atk/atk_Util.h>
#include <nn/atk/atk_WaveFileReader.h>
#include <nn/atk/atkfnd_FileStreamImpl.h>
#include <nn/diag.h>
#include <nn/util.h>
#include <nn/util/util_FormatString.h>

// Release-build form of the SDK abort macro: the message strings are compiled out.
#define NN_ABORT()                                                                                 \
    do {                                                                                           \
        ::nn::diag::detail::AbortImpl("", "", "", 0);                                              \
        __builtin_unreachable();                                                                   \
    } while (false)

namespace nn::atk::detail {
namespace {
using DecoderManagerList =
    util::IntrusiveList<IStreamDataDecoderManager,
                        util::IntrusiveListMemberNodeTraits<IStreamDataDecoderManager,
                                                            &IStreamDataDecoderManager::mNode>>;

/** @brief Decoder managers registered with RegisterStreamDataDecoderManager. */
DecoderManagerList sDecoderManagerList;

/** @brief Result values a failed file stream open reports. */
enum FsResult : u32 {
    FsResult_Unknown = 0x81000000,
    FsResult_PathNotFound,
    FsResult_DataCorrupted,
    FsResult_TargetLocked,
};

/**
 * @brief Checks whether a file stream operation succeeded.
 * @param result Result of the operation.
 * @return True when the operation succeeded.
 */
bool IsSucceeded(fnd::FndResult result) {
    return static_cast<s32>(result.value) >= 0;
}

/**
 * @brief Formats the name of the decoder a decode mode needs.
 * @param pBuffer Destination of the name.
 * @param bufferSize Size of pBuffer in bytes.
 * @param mode Decode mode.
 */
void FormatDecoderName(char* pBuffer, size_t bufferSize, DecodeMode mode) {
    switch (mode) {
    case DecodeMode_Cpu:
        util::SNPrintf(pBuffer, bufferSize, "OpusDecoder");
        break;
    case DecodeMode_Accelerator:
        util::SNPrintf(pBuffer, bufferSize, "HardwareOpusDecoder");
        break;
    default:
        util::SNPrintf(pBuffer, bufferSize, "Decoder");
        break;
    }
}
}  // namespace

/**
 * @brief Copies the playback parameters of a stream sound file.
 * @param rInfo Stream information read from the file's info block.
 * @param isCrc32CheckAvailable Whether the file stores a sample data checksum.
 */
void StreamDataInfoDetail::SetStreamSoundInfo(const StreamSoundFile::StreamSoundInfo& rInfo,
                                              bool isCrc32CheckAvailable) {
    sampleFormat = static_cast<SampleFormat>(WaveFileReader::GetSampleFormat(rInfo.sampleFormat));
    sampleRate = rInfo.sampleRate;
    isLoop = rInfo.loop;
    loopStart = rInfo.loopStart;
    sampleCount = rInfo.loopEnd;
    originalLoopStart = rInfo.originalLoopStart;
    originalLoopEnd = rInfo.originalLoopEnd;
    blockSampleCount = rInfo.blockSampleCount;
    blockSize = rInfo.blockSize;
    lastBlockSampleCount = rInfo.lastBlockSampleCount;
    lastBlockSize = rInfo.lastBlockPaddedSize;
    crc32 = rInfo.crc32;
    this->isCrc32CheckAvailable = isCrc32CheckAvailable;
    regionCount = rInfo.regionCount;
    preSkipSampleCount = 0;
}

namespace driver {
u8 StreamSoundLoader::g_LoadBuffer[LoadBufferSize];

/** @brief Constructs an idle loader with an empty data load task pool. */
StreamSoundLoader::StreamSoundLoader()
    : mFileLoader(nullptr), mFileStream(nullptr), mFilesHook(nullptr), mItemLabel(nullptr),
      mDataLoadTaskPool(), mDecoder(nullptr), mDecoderManager(nullptr) {
    mDataLoadTaskPool.Create(mDataLoadTaskPoolBuffer, sizeof(mDataLoadTaskPoolBuffer));
    std::memset(mFilePath, 0, FilePathMax);
}

/** @brief Waits for every task and releases the decoder and the task pool. */
StreamSoundLoader::~StreamSoundLoader() {
    WaitFinalize();
    ReleaseDecoder();
    mDataLoadTaskPool.Destroy();
}

/** @brief Waits until every task of the loader has finished and releases the data load tasks. */
void StreamSoundLoader::WaitFinalize() {
    mHeaderLoadTask.Wait();
    mCloseTask.Wait();
    ReleaseDataLoadTasks(true);
}

/** @brief Resets the playback state before a new stream is loaded. */
void StreamSoundLoader::Initialize() {
    mLoadingDataBlockIndex = 0;
    mLastBlockIndex = 0xffffffff;
    mLoopStartBlockIndex = 0;
    mLoopStartFilePos = 0;
    mLoopStartBlockSampleOffset = 0;
    mLoopJumpFlag = false;
    mLoadFinishFlag = false;
    mIsPreDecodeEnabled = true;
    mRegionManager.Initialize();
    mSampleFormat = SampleFormat_DspAdpcm;
    mDecodeMode = DecodeMode_Invalid;
    mDecoder = nullptr;
    mDecoderManager = nullptr;
}

/** @brief Cancels the pending tasks and requests the file to be closed. */
void StreamSoundLoader::Finalize() {
    CancelRequest();
    RequestClose();
}

/** @brief Cancels every task of the loader that has not started yet. */
void StreamSoundLoader::CancelRequest() {
    TaskManager::GetInstance().CancelTaskById(reinterpret_cast<uintptr_t>(this));
}

/** @brief Requests the task thread to close the file. */
void StreamSoundLoader::RequestClose() {
    mCloseTask.Wait();
    mCloseTask.mLoader = this;
    TaskManager::GetInstance().AppendTask(&mCloseTask, TaskManager::TaskPriority_Normal);
}

/**
 * @brief Makes a decoder manager available to stream sounds.
 * @param pManager Manager to register; must stay alive until it is unregistered.
 */
void StreamSoundLoader::RegisterStreamDataDecoderManager(IStreamDataDecoderManager* pManager) {
    sDecoderManagerList.push_back(*pManager);
}

/**
 * @brief Removes a decoder manager registered with RegisterStreamDataDecoderManager.
 * @param pManager Manager to unregister.
 */
void StreamSoundLoader::UnregisterStreamDataDecoderManager(IStreamDataDecoderManager* pManager) {
    sDecoderManagerList.erase(sDecoderManagerList.iterator_to(*pManager));
}

/**
 * @brief Sets the log notified around file system reads.
 * @param pFsAccessLog Log to notify, or nullptr to stop logging.
 * @return Result of the file stream, or nullptr when the stream cannot log.
 */
fnd::FileStream* StreamSoundLoader::detail_SetFsAccessLog(fnd::FsAccessLog* pFsAccessLog) {
    if (mFileStream != nullptr && mFileStream->CanSetFsAccessLog()) {
        return mFileStream->SetFsAccessLog(pFsAccessLog);
    }

    return nullptr;
}

/** @brief Gets the read position of the cached file stream. @return Position, or 0. */
size_t StreamSoundLoader::detail_GetCurrentPosition() {
    if (mFileStream != nullptr && mFileStream->IsCacheEnabled()) {
        return mFileStream->GetCurrentPosition();
    }

    return 0;
}

/** @brief Gets the file position the stream cache starts at. @return Position, or 0. */
size_t StreamSoundLoader::detail_GetCachePosition() {
    if (mFileStream != nullptr && mFileStream->IsCacheEnabled()) {
        return mFileStream->GetCachePosition();
    }

    return 0;
}

/** @brief Gets the number of bytes in the stream cache. @return Cached length, or 0. */
size_t StreamSoundLoader::detail_GetCachedLength() {
    if (mFileStream != nullptr && mFileStream->IsCacheEnabled()) {
        return mFileStream->GetCachedLength();
    }

    return 0;
}

/** @brief Requests the task thread to open the file and read its header. */
void StreamSoundLoader::RequestLoadHeader() {
    mHeaderLoadTask.mLoader = this;
    mHeaderLoadTask.SetId(reinterpret_cast<uintptr_t>(this));
    TaskManager::GetInstance().AppendTask(&mHeaderLoadTask, TaskManager::TaskPriority_Normal);
}

/**
 * @brief Requests the task thread to load the next buffer block.
 * @param ppBuffer Destination buffer of every channel.
 * @param bufferBlockIndex Index of the buffer block being filled.
 * @param startOffsetSamples Samples to skip from the current position.
 * @param prefetchOffsetSamples Samples already played from prefetched data.
 * @param priority Task priority.
 */
void StreamSoundLoader::RequestLoadData(void** ppBuffer, u32 bufferBlockIndex,
                                        long startOffsetSamples, long prefetchOffsetSamples,
                                        int priority) {
    StreamDataLoadTask* pTask = mDataLoadTaskPool.Alloc();
    pTask->mBufferBlockIndex = bufferBlockIndex;
    pTask->mStartOffsetSamples = startOffsetSamples;
    pTask->mPrefetchOffsetSamples = prefetchOffsetSamples;
    pTask->mLoader = this;
    pTask->SetId(reinterpret_cast<uintptr_t>(this));

    for (int i = 0; i < mChannelCount; i++) {
        pTask->mBuffer[i] = ppBuffer[i];
    }

    mDataLoadTaskList.push_back(*pTask);
    TaskManager::GetInstance().AppendTask(pTask,
                                          static_cast<TaskManager::TaskPriority>(priority));
}

/** @brief Releases the data load tasks the task thread has finished. */
void StreamSoundLoader::Update() {
    ReleaseDataLoadTasks(false);
}

/** @brief Asks the stream sound player to stop because its data cannot be loaded. */
void StreamSoundLoader::ForceFinish() {
    DriverCommand& rCommandManager = DriverCommand::GetInstanceForTaskThread();
    auto* pCommand = static_cast<DriverCommandStreamSoundForceFinish*>(
        rCommandManager.AllocMemory(sizeof(DriverCommandStreamSoundForceFinish), false));
    pCommand->type = 55;
    pCommand->player = mPlayer;
    rCommandManager.PushCommand(pCommand);
    rCommandManager.FlushCommand(true, false);
}

/** @brief Checks whether data load tasks are pending. @return True while data is loading. */
bool StreamSoundLoader::IsBusy() const {
    if (mLoadFinishFlag) {
        return false;
    }

    return !mDataLoadTaskList.empty();
}

/** @brief Checks whether any task still uses the loader. @return True while tasks run. */
bool StreamSoundLoader::IsInUse() {
    Update();

    if (!mHeaderLoadTask.TryWait()) {
        return true;
    }

    if (!mCloseTask.TryWait()) {
        return true;
    }

    return !mDataLoadTaskList.empty();
}

/**
 * @brief Opens the stream file from memory, the files hook or the file system.
 * @return Result of opening the file.
 */
fnd::FndResult StreamSoundLoader::Open() {
    if (mFileAddress != nullptr) {
        mFileStream = new (mFileStreamBuffer) MemoryFileStream(mFileAddress, mFileSize);
    } else {
        if (mFilesHook != nullptr) {
            mFileStream = mFilesHook->GetIsEnable() ?
                              mFilesHook->OpenFile(mFileStreamBuffer, sizeof(mFileStreamBuffer),
                                                   mCacheBuffer, mCacheSize, mItemLabel,
                                                   SoundArchiveFilesHook::FileTypeStreamBinary) :
                              nullptr;
        }

        if (mFileStream == nullptr) {
            auto* pFileStream = new (mFileStreamBuffer) fnd::FileStreamImpl();
            fnd::FndResult result = pFileStream->Open(mFilePath, fnd::FileStream::AccessMode_Read);
            if (!IsSucceeded(result)) {
                return result;
            }

            if (pFileStream->IsOpened() && IsCacheEnabled()) {
                pFileStream->EnableCache(mCacheBuffer, mCacheSize);
            }

            mFileStream = pFileStream;
        }
    }

    mFileLoader.Reset(mFileStream);
    return {0};
}

/** @brief Releases the decoder and closes the file. */
void StreamSoundLoader::Close() {
    ReleaseDecoder();

    if (mFileStream != nullptr) {
        mFileStream->Close();
        mFileStream = nullptr;
        mFileLoader.Reset(nullptr);
    }
}

/** @brief Reads the stream header and reports it to the stream sound player. */
void StreamSoundLoader::LoadHeader() {
    DriverCommand& rCommandManager = DriverCommand::GetInstanceForTaskThread();
    auto* pCommand = static_cast<DriverCommandStreamSoundLoadHeader*>(
        rCommandManager.AllocMemory(sizeof(DriverCommandStreamSoundLoadHeader), false));
    pCommand->type = 53;
    pCommand->player = mPlayer;
    pCommand->assignNumber = mAssignNumber;

    bool result;
    switch (mFileType) {
    case StreamFileType_Bfstm:
        result = LoadHeader1(pCommand);
        break;
    case StreamFileType_Opus:
        result = LoadHeaderForOpus(pCommand, StreamFileType_Opus, mDecodeMode);
        break;
    default:
        result = false;
        break;
    }

    pCommand->result = result;
    rCommandManager.PushCommand(pCommand);
    rCommandManager.FlushCommand(true, false);
}

/**
 * @brief Reads the header of a bfstm file.
 * @param pCommand Reply receiving the ADPCM parameters of every channel.
 * @return True when the header was read.
 */
bool StreamSoundLoader::LoadHeader1(DriverCommandStreamSoundLoadHeader* pCommand) {
    StreamSoundFileReader reader;
    if (!mFileLoader.LoadFileHeader(&reader, g_LoadBuffer, sizeof(g_LoadBuffer))) {
        return false;
    }

    StreamSoundFile::StreamSoundInfo info;
    if (!reader.ReadStreamSoundInfo(&info)) {
        return false;
    }

    if (mIsLoopFlagEnabled) {
        info.loop = mLoopFlag;
    }

    u32 channelCount = reader.GetChannelCount();
    mChannelCount = channelCount;
    mDataInfo->channelCount = channelCount;
    mDataInfo->SetStreamSoundInfo(info, reader.IsCrc32CheckAvailable());

    if (reader.IsTrackInfoAvailable()) {
        if (!ReadTrackInfoFromStreamSoundFile(reader)) {
            return false;
        }
    }

    mSampleFormat = mDataInfo->sampleFormat;
    if (mSampleFormat == SampleFormat_DspAdpcm) {
        if (!SetAdpcmInfo(reader, channelCount, pCommand->adpcmParam)) {
            return false;
        }
    } else {
        for (u32 i = 0; i < channelCount; i++) {
            pCommand->adpcmParam[i] = nullptr;
        }
    }

    mDataStartFilePos = reader.GetSampleDataOffset();
    mLastBlockIndex = (mDataInfo->sampleCount - 1) / mDataInfo->blockSampleCount;
    mLoopStartBlockIndex = mDataInfo->loopStart / mDataInfo->blockSampleCount;
    mLoopStartFilePos =
        mDataStartFilePos + mDataInfo->blockSize * mLoopStartBlockIndex * mChannelCount;
    mLoopStartBlockSampleOffset = 0;
    mDataInfo->isRegionIndexCheckAvailable = reader.IsRegionIndexCheckAvailable();

    if (!mRegionManager.InitializeRegion(&mFileLoader, mDataInfo)) {
        return false;
    }

    UpdateLoadingDataBlockIndex();
    return true;
}

/**
 * @brief Reads the header of an Opus stream file.
 * @param pCommand Reply whose ADPCM parameters are cleared.
 * @param type Stream file type selecting the decoder manager.
 * @param mode Decode mode selecting the decoder manager.
 * @return True when the header was read.
 */
bool StreamSoundLoader::LoadHeaderForOpus(DriverCommandStreamSoundLoadHeader* pCommand,
                                          StreamFileType type, DecodeMode mode) {
    if (mode == DecodeMode_Default) {
        mode = DecodeMode_Cpu;
    }

    mDecoderManager = SelectStreamDataDecoderManager(type, mode);
    if (mDecoderManager == nullptr) {
        return false;
    }

    if (mDecoder == nullptr) {
        mDecoder = mDecoderManager->AllocDecoder();
        if (mDecoder == nullptr) {
            return false;
        }
    }

    IStreamDataDecoder::DataInfo dataInfo;
    if (!mDecoder->ReadHeaderInfo(&dataInfo, mFileStream)) {
        return false;
    }

    u32 channelCount = dataInfo.channelCount;
    mChannelCount = channelCount;
    mDataInfo->channelCount = channelCount;
    SetStreamSoundInfoForOpus(dataInfo);
    mLastBlockIndex = (mDataInfo->sampleCount - 1) / mDataInfo->blockSampleCount;

    if (mDataInfo->isLoop) {
        mDataInfo->lastBlockSampleCount =
            mDataInfo->sampleCount - mDataInfo->blockSampleCount * mLastBlockIndex;
    } else {
        mDataInfo->lastBlockSampleCount = mDataInfo->blockSampleCount;
    }

    for (u32 i = 0; i < channelCount; i++) {
        pCommand->adpcmParam[i] = nullptr;
    }

    u32 loopStartBlockIndex = mDataInfo->loopStart / mDataInfo->blockSampleCount;
    size_t loopStartBlockSampleOffset =
        mDataInfo->loopStart - loopStartBlockIndex * mDataInfo->blockSampleCount;

    if (mDataInfo->isLoop) {
        size_t sampleCount = mDataInfo->sampleCount;
        if (static_cast<s64>(loopStartBlockSampleOffset) > 0) {
            size_t padding = mDataInfo->blockSampleCount - loopStartBlockSampleOffset;
            mDataInfo->loopStart += padding;
            mDataInfo->lastBlockSampleCount += padding;
            sampleCount += padding;
        }

        mDataInfo->loopStart += mDataInfo->preSkipSampleCount;
        mDataInfo->lastBlockSampleCount += mDataInfo->preSkipSampleCount;
        sampleCount += mDataInfo->preSkipSampleCount;
        while (mDataInfo->lastBlockSampleCount > mDataInfo->blockSampleCount) {
            mDataInfo->lastBlockSampleCount -= mDataInfo->blockSampleCount;
        }

        mLastBlockIndex = (sampleCount - 1) / mDataInfo->blockSampleCount;
        loopStartBlockIndex = mDataInfo->loopStart / mDataInfo->blockSampleCount;
        loopStartBlockSampleOffset =
            mDataInfo->loopStart - loopStartBlockIndex * mDataInfo->blockSampleCount;
    }

    mLoopStartBlockIndex = loopStartBlockIndex;
    mLoopStartBlockSampleOffset = loopStartBlockSampleOffset;
    mDataInfo->isRegionIndexCheckAvailable = false;
    return mRegionManager.InitializeRegion(&mFileLoader, mDataInfo);
}

/**
 * @brief Copies the mixing parameters of every track.
 * @param rReader Reader of the stream file header.
 * @return True when every track was read.
 */
bool StreamSoundLoader::ReadTrackInfoFromStreamSoundFile(StreamSoundFileReader& rReader) {
    u32 trackCount = rReader.GetTrackCount();
    if (trackCount >= StreamDataInfoDetail::TrackCountMax) {
        trackCount = StreamDataInfoDetail::TrackCountMax;
    }

    for (u32 i = 0; i < trackCount; i++) {
        StreamSoundFileReader::TrackInfo trackInfo;
        trackInfo.volume = 0;
        trackInfo.pan = 0;
        trackInfo.channelCount = 0;
        trackInfo.channels[0] = 0;
        trackInfo.channels[1] = 0;
        if (!rReader.ReadStreamTrackInfo(&trackInfo, i)) {
            return false;
        }

        mDataInfo->trackInfo[i].volume = trackInfo.volume;
        mDataInfo->trackInfo[i].pan = trackInfo.pan;
        mDataInfo->trackInfo[i].channelCount = trackInfo.channelCount;
        for (u32 j = 0; j < trackInfo.channelCount; j++) {
            mDataInfo->trackInfo[i].globalChannelIndex[j] = trackInfo.channels[j];
        }

        mDataInfo->trackInfo[i].span = 0;
        mDataInfo->trackInfo[i].flags = 0;
        mDataInfo->trackInfo[i].mainSend = 127;
        mDataInfo->trackInfo[i].fxSend[0] = 0;
        mDataInfo->trackInfo[i].fxSend[1] = 0;
        mDataInfo->trackInfo[i].fxSend[2] = 0;
        mDataInfo->trackInfo[i].lpfFreq = 64;
        mDataInfo->trackInfo[i].biquadType = 0;
        mDataInfo->trackInfo[i].biquadValue = 0;
    }

    return true;
}

/**
 * @brief Copies the DSP ADPCM parameters of every channel.
 * @param rReader Reader of the stream file header.
 * @param channelCount Number of channels.
 * @param ppParam Receives the decoder coefficients of every channel.
 * @return True when every channel was read.
 */
bool StreamSoundLoader::SetAdpcmInfo(StreamSoundFileReader& rReader, int channelCount,
                                     audio::AdpcmParameter** ppParam) {
    for (int i = 0; i < channelCount; i++) {
        DspAdpcmParam param;
        DspAdpcmLoopParam loopParam;
        if (!rReader.ReadDspAdpcmChannelInfo(&param, &loopParam, i)) {
            return false;
        }

        mAdpcmInfo[i].param = param.parameter;
        mAdpcmInfo[i].beginContext.predictorScale = param.context.predictorScale;
        mAdpcmInfo[i].beginContext.previousSample = param.context.previousSample;
        mAdpcmInfo[i].beginContext.previousSample2 = param.context.previousSample2;
        mAdpcmInfo[i].loopContext.predictorScale = loopParam.context.predictorScale;
        mAdpcmInfo[i].loopContext.previousSample = loopParam.context.previousSample;
        mAdpcmInfo[i].loopContext.previousSample2 = loopParam.context.previousSample2;
        ppParam[i] = &mAdpcmInfo[i].param;
    }

    return true;
}

/** @brief Moves the file to the block containing the current sample position. */
void StreamSoundLoader::UpdateLoadingDataBlockIndex() {
    mLoadingDataBlockIndex = mRegionManager.GetCurrentPosition() / mDataInfo->blockSampleCount;
    mFileStream->Seek(mDataStartFilePos +
                          mDataInfo->blockSize * mLoadingDataBlockIndex * mChannelCount,
                      fnd::Stream::SeekOrigin_Begin);
}

/**
 * @brief Finds the registered decoder manager for a file type and decode mode.
 * @param type Stream file type.
 * @param mode Decode mode.
 * @return Matching manager, or nullptr when none is registered.
 */
IStreamDataDecoderManager* StreamSoundLoader::SelectStreamDataDecoderManager(StreamFileType type,
                                                                             DecodeMode mode) {
    for (auto& rManager : sDecoderManagerList) {
        if (rManager.GetStreamFileType() == type && rManager.GetDecodeMode() == mode) {
            return &rManager;
        }
    }

    char decoderName[32];
    FormatDecoderName(decoderName, sizeof(decoderName), mode);
    return nullptr;
}

/**
 * @brief Copies the playback parameters of an Opus stream.
 * @param rInfo Stream information read by the decoder.
 */
void StreamSoundLoader::SetStreamSoundInfoForOpus(const IStreamDataDecoder::DataInfo& rInfo) {
    mDataInfo->sampleFormat = SampleFormat_PcmS16;
    mDataInfo->sampleRate = rInfo.sampleRate;
    mDataInfo->isLoop = mIsLoopFlagEnabled && mLoopFlag;

    mDataInfo->loopStart = mLoopStart;
    mDataInfo->sampleCount = mLoopEnd;
    mDataInfo->originalLoopStart = mLoopStart;
    mDataInfo->originalLoopEnd = mLoopEnd;
    mDataInfo->blockSampleCount = rInfo.blockSampleCount;
    mDataInfo->blockSize = rInfo.blockSize;
    mDataInfo->lastBlockSize = mDataInfo->blockSize;
    mDataInfo->preSkipSampleCount = rInfo.preSkipSampleCount;
}

/**
 * @brief Loads the next buffer block and reports it to the stream sound player.
 * @param ppBuffer Destination buffer of every channel.
 * @param bufferBlockIndex Index of the buffer block being filled.
 * @param startOffsetSamples Samples to skip from the current position.
 * @param prefetchOffsetSamples Samples already played from prefetched data.
 * @param rLogger Logger receiving the task profile.
 */
void StreamSoundLoader::LoadData(void** ppBuffer, u32 bufferBlockIndex, size_t startOffsetSamples,
                                 size_t prefetchOffsetSamples, TaskProfileLogger& rLogger) {
    if (mLoadFinishFlag) {
        return;
    }

    DriverCommand& rCommandManager = DriverCommand::GetInstanceForTaskThread();
    auto* pCommand = static_cast<DriverCommandStreamSoundLoadData*>(
        rCommandManager.AllocMemory(sizeof(DriverCommandStreamSoundLoadData), false));
    pCommand->type = 54;
    pCommand->assignNumber = mAssignNumber;

    bool result = false;
    if (mFileStream != nullptr) {
        switch (mFileType) {
        case StreamFileType_Bfstm:
            result = LoadData1(pCommand, ppBuffer, bufferBlockIndex, startOffsetSamples,
                               prefetchOffsetSamples, rLogger);
            break;
        case StreamFileType_Opus:
            result = LoadDataForOpus(pCommand, ppBuffer, bufferBlockIndex, startOffsetSamples,
                                     prefetchOffsetSamples, rLogger);
            break;
        default:
            break;
        }
    }

    pCommand->result = result;
    pCommand->player = mPlayer;
    rCommandManager.PushCommand(pCommand);
    rCommandManager.FlushCommand(true, false);
}

/**
 * @brief Loads the next buffer block of a bfstm file.
 * @param pCommand Reply describing the loaded data.
 * @param ppBuffer Destination buffer of every channel.
 * @param bufferBlockIndex Index of the buffer block being filled.
 * @param startOffsetSamples Samples to skip from the current position.
 * @param prefetchOffsetSamples Samples already played from prefetched data.
 * @param rLogger Logger receiving the task profile.
 * @return False when the file could not be read.
 */
bool StreamSoundLoader::LoadData1(DriverCommandStreamSoundLoadData* pCommand, void** ppBuffer,
                                  u32 bufferBlockIndex, size_t startOffsetSamples,
                                  size_t prefetchOffsetSamples, TaskProfileLogger& rLogger) {
    os::Tick beginTick = os::GetSystemTick();
    bool isStartOffsetBlock = prefetchOffsetSamples == 0;
    bool hasStartOffset = startOffsetSamples != 0 || prefetchOffsetSamples != 0;
    int loopCount = 0;
    bool isAdpcmStartOffset = false;

    if (hasStartOffset) {
        if (!ApplyStartOffset(startOffsetSamples + prefetchOffsetSamples, &loopCount)) {
            pCommand->loadSamples = 0;
            return true;
        }

        UpdateLoadingDataBlockIndex();
        isAdpcmStartOffset = mSampleFormat == SampleFormat_DspAdpcm;
    }

    s64 startPosition = mRegionManager.GetCurrentPosition();
    pCommand->isStartOffsetOverRegion = false;

    size_t loadSamples = 0;
    size_t loadSize = 0;
    size_t offsetSamples = 0;
    bool isFirstBlock = true;

    while (loadSamples < MinimumLoadSampleCount ||
           static_cast<size_t>(mRegionManager.GetCurrentRegionEndPosition() -
                               mRegionManager.GetCurrentPosition()) < MinimumLoadSampleCount) {
        BlockInfo blockInfo = {};
        CalculateBlockInfo(blockInfo);

        if (isStartOffsetBlock) {
            offsetSamples = blockInfo.startOffsetSamples - blockInfo.startOffsetSamplesAlign;
            if (isAdpcmStartOffset) {
                if (!LoadAdpcmContextForStartOffset()) {
                    return false;
                }
            }
        }

        if (IsCacheEnabled()) {
            if (!LoadOneBlockDataViaCache(ppBuffer, blockInfo, loadSize, isStartOffsetBlock,
                                          isAdpcmStartOffset)) {
                return false;
            }
        } else {
            if (!LoadOneBlockData(ppBuffer, blockInfo, loadSize, isStartOffsetBlock,
                                  isAdpcmStartOffset)) {
                return false;
            }
        }

        mRegionManager.AddPosition(blockInfo.samples);
        loadSamples += blockInfo.samples;
        loadSize += blockInfo.copyByte;
        mLoadingDataBlockIndex++;

        if (mRegionManager.GetCurrentPosition() == mRegionManager.GetCurrentRegionEndPosition()) {
            if (MoveNextRegion(&loopCount)) {
                UpdateLoadingDataBlockIndex();
            }

            if (hasStartOffset && isFirstBlock && loadSamples < MinimumLoadSampleCount) {
                pCommand->isStartOffsetOverRegion = true;
            }

            break;
        }

        isStartOffsetBlock = false;
        isFirstBlock = false;
    }

    pCommand->isAdpcmContextAvailable = false;
    if (mSampleFormat == SampleFormat_DspAdpcm) {
        if (startPosition == 0) {
            for (int i = 0; i < mChannelCount; i++) {
                pCommand->adpcmContext[i] = mAdpcmInfo[i].beginContext;
            }

            pCommand->isAdpcmContextAvailable = true;
        } else if (mDataInfo->isLoop &&
                   startPosition == static_cast<s64>(mDataInfo->loopStart)) {
            for (int i = 0; i < mChannelCount; i++) {
                pCommand->adpcmContext[i] = mAdpcmInfo[i].loopContext;
            }

            pCommand->isAdpcmContextAvailable = true;
        } else if (startPosition == mRegionManager.GetAdpcmContextForStartOffsetFrame()) {
            for (int i = 0; i < mChannelCount; i++) {
                pCommand->adpcmContext[i] = mRegionManager.GetAdpcmContextForStartOffset(i);
            }

            pCommand->isAdpcmContextAvailable = true;
        }
    }

    pCommand->bufferBlockIndex = bufferBlockIndex;
    pCommand->loadSamples = loadSamples;
    pCommand->startSamplePosition = startPosition;
    pCommand->loopCount = loopCount;
    pCommand->offsetSamples = offsetSamples;
    pCommand->loadSize = loadSize;
    pCommand->isLastBlock = mLoadFinishFlag;

    if (rLogger.IsProfilingEnabled()) {
        os::Tick endTick = os::GetSystemTick();
        TaskProfile profile;
        profile.SetType(TaskProfile::TaskProfileType_LoadStreamBlock);

        IStreamDataDecoder::CacheProfile cacheProfile = {};
        if (IsCacheEnabled()) {
            cacheProfile.cachePosition = detail_GetCachePosition();
            cacheProfile.cachedLength = detail_GetCachedLength();
            cacheProfile.currentPosition = detail_GetCurrentPosition();
            cacheProfile.player = mPlayer;
        }

        profile.GetLoadStreamBlock().SetData(beginTick, endTick, cacheProfile);
        rLogger.Record(profile);
    }

    return true;
}

/**
 * @brief Decodes the next block of an Opus stream file.
 * @param pCommand Reply describing the decoded data.
 * @param ppBuffer Destination buffer of every channel.
 * @param bufferBlockIndex Index of the buffer block being filled.
 * @param startOffsetSamples Samples to skip from the current position.
 * @param prefetchOffsetSamples Unused; Opus streams are not prefetched.
 * @param rLogger Logger receiving the task profile.
 * @return False when the data could not be decoded.
 */
bool StreamSoundLoader::LoadDataForOpus(DriverCommandStreamSoundLoadData* pCommand,
                                        void** ppBuffer, u32 bufferBlockIndex,
                                        size_t startOffsetSamples, size_t prefetchOffsetSamples,
                                        TaskProfileLogger& rLogger) {
    os::Tick beginTick = os::GetSystemTick();
    bool isProfiling = false;
    if (rLogger.IsProfilingEnabled() && mDecoder != nullptr) {
        mDecoder->ResetDecodeProfile();
        isProfiling = true;
    }

    os::Tick seekTick(0);
    if (mLoadingDataBlockIndex > mLastBlockIndex && mDataInfo->isLoop) {
        mLoadingDataBlockIndex = mLoopStartBlockIndex;
        os::Tick seekBeginTick = os::GetSystemTick();
        mFileStream->Seek(mLoopStartFilePos, fnd::Stream::SeekOrigin_Begin);
        os::Tick seekEndTick = os::GetSystemTick();

        if (mIsPreDecodeEnabled && mLoopStartBlockIndex != 0) {
            if (!DecodeStreamData(ppBuffer, IStreamDataDecoder::DecodeType_Idling)) {
                return false;
            }
        } else {
            ResetDecoder();
        }

        seekTick = os::Tick(seekEndTick.GetInt64Value() - seekBeginTick.GetInt64Value());
        mRegionManager.SetPosition(mDataInfo->loopStart);
        mLoopJumpFlag = true;
    }

    int loopCount = 0;
    size_t offsetSamples;
    if (startOffsetSamples != 0) {
        if (!ApplyStartOffset(startOffsetSamples, &loopCount)) {
            pCommand->loadSamples = 0;
            return true;
        }

        UpdateLoadingDataBlockIndexForOpus(ppBuffer);
        offsetSamples = mRegionManager.GetCurrentPosition() -
                        mDataInfo->blockSampleCount * mLoadingDataBlockIndex;
    } else if (mLoopJumpFlag) {
        offsetSamples = mLoopStartBlockSampleOffset;
    } else {
        offsetSamples = 0;
    }

    if (IsLoopStartFilePos(mLoadingDataBlockIndex)) {
        mLoopStartFilePos = mFileStream->GetCurrentPosition();
    }

    bool isLastBlock = mLoadingDataBlockIndex == mLastBlockIndex;
    size_t blockSamples =
        isLastBlock ? mDataInfo->lastBlockSampleCount : mDataInfo->blockSampleCount;
    s64 startPosition = mRegionManager.GetCurrentPosition();
    size_t loadSamples = static_cast<s64>(mFileStream->GetCurrentPosition()) <
                                 static_cast<s64>(mFileStream->GetSize()) ?
                             blockSamples :
                             0;

    if (loadSamples != 0) {
        if (!DecodeStreamData(ppBuffer, isLastBlock ? IStreamDataDecoder::DecodeType_Last :
                                                      IStreamDataDecoder::DecodeType_Normal)) {
            return false;
        }

        for (int i = 0; i < mChannelCount; i++) {
            HardwareManager::FlushDataCache(ppBuffer[i], loadSamples * sizeof(s16));
        }

        if (!mDataInfo->isLoop && static_cast<s64>(mFileStream->GetCurrentPosition()) >=
                                      static_cast<s64>(mFileStream->GetSize())) {
            mLastBlockIndex = mLoadingDataBlockIndex;
        }
    }

    mRegionManager.AddPosition(loadSamples);
    mLoopJumpFlag = false;
    mLoadingDataBlockIndex++;

    if (mLoadingDataBlockIndex > mLastBlockIndex) {
        if (mDataInfo->isLoop) {
            loopCount++;
        } else {
            mLoadFinishFlag = true;
        }
    }

    pCommand->isAdpcmContextAvailable = false;
    pCommand->bufferBlockIndex = bufferBlockIndex;
    pCommand->loadSamples = loadSamples;
    pCommand->startSamplePosition = startPosition;
    pCommand->loopCount = loopCount;
    pCommand->offsetSamples = offsetSamples;
    pCommand->loadSize = loadSamples * sizeof(s16);
    pCommand->isLastBlock = mLoadFinishFlag;

    if (isProfiling) {
        os::Tick endTick = os::GetSystemTick();
        IStreamDataDecoder::DecodeProfile decodeProfile = mDecoder->GetDecodeProfile();
        decodeProfile.fsAccessTicks = os::Tick(decodeProfile.fsAccessTicks.GetInt64Value() +
                                               seekTick.GetInt64Value());

        TaskProfile profile;
        profile.SetType(TaskProfile::TaskProfileType_LoadOpusStreamBlock);

        IStreamDataDecoder::CacheProfile cacheProfile = {};
        if (IsCacheEnabled()) {
            cacheProfile.cachePosition = detail_GetCachePosition();
            cacheProfile.cachedLength = detail_GetCachedLength();
            cacheProfile.currentPosition = detail_GetCurrentPosition();
            cacheProfile.player = mPlayer;
        }

        profile.GetLoadOpusStreamBlock().SetData(beginTick, endTick, decodeProfile, cacheProfile);
        rLogger.Record(profile);
    }

    return true;
}

/**
 * @brief Advances the playback position, following region jumps.
 * @param offset Samples to advance.
 * @param pLoopCount Incremented for every region jump.
 * @return False when the stream ended before the position was reached.
 */
bool StreamSoundLoader::ApplyStartOffset(long offset, int* pLoopCount) {
    while (mRegionManager.GetCurrentPosition() + offset >=
           mRegionManager.GetCurrentRegionEndPosition()) {
        s64 restSamples =
            mRegionManager.GetCurrentRegionEndPosition() - mRegionManager.GetCurrentPosition();
        if (!mRegionManager.TryMoveNextRegion(&mFileLoader, mDataInfo)) {
            mLoadFinishFlag = true;
            return false;
        }

        offset -= restSamples;
        (*pLoopCount)++;
    }

    mRegionManager.AddPosition(offset);
    return true;
}

/**
 * @brief Computes the extents of the block at the current position.
 * @param rBlockInfo Receives the block extents.
 */
void StreamSoundLoader::CalculateBlockInfo(BlockInfo& rBlockInfo) {
    if (mLoadingDataBlockIndex == mLastBlockIndex) {
        rBlockInfo.size = mDataInfo->lastBlockSize;
        rBlockInfo.samples = mDataInfo->lastBlockSampleCount;
    } else {
        rBlockInfo.size = mDataInfo->blockSize;
        rBlockInfo.samples = mDataInfo->blockSampleCount;
    }

    rBlockInfo.startOffsetSamples =
        mRegionManager.GetCurrentPosition() % mDataInfo->blockSampleCount;
    rBlockInfo.startOffsetSamplesAlign = rBlockInfo.startOffsetSamples;

    if (mSampleFormat == SampleFormat_DspAdpcm) {
        rBlockInfo.startOffsetSamplesAlign =
            static_cast<int>(rBlockInfo.startOffsetSamples / AdpcmSamplesPerFrame) *
            static_cast<int>(AdpcmSamplesPerFrame);
    }

    rBlockInfo.startOffsetByte =
        Util::GetByteBySample(rBlockInfo.startOffsetSamplesAlign, mSampleFormat);
    rBlockInfo.copyByte = rBlockInfo.size - rBlockInfo.startOffsetByte;
    rBlockInfo.samples -= rBlockInfo.startOffsetSamples;

    if (mRegionManager.GetCurrentPosition() + rBlockInfo.samples >
        mRegionManager.GetCurrentRegionEndPosition()) {
        rBlockInfo.samples =
            mRegionManager.GetCurrentRegionEndPosition() - mRegionManager.GetCurrentPosition();
        if (static_cast<size_t>(rBlockInfo.samples) < MinimumLoadSampleCount) {
            rBlockInfo.copyByte = MinimumLoadSampleCount * sizeof(s16);
        }
    }
}

/**
 * @brief Reads the ADPCM decoder history of the current block from the seek block.
 * @return True when the seek block was read.
 */
bool StreamSoundLoader::LoadAdpcmContextForStartOffset() {
    size_t currentPosition = mFileStream->GetCurrentPosition();
    u16 yn1[ChannelCountMax];
    u16 yn2[ChannelCountMax];
    if (!mFileLoader.ReadSeekBlockData(yn1, yn2, mLoadingDataBlockIndex, mChannelCount)) {
        return false;
    }

    mFileStream->Seek(currentPosition, fnd::Stream::SeekOrigin_Begin);

    for (int i = 0; i < mChannelCount; i++) {
        mRegionManager.GetAdpcmContextForStartOffset(i).previousSample = yn1[i];
        mRegionManager.GetAdpcmContextForStartOffset(i).previousSample2 = yn2[i];
    }

    return true;
}

/**
 * @brief Reads the current block of every channel directly into the channel buffers.
 * @param ppBuffer Destination buffer of every channel.
 * @param rBlockInfo Extents of the block.
 * @param destOffset Byte offset in the channel buffers.
 * @param isStartOffsetBlock Whether the block contains the start offset.
 * @param isAdpcmStartOffset Whether ADPCM contexts must be computed for the start offset.
 * @return True when every channel was read.
 */
bool StreamSoundLoader::LoadOneBlockDataViaCache(void** ppBuffer, const BlockInfo& rBlockInfo,
                                                 long destOffset, bool isStartOffsetBlock,
                                                 bool isAdpcmStartOffset) {
    for (int i = 0; i < mChannelCount; i++) {
        if (mPlayer->IsTaskCancelled()) {
            return false;
        }

        u8* pDest = static_cast<u8*>(ppBuffer[i]) + destOffset;
        if (rBlockInfo.startOffsetByte != 0) {
            SkipStreamBuffer(rBlockInfo.startOffsetByte);
        }

        if (!LoadStreamBuffer(pDest, rBlockInfo.copyByte)) {
            return false;
        }

        HardwareManager::FlushDataCache(pDest, rBlockInfo.copyByte);

        if (isStartOffsetBlock && isAdpcmStartOffset) {
            UpdateAdpcmInfoForStartOffset(pDest, i, rBlockInfo);
        }
    }

    return true;
}

/**
 * @brief Reads the current block of every channel through the shared load buffer.
 * @param ppBuffer Destination buffer of every channel.
 * @param rBlockInfo Extents of the block.
 * @param destOffset Byte offset in the channel buffers.
 * @param isStartOffsetBlock Whether the block contains the start offset.
 * @param isAdpcmStartOffset Whether ADPCM contexts must be computed for the start offset.
 * @return True when every channel was read.
 */
bool StreamSoundLoader::LoadOneBlockData(void** ppBuffer, const BlockInfo& rBlockInfo,
                                         long destOffset, bool isStartOffsetBlock,
                                         bool isAdpcmStartOffset) {
    for (int channel = 0; channel < mChannelCount;) {
        if (mPlayer->IsTaskCancelled()) {
            return false;
        }

        int loadChannelCount = GetLoadChannelCount(channel);
        if (!LoadStreamBuffer(g_LoadBuffer, rBlockInfo, loadChannelCount)) {
            return false;
        }

        for (int i = 0; i < loadChannelCount; i++, channel++) {
            const u8* pSource = g_LoadBuffer + rBlockInfo.size * i;
            u8* pDest = static_cast<u8*>(ppBuffer[channel]) + destOffset;
            std::memcpy(pDest, pSource + rBlockInfo.startOffsetByte, rBlockInfo.copyByte);
            HardwareManager::FlushDataCache(pDest, rBlockInfo.copyByte);

            if (isStartOffsetBlock && isAdpcmStartOffset) {
                UpdateAdpcmInfoForStartOffset(pSource, channel, rBlockInfo);
            }
        }
    }

    return true;
}

/**
 * @brief Moves the playback position to the next region.
 * @param pLoopCount Incremented when the position moved.
 * @return False when the stream has no next region.
 */
bool StreamSoundLoader::MoveNextRegion(int* pLoopCount) {
    if (mRegionManager.TryMoveNextRegion(&mFileLoader, mDataInfo)) {
        (*pLoopCount)++;
        return true;
    }

    mLoadFinishFlag = true;
    return false;
}

/**
 * @brief Decodes the next block into the channel buffers.
 * @param ppBuffer Destination buffer of every channel.
 * @param type How the block is decoded.
 * @return True when the block was decoded.
 */
bool StreamSoundLoader::DecodeStreamData(void** ppBuffer, IStreamDataDecoder::DecodeType type) {
    bool result = false;
    if (mDecoderManager != nullptr) {
        void* buffer[ChannelCountMax];
        for (int i = 0; i < mChannelCount; i++) {
            buffer[i] = ppBuffer[i];
        }

        result = mDecoder->Decode(buffer, mFileStream, mChannelCount, type);
    }

    return result;
}

/** @brief Clears the decoder state before decoding restarts at another position. */
void StreamSoundLoader::ResetDecoder() {
    if (mDecoderManager != nullptr) {
        mDecoder->Reset();
    }
}

/**
 * @brief Moves an Opus stream to the block containing the current sample position.
 * @param ppBuffer Channel buffers the block before the position is decoded into.
 */
void StreamSoundLoader::UpdateLoadingDataBlockIndexForOpus(void** ppBuffer) {
    mLoadingDataBlockIndex = mRegionManager.GetCurrentPosition() / mDataInfo->blockSampleCount;

    for (u32 i = 0; i < mLoadingDataBlockIndex; i++) {
        if (IsLoopStartFilePos(i)) {
            mLoopStartFilePos = mFileStream->GetCurrentPosition();
        }

        if (i == mLoadingDataBlockIndex - 1) {
            if (i != 0) {
                DecodeStreamData(ppBuffer, IStreamDataDecoder::DecodeType_Idling);
            }
        } else {
            mDecoder->Skip(mFileStream);
        }
    }
}

/**
 * @brief Checks whether the loop restarts reading at a block.
 * @param blockIndex Block index.
 * @return True when reading restarts at the block.
 */
bool StreamSoundLoader::IsLoopStartFilePos(u32 blockIndex) {
    if (mLoopStartBlockIndex == 0) {
        return blockIndex == 0;
    }

    if (mIsPreDecodeEnabled) {
        return blockIndex == mLoopStartBlockIndex - 1;
    }

    return blockIndex == mLoopStartBlockIndex;
}

/**
 * @brief Gets how many channels fit into the load buffer.
 * @param channelIndex First channel to load.
 * @return Number of channels to load at once.
 */
int StreamSoundLoader::GetLoadChannelCount(int channelIndex) {
    return channelIndex + 2 > mChannelCount ? mChannelCount - channelIndex : 2;
}

/**
 * @brief Reads the current block of several channels.
 * @param pBuffer Destination.
 * @param rBlockInfo Extents of the block.
 * @param loadChannelCount Number of channels to read.
 * @return True when everything was read.
 */
bool StreamSoundLoader::LoadStreamBuffer(u8* pBuffer, const BlockInfo& rBlockInfo,
                                         u32 loadChannelCount) {
    size_t size = rBlockInfo.size * loadChannelCount;
    return mFileStream->Read(pBuffer, size, nullptr) == size;
}

/**
 * @brief Reads from the file.
 * @param pBuffer Destination.
 * @param size Number of bytes to read.
 * @return True when everything was read.
 */
bool StreamSoundLoader::LoadStreamBuffer(u8* pBuffer, size_t size) {
    return mFileStream->Read(pBuffer, size, nullptr) == size;
}

/**
 * @brief Skips bytes of the file.
 * @param size Number of bytes to skip.
 * @return True when the file position moved.
 */
bool StreamSoundLoader::SkipStreamBuffer(size_t size) {
    return IsSucceeded(mFileStream->Seek(size, fnd::Stream::SeekOrigin_Current));
}

/**
 * @brief Computes the ADPCM decoder state at the start offset.
 * @param pData Start of the channel's block data.
 * @param channelIndex Channel index.
 * @param rBlockInfo Extents of the block.
 */
void StreamSoundLoader::UpdateAdpcmInfoForStartOffset(const void* pData, int channelIndex,
                                                      const BlockInfo& rBlockInfo) {
    AdpcmContext& rContext = mRegionManager.GetAdpcmContextForStartOffset(channelIndex);
    rContext.predictorScale = *static_cast<const u8*>(pData);
    u32 offset = static_cast<u32>(rBlockInfo.startOffsetSamples / AdpcmSamplesPerFrame) *
                 static_cast<u32>(AdpcmSamplesPerFrame);
    MultiVoice::CalcOffsetAdpcmParam(&rContext, mAdpcmInfo[channelIndex].param, offset, pData);
    mRegionManager.SetAdpcmContextForStartOffsetFrame(mRegionManager.GetCurrentPosition());
}

// The task functions are inline: they were emitted only through the task vtables.

/** @brief Destroys the task. */
inline StreamSoundLoader::StreamHeaderLoadTask::~StreamHeaderLoadTask() {}

/**
 * @brief Opens the file and reads its header; stops the sound when the file cannot be opened.
 * @param rLogger Unused.
 */
inline void StreamSoundLoader::StreamHeaderLoadTask::Execute(TaskProfileLogger& rLogger) {
    fnd::FndResult result = mLoader->Open();
    if (IsSucceeded(result)) {
        mLoader->LoadHeader();
        return;
    }

    char resultName[32];
    switch (result.value) {
    case FsResult_Unknown:
        util::SNPrintf(resultName, sizeof(resultName), "");
        break;
    case FsResult_PathNotFound:
        util::SNPrintf(resultName, sizeof(resultName), "fs::ResultPathNotFound");
        break;
    case FsResult_DataCorrupted:
        util::SNPrintf(resultName, sizeof(resultName), "fs::ResultDataCorrupted");
        break;
    case FsResult_TargetLocked:
        util::SNPrintf(resultName, sizeof(resultName), "fs::ResultTargetLocked");
        break;
    default:
        NN_UNEXPECTED_DEFAULT;
    }

    if (mLoader->mIsAbortOnOpenFailure) {
        NN_ABORT();
    }

    mLoader->ForceFinish();
}

/** @brief Destroys the task. */
inline StreamSoundLoader::StreamCloseTask::~StreamCloseTask() {}

/**
 * @brief Closes the file.
 * @param rLogger Unused.
 */
inline void StreamSoundLoader::StreamCloseTask::Execute(TaskProfileLogger& rLogger) {
    mLoader->Close();
}

/** @brief Destroys the task. */
inline StreamSoundLoader::StreamDataLoadTask::~StreamDataLoadTask() {}

/**
 * @brief Loads the requested buffer block.
 * @param rLogger Logger receiving the task profile.
 */
inline void StreamSoundLoader::StreamDataLoadTask::Execute(TaskProfileLogger& rLogger) {
    mLoader->LoadData(mBuffer, mBufferBlockIndex, mStartOffsetSamples, mPrefetchOffsetSamples,
                      rLogger);
}
}  // namespace driver

// The file loader functions below were compiled separately from the stream sound loader and are
// called rather than inlined by it.

/**
 * @brief Reads the header and info block of a stream sound file.
 * @param pReader Reader initialized with the loaded header.
 * @param pBuffer Destination of the header and info block.
 * @param bufferSize Size of pBuffer in bytes.
 * @return True when the header was read.
 */
NOINLINE bool StreamSoundFileLoader::LoadFileHeader(StreamSoundFileReader* pReader, void* pBuffer,
                                           size_t bufferSize) {
    static const size_t HeaderSize = 0x60;

    mFileStream->Seek(0, fnd::Stream::SeekOrigin_Begin);

    u8 headerBuffer[HeaderSize + 64];
    void* pHeader = reinterpret_cast<void*>(
        (reinterpret_cast<uintptr_t>(headerBuffer) + 63) & ~static_cast<uintptr_t>(63));
    if (mFileStream->Read(pHeader, HeaderSize, nullptr) != HeaderSize) {
        return false;
    }

    if (!StreamSoundFileReader::IsValidFileHeader(pHeader)) {
        return false;
    }

    const auto* pFileHeader = static_cast<const StreamSoundFile::FileHeader*>(pHeader);
    u32 loadSize = pFileHeader->GetInfoBlockOffset() + pFileHeader->GetInfoBlockSize();
    if (loadSize > bufferSize) {
        return false;
    }

    mFileStream->Seek(0, fnd::Stream::SeekOrigin_Begin);
    if (mFileStream->Read(pBuffer, loadSize, nullptr) != static_cast<s32>(loadSize)) {
        return false;
    }

    pReader->Initialize(pBuffer);
    mSeekBlockOffset = pReader->GetSeekBlockOffset();
    mRegionBlockOffset = pReader->GetRegionDataOffset();
    mRegionInfoBytes = pReader->GetRegionInfoBytes();
    return true;
}

/**
 * @brief Reads the ADPCM decoder history of a block from the seek block.
 * @param pYn1 Receives the last sample of every channel.
 * @param pYn2 Receives the second to last sample of every channel.
 * @param blockIndex Block index.
 * @param channelCount Number of channels.
 * @return True when the seek block was read.
 */
NOINLINE bool StreamSoundFileLoader::ReadSeekBlockData(u16* pYn1, u16* pYn2, int blockIndex,
                                              int channelCount) {
    static const size_t SeekBufferSize = 0xc0;
    static const size_t ReadSizeMax = 0x40;

    size_t readSize = sizeof(u16) * 2 * channelCount;
    long position = mSeekBlockOffset + readSize * blockIndex + sizeof(BinaryBlockHeader);
    mFileStream->Seek(position, fnd::Stream::SeekOrigin_Begin);
    if (readSize > ReadSizeMax) {
        return false;
    }

    u8 seekBuffer[SeekBufferSize + 64];
    auto* pSeekData = reinterpret_cast<const u16*>(
        (reinterpret_cast<uintptr_t>(seekBuffer) + 63) & ~static_cast<uintptr_t>(63));
    if (mFileStream->Read(const_cast<u16*>(pSeekData), readSize, nullptr) != readSize) {
        return false;
    }

    for (int i = 0; i < channelCount; i++) {
        pYn1[i] = pSeekData[i * 2];
        pYn2[i] = pSeekData[i * 2 + 1];
    }

    return true;
}

/**
 * @brief Read a stream region's playback information.
 * @param pInfo Destination for the region information; must not be null.
 * @param index Zero-based region index in the stream resource.
 * @return Whether the region information was read successfully.
 */
bool StreamSoundFileLoader::ReadRegionInfo(StreamSoundFile::RegionInfo* pInfo, u32 index) const {
    if (mRegionBlockOffset == 0 || mRegionInfoBytes == 0) {
        return false;
    }

    mFileStream->Seek(mRegionBlockOffset + static_cast<size_t>(mRegionInfoBytes) * index,
                      fnd::Stream::SeekOrigin_Begin);

    u8 regionBuffer[sizeof(StreamSoundFile::RegionInfo) + 64];
    void* pRegion = reinterpret_cast<void*>(
        (reinterpret_cast<uintptr_t>(regionBuffer) + 63) & ~static_cast<uintptr_t>(63));
    if (mFileStream->Read(pRegion, sizeof(StreamSoundFile::RegionInfo), nullptr) !=
        sizeof(StreamSoundFile::RegionInfo)) {
        return false;
    }

    std::memcpy(pInfo, pRegion, sizeof(StreamSoundFile::RegionInfo));
    return true;
}
}  // namespace nn::atk::detail
