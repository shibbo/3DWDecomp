#pragma once

#include <nn/types.h>

namespace nn::atk::detail {
struct SoundInstanceConfig;
}  // namespace nn::atk::detail

namespace nn::atk::detail::driver {
/** @brief Owner of the pool of driver channels. */
class ChannelManager {
public:
    static ChannelManager& GetInstance();
    size_t GetRequiredMemSize(int channelCount, const SoundInstanceConfig& rConfig);
    void Initialize(void* pBuffer, size_t bufferSize, int channelCount,
                    const SoundInstanceConfig& rConfig);
    void Finalize();
};
}  // namespace nn::atk::detail::driver
