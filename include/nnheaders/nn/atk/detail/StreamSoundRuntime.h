/**
 * @file StreamSoundRuntime.h
 * @brief Stream sound runtime information.
 */

#pragma once

#include <nn/types.h>
#include <nn/atk/atk_LoaderManager.h>
#include <nn/atk/atk_StreamBufferPool.h>
#include <nn/atk/atk_BasicSound.h>
#include <nn/atk/atk_SoundArchive.h>
#include <nn/atk/atk_SoundStartable.h>

namespace nn {
namespace atk {
class OutputReceiver;
class SoundDataManager;
namespace detail {
class StartInfoReader;
class StreamSound;
namespace driver {
class StreamSoundLoader;
}
class StreamSoundRuntime {
  public:
    StreamSoundRuntime();
    ~StreamSoundRuntime();

    static size_t GetRequiredStreamInstanceSize(int soundCount);
    static size_t GetRequiredMemorySize(const SoundArchive::SoundArchivePlayerInfo& rInfo,
                                        int alignment);
    static int GetRequiredStreamBufferTimes(const SoundArchive* pArchive);
    static size_t GetRequiredStreamCacheSize(const SoundArchive* pArchive, size_t cacheSizePerSound);
    size_t GetRequiredStreamBufferSize(const SoundArchive* pArchive) const;
    bool Initialize(int soundCount, void** ppBuffer, const void* pEnd, void* pInstanceBuffer,
                    size_t instanceBufferSize);
    void Finalize();
    bool SetupStreamBuffer(const SoundArchive* pArchive, void* pBuffer, size_t size);
    bool SetupStreamCacheBuffer(const SoundArchive* pArchive, void* pBuffer, size_t size);
    void SetupUserParam(void** ppBuffer, size_t userParamSize);
    void Update();
    int GetActiveCount() const;
    int GetActiveChannelCount() const;
    int GetActiveTrackCount() const;
    StreamSound* AllocSound(SoundArchive::ItemId soundId, int priority, int ambientPriority,
                            BasicSound::AmbientInfo* pAmbientInfo, OutputReceiver* pOutputReceiver);
    SoundStartable::StartResult PrepareImpl(const SoundArchive* pArchive,
                                            const SoundDataManager* pDataManager,
                                            SoundArchive::ItemId soundId, StreamSound* pSound,
                                            const SoundArchive::SoundInfo* pSoundInfo,
                                            const StartInfoReader& rStartInfoReader);

  private:
    void* mInstanceMemory;
    size_t mInstanceMemorySize;
    u8 _10[8];
    util::IntrusiveListNode mActiveSounds;
    util::IntrusiveListNode mFreeSounds;
    LoaderManager<driver::StreamSoundLoader> mLoaders;
    driver::StreamBufferPool mStreamBufferPool;
    driver::StreamBufferPool* mCurrentStreamBufferPool;
    int mStreamBufferTimes;
};
static_assert(sizeof(StreamSoundRuntime) == 0xb8, "StreamSoundRuntime size");
} // namespace detail
} // namespace atk
} // namespace nn
