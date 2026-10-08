#pragma once

#include <nn/types.h>

namespace nn::atk {

/** @brief Sequence variable storage, which NW4F declares volatile. */
typedef volatile s16 vs16;

/** @brief Pause policy shared by sound players and individual sounds. */
enum PauseMode {
    PauseMode_Default,
    PauseMode_PauseImmediately,
};

/** @brief Encoding of wave and stream sample data. */
enum SampleFormat : int {
    SampleFormat_PcmS8,
    SampleFormat_PcmS16,
    SampleFormat_DspAdpcm,
};

enum OutputMode {
    OutputMode_Monaural,
    OutputMode_Stereo,
    OutputMode_Surround,
    OutputMode_Dpl2,
    OutputMode_Count
};

enum AuxBus {
    AuxBus_A,
    AuxBus_B,
    AuxBus_C,
    AuxBus_Count
};

enum OutputDevice {
    OutputDevice_Main,
    OutputDevice_Count
};

enum SampleRateConverterType {
    SampleRateConverterType_None,
    SampleRateConverterType_Linear,
    SampleRateConverterType_4Tap,
    SampleRateConverterType_Count
};

enum ChannelIndex {
    ChannelIndex_FrontLeft = 0,
    ChannelIndex_FrontRight = 1,
    ChannelIndex_RearLeft = 2,
    ChannelIndex_RearRight = 3,
    ChannelIndex_FrontCenter,
    ChannelIndex_Lfe,
    ChannelIndex_Count
};

namespace detail::Util {
template <typename T>
class Singleton {
public:
    static T& GetInstance();
};
}  // namespace detail::Util
}  // namespace nn::atk
