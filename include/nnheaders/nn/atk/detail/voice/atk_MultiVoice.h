#pragma once

#include <nn/atk/atk_BasicSound.h>
#include <nn/atk/atk_Global.h>
#include <nn/atk/atk_OutputParam.h>
#include <nn/atk/atk_WaveBuffer.h>
#include <nn/atk/detail/voice/atk_Voice.h>
#include <nn/types.h>
#include <nn/util/util_IntrusiveList.h>

namespace nn::atk {
class OutputReceiver;
namespace detail {
template <typename T>
class ValueArray;
class BusMixVolumePacket;
class OutputAdditionalParam;
class VolumeThroughModePacket;
struct OutputBusMixVolume;
}  // namespace detail
}  // namespace nn::atk

namespace nn::atk::detail::driver {
/** @brief Driver voice playing up to two wave channels as one unit. */
class MultiVoice {
public:
    static const int WaveChannelMax = 2;

    /** @brief Why a voice callback was invoked. */
    enum VoiceCallbackStatus {
        VoiceCallbackStatus_FinishWave,
        VoiceCallbackStatus_Cancel,
        VoiceCallbackStatus_DropVoice,
        VoiceCallbackStatus_DropDsp,
    };

    /** @brief How the wave channels of the voice are mixed. */
    enum VoiceMode {
        VoiceMode_Mono,
        VoiceMode_StereoLeft,
        VoiceMode_StereoRight,
    };

    /** @brief Parameters that changed since the last Calc() or Update(). */
    enum UpdateFlag {
        UpdateFlag_Start = 1 << 0,
        UpdateFlag_Pause = 1 << 1,
        UpdateFlag_Src = 1 << 2,
        UpdateFlag_Mix = 1 << 3,
        UpdateFlag_Lpf = 1 << 4,
        UpdateFlag_Biquad = 1 << 5,
        UpdateFlag_Ve = 1 << 6,
    };

    /** @brief Per-channel volumes of every bus before the send levels are applied. */
    struct PreMixVolume {
        float volume[OutputMix::ChannelGainCount];
    };

    typedef void (*VoiceCallback)(MultiVoice* pVoice, VoiceCallbackStatus status, void* pArg);

    MultiVoice();
    explicit MultiVoice(OutputAdditionalParam* pAdditionalParam);
    ~MultiVoice();

