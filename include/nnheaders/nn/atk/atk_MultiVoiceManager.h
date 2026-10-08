#pragma once

#include <nn/atk/atk_StreamTrack.h>
#include <nn/types.h>

namespace nn::atk::detail {
struct SoundInstanceConfig;
}  // namespace nn::atk::detail

namespace nn::atk::detail::driver {
/** @brief Driver-side owner of every active multi-channel voice. */
class MultiVoiceManager {
  public:
    static MultiVoiceManager& GetInstance();
    size_t GetRequiredMemSize(int voiceCount, const SoundInstanceConfig& rConfig);
    void Initialize(void* pBuffer, size_t bufferSize, const SoundInstanceConfig& rConfig);
    void Finalize();
    MultiVoice* AllocVoice(int channelCount, int priority, MultiVoice::VoiceCallback callback,
                           void* pCallbackArg);
    void FreeVoice(MultiVoice* pVoice);
    void ChangeVoicePriority(MultiVoice* pVoice);
    /**
     * @brief Synchronously refresh parameters of all voices.
     * @param updateFlag Bit set selecting which parameters to recompute.
     */
    void UpdateAllVoicesSync(u32 updateFlag);
};
} // namespace nn::atk::detail::driver
