#pragma once

#include <nn/atk/atk_BasicSound.h>
#include <nn/atk/atk_SoundArchive.h>
#include <nn/atk/atk_SoundStartable.h>
#include <nn/types.h>

namespace nn::atk {
class OutputReceiver;
class SoundDataManager;

namespace detail {
class WaveSound;
class StartInfoReader;

/** @brief Owns the wave sound instances of a sound archive player. */
class WaveSoundRuntime {
public:
    WaveSoundRuntime();
    ~WaveSoundRuntime();

    static size_t GetRequiredMemorySize(const SoundArchive::SoundArchivePlayerInfo& rInfo,
                                        int alignment);

    bool Initialize(int soundCount, void** ppBuffer, const void* pEnd);
    void Finalize();
    void SetupUserParam(void** ppBuffer, size_t userParamSize);
    void Update();

    WaveSound* AllocSound(SoundArchive::ItemId soundId, int priority, int ambientPriority,
                          BasicSound::AmbientInfo* pAmbientInfo, OutputReceiver* pOutputReceiver);
    SoundStartable::StartResult PrepareImpl(const SoundArchive* pArchive,
                                            const SoundDataManager* pDataManager,
                                            SoundArchive::ItemId soundId, WaveSound* pSound,
                                            const SoundArchive::SoundInfo* pSoundInfo,
                                            const StartInfoReader& rStartInfoReader);

private:
    u8 _0[0x88];
};
static_assert(sizeof(WaveSoundRuntime) == 0x88, "WaveSoundRuntime size");
}  // namespace detail
}  // namespace nn::atk
