#pragma once

#include <nn/atk/atk_AddonSoundArchiveContainer.h>
#include <nn/atk/atk_BasicSound.h>
#include <nn/atk/atk_SoundArchive.h>
#include <nn/atk/atk_SoundPlayer.h>
#include <nn/atk/atk_SoundStartable.h>
#include <nn/atk/detail/SoundArchiveManager.h>
#include <nn/atk/detail/StreamSoundRuntime.h>
#include <nn/atk/detail/atk_AdvancedWaveSoundRuntime.h>
#include <nn/atk/detail/seq/atk_SequenceSoundRuntime.h>
#include <nn/atk/detail/wsd/atk_WaveSoundRuntime.h>
#include <nn/audio.h>
#include <nn/os.h>

namespace nn::atk {
class OutputReceiver;
class SoundDataManager;

namespace detail {
class PlayerHeap;
class SoundArchiveFilesHook;
}  // namespace detail

/** @brief Playback information of a wave sound, as stored in its wave file. */
struct WaveSoundDataInfo {
    bool loopFlag;
    int sampleRate;
    s64 loopStart;
    s64 loopEnd;
    s64 compatibleLoopStart;
    s64 compatibleLoopEnd;
    int channelCount;
};

/** @brief Playback information of a stream sound, as stored in its stream file. */
struct StreamSoundDataInfo {
    bool loopFlag;
    int sampleRate;
    s64 loopStart;
    s64 loopEnd;
    s64 compatibleLoopStart;
    s64 compatibleLoopEnd;
    int channelCount;
};

/** @brief Frame range of one named region of a stream sound. */
struct StreamSoundRegionDataInfo {
    u32 startSamplePosition;
    u32 endSamplePosition;
    int regionNo;
    char regionName[64];
};
static_assert(sizeof(StreamSoundRegionDataInfo) == 0x4c);

/** @brief One marker of a stream sound. */
struct StreamSoundMarkerInfo {
    u32 position;
    char name[64];
};
static_assert(sizeof(StreamSoundMarkerInfo) == 0x44);

/** @brief Plays the sounds of a sound archive and of the addon archives added to it. */
class SoundArchivePlayer : public SoundStartable {
public:
    struct InitializeParam {
        InitializeParam()
            : pSoundArchive(nullptr), pSoundDataManager(nullptr), pSetupBuffer(nullptr),
              setupBufferSize(0), pStreamBuffer(nullptr), streamBufferSize(0),
              pStreamCacheBuffer(nullptr), streamCacheSize(0),
              enablePreparingStreamInstanceBufferFromSetupBuffer(true),
              pStreamInstanceBuffer(nullptr), streamInstanceBufferSize(0),
              userParamSizePerSound(0), addonSoundArchiveCount(0) {}

        const SoundArchive* pSoundArchive;
        const SoundDataManager* pSoundDataManager;
        void* pSetupBuffer;
        size_t setupBufferSize;
        void* pStreamBuffer;
        size_t streamBufferSize;
        void* pStreamCacheBuffer;
        size_t streamCacheSize;
        bool enablePreparingStreamInstanceBufferFromSetupBuffer;
        void* pStreamInstanceBuffer;
        size_t streamInstanceBufferSize;
        size_t userParamSizePerSound;
        int addonSoundArchiveCount;
    };
    static_assert(sizeof(InitializeParam) == 0x68);

    /** @brief Number of stream sound instances, channels and tracks in use. */
    struct StreamSoundInstanceState {
        int activeStreamSoundCount;
        int activeStreamChannelCount;
        int activeStreamTrackCount;
    };

    /** @brief How a sound behaves when the same sound is already playing. */
    enum SinglePlayType {
        SinglePlayType_None,
        SinglePlayType_PrioritizeOldest,
        SinglePlayType_PrioritizeOldestWithDuration,
        SinglePlayType_PrioritizeNewest,
        SinglePlayType_PrioritizeNewestWithDuration,
    };

    SoundArchivePlayer();
    ~SoundArchivePlayer() override;

