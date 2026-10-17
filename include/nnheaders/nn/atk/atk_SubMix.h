#pragma once
#include <nn/atk/atk_FinalMix.h>

namespace nn::atk {
namespace detail::driver {
class HardwareManager;
}

class SubMix : public OutputMixer {
    friend class detail::driver::HardwareManager;

public:
    static size_t GetRequiredMemorySize(int busCount, int channelCount, int destinationBusCount,
                                        int destinationChannelCount, bool isEffectEnabled,
                                        bool isInternalCall);
    bool Initialize(int busCount, int channelCount, int destinationBusCount,
                    int destinationChannelCount, bool isEffectEnabled, bool isInternalCall,
                    void* buffer, size_t bufferSize);
    void Finalize();
    bool AppendEffect(EffectBase* pEffect, int bus, void* pBuffer, size_t bufferSize);
    bool AppendEffect(EffectAux* pEffect, int bus, void* pBuffer, size_t bufferSize);
    bool RemoveEffect(EffectBase* pEffect, int bus);
    bool RemoveEffect(EffectAux* pEffect, int bus);
    bool ClearEffect(int bus);
    void Update(int audioFrameCount);
    void SetDestination(OutputReceiver* pReceiver);
    void SetSend(int sourceBus, int destinationBus, float volume);
    void SetSendImpl(int sourceBus, int sourceChannel, int destinationBus, int destinationChannel,
                     float volume);
    float GetSendImpl(int sourceBus, int sourceChannel, int destinationBus,
                      int destinationChannel) const;
    void SetBusVolume(int bus, float volume, int fadeFrames);
    float GetBusVolume(int bus) const;
    void UpdateBusMixVolume(int bus);
    void SetBusMute(int bus, bool isMute);
    void SetMuteUnusedEffectChannel(bool isUnusedEffectChannelMuted);
    ReceiverType GetReceiverType() const override;
    int GetChannelCount() const override;
    int GetBusCount() const override;
    void AddReferenceCount(int count) override;
    bool IsSoundSendClampEnabled(int bus) const override;

    audio::SubMixType* GetSubMix() { return &mSubMix; }
private:
    util::IntrusiveListNode mLinkNode;
    audio::SubMixType mSubMix;
    // Remaining routing, volume, synchronization, and state fields await reconstruction.
    u8 _58[0xf8 - 0x58];
};
}
