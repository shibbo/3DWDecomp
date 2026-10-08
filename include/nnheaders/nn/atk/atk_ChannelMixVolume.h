#pragma once
#include <nn/types.h>

namespace nn::atk {
struct MixVolume { float frontLeft, frontRight, rearLeft, rearRight, frontCenter, lfe; };
class ChannelMixVolume {
public:
    ChannelMixVolume();
    explicit ChannelMixVolume(const MixVolume& volume);
    ChannelMixVolume(const float* volumes, int count);
    void InitializeChannelVolume();
    bool SetChannelCount(int count);
    bool SetChannelVolume(int index, float volume);
    bool SetChannelVolume(int index, const float* volumes, int count);
    float GetChannelVolume(int index) const;
    /** @brief Gets the number of channels with a volume. @return Channel count. */
    int GetChannelCount() const { return mChannelCount; }

private:
    int mChannelCount;
    float mVolumes[24];
};
static_assert(sizeof(ChannelMixVolume) == 0x64, "ChannelMixVolume size");
}
