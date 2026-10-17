#pragma once

#include <nn/atk/atk_Global.h>
#include <nn/atk/atk_MixParameter.h>

namespace nn::atk::detail {
/**
 * @brief Per-output mixing parameters (volume, mix matrix, pan and sends) for one output device.
 */
struct OutputParam {
    static const int WaveChannelMax = 2;

    /** @brief Creates the parameters with a unity mix matrix; the remaining fields are left unset. */
    OutputParam() {
        for (int ch = 0; ch < WaveChannelMax; ch++) {
            for (int i = ChannelIndex_Count - 1; i >= 0; i--) {
                mixParameter[ch].ch[i] = 1.0f;
            }
        }
    }

    float volume;
    u32 mixMode;
    MixParameter mixParameter[WaveChannelMax];
    float pan;
    float span;
    float mainSend;
    float fxSend[AuxBus_Count];
};
static_assert(sizeof(OutputParam) == 0x50, "OutputParam size");
}  // namespace nn::atk::detail

namespace nn::atk {
using detail::OutputParam;
}  // namespace nn::atk

// Kept for files that used this header to reach BasicSound; BasicSound itself needs OutputParam.
#include <nn/atk/atk_BasicSound.h>