    bool Initialize(const InitializeParam& rParam);
    void Finalize();
    void StopAllSound(int fadeFrames, bool isCancel);
    void DisposeInstances();
    bool IsAvailable() const;
    void Update();

    static size_t GetRequiredMemSize(const SoundArchive* pArchive);
    static size_t GetRequiredMemSize(const SoundArchive* pArchive, size_t userParamSizePerSound,
                                     int addonSoundArchiveCount);
    static size_t GetRequiredMemSize(const SoundArchive* pArchive, size_t userParamSizePerSound);
    static size_t GetRequiredMemSize(const InitializeParam& rParam);
    static size_t GetRequiredStreamInstanceSize(const SoundArchive* pArchive);
    size_t GetRequiredStreamBufferSize(const SoundArchive* pArchive) const;
    int GetRequiredStreamBufferTimes(const SoundArchive* pArchive) const;
    static size_t GetRequiredStreamCacheSize(const SoundArchive* pArchive, size_t cacheSizePerSound);
    static size_t GetRequiredWorkBufferSizeToReadStreamSoundHeader();

    /** @brief Counts the players of the main archive. @return Number of sound players. */
    u32 GetSoundPlayerCount() const { return m_SoundPlayerCount; }
    SoundPlayer& GetSoundPlayer(SoundArchive::ItemId playerId);
    const SoundPlayer& GetSoundPlayer(SoundArchive::ItemId playerId) const;
    SoundPlayer& GetSoundPlayer(const char* pPlayerName);
    const SoundPlayer& GetSoundPlayer(const char* pPlayerName) const;

    const SoundArchive& GetSoundArchive() const;
    const AddonSoundArchive* GetAddonSoundArchive(const char* pName) const;
    const AddonSoundArchive* GetAddonSoundArchive(int index) const;
    const char* GetAddonSoundArchiveName(int index) const;
    os::Tick GetAddonSoundArchiveAddTick(int index) const;
    const SoundDataManager* GetAddonSoundDataManager(const char* pName) const;
    /** @brief Counts the addon archives that are added. @return Number of addon archives. */
    int GetAddonSoundArchiveCount() const { return m_SoundArchiveManager.GetAddonSoundArchiveCount(); }

    void AddAddonSoundArchive(const char* pName, const AddonSoundArchive* pArchive,
                              const SoundDataManager* pDataManager);
    void RemoveAddonSoundArchive(const AddonSoundArchive* pArchive);
    void SetDefaultOutputReceiver(OutputReceiver* pOutputReceiver);
    const void* detail_GetFileAddress(SoundArchive::FileId fileId) const;

    bool IsSoundArchiveFileHooksEnabled() const;
    void LockSoundArchiveFileHooks();
    void UnlockSoundArchiveFileHooks();
    bool IsSequenceSoundEdited(const char* pLabel) const;
    bool IsStreamSoundEdited(const char* pLabel) const;
    bool IsWaveSoundEdited(const char* pLabel) const;

    void SetSequenceUserProcCallback(SequenceUserProcCallback callback, void* pArg);
    static void SetSequenceSkipIntervalTick(int tick);
    static int GetSequenceSkipIntervalTick();

    Result ReadWaveSoundDataInfo(WaveSoundDataInfo* pInfo, SoundArchive::ItemId soundId,
                                 const SoundArchive* pArchive,
                                 const SoundDataManager* pDataManager) const;
    Result ReadWaveSoundDataInfo(WaveSoundDataInfo* pInfo, SoundArchive::ItemId soundId,
                                 const char* pSoundArchiveName) const;
    Result ReadWaveSoundDataInfo(WaveSoundDataInfo* pInfo, const char* pLabel,
                                 const char* pSoundArchiveName) const;
    Result ReadWaveSoundDataInfo(WaveSoundDataInfo* pInfo, SoundArchive::ItemId soundId) const;
    Result ReadWaveSoundDataInfo(WaveSoundDataInfo* pInfo, const char* pLabel) const;

