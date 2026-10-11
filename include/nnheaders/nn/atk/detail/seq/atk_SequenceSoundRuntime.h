#pragma once

#include <nn/atk/atk_BasicSound.h>
#include <nn/atk/atk_SoundArchive.h>
#include <nn/atk/atk_SoundStartable.h>
#include <nn/atk/detail/SoundArchiveManager.h>
#include <nn/types.h>

namespace nn::atk {
class OutputReceiver;
struct SequenceUserProcCallbackParam;
typedef void (*SequenceUserProcCallback)(u16 procId, SequenceUserProcCallbackParam* pParam,
                                         void* pArg);

namespace detail {
class SequenceSound;
class StartInfoReader;

/** @brief Owns the sequence sound instances and sequence tracks of a sound archive player. */
class SequenceSoundRuntime {
public:
    SequenceSoundRuntime();
    ~SequenceSoundRuntime();

    static size_t GetRequiredMemorySize(const SoundArchive::SoundArchivePlayerInfo& rInfo,
                                        int alignment);
    static size_t
    GetRequiredSequenceTrackMemorySize(const SoundArchive::SoundArchivePlayerInfo& rInfo,
                                       int alignment);

    bool Initialize(int soundCount, void** ppBuffer, const void* pEnd);
    void Finalize();
    bool SetupSequenceTrack(int trackCount, void** ppBuffer, const void* pEnd);
    void SetupUserParam(void** ppBuffer, size_t userParamSize);
    void Update();
    static void SetSequenceSkipIntervalTick(int tick);
    static int GetSequenceSkipIntervalTick();

    SequenceSound* AllocSound(SoundArchive::ItemId soundId, int priority, int ambientPriority,
                              BasicSound::AmbientInfo* pAmbientInfo,
                              OutputReceiver* pOutputReceiver);
    SoundStartable::StartResult PrepareImpl(const SoundArchiveManager::SnapShot& rSnapShot,
                                            SoundArchive::ItemId soundId, SequenceSound* pSound,
                                            const SoundArchive::SoundInfo* pSoundInfo,
                                            const StartInfoReader& rStartInfoReader);

    /**
     * @brief Sets the user procedure callback handed to every new sequence.
     * @param callback Callback invoked by user procedure commands, or nullptr.
     * @param pArg Argument passed unchanged to the callback.
     */
    void SetSequenceUserProcCallback(SequenceUserProcCallback callback, void* pArg) {
        m_SequenceUserProcCallback = callback;
        m_pSequenceUserProcCallbackArg = pArg;
    }

    /**
     * @brief Sets the archive manager used to resolve sequence banks.
     * @param pManager Archive manager owned by the sound archive player.
     */
    void SetSoundArchiveManager(SoundArchiveManager* pManager) { m_pSoundArchiveManager = pManager; }

private:
    u8 _0[0xc8];
    SequenceUserProcCallback m_SequenceUserProcCallback;
    void* m_pSequenceUserProcCallbackArg;
    SoundArchiveManager* m_pSoundArchiveManager;
    u8 _e0[0xe8 - 0xe0];
};
static_assert(sizeof(SequenceSoundRuntime) == 0xe8, "SequenceSoundRuntime size");
}  // namespace detail
}  // namespace nn::atk