    bool Alloc(int channelCount, int priority, VoiceCallback callback, void* pCallbackArg);
    void InitParam(VoiceCallback callback, void* pCallbackArg);
    void Free();
    void Start();
    void Stop();
    void StopAllSdkVoice();
    void UpdateVoiceStatus();
    bool IsPlayFinished() const;
    void Pause(bool isPause);
    void Calc();
    void CalcSrc(bool isInitialize);
    void CalcVe();
    void CalcMix();
    void CalcLpf();
    void CalcBiquadFilter();
    void Update();
    void RunAllSdkVoice();
    void PauseAllSdkVoice();
    void SetSampleFormat(SampleFormat format);
    void SetSampleRate(int sampleRate);
    void SetVolume(float volume);
    void SetPitch(float pitch);
    void SetPanMode(PanMode mode);
    void SetPanCurve(PanCurve curve);
    void SetLpfFreq(float lpfFreq);
    void SetBiquadFilter(int type, float value);
    void SetPriority(int priority);
    void SetOutputLine(u32 outputLine);
    void SetOutputParamImpl(const OutputParam& rParam, OutputParam& rOutputParam);
    void SetOutputAdditionalParamImpl(const ValueArray<float>* pAdditionalSend,
                                      const BusMixVolumePacket* pBusMixVolumePacket,
                                      const OutputBusMixVolume* pBusMixVolume,
                                      const VolumeThroughModePacket* pVolumeThroughModePacket);
    void SetOutputBusMixVolumeImpl(const BusMixVolumePacket& rPacket,
                                   const OutputBusMixVolume& rBusMixVolume,
                                   BusMixVolumePacket& rOutputPacket);
    void SetOutputVolumeThroughModePacketImpl(const VolumeThroughModePacket& rPacket,
                                              VolumeThroughModePacket& rOutputPacket);
    void SetTvParam(const OutputParam& rParam);
    void SetTvAdditionalParam(const OutputAdditionalParam& rParam);
    void SetTvAdditionalParam(const ValueArray<float>* pAdditionalSend,
                              const BusMixVolumePacket* pBusMixVolumePacket,
                              const OutputBusMixVolume* pBusMixVolume,
                              const VolumeThroughModePacket* pVolumeThroughModePacket);
    void SetOutputReceiver(OutputReceiver* pReceiver);
    void CalcPreMixVolume(PreMixVolume* pPreMixVolume, const OutputParam& rParam,
                          const OutputAdditionalParam* pAdditionalParam, int channelIndex,
                          OutputDevice device);
    void CalcTvMix(OutputMix* pMix, const PreMixVolume& rPreMixVolume);
    void CalcMixImpl(OutputMix* pMix, u32 outputDeviceIndex, const OutputParam& rParam,
                     const OutputAdditionalParam* pAdditionalParam,
                     const PreMixVolume& rPreMixVolume);
    const Voice* detail_GetSdkVoice(int channel) const;
    size_t GetCurrentPlayingSample() const;
    SampleFormat GetFormat() const;
    bool IsRun() const;
    void SetInterpolationType(u8 type);
    void SetVoiceMode(VoiceMode mode);
    VoiceMode GetVoiceMode();
    void AppendWaveBuffer(int channel, WaveBuffer* pBuffer, bool isLast);
    void SetAdpcmParam(int channel, const audio::AdpcmParameter& rParam);
    static size_t FrameToByte(s64 frame, SampleFormat format);
    static void CalcOffsetAdpcmParam(AdpcmContext* pContext, const audio::AdpcmParameter& rParam,
                                     s64 offset, const void* pData);

    /** @brief Gets the number of wave channels in use. @return Channel count. */
    int GetChannelCount() const { return mChannelCount; }

    /**
     * @brief Gets the driver voice of one wave channel.
     * @param channel Channel index, in [0, GetChannelCount()).
     * @return The channel's voice.
     */
    const Voice& GetVoice(int channel) const { return mVoice[channel]; }

    /**
     * @brief Selects the frame rate the voice's parameters are updated at.
     * @param updateType An UpdateType value.
     */
    void SetUpdateType(int updateType) { mUpdateType = updateType; }

private:
    /** @brief Releases the renderer voices of every wave channel. */
    void FreeAllSdkVoice() {
        for (int i = 0; i < mChannelCount; i++) {
            mVoice[i].Free();
        }
    }

    Voice mVoice[WaveChannelMax];
    VoiceMode mVoiceMode;
    int mChannelCount;
    VoiceCallback mCallback;
    void* mpCallbackArg;
    bool mIsActive;
    bool mIsStart;
    bool mIsStarted;
    bool mIsPause;
    bool mIsPausing;
    const WaveBuffer* mpLastWaveBuffer;
    u16 mSyncFlag;
    u8 mBiquadType;
    u8 _1eb;
    float mVolume;
    float mPitch;
    PanMode mPanMode;
    PanCurve mPanCurve;
    float mLpfFreq;
    float mBiquadValue;
    int mPriority;
    u32 mOutputLineFlag;
    OutputParam mTvParam;
    OutputAdditionalParam* mpTvAdditionalParam;
    SampleFormat mFormat;
    // Parameters between the sample format and the update type await reconstruction.
    u8 _26c[0xc];
    int mUpdateType;
    OutputReceiver* mpOutputReceiver;

public:
    util::IntrusiveListNode mLinkNode;
};
static_assert(sizeof(MultiVoice) == 0x298, "MultiVoice size");
}  // namespace nn::atk::detail::driver
