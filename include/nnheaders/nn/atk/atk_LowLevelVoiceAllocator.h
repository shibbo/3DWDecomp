#pragma once

#include <nn/types.h>

namespace nn::atk::detail {
/** @brief Fixed-capacity pool of low-level (renderer-side) voices. */
class LowLevelVoiceAllocator {
  public:
    static size_t GetRequiredMemSize(int voiceCount);
    void Initialize(int voiceCount, void* buffer, size_t bufferSize);
    void Finalize();
    int GetDroppedVoiceCount() const;

  private:
    // Voice table, free list and drop counters await reconstruction.
    u8 _0[0x28];
};
static_assert(sizeof(LowLevelVoiceAllocator) == 0x28, "Low-level voice allocator size");
} // namespace nn::atk::detail