    Result ReadStreamSoundDataInfo(StreamSoundDataInfo* pInfo, const SoundArchive* pArchive,
                                   SoundArchive::ItemId soundId) const;
    bool IsStreamSoundPlaying(SoundArchive::ItemId soundId, const SoundArchive* pArchive) const;
    Result ReadStreamSoundDataInfo(StreamSoundDataInfo* pInfo, SoundArchive::ItemId soundId,
                                   const char* pSoundArchiveName) const;
    Result ReadStreamSoundDataInfo(StreamSoundDataInfo* pInfo, const char* pLabel,
                                   const char* pSoundArchiveName) const;
    Result ReadStreamSoundDataInfo(StreamSoundDataInfo* pInfo, SoundArchive::ItemId soundId) const;
    Result ReadStreamSoundDataInfo(StreamSoundDataInfo* pInfo, const char* pLabel) const;

    Result ReadStreamSoundRegionDataInfo(StreamSoundRegionDataInfo* pInfo,
                                         SoundArchive::ItemId soundId,
                                         const char* const* ppRegionNames, int regionCount,
                                         const SoundArchive* pArchive, void* pWorkBuffer,
                                         size_t workBufferSize) const;
    Result ReadStreamSoundRegionDataInfo(StreamSoundRegionDataInfo* pInfo,
                                         SoundArchive::ItemId soundId, const char* pRegionName,
                                         void* pWorkBuffer, size_t workBufferSize) const;
    Result ReadStreamSoundRegionDataInfo(StreamSoundRegionDataInfo* pInfo,
                                         SoundArchive::ItemId soundId,
                                         const char* const* ppRegionNames, int regionCount,
                                         void* pWorkBuffer, size_t workBufferSize,
                                         const char* pSoundArchiveName) const;
    Result ReadStreamSoundRegionDataInfo(StreamSoundRegionDataInfo* pInfo, const char* pLabel,
                                         const char* pRegionName, void* pWorkBuffer,
                                         size_t workBufferSize) const;
    Result ReadStreamSoundRegionDataInfo(StreamSoundRegionDataInfo* pInfo, const char* pLabel,
                                         const char* const* ppRegionNames, int regionCount,
                                         void* pWorkBuffer, size_t workBufferSize,
                                         const char* pSoundArchiveName) const;
    Result ReadStreamSoundRegionDataInfo(StreamSoundRegionDataInfo* pInfo,
                                         SoundArchive::ItemId soundId,
                                         const char* const* ppRegionNames, int regionCount,
                                         void* pWorkBuffer, size_t workBufferSize) const;
    Result ReadStreamSoundRegionDataInfo(StreamSoundRegionDataInfo* pInfo, const char* pLabel,
                                         const char* const* ppRegionNames, int regionCount,
                                         void* pWorkBuffer, size_t workBufferSize) const;
    Result ReadStreamSoundRegionDataInfo(StreamSoundRegionDataInfo* pInfo, const char* pLabel,
                                         const char* pRegionName, void* pWorkBuffer,
                                         size_t workBufferSize,
                                         const char* pSoundArchiveName) const;
    Result ReadStreamSoundRegionDataInfo(StreamSoundRegionDataInfo* pInfo,
                                         SoundArchive::ItemId soundId, const char* pRegionName,
                                         void* pWorkBuffer, size_t workBufferSize,
                                         const char* pSoundArchiveName) const;

    Result ReadMarkerInfoArray(StreamSoundMarkerInfo* pInfoArray, int* pCount, int infoCount,
                               SoundArchive::ItemId soundId);
    Result ReadMarkerInfoArray(StreamSoundMarkerInfo* pInfoArray, int* pCount, int infoCount,
                               SoundArchive::ItemId soundId, const char* pSoundArchiveName);
    Result ReadMarkerInfoArray(StreamSoundMarkerInfo* pInfoArray, int* pCount, int infoCount,
                               const char* pLabel);
    Result ReadMarkerInfoArray(StreamSoundMarkerInfo* pInfoArray, int* pCount, int infoCount,
                               const char* pLabel, const char* pSoundArchiveName);

    void DumpMemory() const;
    bool ReadStreamSoundInstanceState(StreamSoundInstanceState* pState) const;

