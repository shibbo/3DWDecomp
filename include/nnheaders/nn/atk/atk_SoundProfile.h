#pragma once

#include <nn/os.h>
#include <nn/types.h>

namespace nn::atk {
/** @brief Processing times measured for one audio frame. */
struct SoundProfile {
    static const int VoiceProfileCountMax = 192;

    /** @brief Start and end of the rendering of one voice. */
    struct VoiceProfile {
        /** @brief Gets the time spent rendering the voice. @return Elapsed ticks. */
        s64 GetProcessTick() const { return end - begin; }

        s64 begin;
        s64 end;
    };

    // Frame-level timings preceding the voice count await reconstruction.
    u8 _0[0x7c];
    u32 voiceProfileCount;
    u8 _80[0xa8 - 0x80];
    VoiceProfile voiceProfile[VoiceProfileCountMax];
    u32 voiceIdTable[VoiceProfileCountMax];
};
}  // namespace nn::atk
