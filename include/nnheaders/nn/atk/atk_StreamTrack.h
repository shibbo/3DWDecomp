#pragma once
#include <nn/atk/atk_DecodeAdpcm.h>
#include <nn/atk/atk_Global.h>
#include <nn/atk/atk_OutputParam.h>
#include <nn/atk/atk_WaveBuffer.h>
#include <nn/atk/detail/voice/atk_MultiVoice.h>
#include <nn/types.h>

namespace nn::atk::detail::driver {
/**
 * @brief One channel of a stream sound: its ring of sample buffers and the voice playing them.
 *
 * The decoder states sit on their own cache lines; the padding is spelled out because the
 * players holding channels are allocated with plain (not over-aligned) operator new.
 */
class StreamChannel {
public:
    static const int BufferBlockCountMax = 32;

    /** @brief DSP ADPCM decoder state of one buffer block, kept on its own cache line. */
    struct BlockAdpcmContext {
        AdpcmContext context;
        u8 _6[0x3a];
    };

    void AppendWaveBuffer(WaveBuffer* buffer, bool last);

    void* mBufferAddress;
    MultiVoice* mVoice;
    WaveBuffer mWaveBuffer[BufferBlockCountMax];
    u8 _810[0x30];
    BlockAdpcmContext mAdpcmContext[BufferBlockCountMax];
    int mUpdateType;
    u8 _1044[0x3c];
};
static_assert(sizeof(StreamChannel) == 0x1080, "StreamChannel size");

/** @brief One track of a stream sound: the channels it plays and its mixing parameters. */
class StreamTrack {
public:
    static const int ChannelCountMax = 2;

    /** @brief Mixing parameters read from the stream file. */
    struct TrackInfo {
        u8 channelCount;
        u8 volume;
        u8 pan;
        u8 span;
        u8 mainSend;
        u8 fxSend[AuxBus_Count];
        u8 lpfFreq;
        u8 biquadType;
        u8 biquadValue;
        u8 flags;
    };

    bool mActiveFlag;
    StreamChannel* mChannels[ChannelCountMax];
    TrackInfo mTrackInfo;
    float mVolume;
    int mOutputLine;
    OutputParam mTvParam;
};
static_assert(sizeof(StreamTrack) == 0x80, "StreamTrack size");
}  // namespace nn::atk::detail::driver
