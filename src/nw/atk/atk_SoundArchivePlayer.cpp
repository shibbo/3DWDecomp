#include <nn/atk/atk_SoundArchivePlayer.h>

#include <algorithm>
#include <new>
#include <nn/atk/atk_Debug.h>
#include <nn/atk/atk_DriverCommand.h>
#include <nn/atk/atk_ExternalSoundPlayer.h>
#include <nn/atk/atk_HardwareManager.h>
#include <nn/atk/atk_OutputAdditionalParam.h>
#include <nn/atk/atk_PlayerHeap.h>
#include <nn/atk/atk_Result.h>
#include <nn/atk/atk_SoundActor.h>
#include <nn/atk/atk_SoundArchiveFilesHook.h>
#include <nn/atk/atk_SoundArchiveParametersHook.h>
#include <nn/atk/atk_SoundDataManager.h>
#include <nn/atk/atk_SoundHandle.h>
#include <nn/atk/atk_SoundSystem.h>
#include <nn/atk/atk_SpecialSoundHandle.h>
#include <nn/atk/atk_StartInfoReader.h>
#include <nn/atk/atk_StreamSoundFileLoader.h>
#include <nn/atk/atk_Util.h>
#include <nn/atk/atk_WarningLogger.h>
#include <nn/atk/atk_WaveFileReader.h>
#include <nn/atk/atk_WaveSoundHandle.h>
#include <nn/atk/atkfnd_FileStreamImpl.h>
#include <nn/atk/detail/atk_AdvancedWaveSound.h>
#include <nn/fs.h>
#include <nn/time.h>
#include <nn/util.h>
#include <nn/util/util_BitUtil.h>
#include <nn/util/util_BytePtr.h>
#include <nn/util/util_StringUtil.h>

