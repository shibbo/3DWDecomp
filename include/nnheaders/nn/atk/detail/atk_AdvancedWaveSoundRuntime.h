#pragma once

#include <nn/atk/atk_BasicSound.h>
#include <nn/atk/atk_SoundArchive.h>
#include <nn/atk/atk_SoundStartable.h>
#include <nn/types.h>

namespace nn::atk {
class OutputReceiver;
class SoundDataManager;

namespace detail {
class AdvancedWaveSound;
class StartInfoReader;

/** @brief Owns the advanced wave sound instances of a sound archive player. */
class AdvancedWaveSoundRuntime {
public:
    AdvancedWaveSoundRuntime();
    ~AdvancedWaveSoundRuntime();

    static size_t GetRequiredMemorySize(const SoundArchive::SoundArchivePlayerInfo& rInfo,
                                        size_t alignment);

    bool Initialize(int soundCount, void** ppBuffer, const void* pEnd);
    void Finalize();
    void SetupUserParam(void** ppBuffer, size_t userParamSize);
    void Update();

    AdvancedWaveSound* AllocSound(SoundArchive::ItemId soundId, int priority, int ambientPriority,
                                  BasicSound::AmbientInfo* pAmbientInfo,
                                  OutputReceiver* pOutputReceiver);
    SoundStartable::StartResult PrepareImpl(const SoundArchive* pArchive,
                                            const SoundDataManager* pDataManager,
                                            SoundArchive::ItemId soundId,
                                            AdvancedWaveSound* pSound,
                                            const SoundArchive::SoundInfo* pSoundInfo,
                                            const StartInfoReader& rStartInfoReader);

private:
    u8 _0[0x38];
};
static_assert(sizeof(AdvancedWaveSoundRuntime) == 0x38, "AdvancedWaveSoundRuntime size");
}  // namespace detail
}  // namespace nn::atk
