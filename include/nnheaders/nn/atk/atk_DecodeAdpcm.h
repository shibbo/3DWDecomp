#pragma once
#include <nn/types.h>

namespace nn::audio {
struct AdpcmParameter { s16 coefficients[16]; };
}
namespace nn::atk {
struct AdpcmContext { u16 predictorScale; s16 previousSample, previousSample2; };
/** @brief DSP ADPCM decoder state as stored in files and load requests, without padding. */
struct AdpcmContextNotAligned {
    u16 predictorScale;
    s16 previousSample;
    s16 previousSample2;
};
namespace detail {
void DecodeDspAdpcm(long offset, AdpcmContext& context, const nn::audio::AdpcmParameter& parameter,
                    const void* data, size_t count, s16* output);
}
}