    Result CheckStreamSoundFileExisting(SoundArchive::ItemId soundId) const;
    Result CheckStreamSoundFileExisting(SoundArchive::ItemId soundId,
                                        const char* pSoundArchiveName) const;
    Result CheckStreamSoundFileExisting(const char* pLabel) const;
    Result CheckStreamSoundFileExisting(const char* pLabel, const char* pSoundArchiveName) const;
    Result CheckStreamSoundFileExisting(const SoundArchive* pArchive,
                                        SoundArchive::ItemId soundId) const;

protected:
    StartResult detail_SetupSound(SoundHandle* pHandle, u32 soundId, bool holdFlag,
                                  const char* pSoundArchiveName,
                                  const StartInfo* pStartInfo) override;
    StartResult detail_SetupSoundImpl(SoundHandle* pHandle, u32 soundId,
                                      detail::BasicSound::AmbientInfo* pAmbientInfo,
                                      SoundActor* pActor, bool holdFlag,
                                      const char* pSoundArchiveName, const StartInfo* pStartInfo);

private:
    bool SetupMram(const SoundArchive* pArchive, void* pBuffer, size_t bufferSize,
                   size_t userParamSizePerSound, int addonSoundArchiveCount,
                   void* pStreamInstanceBuffer, size_t streamInstanceBufferSize);
    bool SetupSoundPlayer(const SoundArchive* pArchive, void** ppBuffer, const void* pEnd);
    bool SetupAddonSoundArchiveContainer(int containerCount, void** ppBuffer, const void* pEnd);
    bool SetupUserParamForBasicSound(const SoundArchive::SoundArchivePlayerInfo& rInfo,
                                     void** ppBuffer, const void* pEnd, size_t userParamSize);
    detail::PlayerHeap* CreatePlayerHeap(void** ppBuffer, const void* pEnd, size_t heapSize);
    void EnableHook(const SoundArchive* pArchive, bool isEnabled);
    StartResult PreprocessSinglePlay(const SoundArchive::SoundInfo& rInfo,
                                     SoundArchive::ItemId soundId, const SoundArchive* pArchive,
                                     SoundPlayer& rPlayer);
    void SetCommonSoundParam(detail::BasicSound* pSound, const SoundArchive::SoundInfo* pInfo);
    Result ReadMarkerInfoArrayImpl(StreamSoundMarkerInfo* pInfoArray, int* pCount, int infoCount,
                                   SoundArchive::ItemId soundId, const SoundArchive* pArchive);

    u32 detail_GetItemId(const char* pString) override;
    u32 detail_GetItemId(const char* pString, const char* pSoundArchiveName) override;

    detail::SoundArchiveManager m_SoundArchiveManager;
    u32 m_SoundPlayerCount;
    SoundPlayer* m_pSoundPlayers;
    detail::SequenceSoundRuntime m_SequenceSoundRuntime;
    detail::WaveSoundRuntime m_WaveSoundRuntime;
    detail::AdvancedWaveSoundRuntime m_AdvancedWaveSoundRuntime;
    detail::StreamSoundRuntime m_StreamSoundRuntime;
    size_t m_SoundUserParamSize;
    int m_AddonSoundArchiveContainerCount;
    detail::AddonSoundArchiveContainer* m_pAddonSoundArchiveContainers;
    os::Tick m_AddonSoundArchiveLastAddTick;
    audio::MemoryPoolType m_MemoryPool;
    bool m_IsMemoryPoolAttached;
    audio::MemoryPoolType m_MemoryPoolForPlayerHeap;
    bool m_IsMemoryPoolForPlayerHeapAttached;
    detail::SoundArchiveFilesHook* m_pSoundArchiveFilesHook;
    bool m_IsEnableWarningPrint;
    bool m_IsInitialized;
    bool m_IsAdvancedWaveSoundEnabled;
    OutputReceiver* m_pDefaultOutputReceiver;
    u8 _308[0x310 - 0x308];
};
static_assert(sizeof(SoundArchivePlayer) == 0x310);
}  // namespace nn::atk
