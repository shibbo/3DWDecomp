#pragma once

#include <nn/types.h>

namespace nn::atk {
/** @brief Records the final device output into a user-provided stream. */
class DeviceOutRecorder {
  public:
    /**
     * @brief Append interleaved output samples to the recording.
     * @param pSamples Interleaved 16-bit samples.
     * @param sampleCount Number of samples (across all channels) in pSamples.
     */
    void RecordSamples(const s16* pSamples, u32 sampleCount);
};
} // namespace nn::atk
