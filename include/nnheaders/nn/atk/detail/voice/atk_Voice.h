#pragma once

#include <nn/atk/atk_BiquadFilterCallback.h>
#include <nn/atk/atk_DecodeAdpcm.h>
#include <nn/atk/atk_Global.h>
#include <nn/atk/atk_WaveBuffer.h>
#include <nn/types.h>

namespace nn::atk {
class OutputReceiver;

/** @brief Gains from one wave channel into every channel of every bus of an output receiver. */
struct OutputMix {
    static const int ChannelGainCount = 24;

    float channelGain[ChannelGainCount];
};
static_assert(sizeof(OutputMix) == 0x60, "OutputMix size");
}  // namespace nn::atk

namespace nn::atk::detail {
/** @brief Renderer-side voice backing one channel of a driver voice. */
class LowLevelVoice {
public:
    /** @brief Gets the identifier the profiler records this voice under. @return Identifier. */
    u32 GetVoiceId() const { return mVoiceId; }

private:
    // Renderer voice state preceding the identifier awaits reconstruction.
    u8 _0[0xe8];
    u32 mVoiceId;
};

/** @brief Playback state requested for a voice. */
enum VoiceState {
    VoiceState_Play,
    VoiceState_Stop,
    VoiceState_Pause,
};

/** @brief Driver voice for one wave channel. */
class Voice {
public:
    Voice();
    ~Voice();

    bool AllocVoice(u32 priority);
    void Free();
    void SetState(VoiceState state);
    bool IsAvailable() const;
    void UpdateParam();
    void SetPriority(u32 priority);
    void SetMonoFilter(bool enable, u16 cutoff);
    void SetBiquadFilter(bool enable, const BiquadFilterCoefficients* pCoefficients);
    size_t GetPlayPosition() const;
    void AppendWaveBuffer(WaveBuffer* pBuffer);

    /** @brief Gets the requested playback state. @return Playback state. */
    VoiceState GetState() const { return mState; }

    /** @brief Sets the output volume. @param volume Linear gain. */
    void SetVolume(float volume) { mVolume = volume; }

    /** @brief Sets the pitch. @param pitch Frequency ratio. */
    void SetPitch(float pitch) { mPitch = pitch; }

    /** @brief Sets the gains into the TV output receiver. @param rMix Channel gains. */
    void SetTvMix(const OutputMix& rMix) { mTvMix = rMix; }

    /** @brief Sets the sample interpolation type. @param type Interpolation type. */
    void SetInterpolationType(u8 type) { mInterpolationType = type; }

    /** @brief Sets the sample data encoding. @param format Sample format. */
    void SetSampleFormat(SampleFormat format) { mSampleFormat = format; }

    /** @brief Sets the sample rate of the wave data. @param sampleRate Sample rate in hertz. */
    void SetSampleRate(int sampleRate) { mSampleRate = sampleRate; }

    /** @brief Sets the DSP ADPCM coefficients. @param rParam Decoder coefficients. */
    void SetAdpcmParam(const audio::AdpcmParameter& rParam) { mAdpcmParam = rParam; }

    /** @brief Sets the receiver mixed into. @param pReceiver Output receiver. */
    void SetOutputReceiver(OutputReceiver* pReceiver) { mpOutputReceiver = pReceiver; }

    /** @brief Gets the renderer voice in use. @return Renderer voice, or nullptr if none. */
    LowLevelVoice* GetLowLevelVoice() const { return mLowLevelVoice; }

private:
    u8 _0[4];
    VoiceState mState;
    float mVolume;
    float mPitch;
    OutputMix mTvMix;
    u8 _70[0xe];
    u8 mInterpolationType;
    u8 _7f;
    SampleFormat mSampleFormat;
    int mSampleRate;
    audio::AdpcmParameter mAdpcmParam;
    OutputReceiver* mpOutputReceiver;
    // Playback state following the receiver awaits reconstruction.
    u8 _b0[0x28];
    LowLevelVoice* mLowLevelVoice;
};
static_assert(sizeof(Voice) == 0xe0, "Voice size");
}  // namespace nn::atk::detail