namespace nn::atk {
namespace {
typedef SoundStartable::StartResult StartResult;

/** @brief Size of the buffer that receives a stream sound file path. */
const size_t StreamFilePathBufferSize = 769;
/** @brief Size of the work buffer needed to load a stream sound file header. */
const size_t StreamSoundHeaderWorkBufferSize = 0x5300;
/** @brief Alignment of the stream sound header work buffer. */
const size_t StreamSoundHeaderWorkBufferAlignment = 0x100;
/** @brief Size of the region name stored in a stream sound file. */
const int RegionNameLength = 64;
/** @brief Size of the marker name stored in a stream sound file. */
const int MarkerNameLength = 64;

const DebugWarningFlag DebugWarningFlag_NotEnoughSequenceSound = static_cast<DebugWarningFlag>(1);
const DebugWarningFlag DebugWarningFlag_NotEnoughStreamSound = static_cast<DebugWarningFlag>(2);
const DebugWarningFlag DebugWarningFlag_NotEnoughWaveSound = static_cast<DebugWarningFlag>(3);

/** @brief Marker entry as stored in the marker block of a stream sound file. */
struct MarkerFileInfo {
    u32 position;
    u32 reserved;
    char name[MarkerNameLength];
};
static_assert(sizeof(MarkerFileInfo) == 0x48);

/** @brief Region entry as stored in the region block of a stream sound file. */
struct RegionFileInfo {
    u32 startSamplePosition;
    u32 endSamplePosition;
    u8 _8[0xc0 - 0x8];
    char name[RegionNameLength];
};
static_assert(sizeof(RegionFileInfo) == sizeof(detail::StreamSoundFile::RegionInfo));

/** @brief Header of the info block of a stream sound file. */
struct InfoBlockHeader {
    detail::BinaryBlockHeader header;
    detail::StreamSoundFile::InfoBlockBody body;
};
static_assert(sizeof(InfoBlockHeader) == 0x20);

/**
 * @brief Gets the index part of an item id.
 * @param itemId Archive item id.
 * @return Index of the item within its item type.
 */
inline u32 GetItemIndex(u32 itemId) {
    return itemId & 0xffffff;
}

/**
 * @brief Checks whether a pointer is aligned.
 * @param pAddress Address to check.
 * @param alignment Power-of-two alignment.
 * @return True when the address is a multiple of the alignment.
 */
inline bool IsAligned(const void* pAddress, size_t alignment) {
    return (reinterpret_cast<uintptr_t>(pAddress) & (alignment - 1)) == 0;
}

/**
 * @brief Gets the signed distance between two addresses.
 * @param pAddress Address to measure.
 * @param pEnd Reference address.
 * @return Bytes from pEnd to pAddress; positive when pAddress lies past pEnd.
 */
inline ptrdiff_t GetOffset(const void* pAddress, const void* pEnd) {
    return static_cast<const u8*>(pAddress) - static_cast<const u8*>(pEnd);
}

/**
 * @brief Gets the size of one sound player together with its optional output parameters.
 * @param rConfig Sound instance configuration.
 * @return Bytes needed per sound player.
 */
inline size_t GetSoundPlayerInstanceSize(const detail::SoundInstanceConfig& rConfig) {
    size_t paramSize = detail::OutputAdditionalParam::GetRequiredMemSize(rConfig);
    if (paramSize == 0) {
        return sizeof(SoundPlayer);
    }
    return sizeof(SoundPlayer) + sizeof(detail::OutputAdditionalParam) + paramSize;
}

/**
 * @brief Selects where a starting sound is output.
 * @param rStartInfoReader Start parameters of the sound.
 * @param pDefaultOutputReceiver Receiver used when the start parameters name none, or nullptr.
 * @return Sub mix when sub mixes are enabled; otherwise the requested, default or final mix.
 */
inline OutputReceiver* GetStartOutputReceiver(const detail::StartInfoReader& rStartInfoReader,
                                              OutputReceiver* pDefaultOutputReceiver) {
    detail::driver::HardwareManager& rHardwareManager =
        detail::driver::HardwareManager().GetInstance();

    if (rHardwareManager.IsSubMixEnabled()) {
        return rHardwareManager.GetSubMix(rStartInfoReader.subMixIndex);
    }

    if (rStartInfoReader.pOutputReceiver != nullptr) {
        return rStartInfoReader.pOutputReceiver;
    }

    if (pDefaultOutputReceiver != nullptr) {
        return pDefaultOutputReceiver;
    }

    return &rHardwareManager.GetFinalMix();
}

/**
 * @brief Checks whether a file stream operation succeeded.
 * @param result Result returned by the stream.
 * @return True when the result is not an error.
 */
inline bool IsSucceeded(detail::fnd::FndResult result) {
    return static_cast<s32>(result.value) >= 0;
}

/**
 * @brief Compares two time spans.
 * @param rLhs Left time span.
 * @param rRhs Right time span.
 * @return True when rLhs is shorter than rRhs.
 */
inline bool IsShorter(const TimeSpan& rLhs, const TimeSpan& rRhs) {
    return static_cast<s64>(rLhs.nanoseconds) < static_cast<s64>(rRhs.nanoseconds);
}

/**
 * @brief Compares two strings up to a maximum length.
 * @param pLhs First string.
 * @param pRhs Second string.
 * @param count Maximum number of characters to compare.
 * @return Difference of the first differing characters, or 0 when the strings are equal.
 */
inline int Strncmp(const char* pLhs, const char* pRhs, int count) {
    for (int i = 0; i < count; i++) {
        char lhs = pLhs[i];
        char rhs = pRhs[i];
        if (lhs == '\0' || lhs != rhs) {
            return lhs - rhs;
        }
    }

    return 0;
}
}  // namespace

/**
 * @brief Sets up the players, sound instances and stream buffers from caller-owned memory.
 * @param rParam Archive, data manager and buffers to use.
 * @return True on success; false if already initialized or a buffer is too small.
 */
bool SoundArchivePlayer::Initialize(const InitializeParam& rParam) {
    if (m_IsInitialized) {
        return false;
    }

    if (!SoundSystem::IsInitialized()) {
        return false;
    }

    void* pSetupBuffer = rParam.pSetupBuffer;
    size_t setupBufferSize = rParam.setupBufferSize;
    void* pStreamInstanceBuffer;
    size_t streamInstanceBufferSize;

    if (rParam.enablePreparingStreamInstanceBufferFromSetupBuffer) {
        streamInstanceBufferSize =
            util::align_up(GetRequiredStreamInstanceSize(rParam.pSoundArchive), 0x1000);
        pStreamInstanceBuffer = pSetupBuffer;

        if (streamInstanceBufferSize != 0) {
            pSetupBuffer = static_cast<u8*>(pStreamInstanceBuffer) + streamInstanceBufferSize;
            setupBufferSize -= streamInstanceBufferSize;
            SoundSystem::AttachMemoryPool(&m_MemoryPool, pStreamInstanceBuffer,
                                          streamInstanceBufferSize);
            m_IsMemoryPoolAttached = true;
        }
    } else {
        pStreamInstanceBuffer = rParam.pStreamInstanceBuffer;
        streamInstanceBufferSize = rParam.streamInstanceBufferSize;
    }

    if (!SetupMram(rParam.pSoundArchive, pSetupBuffer, setupBufferSize,
                   rParam.userParamSizePerSound, rParam.addonSoundArchiveCount,
                   pStreamInstanceBuffer, streamInstanceBufferSize)) {
        return false;
    }

    if (!m_StreamSoundRuntime.SetupStreamBuffer(rParam.pSoundArchive, rParam.pStreamBuffer,
                                                rParam.streamBufferSize)) {
        return false;
    }

    m_SoundArchiveManager.Initialize(rParam.pSoundArchive, rParam.pSoundDataManager);
    m_AddonSoundArchiveLastAddTick = os::Tick(0);
    m_SequenceSoundRuntime.SetSoundArchiveManager(&m_SoundArchiveManager);

    if (rParam.pStreamCacheBuffer != nullptr) {
        if (!m_StreamSoundRuntime.SetupStreamCacheBuffer(
                rParam.pSoundArchive, rParam.pStreamCacheBuffer, rParam.streamCacheSize)) {
            return false;
        }
    }

    m_IsInitialized = true;
    return true;
}

/** @brief Stops every sound, releases all instances and detaches the memory pools. */
void SoundArchivePlayer::Finalize() {
    if (!m_IsInitialized) {
        return;
    }

    StopAllSound(0, true);
    DisposeInstances();

    if (m_IsMemoryPoolAttached) {
        SoundSystem::DetachMemoryPool(&m_MemoryPool);
        m_IsMemoryPoolAttached = false;
    }

    if (m_IsMemoryPoolForPlayerHeapAttached) {
        SoundSystem::DetachMemoryPool(&m_MemoryPoolForPlayerHeap);
        m_IsMemoryPoolForPlayerHeapAttached = false;
    }

    m_pDefaultOutputReceiver = nullptr;
    m_IsInitialized = false;
}

/**
 * @brief Stops the sounds of every player.
 * @param fadeFrames Fade-out length in frames.
 * @param isCancel True to wait until the sound thread has processed the stop.
 */
void SoundArchivePlayer::StopAllSound(int fadeFrames, bool isCancel) {
    for (u32 i = 0; i < m_SoundPlayerCount; i++) {
        m_pSoundPlayers[i].StopAllSound(fadeFrames);
    }

    if (isCancel) {
        detail::DriverCommand& rDriverCommand = detail::DriverCommand::GetInstance();
        u32 tag = rDriverCommand.FlushCommand(true, true);
        rDriverCommand.WaitCommandReply(tag);
    }
}

/** @brief Releases the players, the addon containers and every sound instance. */
void SoundArchivePlayer::DisposeInstances() {
    m_pAddonSoundArchiveContainers = nullptr;
    m_AddonSoundArchiveContainerCount = 0;
    m_SoundPlayerCount = 0;
    m_pSoundPlayers = nullptr;

    m_SoundArchiveManager.Finalize();
    m_SequenceSoundRuntime.Finalize();

    if (m_IsAdvancedWaveSoundEnabled) {
        m_AdvancedWaveSoundRuntime.Finalize();
    } else {
        m_WaveSoundRuntime.Finalize();
    }

    m_StreamSoundRuntime.Finalize();
}

/**
 * @brief Calculates the setup buffer size for an archive without user parameters.
 * @param pArchive Archive to play.
 * @return Required setup buffer size in bytes.
 */
size_t SoundArchivePlayer::GetRequiredMemSize(const SoundArchive* pArchive) {
    return GetRequiredMemSize(pArchive, 0, 0);
}

/**
 * @brief Calculates the setup buffer size for an archive.
 * @param pArchive Archive to play.
 * @param userParamSizePerSound User parameter bytes reserved for every sound.
 * @param addonSoundArchiveCount Maximum number of addon archives.
 * @return Required setup buffer size in bytes.
 */
size_t SoundArchivePlayer::GetRequiredMemSize(const SoundArchive* pArchive,
                                              size_t userParamSizePerSound,
                                              int addonSoundArchiveCount) {
    detail::SoundInstanceConfig config = SoundSystem::GetSoundInstanceConfig();
    config.enableBusMixVolume = false;
    config.enableVolumeThroughMode = false;
    u32 playerCount = pArchive->GetPlayerCount();
    size_t size = util::align_up(playerCount * GetSoundPlayerInstanceSize(config), 0x1000);

    for (u32 i = 0; i < playerCount; i++) {
        SoundArchive::PlayerInfo playerInfo;
        if (!pArchive->ReadPlayerInfo(&playerInfo, SoundArchive::GetPlayerIdFromIndex(i))) {
            continue;
        }

        if (playerInfo.playerHeapSize == 0) {
            continue;
        }

        for (int j = 0; j < playerInfo.playableSoundMax; j++) {
            size += util::align_up(sizeof(detail::PlayerHeap), 0x1000);
            size += util::align_up(playerInfo.playerHeapSize, 0x1000);
        }
    }

    SoundArchive::SoundArchivePlayerInfo info;
    if (pArchive->ReadSoundArchivePlayerInfo(&info)) {
        size += detail::StreamSoundRuntime::GetRequiredMemorySize(info, 0x40);
        bool isAdvancedWaveSoundEnabled = info.isAdvancedWaveSoundEnabled;
        size += detail::SequenceSoundRuntime::GetRequiredMemorySize(info, 0x40);

        if (isAdvancedWaveSoundEnabled) {
            size += detail::AdvancedWaveSoundRuntime::GetRequiredMemorySize(info, 0x40);
        } else {
            size += detail::WaveSoundRuntime::GetRequiredMemorySize(info, 0x40);
        }

        size += detail::SequenceSoundRuntime::GetRequiredSequenceTrackMemorySize(info, 0x40);
    }

    if (addonSoundArchiveCount > 0) {
        size += util::align_up(
            addonSoundArchiveCount * static_cast<s64>(sizeof(detail::AddonSoundArchiveContainer)),
            0x40);
    }

    if (userParamSizePerSound != 0) {
        u32 soundCount = info.sequenceSoundCount + info.waveSoundCount + info.streamSoundCount;
        size += util::align_up(userParamSizePerSound, 4) * soundCount;
    }

    return size;
}

/**
 * @brief Calculates the setup buffer size for an archive without addon archives.
 * @param pArchive Archive to play.
 * @param userParamSizePerSound User parameter bytes reserved for every sound.
 * @return Required setup buffer size in bytes.
 */
size_t SoundArchivePlayer::GetRequiredMemSize(const SoundArchive* pArchive,
                                              size_t userParamSizePerSound) {
    return GetRequiredMemSize(pArchive, userParamSizePerSound, 0);
}

/**
 * @brief Calculates the setup buffer size for an initialization parameter set.
 * @param rParam Parameters that will be passed to Initialize.
 * @return Required setup buffer size in bytes.
 */
size_t SoundArchivePlayer::GetRequiredMemSize(const InitializeParam& rParam) {
    const SoundArchive* pArchive = rParam.pSoundArchive;
    bool isStreamInstanceBufferInSetupBuffer =
        rParam.enablePreparingStreamInstanceBufferFromSetupBuffer;
    size_t size = GetRequiredMemSize(pArchive, rParam.userParamSizePerSound,
                                     rParam.addonSoundArchiveCount);

    if (!isStreamInstanceBufferInSetupBuffer) {
        size -= util::align_up(GetRequiredStreamInstanceSize(pArchive), 0x1000);
    }

    return size;
}

/**
 * @brief Calculates the stream sound instance buffer size of an archive.
 * @param pArchive Archive to play.
 * @return Required instance buffer size, or 0 if the player info cannot be read.
 */
size_t SoundArchivePlayer::GetRequiredStreamInstanceSize(const SoundArchive* pArchive) {
    SoundArchive::SoundArchivePlayerInfo info;
    if (!pArchive->ReadSoundArchivePlayerInfo(&info)) {
        return 0;
    }

    return detail::StreamSoundRuntime::GetRequiredStreamInstanceSize(info.streamSoundCount);
}

/**
 * @brief Calculates the stream buffer size of an archive.
 * @param pArchive Archive to play.
 * @return Required stream buffer size in bytes.
 */
size_t SoundArchivePlayer::GetRequiredStreamBufferSize(const SoundArchive* pArchive) const {
    return m_StreamSoundRuntime.GetRequiredStreamBufferSize(pArchive);
}

/**
 * @brief Gets how many stream blocks are buffered for the streams of an archive.
 * @param pArchive Archive to play.
 * @return Stream buffer multiplier.
 */
int SoundArchivePlayer::GetRequiredStreamBufferTimes(const SoundArchive* pArchive) const {
    return detail::StreamSoundRuntime::GetRequiredStreamBufferTimes(pArchive);
}

/**
 * @brief Calculates the stream cache size of an archive.
 * @param pArchive Archive to play.
 * @param cacheSizePerSound Cache bytes for each stream sound.
 * @return Required stream cache size in bytes.
 */
size_t SoundArchivePlayer::GetRequiredStreamCacheSize(const SoundArchive* pArchive,
                                                      size_t cacheSizePerSound) {
    return detail::StreamSoundRuntime::GetRequiredStreamCacheSize(pArchive, cacheSizePerSound);
}

/**
 * @brief Carves the players, sound instances and user parameters out of the setup buffer.
 * @param pArchive Archive to play.
 * @param pBuffer Setup buffer, aligned to 4 KiB.
 * @param bufferSize Setup buffer size in bytes.
 * @param userParamSizePerSound User parameter bytes reserved for every sound.
 * @param addonSoundArchiveCount Maximum number of addon archives.
 * @param pStreamInstanceBuffer Buffer for the stream sound instances.
 * @param streamInstanceBufferSize Stream sound instance buffer size in bytes.
 * @return True when everything fits in the setup buffer.
 */
bool SoundArchivePlayer::SetupMram(const SoundArchive* pArchive, void* pBuffer, size_t bufferSize,
                                   size_t userParamSizePerSound, int addonSoundArchiveCount,
                                   void* pStreamInstanceBuffer, size_t streamInstanceBufferSize) {
    if (!IsAligned(pBuffer, 0x1000)) {
        return false;
    }

    void* pCursor = pBuffer;
    const void* pEnd = static_cast<u8*>(pBuffer) + bufferSize;

    if (!SetupSoundPlayer(pArchive, &pCursor, pEnd)) {
        return false;
    }

    SoundArchive::SoundArchivePlayerInfo info;
    if (pArchive->ReadSoundArchivePlayerInfo(&info)) {
        if (!m_StreamSoundRuntime.Initialize(info.streamSoundCount, &pCursor, pEnd,
                                             pStreamInstanceBuffer, streamInstanceBufferSize)) {
            return false;
        }

        m_IsAdvancedWaveSoundEnabled = info.isAdvancedWaveSoundEnabled;

        if (!m_SequenceSoundRuntime.Initialize(info.sequenceSoundCount, &pCursor, pEnd)) {
            return false;
        }

        if (m_IsAdvancedWaveSoundEnabled) {
            if (!m_AdvancedWaveSoundRuntime.Initialize(info.waveSoundCount, &pCursor, pEnd)) {
                return false;
            }
        } else {
            if (!m_WaveSoundRuntime.Initialize(info.waveSoundCount, &pCursor, pEnd)) {
                return false;
            }
        }

        if (!m_SequenceSoundRuntime.SetupSequenceTrack(info.sequenceTrackCount, &pCursor, pEnd)) {
            return false;
        }
    }

    if (addonSoundArchiveCount > 0) {
        if (!SetupAddonSoundArchiveContainer(addonSoundArchiveCount, &pCursor, pEnd)) {
            return false;
        }
    }

    if (userParamSizePerSound != 0) {
        if (!SetupUserParamForBasicSound(info, &pCursor, pEnd, userParamSizePerSound)) {
            return false;
        }
    }

    return true;
}

/**
 * @brief Creates the sound players of an archive and their player heaps.
 * @param pArchive Archive to play.
 * @param ppBuffer Allocation cursor, aligned to 4 KiB; advanced past the used memory.
 * @param pEnd End of the setup buffer.
 * @return True when every player and player heap fits.
 */
bool SoundArchivePlayer::SetupSoundPlayer(const SoundArchive* pArchive, void** ppBuffer,
                                          const void* pEnd) {
    if (!IsAligned(*ppBuffer, 0x1000)) {
        return false;
    }

    detail::SoundInstanceConfig config = SoundSystem::GetSoundInstanceConfig();
    config.enableBusMixVolume = false;
    config.enableVolumeThroughMode = false;
    size_t paramBufferSize = detail::OutputAdditionalParam::GetRequiredMemSize(config);
    size_t paramSize = paramBufferSize != 0 ? sizeof(detail::OutputAdditionalParam) : 0;
    u32 playerCount = pArchive->GetPlayerCount();
    size_t requiredSize = util::align_up(playerCount * GetSoundPlayerInstanceSize(config), 0x1000);

    SoundPlayer* pPlayers = static_cast<SoundPlayer*>(*ppBuffer);
    void* pHeapBuffer = reinterpret_cast<u8*>(pPlayers) + requiredSize;
    if (GetOffset(pHeapBuffer, pEnd) > 0) {
        return false;
    }

    *ppBuffer = pHeapBuffer;
    m_pSoundPlayers = pPlayers;
    m_SoundPlayerCount = playerCount;

    u8* pParamBuffer = reinterpret_cast<u8*>(&pPlayers[playerCount]);
    SoundPlayer* pPlayerBuffer = pPlayers;
    for (u32 i = 0; i < playerCount; i++, pPlayerBuffer++) {
        detail::OutputAdditionalParam* pParam = nullptr;
        if (paramBufferSize != 0) {
            pParam = new (pParamBuffer) detail::OutputAdditionalParam();
            void* pParamMemory = pParamBuffer + paramSize;
            pParam->Initialize(pParamMemory, paramBufferSize, config);
            pParamBuffer = static_cast<u8*>(pParamMemory) + paramBufferSize;
        }

        SoundPlayer* pPlayer = new (pPlayerBuffer) SoundPlayer(pParam);

        SoundArchive::PlayerInfo playerInfo;
        if (!pArchive->ReadPlayerInfo(&playerInfo, SoundArchive::GetPlayerIdFromIndex(i))) {
            continue;
        }

        pPlayer->SetPlayableSoundCount(playerInfo.playableSoundMax);

        if (playerInfo.playerHeapSize == 0) {
            continue;
        }

        for (int j = 0; j < playerInfo.playableSoundMax; j++) {
            detail::PlayerHeap* pHeap =
                CreatePlayerHeap(ppBuffer, pEnd, playerInfo.playerHeapSize);
            if (pHeap == nullptr) {
                return false;
            }

            pPlayer->detail_AppendPlayerHeap(pHeap);
        }

        pPlayer->detail_SetPlayableSoundLimit(playerInfo.playableSoundMax);
    }

    size_t heapSize = GetOffset(*ppBuffer, pHeapBuffer);
    if (heapSize != 0) {
        SoundSystem::AttachMemoryPool(&m_MemoryPoolForPlayerHeap, pHeapBuffer, heapSize);
        m_IsMemoryPoolForPlayerHeapAttached = true;
    }

    return true;
}

/**
 * @brief Creates the containers that hold the addon archives.
 * @param containerCount Number of containers to create.
 * @param ppBuffer Allocation cursor, aligned to 64 bytes; advanced past the containers.
 * @param pEnd End of the setup buffer.
 * @return True when the containers fit.
 */
bool SoundArchivePlayer::SetupAddonSoundArchiveContainer(int containerCount, void** ppBuffer,
                                                         const void* pEnd) {
    void* pBuffer = *ppBuffer;
    if (!IsAligned(pBuffer, 0x40)) {
        return false;
    }

    void* pNext = static_cast<u8*>(pBuffer) +
                  util::align_up(containerCount * static_cast<s64>(
                                                      sizeof(detail::AddonSoundArchiveContainer)),
                                 0x40);
    if (GetOffset(pNext, pEnd) > 0) {
        return false;
    }

    *ppBuffer = pNext;
    m_pAddonSoundArchiveContainers = static_cast<detail::AddonSoundArchiveContainer*>(pBuffer);
    m_AddonSoundArchiveContainerCount = containerCount;

    for (int i = 0; i < m_AddonSoundArchiveContainerCount; i++) {
        new (&m_pAddonSoundArchiveContainers[i]) detail::AddonSoundArchiveContainer();
    }

    return true;
}

/**
 * @brief Reserves the user parameter area of every sound.
 * @param rInfo Player information of the archive.
 * @param ppBuffer Allocation cursor, aligned to 64 bytes; advanced past the user parameters.
 * @param pEnd End of the setup buffer.
 * @param userParamSize User parameter bytes reserved for every sound.
 * @return True when the user parameters fit.
 */
bool SoundArchivePlayer::SetupUserParamForBasicSound(
    const SoundArchive::SoundArchivePlayerInfo& rInfo, void** ppBuffer, const void* pEnd,
    size_t userParamSize) {
    if (!IsAligned(*ppBuffer, 0x40)) {
        return false;
    }

    int soundCount = rInfo.sequenceSoundCount + rInfo.waveSoundCount + rInfo.streamSoundCount;
    size_t alignedSize = util::align_up(userParamSize, 4);
    void* pBuffer = *ppBuffer;
    *ppBuffer = static_cast<u8*>(*ppBuffer) + alignedSize * soundCount;

    if (m_IsAdvancedWaveSoundEnabled) {
        m_AdvancedWaveSoundRuntime.SetupUserParam(&pBuffer, alignedSize);
    } else {
        m_WaveSoundRuntime.SetupUserParam(&pBuffer, alignedSize);
    }

    m_SequenceSoundRuntime.SetupUserParam(&pBuffer, alignedSize);
    m_StreamSoundRuntime.SetupUserParam(&pBuffer, alignedSize);
    m_SoundUserParamSize = alignedSize;

    return GetOffset(pBuffer, pEnd) <= 0;
}

/**
 * @brief Creates one player heap.
 * @param ppBuffer Allocation cursor, aligned to 4 KiB; advanced past the heap.
 * @param pEnd End of the setup buffer.
 * @param heapSize Heap size in bytes.
 * @return Created heap, or nullptr if it does not fit.
 */
detail::PlayerHeap* SoundArchivePlayer::CreatePlayerHeap(void** ppBuffer, const void* pEnd,
                                                         size_t heapSize) {
    void* pHeapObject = *ppBuffer;
    if (!IsAligned(pHeapObject, 0x1000)) {
        return nullptr;
    }

    void* pNext = util::BytePtr(pHeapObject, sizeof(detail::PlayerHeap)).AlignUp(0x1000).Get();
    if (GetOffset(pNext, pEnd) > 0) {
        return nullptr;
    }

    *ppBuffer = pNext;
    detail::PlayerHeap* pHeap = new (pHeapObject) detail::PlayerHeap();

    void* pHeapMemory = *ppBuffer;
    void* pHeapEnd = static_cast<u8*>(pHeapMemory) + util::align_up(heapSize, 0x1000);
    if (GetOffset(pHeapEnd, pEnd) > 0) {
        return nullptr;
    }

    *ppBuffer = pHeapEnd;
    bool isCreated = pHeap->Create(pHeapMemory, heapSize);
    return isCreated ? pHeap : nullptr;
}

/** @brief Updates every player and sound runtime, then flushes the driver commands. */
void SoundArchivePlayer::Update() {
    detail::Util::Singleton<detail::Util::WarningLogger>::GetInstance().SwapBuffer();
    if (m_IsEnableWarningPrint) {
        detail::Util::Singleton<detail::Util::WarningLogger>::GetInstance().Print();
    }

    for (u32 i = 0; i < GetSoundPlayerCount(); i++) {
        GetSoundPlayer(i).Update();
    }

    m_SequenceSoundRuntime.Update();

    if (m_IsAdvancedWaveSoundEnabled) {
        m_AdvancedWaveSoundRuntime.Update();
    } else {
        m_WaveSoundRuntime.Update();
    }

    m_StreamSoundRuntime.Update();

    detail::DriverCommand& rDriverCommand = detail::DriverCommand::GetInstance();
    rDriverCommand.RecvCommandReply();
    rDriverCommand.FlushCommand(false, false);
}

/**
 * @brief Gets a sound player.
 * @param playerId Player item id.
 * @return Sound player with the id's index.
 */
SoundPlayer& SoundArchivePlayer::GetSoundPlayer(SoundArchive::ItemId playerId) {
    u32 index = GetItemIndex(playerId);
    return m_pSoundPlayers[index];
}

/** @brief Gets the main archive. @return Archive passed to Initialize. */
const SoundArchive& SoundArchivePlayer::GetSoundArchive() const {
    return *m_SoundArchiveManager.GetMainSoundArchive();
}

/**
 * @brief Finds an addon archive by name.
 * @param pName Name passed to AddAddonSoundArchive.
 * @return Addon archive, or nullptr if none has the name.
 */
const AddonSoundArchive* SoundArchivePlayer::GetAddonSoundArchive(const char* pName) const {
    return m_SoundArchiveManager.GetAddonSoundArchive(pName);
}

/**
 * @brief Gets an addon archive by index.
 * @param index Index in [0, GetAddonSoundArchiveCount()).
 * @return Addon archive.
 */
const AddonSoundArchive* SoundArchivePlayer::GetAddonSoundArchive(int index) const {
    return m_SoundArchiveManager.GetAddonSoundArchiveContainer(index)->GetSoundArchive();
}

/**
 * @brief Gets the name of an addon archive.
 * @param index Index in [0, GetAddonSoundArchiveCount()).
 * @return Name passed to AddAddonSoundArchive.
 */
const char* SoundArchivePlayer::GetAddonSoundArchiveName(int index) const {
    return m_SoundArchiveManager.GetAddonSoundArchiveContainer(index)->GetSoundArchiveName();
}

/**
 * @brief Gets when an addon archive was added.
 * @param index Index in [0, GetAddonSoundArchiveCount()).
 * @return Tick of the AddAddonSoundArchive call.
 */
os::Tick SoundArchivePlayer::GetAddonSoundArchiveAddTick(int index) const {
    return m_SoundArchiveManager.GetAddonSoundArchiveContainer(index)->GetAddTick();
}

/**
 * @brief Finds the data manager of an addon archive by name.
 * @param pName Name passed to AddAddonSoundArchive.
 * @return Data manager, or nullptr if no addon archive has the name.
 */
const SoundDataManager* SoundArchivePlayer::GetAddonSoundDataManager(const char* pName) const {
    return m_SoundArchiveManager.GetAddonSoundDataManager(pName);
}

/**
 * @brief Gets a sound player.
 * @param playerId Player item id.
 * @return Sound player with the id's index.
 */
const SoundPlayer& SoundArchivePlayer::GetSoundPlayer(SoundArchive::ItemId playerId) const {
    u32 index = GetItemIndex(playerId);
    return m_pSoundPlayers[index];
}

/**
 * @brief Gets a sound player by label.
 * @param pPlayerName Player label in the main archive.
 * @return Sound player with the label.
 */
SoundPlayer& SoundArchivePlayer::GetSoundPlayer(const char* pPlayerName) {
    return GetSoundPlayer(GetSoundArchive().GetItemId(pPlayerName));
}

/**
 * @brief Gets a sound player by label.
 * @param pPlayerName Player label in the main archive.
 * @return Sound player with the label.
 */
const SoundPlayer& SoundArchivePlayer::GetSoundPlayer(const char* pPlayerName) const {
    return GetSoundPlayer(GetSoundArchive().GetItemId(pPlayerName));
}

/**
 * @brief Gets the loaded data of a main archive file.
 * @param fileId File id in the main archive.
 * @return File data, or nullptr if there is no data manager or the file is not loaded.
 */
const void* SoundArchivePlayer::detail_GetFileAddress(SoundArchive::FileId fileId) const {
    const SoundDataManager* pDataManager = m_SoundArchiveManager.GetMainSoundDataManager();
    if (pDataManager == nullptr) {
        return nullptr;
    }

    return pDataManager->detail_GetFileAddress(fileId);
}

/**
 * @brief Adds an addon archive to the first free container.
 * @param pName Name used to start sounds of the archive.
 * @param pArchive Addon archive to add.
 * @param pDataManager Data manager of the addon archive.
 */
void SoundArchivePlayer::AddAddonSoundArchive(const char* pName, const AddonSoundArchive* pArchive,
                                              const SoundDataManager* pDataManager) {
    detail::AddonSoundArchiveContainer* pContainer = nullptr;
    for (int i = 0; i < m_AddonSoundArchiveContainerCount; i++) {
        if (!m_pAddonSoundArchiveContainers[i].IsActive()) {
            pContainer = &m_pAddonSoundArchiveContainers[i];
            break;
        }
    }

    pContainer->Initialize(pName, pArchive, pDataManager);
    pArchive->SetParametersHook(m_SoundArchiveManager.GetParametersHook());
    m_SoundArchiveManager.Add(*pContainer);

    os::Tick tick = os::GetSystemTick();
    pContainer->SetAddTick(tick);
    m_AddonSoundArchiveLastAddTick = tick;
}

/**
 * @brief Removes an addon archive.
 * @param pArchive Addon archive passed to AddAddonSoundArchive.
 */
void SoundArchivePlayer::RemoveAddonSoundArchive(const AddonSoundArchive* pArchive) {
    for (int i = 0; i < m_AddonSoundArchiveContainerCount; i++) {
        detail::AddonSoundArchiveContainer& rContainer = m_pAddonSoundArchiveContainers[i];
        if (rContainer.IsActive() && rContainer.GetSoundArchive() == pArchive) {
            pArchive->SetParametersHook(nullptr);
            rContainer.Finalize();
            m_SoundArchiveManager.Remove(rContainer);
            return;
        }
    }
}

/**
 * @brief Sets where sounds are output when the start info names no receiver.
 * @param pOutputReceiver Output receiver, or nullptr to use the final mix.
 */
void SoundArchivePlayer::SetDefaultOutputReceiver(OutputReceiver* pOutputReceiver) {
    m_pDefaultOutputReceiver = pOutputReceiver;
}

/**
 * @brief Sets up a sound without ambient information or actor.
 * @param pHandle Handle that receives the sound.
 * @param soundId Sound item id.
 * @param holdFlag True when the sound is held.
 * @param pSoundArchiveName Addon archive name, or nullptr for the main archive.
 * @param pStartInfo Start parameters, or nullptr.
 * @return Result of the setup.
 */
SoundStartable::StartResult SoundArchivePlayer::detail_SetupSound(SoundHandle* pHandle,
                                                                  u32 soundId, bool holdFlag,
                                                                  const char* pSoundArchiveName,
                                                                  const StartInfo* pStartInfo) {
    return detail_SetupSoundImpl(pHandle, soundId, nullptr, nullptr, holdFlag, pSoundArchiveName,
                                 pStartInfo);
}

/**
 * @brief Allocates, prepares and attaches a sound.
 * @param pHandle Handle that receives the sound.
 * @param soundId Sound item id.
 * @param pAmbientInfo Ambient information, or nullptr.
 * @param pActor Actor that starts the sound, or nullptr.
 * @param holdFlag True when the sound is held.
 * @param pSoundArchiveName Addon archive name, or nullptr for the main archive.
 * @param pStartInfo Start parameters, or nullptr.
 * @return Result of the setup.
 */
SoundStartable::StartResult SoundArchivePlayer::detail_SetupSoundImpl(
    SoundHandle* pHandle, u32 soundId, detail::BasicSound::AmbientInfo* pAmbientInfo,
    SoundActor* pActor, bool holdFlag, const char* pSoundArchiveName,
    const StartInfo* pStartInfo) {
    m_SoundArchiveManager.ChangeTargetArchive(pSoundArchiveName);
    detail::SoundArchiveManager::SnapShot snapShot = m_SoundArchiveManager.GetSnapShot();
    const SoundArchive* pArchive = snapShot.GetCurrentSoundArchive();

    if (!m_SoundArchiveManager.IsAvailable()) {
        return StartResult(StartResult::ResultCode_ErrorNotAvailable);
    }

    if (pHandle->IsAttachedSound()) {
        pHandle->DetachSound();
    }

    const char* pLabel = pArchive->GetItemLabel(soundId);
    bool isHookDisabled = false;
    SoundArchive::SoundType soundType;

    if (m_pSoundArchiveFilesHook != nullptr && pLabel != nullptr &&
        m_pSoundArchiveFilesHook->GetIsEnable()) {
        LockSoundArchiveFileHooks();

        if (m_pSoundArchiveFilesHook->GetIsEnable() &&
            m_pSoundArchiveFilesHook->IsTargetItem(pLabel)) {
            detail::SoundArchiveParametersHook* pParametersHook = pArchive->GetParametersHook();
            soundType = pParametersHook->GetIsEnable() ? pParametersHook->GetSoundType(pLabel) :
                                                         SoundArchive::SoundType_Invalid;

            if (pSoundArchiveName != nullptr) {
                pParametersHook = pArchive->GetParametersHook();
                soundId = pParametersHook->GetIsEnable() ? pParametersHook->GetItemId(pLabel) :
                                                           SoundArchive::InvalidId;
            }

            switch (soundType) {
            case SoundArchive::SoundType_Sequence:
                isHookDisabled = !IsSequenceSoundEdited(pLabel);
                break;
            case SoundArchive::SoundType_Stream:
                isHookDisabled = !IsStreamSoundEdited(pLabel);
                break;
            case SoundArchive::SoundType_Wave:
                isHookDisabled = !IsWaveSoundEdited(pLabel);
                break;
            default:
                break;
            }

            if (isHookDisabled) {
                pArchive->GetParametersHook()->SetIsEnable(false);
                m_pSoundArchiveFilesHook->SetIsEnable(false);
            }
        } else {
            pArchive->GetParametersHook()->SetIsEnable(false);
            m_pSoundArchiveFilesHook->SetIsEnable(false);
            isHookDisabled = true;
            soundType = pArchive->GetSoundType(soundId);
        }
    } else {
        if (soundId == SoundArchive::InvalidId) {
            return StartResult(StartResult::ResultCode_ErrorInvalidLabelString);
        }

        soundType = pArchive->GetSoundType(soundId);
    }

    SoundArchive::SoundInfo soundInfo;
    if (!pArchive->ReadSoundInfo(&soundInfo, soundId)) {
        EnableHook(pArchive, isHookDisabled);
        return StartResult(StartResult::ResultCode_ErrorInvalidSoundId);
    }

    detail::StartInfoReader startInfoReader(soundInfo);
    startInfoReader.Read(pStartInfo);

    OutputReceiver* pOutputReceiver =
        GetStartOutputReceiver(startInfoReader, m_pDefaultOutputReceiver);

    if (soundInfo.singlePlayType != SinglePlayType_None) {
        StartResult result = PreprocessSinglePlay(soundInfo, soundId, pArchive,
                                                  GetSoundPlayer(startInfoReader.playerId));
        if (!result.IsSuccess()) {
            EnableHook(pArchive, isHookDisabled);
            return result;
        }
    }

    int priority = startInfoReader.playerPriority;
    if (holdFlag) {
        priority--;
    }

    int ambientPriority = 0;
    if (pAmbientInfo != nullptr) {
        ambientPriority = detail::BasicSound::GetAmbientPriority(*pAmbientInfo, soundId);
    }

    int startPriority = std::min(std::max(priority + ambientPriority, 0), 127);

    detail::ExternalSoundPlayer* pExternalPlayer = nullptr;
    if (pActor != nullptr) {
        pExternalPlayer = pActor->detail_GetActorPlayer(startInfoReader.actorPlayerId);
        if (pExternalPlayer == nullptr) {
            EnableHook(pArchive, isHookDisabled);
            return StartResult(StartResult::ResultCode_ErrorInvalidParameter);
        }
    }

    SoundArchive::ItemId playerId = startInfoReader.playerId;
    if (!IsSoundArchiveFileHooksEnabled() && pArchive->IsAddon()) {
        playerId = snapShot.GetMainSoundArchive()->GetItemId(
            pArchive->GetItemLabel(soundInfo.playerId));
    }

    if (playerId == SoundArchive::InvalidId) {
        EnableHook(pArchive, isHookDisabled);
        return StartResult(StartResult::ResultCode_ErrorInvalidParameter);
    }

    SoundPlayer& rPlayer = GetSoundPlayer(playerId);
    if (!rPlayer.detail_CanPlaySound(startPriority)) {
        EnableHook(pArchive, isHookDisabled);
        return StartResult(StartResult::ResultCode_ErrorLowPriority);
    }

    if (pExternalPlayer != nullptr && !pExternalPlayer->CanPlaySound(startPriority)) {
        EnableHook(pArchive, isHookDisabled);
        return StartResult(StartResult::ResultCode_ErrorLowPriority);
    }

    detail::SequenceSound* pSequenceSound = nullptr;
    detail::StreamSound* pStreamSound = nullptr;
    detail::WaveSound* pWaveSound = nullptr;
    detail::AdvancedWaveSound* pAdvancedWaveSound = nullptr;
    detail::BasicSound* pSound;

    switch (soundType) {
    case SoundArchive::SoundType_Sequence:
        pSequenceSound = m_SequenceSoundRuntime.AllocSound(soundId, priority, ambientPriority,
                                                           pAmbientInfo, pOutputReceiver);
        if (pSequenceSound == nullptr) {
            detail::Debug_GetWarningFlag(DebugWarningFlag_NotEnoughSequenceSound);
            EnableHook(pArchive, isHookDisabled);
            return StartResult(StartResult::ResultCode_ErrorNotEnoughInstance);
        }

        pSound = pSequenceSound;
        break;
    case SoundArchive::SoundType_Stream:
        pStreamSound = m_StreamSoundRuntime.AllocSound(soundId, priority, ambientPriority,
                                                       pAmbientInfo, pOutputReceiver);
        if (pStreamSound == nullptr) {
            detail::Debug_GetWarningFlag(DebugWarningFlag_NotEnoughStreamSound);
            EnableHook(pArchive, isHookDisabled);
            return StartResult(StartResult::ResultCode_ErrorNotEnoughInstance);
        }

        pSound = pStreamSound;
        break;
    case SoundArchive::SoundType_Wave:
        if (m_IsAdvancedWaveSoundEnabled) {
            pAdvancedWaveSound = m_AdvancedWaveSoundRuntime.AllocSound(
                soundId, priority, ambientPriority, pAmbientInfo, pOutputReceiver);
            if (pAdvancedWaveSound == nullptr) {
                detail::Debug_GetWarningFlag(DebugWarningFlag_NotEnoughWaveSound);
                EnableHook(pArchive, isHookDisabled);
                return StartResult(StartResult::ResultCode_ErrorNotEnoughInstance);
            }

            pSound = pAdvancedWaveSound;
        } else {
            pWaveSound = m_WaveSoundRuntime.AllocSound(soundId, priority, ambientPriority,
                                                       pAmbientInfo, pOutputReceiver);
            if (pWaveSound == nullptr) {
                detail::Debug_GetWarningFlag(DebugWarningFlag_NotEnoughWaveSound);
                EnableHook(pArchive, isHookDisabled);
                return StartResult(StartResult::ResultCode_ErrorNotEnoughInstance);
            }

            pSound = pWaveSound;
        }
        break;
    default:
        EnableHook(pArchive, isHookDisabled);
        return StartResult(StartResult::ResultCode_ErrorInvalidSoundId);
    }

    if (!rPlayer.detail_AppendSound(pSound)) {
        pSound->Finalize();
        EnableHook(pArchive, isHookDisabled);
        return StartResult(StartResult::ResultCode_ErrorUnknown);
    }

    if (pStartInfo != nullptr &&
        (pStartInfo->enableFlag & StartInfo::EnableFlagBit_SoundStopCallback) != 0) {
        pSound->SetSoundStopCallback(pStartInfo->soundStopCallback);
    }

    pSound->ResetOutputLine();

    StartResult prepareResult;
    switch (soundType) {
    case SoundArchive::SoundType_Sequence:
        prepareResult = m_SequenceSoundRuntime.PrepareImpl(snapShot, soundId, pSequenceSound,
                                                           &soundInfo, startInfoReader);
        if (!prepareResult.IsSuccess()) {
            pSequenceSound->Finalize();
            EnableHook(pArchive, isHookDisabled);
            return prepareResult;
        }
        break;
    case SoundArchive::SoundType_Stream:
        prepareResult = m_StreamSoundRuntime.PrepareImpl(
            pArchive, snapShot.GetCurrentSoundDataManager(), soundId, pStreamSound, &soundInfo,
            startInfoReader);
        if (!prepareResult.IsSuccess()) {
            pStreamSound->Finalize();
            EnableHook(pArchive, isHookDisabled);
            return prepareResult;
        }
        break;
    case SoundArchive::SoundType_Wave:
        if (m_IsAdvancedWaveSoundEnabled) {
            prepareResult = m_AdvancedWaveSoundRuntime.PrepareImpl(
                pArchive, snapShot.GetCurrentSoundDataManager(), soundId, pAdvancedWaveSound,
                &soundInfo, startInfoReader);
            if (!prepareResult.IsSuccess()) {
                pAdvancedWaveSound->Finalize();
                EnableHook(pArchive, isHookDisabled);
                return prepareResult;
            }
        } else {
            prepareResult = m_WaveSoundRuntime.PrepareImpl(
                pArchive, snapShot.GetCurrentSoundDataManager(), soundId, pWaveSound, &soundInfo,
                startInfoReader);
            if (!prepareResult.IsSuccess()) {
                pWaveSound->Finalize();
                EnableHook(pArchive, isHookDisabled);
                return prepareResult;
            }
        }
        break;
    default:
        pSound->Finalize();
        EnableHook(pArchive, isHookDisabled);
        return StartResult(StartResult::ResultCode_ErrorInvalidSoundId);
    }

    EnableHook(pArchive, isHookDisabled);
    SetCommonSoundParam(pSound, &soundInfo);

    if (pExternalPlayer != nullptr) {
        if (!pExternalPlayer->AppendSound(pSound)) {
            pSound->Finalize();
            return StartResult(StartResult::ResultCode_ErrorUnknown);
        }
    }

    if (pActor != nullptr) {
        pSound->AttachSoundActor(pActor);
    }

    if (holdFlag) {
        pSound->SetPlayerPriority(startInfoReader.playerPriority);
    }

    pSound->SetSoundArchive(pArchive);
    pSound->SetSetupTick(os::GetSystemTick());
    pHandle->detail_AttachSound(pSound);

    return StartResult(StartResult::ResultCode_Success);
}

/** @brief Checks whether the file hook is active. @return True when a hook is set and enabled. */
bool SoundArchivePlayer::IsSoundArchiveFileHooksEnabled() const {
    return m_pSoundArchiveFilesHook != nullptr && m_pSoundArchiveFilesHook->GetIsEnable();
}

/** @brief Locks the file hook if it is active. */
void SoundArchivePlayer::LockSoundArchiveFileHooks() {
    if (m_pSoundArchiveFilesHook != nullptr && m_pSoundArchiveFilesHook->GetIsEnable()) {
        m_pSoundArchiveFilesHook->Lock();
    }
}

/**
 * @brief Checks whether the file hook provides the sequence data of a sound.
 * @param pLabel Sound label.
 * @return True when the hook provides edited sequence data.
 */
bool SoundArchivePlayer::IsSequenceSoundEdited(const char* pLabel) const {
    if (pLabel == nullptr) {
        return false;
    }

    if (!m_pSoundArchiveFilesHook->GetIsEnable()) {
        return false;
    }

    return m_pSoundArchiveFilesHook->GetFileAddress(
               pLabel, detail::SoundArchiveFilesHook::ItemTypeSequenceSound,
               detail::SoundArchiveFilesHook::FileTypeSequenceBinary, 0) != nullptr;
}

/**
 * @brief Checks whether a stream sound is edited.
 * @param pLabel Sound label.
 * @return True for any labeled sound, since stream files are always read from the hook.
 */
bool SoundArchivePlayer::IsStreamSoundEdited(const char* pLabel) const {
    return pLabel != nullptr;
}

/**
 * @brief Checks whether the file hook provides the wave sound data of a sound.
 * @param pLabel Sound label.
 * @return True when the hook provides edited wave sound data.
 */
bool SoundArchivePlayer::IsWaveSoundEdited(const char* pLabel) const {
    if (pLabel == nullptr) {
        return false;
    }

    if (!m_pSoundArchiveFilesHook->GetIsEnable()) {
        return false;
    }

    return m_pSoundArchiveFilesHook->GetFileAddress(
               pLabel, detail::SoundArchiveFilesHook::ItemTypeWaveSound,
               detail::SoundArchiveFilesHook::FileTypeWaveSoundBinary, 0) != nullptr;
}

/**
 * @brief Re-enables the hooks disabled for a sound and unlocks the file hook.
 * @param pArchive Archive whose parameter hook was disabled.
 * @param isEnabled True to enable the parameter and file hooks again.
 */
void SoundArchivePlayer::EnableHook(const SoundArchive* pArchive, bool isEnabled) {
    if (isEnabled) {
        pArchive->GetParametersHook()->SetIsEnable(true);
        m_pSoundArchiveFilesHook->SetIsEnable(true);
    }

    UnlockSoundArchiveFileHooks();
}

/**
 * @brief Applies the single play rule of a sound before it starts.
 * @param rInfo Sound information.
 * @param soundId Sound item id.
 * @param pArchive Archive of the sound.
 * @param rPlayer Player the sound starts on.
 * @return Success, or a cancel result when an older instance has priority.
 */
SoundStartable::StartResult SoundArchivePlayer::PreprocessSinglePlay(
    const SoundArchive::SoundInfo& rInfo, SoundArchive::ItemId soundId,
    const SoundArchive* pArchive, SoundPlayer& rPlayer) {
    os::Tick currentTick = os::GetSystemTick();
    TimeSpan effectiveDuration =
        TimeSpan::FromMilliSeconds(rInfo.singlePlayEffectiveDuration);
    SoundPlayer::SoundList& rSoundList = rPlayer.detail_GetSoundList();

    switch (rInfo.singlePlayType) {
    case SinglePlayType_None:
        return StartResult(StartResult::ResultCode_Success);
    case SinglePlayType_PrioritizeOldest:
        effectiveDuration = TimeSpan::FromDays(365);
        [[fallthrough]];
    case SinglePlayType_PrioritizeOldestWithDuration: {
        u8 isFound = false;
        os::Tick setupTick = 0;

        for (auto it = rSoundList.begin(); it != rSoundList.end();) {
            detail::BasicSound& rSound = *it++;
            SoundHandle handle;
            handle.detail_AttachSoundAsTempHandle(&rSound);

            if (handle.GetId() == soundId &&
                handle.detail_GetAttachedSound()->GetSoundArchive() == pArchive) {
                setupTick = handle.detail_GetAttachedSound()->GetSetupTick();
                isFound = true;
            }
        }

        if (isFound != 0 &&
            IsShorter(os::ConvertToTimeSpan(currentTick - setupTick), effectiveDuration)) {
            return StartResult(StartResult::ResultCode_CanceledForSinglePlay);
        }

        return StartResult(StartResult::ResultCode_Success);
    }
    case SinglePlayType_PrioritizeNewest:
        effectiveDuration = TimeSpan::FromDays(365);
        [[fallthrough]];
    case SinglePlayType_PrioritizeNewestWithDuration:
        for (auto it = rSoundList.begin(); it != rSoundList.end();) {
            detail::BasicSound& rSound = *it++;
            SoundHandle handle;
            handle.detail_AttachSoundAsTempHandle(&rSound);

            if (handle.GetId() == soundId &&
                handle.detail_GetAttachedSound()->GetSoundArchive() == pArchive) {
                os::Tick setupTick = handle.detail_GetAttachedSound()->GetSetupTick();
                if (IsShorter(os::ConvertToTimeSpan(currentTick - setupTick), effectiveDuration)) {
                    handle.Stop(0);
                }
            }
        }

        return StartResult(StartResult::ResultCode_Success);
    default:
        NN_UNEXPECTED_DEFAULT;
    }
}

/**
 * @brief Applies the volume and pan settings of the sound information to a sound.
 * @param pSound Sound to configure.
 * @param pInfo Sound information.
 */
void SoundArchivePlayer::SetCommonSoundParam(detail::BasicSound* pSound,
                                             const SoundArchive::SoundInfo* pInfo) {
    pSound->SetInitialVolume(static_cast<f32>(pInfo->volume) / 127.0f);
    pSound->SetPanMode(static_cast<PanMode>(pInfo->panMode));
    pSound->SetPanCurve(static_cast<PanCurve>(pInfo->panCurve));
}

/** @brief Unlocks the file hook if it is active. */
void SoundArchivePlayer::UnlockSoundArchiveFileHooks() {
    if (m_pSoundArchiveFilesHook != nullptr && m_pSoundArchiveFilesHook->GetIsEnable()) {
        m_pSoundArchiveFilesHook->Unlock();
    }
}

/**
 * @brief Sets the user procedure callback of the sequences.
 * @param callback Callback invoked by user procedure commands, or nullptr.
 * @param pArg Argument passed unchanged to the callback.
 */
void SoundArchivePlayer::SetSequenceUserProcCallback(SequenceUserProcCallback callback,
                                                     void* pArg) {
    m_SequenceSoundRuntime.SetSequenceUserProcCallback(callback, pArg);
}

/**
 * @brief Sets how many ticks a sequence skips per update while skipping.
 * @param tick Skip interval in ticks.
 */
void SoundArchivePlayer::SetSequenceSkipIntervalTick(int tick) {
    detail::SequenceSoundRuntime::SetSequenceSkipIntervalTick(tick);
}

/** @brief Gets the sequence skip interval. @return Skip interval in ticks. */
int SoundArchivePlayer::GetSequenceSkipIntervalTick() {
    return detail::SequenceSoundRuntime::GetSequenceSkipIntervalTick();
}

/**
 * @brief Reads the playback information of a wave sound from its loaded wave file.
 * @param pInfo Receives the playback information.
 * @param soundId Wave sound item id.
 * @param pArchive Archive of the sound.
 * @param pDataManager Data manager that loaded the sound data.
 * @return Success, or the reason the information could not be read.
 */
Result SoundArchivePlayer::ReadWaveSoundDataInfo(WaveSoundDataInfo* pInfo,
                                                 SoundArchive::ItemId soundId,
                                                 const SoundArchive* pArchive,
                                                 const SoundDataManager* pDataManager) const {
    SoundArchive::SoundInfo soundInfo;
    if (!pArchive->ReadSoundInfo(&soundInfo, soundId)) {
        return ResultInvalidSoundId();
    }

    SoundArchive::WaveSoundInfo waveSoundInfo;
    if (!pArchive->detail_ReadWaveSoundInfo(soundId, &waveSoundInfo)) {
        return ResultInvalidSoundId();
    }

    const void* pWaveSoundFile = pDataManager->detail_GetFileAddress(soundInfo.fileId);
    if (pWaveSoundFile == nullptr) {
        return ResultFileNotLoaded();
    }

    const void* pWaveFile = detail::Util::GetWaveFileOfWaveSound(pWaveSoundFile,
                                                                 waveSoundInfo.index, *pArchive,
                                                                 *pDataManager);
    if (pWaveFile == nullptr) {
        return ResultFileNotLoaded();
    }

    detail::WaveFileReader reader(pWaveFile, 0);
    detail::WaveInfo waveInfo;
    if (!reader.ReadWaveInfo(&waveInfo, nullptr)) {
        return ResultInvalidFileFormat();
    }

    pInfo->loopFlag = waveInfo.loop;
    pInfo->sampleRate = waveInfo.sampleRate;
    pInfo->loopStart = waveInfo.originalLoopStart;
    pInfo->loopEnd = waveInfo.originalLoopStart + (waveInfo.loopEnd - waveInfo.loopStart);
    pInfo->compatibleLoopStart = waveInfo.loopStart;
    pInfo->compatibleLoopEnd = waveInfo.loopEnd;
    pInfo->channelCount = std::min(waveInfo.channelCount, 2);

    return ResultSuccess();
}

/**
 * @brief Reads the playback information of a wave sound.
 * @param pInfo Receives the playback information.
 * @param soundId Wave sound item id.
 * @param pSoundArchiveName Addon archive name, or nullptr for the main archive.
 * @return Success, or the reason the information could not be read.
 */
Result SoundArchivePlayer::ReadWaveSoundDataInfo(WaveSoundDataInfo* pInfo,
                                                 SoundArchive::ItemId soundId,
                                                 const char* pSoundArchiveName) const {
    const SoundArchive* pArchive;
    const SoundDataManager* pDataManager;
    if (pSoundArchiveName != nullptr) {
        pArchive = m_SoundArchiveManager.GetAddonSoundArchive(pSoundArchiveName);
        pDataManager = m_SoundArchiveManager.GetAddonSoundDataManager(pSoundArchiveName);
    } else {
        pArchive = m_SoundArchiveManager.GetMainSoundArchive();
        pDataManager = m_SoundArchiveManager.GetMainSoundDataManager();
    }

    return ReadWaveSoundDataInfo(pInfo, soundId, pArchive, pDataManager);
}

/**
 * @brief Reads the playback information of a wave sound by label.
 * @param pInfo Receives the playback information.
 * @param pLabel Wave sound label.
 * @param pSoundArchiveName Addon archive name, or nullptr for the main archive.
 * @return Success, or the reason the information could not be read.
 */
Result SoundArchivePlayer::ReadWaveSoundDataInfo(WaveSoundDataInfo* pInfo, const char* pLabel,
                                                 const char* pSoundArchiveName) const {
    const SoundArchive* pArchive;
    const SoundDataManager* pDataManager;
    if (pSoundArchiveName != nullptr) {
        pArchive = m_SoundArchiveManager.GetAddonSoundArchive(pSoundArchiveName);
        pDataManager = m_SoundArchiveManager.GetAddonSoundDataManager(pSoundArchiveName);
    } else {
        pArchive = m_SoundArchiveManager.GetMainSoundArchive();
        pDataManager = m_SoundArchiveManager.GetMainSoundDataManager();
    }

    SoundArchive::ItemId soundId = pArchive->GetItemId(pLabel);
    if (soundId == SoundArchive::InvalidId) {
        return ResultInvalidLabelString();
    }

    return ReadWaveSoundDataInfo(pInfo, soundId, pArchive, pDataManager);
}

/**
 * @brief Reads the playback information of a main archive wave sound.
 * @param pInfo Receives the playback information.
 * @param soundId Wave sound item id.
 * @return Success, or the reason the information could not be read.
 */
Result SoundArchivePlayer::ReadWaveSoundDataInfo(WaveSoundDataInfo* pInfo,
                                                 SoundArchive::ItemId soundId) const {
    return ReadWaveSoundDataInfo(pInfo, soundId, m_SoundArchiveManager.GetMainSoundArchive(),
                                 m_SoundArchiveManager.GetMainSoundDataManager());
}

/**
 * @brief Reads the playback information of a main archive wave sound by label.
 * @param pInfo Receives the playback information.
 * @param pLabel Wave sound label.
 * @return Success, or the reason the information could not be read.
 */
Result SoundArchivePlayer::ReadWaveSoundDataInfo(WaveSoundDataInfo* pInfo,
                                                 const char* pLabel) const {
    const SoundArchive* pArchive = m_SoundArchiveManager.GetMainSoundArchive();
    const SoundDataManager* pDataManager = m_SoundArchiveManager.GetMainSoundDataManager();

    SoundArchive::ItemId soundId = pArchive->GetItemId(pLabel);
    if (soundId == SoundArchive::InvalidId) {
        return ResultInvalidLabelString();
    }

    return ReadWaveSoundDataInfo(pInfo, soundId, pArchive, pDataManager);
}

/**
 * @brief Reads the playback information of a stream sound from its stream file.
 * @param pInfo Receives the playback information.
 * @param pArchive Archive of the sound.
 * @param soundId Stream sound item id.
 * @return Success, or the reason the information could not be read.
 */
Result SoundArchivePlayer::ReadStreamSoundDataInfo(StreamSoundDataInfo* pInfo,
                                                   const SoundArchive* pArchive,
                                                   SoundArchive::ItemId soundId) const {
    if (IsStreamSoundPlaying(soundId, pArchive)) {
        return ResultStreamSoundPlaying();
    }

    char filePath[StreamFilePathBufferSize];
    if (!pArchive->ReadStreamSoundFilePath(filePath, sizeof(filePath), soundId)) {
        return ResultStreamFilePathNotFound();
    }

    detail::fnd::FileStreamImpl fileStream;
    if (!IsSucceeded(fileStream.Open(filePath, detail::fnd::FileStream::AccessMode_Read))) {
        fileStream.Close();
        return ResultFileAccessFailed();
    }

    alignas(64) u8 headerBuffer[0x60];
    if (fileStream.Read(headerBuffer, sizeof(headerBuffer), nullptr) != sizeof(headerBuffer)) {
        fileStream.Close();
        return ResultFileAccessFailed();
    }

    if (!detail::StreamSoundFileReader::IsValidFileHeader(headerBuffer)) {
        fileStream.Close();
        return ResultInvalidFileFormat();
    }

    const auto* pHeader = reinterpret_cast<const detail::StreamSoundFile::FileHeader*>(headerBuffer);
    fileStream.Seek(pHeader->GetInfoBlockOffset(), detail::fnd::Stream::SeekOrigin_Begin);

    InfoBlockHeader infoBlock;
    if (fileStream.Read(&infoBlock, sizeof(infoBlock), nullptr) != sizeof(infoBlock)) {
        fileStream.Close();
        return ResultFileAccessFailed();
    }

    fileStream.Seek(pHeader->GetInfoBlockOffset() + sizeof(detail::BinaryBlockHeader) +
                        infoBlock.body.sound.offset,
                    detail::fnd::Stream::SeekOrigin_Begin);

    detail::StreamSoundFile::StreamSoundInfo soundInfo;
    size_t readSize = fileStream.Read(&soundInfo, sizeof(soundInfo), nullptr);
    fileStream.Close();
    if (readSize != sizeof(soundInfo)) {
        return ResultFileAccessFailed();
    }

    pInfo->channelCount = soundInfo.channelCount;
    pInfo->sampleRate = soundInfo.sampleRate;
    pInfo->loopFlag = soundInfo.loop;
    pInfo->compatibleLoopStart = soundInfo.loopStart;
    pInfo->compatibleLoopEnd = soundInfo.loopEnd;

    if (detail::StreamSoundFileReader::IsOriginalLoopAvailableImpl(pHeader)) {
        pInfo->loopStart = soundInfo.originalLoopStart;
        pInfo->loopEnd = soundInfo.originalLoopEnd;
    } else {
        pInfo->loopStart = soundInfo.loopStart;
        pInfo->loopEnd = soundInfo.loopEnd;
    }

    return ResultSuccess();
}

/**
 * @brief Checks whether a stream sound of an archive is playing on any player.
 * @param soundId Stream sound item id.
 * @param pArchive Archive of the sound.
 * @return True when an instance of the sound is attached to a player.
 */
bool SoundArchivePlayer::IsStreamSoundPlaying(SoundArchive::ItemId soundId,
                                              const SoundArchive* pArchive) const {
    u8 isPlaying = false;
    for (u32 i = 0; i < m_SoundPlayerCount; i++) {
        SoundPlayer::SoundList& rSoundList = m_pSoundPlayers[i].detail_GetSoundList();
        for (auto it = rSoundList.begin(); it != rSoundList.end();) {
            detail::BasicSound& rSound = *it++;
            SoundHandle handle;
            handle.detail_AttachSoundAsTempHandle(&rSound);

            if (handle.GetId() == soundId &&
                handle.detail_GetAttachedSound()->GetSoundArchive() == pArchive) {
                isPlaying = true;
            }
        }
    }

    return isPlaying != 0;
}

/**
 * @brief Reads the playback information of a stream sound.
 * @param pInfo Receives the playback information.
 * @param soundId Stream sound item id.
 * @param pSoundArchiveName Addon archive name, or nullptr for the main archive.
 * @return Success, or the reason the information could not be read.
 */
Result SoundArchivePlayer::ReadStreamSoundDataInfo(StreamSoundDataInfo* pInfo,
                                                   SoundArchive::ItemId soundId,
                                                   const char* pSoundArchiveName) const {
    const SoundArchive* pArchive =
        pSoundArchiveName != nullptr ?
            m_SoundArchiveManager.GetAddonSoundArchive(pSoundArchiveName) :
            m_SoundArchiveManager.GetMainSoundArchive();

    return ReadStreamSoundDataInfo(pInfo, pArchive, soundId);
}

/**
 * @brief Reads the playback information of a stream sound by label.
 * @param pInfo Receives the playback information.
 * @param pLabel Stream sound label.
 * @param pSoundArchiveName Addon archive name, or nullptr for the main archive.
 * @return Success, or the reason the information could not be read.
 */
Result SoundArchivePlayer::ReadStreamSoundDataInfo(StreamSoundDataInfo* pInfo, const char* pLabel,
                                                   const char* pSoundArchiveName) const {
    const SoundArchive* pArchive =
        pSoundArchiveName != nullptr ?
            m_SoundArchiveManager.GetAddonSoundArchive(pSoundArchiveName) :
            m_SoundArchiveManager.GetMainSoundArchive();

    SoundArchive::ItemId soundId = pArchive->GetItemId(pLabel);
    if (soundId == SoundArchive::InvalidId) {
        return ResultInvalidLabelString();
    }

    return ReadStreamSoundDataInfo(pInfo, pArchive, soundId);
}

/**
 * @brief Reads the playback information of a main archive stream sound.
 * @param pInfo Receives the playback information.
 * @param soundId Stream sound item id.
 * @return Success, or the reason the information could not be read.
 */
Result SoundArchivePlayer::ReadStreamSoundDataInfo(StreamSoundDataInfo* pInfo,
                                                   SoundArchive::ItemId soundId) const {
    return ReadStreamSoundDataInfo(pInfo, m_SoundArchiveManager.GetMainSoundArchive(), soundId);
}

/**
 * @brief Reads the playback information of a main archive stream sound by label.
 * @param pInfo Receives the playback information.
 * @param pLabel Stream sound label.
 * @return Success, or the reason the information could not be read.
 */
Result SoundArchivePlayer::ReadStreamSoundDataInfo(StreamSoundDataInfo* pInfo,
                                                   const char* pLabel) const {
    const SoundArchive* pArchive = m_SoundArchiveManager.GetMainSoundArchive();

    SoundArchive::ItemId soundId = pArchive->GetItemId(pLabel);
    if (soundId == SoundArchive::InvalidId) {
        return ResultInvalidLabelString();
    }

    return ReadStreamSoundDataInfo(pInfo, pArchive, soundId);
}

/**
 * @brief Gets the work buffer size needed to read a stream sound header.
 * @return Work buffer size in bytes.
 */
size_t SoundArchivePlayer::GetRequiredWorkBufferSizeToReadStreamSoundHeader() {
    return StreamSoundHeaderWorkBufferSize;
}

/**
 * @brief Reads the sample ranges of named regions of a stream sound.
 * @param pInfo Receives one entry per region name.
 * @param soundId Stream sound item id.
 * @param ppRegionNames Names of the regions to look up.
 * @param regionCount Number of region names.
 * @param pArchive Archive of the sound.
 * @param pWorkBuffer Work buffer for the stream sound header.
 * @param workBufferSize Work buffer size in bytes.
 * @return Success, or the reason the regions could not be read.
 */
Result SoundArchivePlayer::ReadStreamSoundRegionDataInfo(StreamSoundRegionDataInfo* pInfo,
                                                         SoundArchive::ItemId soundId,
                                                         const char* const* ppRegionNames,
                                                         int regionCount,
                                                         const SoundArchive* pArchive,
                                                         void* pWorkBuffer,
                                                         size_t workBufferSize) const {
    if (IsStreamSoundPlaying(soundId, pArchive)) {
        return ResultStreamSoundPlaying();
    }

    char filePath[StreamFilePathBufferSize];
    if (!pArchive->ReadStreamSoundFilePath(filePath, sizeof(filePath), soundId)) {
        return ResultStreamFilePathNotFound();
    }

    detail::fnd::FileStreamImpl fileStream;
    if (!IsSucceeded(fileStream.Open(filePath, detail::fnd::FileStream::AccessMode_Read))) {
        fileStream.Close();
        return ResultFileAccessFailed();
    }

    detail::StreamSoundFileLoader loader(&fileStream);
    void* pHeaderBuffer =
        util::BytePtr(pWorkBuffer).AlignUp(StreamSoundHeaderWorkBufferAlignment).Get();
    detail::StreamSoundFileReader reader;
    if (!loader.LoadFileHeader(&reader, pHeaderBuffer, StreamSoundHeaderWorkBufferSize)) {
        fileStream.Close();
        return ResultStreamFileHeaderLoadFailed();
    }

    detail::StreamSoundFile::StreamSoundInfo soundInfo;
    if (!reader.ReadStreamSoundInfo(&soundInfo)) {
        fileStream.Close();
        return ResultUnknown();
    }

    if (soundInfo.regionCount == 0) {
        fileStream.Close();
        return ResultRegionNotFound();
    }

    for (int i = 0; i < regionCount; i++) {
        pInfo[i].regionNo = -1;
    }

    int foundCount = 0;
    for (u32 regionNo = 0; regionNo < soundInfo.regionCount && foundCount != regionCount;
         regionNo++) {
        RegionFileInfo regionInfo;
        if (!loader.ReadRegionInfo(
                reinterpret_cast<detail::StreamSoundFile::RegionInfo*>(&regionInfo), regionNo)) {
            continue;
        }

        for (int i = 0; i < regionCount; i++) {
            if (pInfo[i].regionNo != -1) {
                continue;
            }

            if (Strncmp(regionInfo.name, ppRegionNames[i], RegionNameLength) != 0) {
                continue;
            }

            pInfo[i].startSamplePosition = regionInfo.startSamplePosition;
            pInfo[i].endSamplePosition = regionInfo.endSamplePosition;
            pInfo[i].regionNo = regionNo;
            util::Strlcpy(pInfo[i].regionName, regionInfo.name, RegionNameLength);
            foundCount++;
        }
    }

    fileStream.Close();

    if (foundCount == regionCount) {
        return ResultSuccess();
    }

    if (foundCount >= 0 && foundCount < regionCount) {
        return ResultRegionNameNotFound();
    }

    return ResultUnknown();
}

/**
 * @brief Reads the sample range of one named region of a main archive stream sound.
 * @param pInfo Receives the region.
 * @param soundId Stream sound item id.
 * @param pRegionName Region name.
 * @param pWorkBuffer Work buffer for the stream sound header.
 * @param workBufferSize Work buffer size in bytes.
 * @return Success, or the reason the region could not be read.
 */
Result SoundArchivePlayer::ReadStreamSoundRegionDataInfo(StreamSoundRegionDataInfo* pInfo,
                                                         SoundArchive::ItemId soundId,
                                                         const char* pRegionName,
                                                         void* pWorkBuffer,
                                                         size_t workBufferSize) const {
    return ReadStreamSoundRegionDataInfo(pInfo, soundId, &pRegionName, 1,
                                         m_SoundArchiveManager.GetMainSoundArchive(),
                                         pWorkBuffer, workBufferSize);
}

/**
 * @brief Reads the sample ranges of named regions of a stream sound.
 * @param pInfo Receives one entry per region name.
 * @param soundId Stream sound item id.
 * @param ppRegionNames Names of the regions to look up.
 * @param regionCount Number of region names.
 * @param pWorkBuffer Work buffer for the stream sound header.
 * @param workBufferSize Work buffer size in bytes.
 * @param pSoundArchiveName Addon archive name, or nullptr for the main archive.
 * @return Success, or the reason the regions could not be read.
 */
Result SoundArchivePlayer::ReadStreamSoundRegionDataInfo(
    StreamSoundRegionDataInfo* pInfo, SoundArchive::ItemId soundId,
    const char* const* ppRegionNames, int regionCount, void* pWorkBuffer, size_t workBufferSize,
    const char* pSoundArchiveName) const {
    const SoundArchive* pArchive =
        pSoundArchiveName != nullptr ?
            m_SoundArchiveManager.GetAddonSoundArchive(pSoundArchiveName) :
            m_SoundArchiveManager.GetMainSoundArchive();

    return ReadStreamSoundRegionDataInfo(pInfo, soundId, ppRegionNames, regionCount, pArchive,
                                         pWorkBuffer, workBufferSize);
}

/**
 * @brief Reads the sample range of one named region of a main archive stream sound by label.
 * @param pInfo Receives the region.
 * @param pLabel Stream sound label.
 * @param pRegionName Region name.
 * @param pWorkBuffer Work buffer for the stream sound header.
 * @param workBufferSize Work buffer size in bytes.
 * @return Success, or the reason the region could not be read.
 */
Result SoundArchivePlayer::ReadStreamSoundRegionDataInfo(StreamSoundRegionDataInfo* pInfo,
                                                         const char* pLabel,
                                                         const char* pRegionName,
                                                         void* pWorkBuffer,
                                                         size_t workBufferSize) const {
    const SoundArchive* pArchive = m_SoundArchiveManager.GetMainSoundArchive();

    SoundArchive::ItemId soundId = pArchive->GetItemId(pLabel);
    if (soundId == SoundArchive::InvalidId) {
        return ResultInvalidLabelString();
    }

    return ReadStreamSoundRegionDataInfo(pInfo, soundId, &pRegionName, 1, pArchive, pWorkBuffer,
                                         workBufferSize);
}

/**
 * @brief Reads the sample ranges of named regions of a stream sound by label.
 * @param pInfo Receives one entry per region name.
 * @param pLabel Stream sound label.
 * @param ppRegionNames Names of the regions to look up.
 * @param regionCount Number of region names.
 * @param pWorkBuffer Work buffer for the stream sound header.
 * @param workBufferSize Work buffer size in bytes.
 * @param pSoundArchiveName Addon archive name, or nullptr for the main archive.
 * @return Success, or the reason the regions could not be read.
 */
Result SoundArchivePlayer::ReadStreamSoundRegionDataInfo(
    StreamSoundRegionDataInfo* pInfo, const char* pLabel, const char* const* ppRegionNames,
    int regionCount, void* pWorkBuffer, size_t workBufferSize,
    const char* pSoundArchiveName) const {
    const SoundArchive* pArchive =
        pSoundArchiveName != nullptr ?
            m_SoundArchiveManager.GetAddonSoundArchive(pSoundArchiveName) :
            m_SoundArchiveManager.GetMainSoundArchive();

    SoundArchive::ItemId soundId = pArchive->GetItemId(pLabel);
    if (soundId == SoundArchive::InvalidId) {
        return ResultInvalidLabelString();
    }

    return ReadStreamSoundRegionDataInfo(pInfo, soundId, ppRegionNames, regionCount, pArchive,
                                         pWorkBuffer, workBufferSize);
}

/**
 * @brief Reads the sample ranges of named regions of a main archive stream sound.
 * @param pInfo Receives one entry per region name.
 * @param soundId Stream sound item id.
 * @param ppRegionNames Names of the regions to look up.
 * @param regionCount Number of region names.
 * @param pWorkBuffer Work buffer for the stream sound header.
 * @param workBufferSize Work buffer size in bytes.
 * @return Success, or the reason the regions could not be read.
 */
Result SoundArchivePlayer::ReadStreamSoundRegionDataInfo(StreamSoundRegionDataInfo* pInfo,
                                                         SoundArchive::ItemId soundId,
                                                         const char* const* ppRegionNames,
                                                         int regionCount, void* pWorkBuffer,
                                                         size_t workBufferSize) const {
    return ReadStreamSoundRegionDataInfo(pInfo, soundId, ppRegionNames, regionCount,
                                         m_SoundArchiveManager.GetMainSoundArchive(),
                                         pWorkBuffer, workBufferSize);
}

/**
 * @brief Reads the sample ranges of named regions of a main archive stream sound by label.
 * @param pInfo Receives one entry per region name.
 * @param pLabel Stream sound label.
 * @param ppRegionNames Names of the regions to look up.
 * @param regionCount Number of region names.
 * @param pWorkBuffer Work buffer for the stream sound header.
 * @param workBufferSize Work buffer size in bytes.
 * @return Success, or the reason the regions could not be read.
 */
Result SoundArchivePlayer::ReadStreamSoundRegionDataInfo(StreamSoundRegionDataInfo* pInfo,
                                                         const char* pLabel,
                                                         const char* const* ppRegionNames,
                                                         int regionCount, void* pWorkBuffer,
                                                         size_t workBufferSize) const {
    const SoundArchive* pArchive = m_SoundArchiveManager.GetMainSoundArchive();

    SoundArchive::ItemId soundId = pArchive->GetItemId(pLabel);
    if (soundId == SoundArchive::InvalidId) {
        return ResultInvalidLabelString();
    }

    return ReadStreamSoundRegionDataInfo(pInfo, soundId, ppRegionNames, regionCount, pArchive,
                                         pWorkBuffer, workBufferSize);
}

/**
 * @brief Reads the sample range of one named region of a stream sound by label.
 * @param pInfo Receives the region.
 * @param pLabel Stream sound label.
 * @param pRegionName Region name.
 * @param pWorkBuffer Work buffer for the stream sound header.
 * @param workBufferSize Work buffer size in bytes.
 * @param pSoundArchiveName Addon archive name, or nullptr for the main archive.
 * @return Success, or the reason the region could not be read.
 */
Result SoundArchivePlayer::ReadStreamSoundRegionDataInfo(StreamSoundRegionDataInfo* pInfo,
                                                         const char* pLabel,
                                                         const char* pRegionName,
                                                         void* pWorkBuffer, size_t workBufferSize,
                                                         const char* pSoundArchiveName) const {
    const SoundArchive* pArchive =
        pSoundArchiveName != nullptr ?
            m_SoundArchiveManager.GetAddonSoundArchive(pSoundArchiveName) :
            m_SoundArchiveManager.GetMainSoundArchive();

    SoundArchive::ItemId soundId = pArchive->GetItemId(pLabel);
    if (soundId == SoundArchive::InvalidId) {
        return ResultInvalidLabelString();
    }

    return ReadStreamSoundRegionDataInfo(pInfo, soundId, &pRegionName, 1, pArchive, pWorkBuffer,
                                         workBufferSize);
}

/**
 * @brief Reads the sample range of one named region of a stream sound.
 * @param pInfo Receives the region.
 * @param soundId Stream sound item id.
 * @param pRegionName Region name.
 * @param pWorkBuffer Work buffer for the stream sound header.
 * @param workBufferSize Work buffer size in bytes.
 * @param pSoundArchiveName Addon archive name, or nullptr for the main archive.
 * @return Success, or the reason the region could not be read.
 */
Result SoundArchivePlayer::ReadStreamSoundRegionDataInfo(StreamSoundRegionDataInfo* pInfo,
                                                         SoundArchive::ItemId soundId,
                                                         const char* pRegionName,
                                                         void* pWorkBuffer, size_t workBufferSize,
                                                         const char* pSoundArchiveName) const {
    const SoundArchive* pArchive =
        pSoundArchiveName != nullptr ?
            m_SoundArchiveManager.GetAddonSoundArchive(pSoundArchiveName) :
            m_SoundArchiveManager.GetMainSoundArchive();

    return ReadStreamSoundRegionDataInfo(pInfo, soundId, &pRegionName, 1, pArchive, pWorkBuffer,
                                         workBufferSize);
}

/**
 * @brief Reads the markers of a main archive stream sound.
 * @param pInfoArray Receives the markers.
 * @param pCount Receives the number of markers in the file.
 * @param infoCount Capacity of pInfoArray.
 * @param soundId Stream sound item id.
 * @return Success, or the reason the markers could not be read.
 */
Result SoundArchivePlayer::ReadMarkerInfoArray(StreamSoundMarkerInfo* pInfoArray, int* pCount,
                                               int infoCount, SoundArchive::ItemId soundId) {
    return ReadMarkerInfoArrayImpl(pInfoArray, pCount, infoCount, soundId,
                                   m_SoundArchiveManager.GetMainSoundArchive());
}

/**
 * @brief Reads the markers of a stream sound.
 * @param pInfoArray Receives the markers.
 * @param pCount Receives the number of markers in the file.
 * @param infoCount Capacity of pInfoArray.
 * @param soundId Stream sound item id.
 * @param pSoundArchiveName Addon archive name, or nullptr for the main archive.
 * @return Success, or the reason the markers could not be read.
 */
Result SoundArchivePlayer::ReadMarkerInfoArray(StreamSoundMarkerInfo* pInfoArray, int* pCount,
                                               int infoCount, SoundArchive::ItemId soundId,
                                               const char* pSoundArchiveName) {
    const SoundArchive* pArchive =
        pSoundArchiveName != nullptr ?
            m_SoundArchiveManager.GetAddonSoundArchive(pSoundArchiveName) :
            m_SoundArchiveManager.GetMainSoundArchive();

    return ReadMarkerInfoArrayImpl(pInfoArray, pCount, infoCount, soundId, pArchive);
}

/**
 * @brief Reads the markers of a main archive stream sound by label.
 * @param pInfoArray Receives the markers.
 * @param pCount Receives the number of markers in the file.
 * @param infoCount Capacity of pInfoArray.
 * @param pLabel Stream sound label.
 * @return Success, or the reason the markers could not be read.
 */
Result SoundArchivePlayer::ReadMarkerInfoArray(StreamSoundMarkerInfo* pInfoArray, int* pCount,
                                               int infoCount, const char* pLabel) {
    const SoundArchive* pArchive = m_SoundArchiveManager.GetMainSoundArchive();

    SoundArchive::ItemId soundId = pArchive->GetItemId(pLabel);
    if (soundId == SoundArchive::InvalidId) {
        return ResultInvalidLabelString();
    }

    return ReadMarkerInfoArrayImpl(pInfoArray, pCount, infoCount, soundId, pArchive);
}

/**
 * @brief Reads the markers of a stream sound by label.
 * @param pInfoArray Receives the markers.
 * @param pCount Receives the number of markers in the file.
 * @param infoCount Capacity of pInfoArray.
 * @param pLabel Stream sound label.
 * @param pSoundArchiveName Addon archive name, or nullptr for the main archive.
 * @return Success, or the reason the markers could not be read.
 */
Result SoundArchivePlayer::ReadMarkerInfoArray(StreamSoundMarkerInfo* pInfoArray, int* pCount,
                                               int infoCount, const char* pLabel,
                                               const char* pSoundArchiveName) {
    const SoundArchive* pArchive =
        pSoundArchiveName != nullptr ?
            m_SoundArchiveManager.GetAddonSoundArchive(pSoundArchiveName) :
            m_SoundArchiveManager.GetMainSoundArchive();

    SoundArchive::ItemId soundId = pArchive->GetItemId(pLabel);
    if (soundId == SoundArchive::InvalidId) {
        return ResultInvalidLabelString();
    }

    return ReadMarkerInfoArrayImpl(pInfoArray, pCount, infoCount, soundId, pArchive);
}

/**
 * @brief Reads the markers of a stream sound from its stream file.
 * @param pInfoArray Receives the markers.
 * @param pCount Receives the number of markers in the file.
 * @param infoCount Capacity of pInfoArray.
 * @param soundId Stream sound item id.
 * @param pArchive Archive of the sound.
 * @return Success, or the reason the markers could not be read.
 */
Result SoundArchivePlayer::ReadMarkerInfoArrayImpl(StreamSoundMarkerInfo* pInfoArray,
                                                   int* pCount, int infoCount,
                                                   SoundArchive::ItemId soundId,
                                                   const SoundArchive* pArchive) {
    if (IsStreamSoundPlaying(soundId, pArchive)) {
        return ResultStreamSoundPlaying();
    }

    char filePath[StreamFilePathBufferSize];
    if (!pArchive->ReadStreamSoundFilePath(filePath, sizeof(filePath), soundId)) {
        return ResultStreamFilePathNotFound();
    }

    detail::fnd::FileStreamImpl fileStream;
    if (!IsSucceeded(fileStream.Open(filePath, detail::fnd::FileStream::AccessMode_Read))) {
        fileStream.Close();
        return ResultFileAccessFailed();
    }

    size_t fileSize = fileStream.GetSize();
    u8 headerBuffer[0x50];
    if (fileSize < sizeof(headerBuffer)) {
        fileStream.Close();
        return ResultFileAccessFailed();
    }

    fileStream.Read(headerBuffer, sizeof(headerBuffer), nullptr);
    const auto* pHeader = reinterpret_cast<const detail::StreamSoundFile::FileHeader*>(headerBuffer);
    if (!pHeader->HasMarkerBlock()) {
        fileStream.Close();
        return ResultInvalidFileFormat();
    }

    u32 markerOffset = pHeader->GetMarkerBlockOffset() + sizeof(detail::BinaryBlockHeader);
    size_t markerArrayOffset = static_cast<size_t>(markerOffset) + sizeof(u32);
    if (fileSize < markerArrayOffset) {
        fileStream.Close();
        return ResultFileAccessFailed();
    }

    fileStream.Seek(markerOffset, detail::fnd::Stream::SeekOrigin_Begin);
    u32 markerCount = 0;
    fileStream.Read(&markerCount, sizeof(markerCount), nullptr);

    int count = pHeader->GetMarkerBlockSize() / sizeof(MarkerFileInfo);
    *pCount = count;
    int readCount = count > infoCount ? infoCount : count;
    if (fileSize < markerArrayOffset + readCount * static_cast<s64>(sizeof(MarkerFileInfo))) {
        fileStream.Close();
        return ResultFileAccessFailed();
    }

    for (int i = 0; i < readCount; i++) {
        MarkerFileInfo markerInfo;
        fileStream.Read(&markerInfo, sizeof(markerInfo), nullptr);
        pInfoArray[i].position = markerInfo.position;
        util::Strlcpy(pInfoArray[i].name, markerInfo.name, MarkerNameLength);
    }

    fileStream.Close();
    return ResultSuccess();
}

/** @brief Prints the memory usage of the player; does nothing in release builds. */
void SoundArchivePlayer::DumpMemory() const {}

/**
 * @brief Reads how many stream sound instances are in use.
 * @param pState Receives the counts.
 * @return False if pState is nullptr.
 */
bool SoundArchivePlayer::ReadStreamSoundInstanceState(StreamSoundInstanceState* pState) const {
    if (pState == nullptr) {
        return false;
    }

    pState->activeStreamSoundCount = m_StreamSoundRuntime.GetActiveCount();
    pState->activeStreamChannelCount = m_StreamSoundRuntime.GetActiveChannelCount();
    pState->activeStreamTrackCount = m_StreamSoundRuntime.GetActiveTrackCount();
    return true;
}

/**
 * @brief Checks whether the file of a main archive stream sound exists.
 * @param soundId Stream sound item id.
 * @return Result of the file system lookup.
 */
Result SoundArchivePlayer::CheckStreamSoundFileExisting(SoundArchive::ItemId soundId) const {
    char filePath[StreamFilePathBufferSize];
    if (!GetSoundArchive().ReadStreamSoundFilePath(filePath, sizeof(filePath), soundId)) {
        return ResultStreamFilePathNotFound();
    }

    fs::DirectoryEntryType entryType;
    return fs::GetEntryType(&entryType, filePath);
}

/**
 * @brief Checks whether the file of a stream sound exists.
 * @param soundId Stream sound item id.
 * @param pSoundArchiveName Addon archive name, or nullptr for the main archive.
 * @return Result of the file system lookup.
 */
Result SoundArchivePlayer::CheckStreamSoundFileExisting(SoundArchive::ItemId soundId,
                                                        const char* pSoundArchiveName) const {
    const SoundArchive* pArchive =
        pSoundArchiveName != nullptr ?
            m_SoundArchiveManager.GetAddonSoundArchive(pSoundArchiveName) :
            m_SoundArchiveManager.GetMainSoundArchive();

    char filePath[StreamFilePathBufferSize];
    if (!pArchive->ReadStreamSoundFilePath(filePath, sizeof(filePath), soundId)) {
        return ResultStreamFilePathNotFound();
    }

    fs::DirectoryEntryType entryType;
    return fs::GetEntryType(&entryType, filePath);
}

/**
 * @brief Checks whether the file of a main archive stream sound exists.
 * @param pLabel Stream sound label.
 * @return Result of the file system lookup.
 */
Result SoundArchivePlayer::CheckStreamSoundFileExisting(const char* pLabel) const {
    const SoundArchive* pArchive = m_SoundArchiveManager.GetMainSoundArchive();

    SoundArchive::ItemId soundId = pArchive->GetItemId(pLabel);
    if (soundId == SoundArchive::InvalidId) {
        return ResultInvalidLabelString();
    }

    return CheckStreamSoundFileExisting(pArchive, soundId);
}

/**
 * @brief Checks whether the file of a stream sound exists.
 * @param pLabel Stream sound label.
 * @param pSoundArchiveName Addon archive name, or nullptr for the main archive.
 * @return Result of the file system lookup.
 */
Result SoundArchivePlayer::CheckStreamSoundFileExisting(const char* pLabel,
                                                        const char* pSoundArchiveName) const {
    const SoundArchive* pArchive =
        pSoundArchiveName != nullptr ?
            m_SoundArchiveManager.GetAddonSoundArchive(pSoundArchiveName) :
            m_SoundArchiveManager.GetMainSoundArchive();

    SoundArchive::ItemId soundId = pArchive->GetItemId(pLabel);
    if (soundId == SoundArchive::InvalidId) {
        return ResultInvalidLabelString();
    }

    return CheckStreamSoundFileExisting(pArchive, soundId);
}

/**
 * @brief Checks whether the file of a stream sound exists.
 * @param pArchive Archive of the sound.
 * @param soundId Stream sound item id.
 * @return Result of the file system lookup.
 */
Result SoundArchivePlayer::CheckStreamSoundFileExisting(const SoundArchive* pArchive,
                                                        SoundArchive::ItemId soundId) const {
    char filePath[StreamFilePathBufferSize];
    if (!pArchive->ReadStreamSoundFilePath(filePath, sizeof(filePath), soundId)) {
        return ResultStreamFilePathNotFound();
    }

    fs::DirectoryEntryType entryType;
    return fs::GetEntryType(&entryType, filePath);
}

/**
 * @brief Resolves a sound label in the main archive.
 * @param pString Sound label.
 * @return Sound item id, or SoundArchive::InvalidId.
 */
u32 SoundArchivePlayer::detail_GetItemId(const char* pString) {
    return detail_GetItemId(pString, nullptr);
}

/**
 * @brief Resolves a sound label in an archive.
 * @param pString Sound label.
 * @param pSoundArchiveName Addon archive name, or nullptr for the main archive.
 * @return Sound item id, or SoundArchive::InvalidId.
 */
u32 SoundArchivePlayer::detail_GetItemId(const char* pString, const char* pSoundArchiveName) {
    m_SoundArchiveManager.ChangeTargetArchive(pSoundArchiveName);
    return m_SoundArchiveManager.GetCurrentSoundArchive()->GetItemId(pString);
}
}  // namespace nn::atk
