#include <nn/atk/detail/voice/atk_MultiVoice.h>

#include <algorithm>
#include <attributes.h>
#include <nn/atk/atk_FinalMix.h>
#include <nn/atk/atk_HardwareManager.h>
#include <nn/atk/atk_MultiVoiceManager.h>
#include <nn/atk/atk_OutputAdditionalParam.h>
#include <nn/atk/atk_Util.h>

namespace nn::atk::detail::driver {
namespace {
/** @brief Buses every output receiver has: the main bus and the three aux buses. */
const int DefaultBusCount = AuxBus_Count + 1;

/** @brief Number of output lines a voice can be routed to. */
const int OutputLineCount = 32;

/** @brief Samples in one DSP ADPCM frame. */
const int AdpcmFrameSampleCount = 14;

/** @brief Nibbles in one DSP ADPCM frame, including its header byte. */
const int AdpcmFrameNibbleCount = 16;

/** @brief Cutoff frequency at or above which the low-pass filter is bypassed. */
const u16 LpfFreqMax = 16000;

/** @brief Renderer voice state change requested by MultiVoice::Update(). */
enum VoiceStateRequest {
    VoiceStateRequest_None,
    VoiceStateRequest_Run,
    VoiceStateRequest_Pause,
};

/** @brief Volume-through-mode bit that applies the binary volume to a bus. */
const u8 VolumeThroughMode_Binary = 1 << 0;

/** @brief Index of the main bus; its send is stored relative to unity gain. */
const int MainBus = 0;

/**
 * @brief Reads the send level of one of the default buses.
 * @param rParam Output parameters.
 * @param bus Bus index, less than DefaultBusCount; the main send and the aux sends are contiguous.
 * @return Stored send level.
 */
inline float GetBusSend(const OutputParam& rParam, int bus) {
    return (&rParam.mainSend)[bus];
}

/**
 * @brief Clamps a send level to [0, 1].
 * @param send Send level.
 * @return Clamped send level.
 */
inline float ClampSend(float send) {
    return send > 1.0f ? 1.0f : (send < 0.0f ? 0.0f : send);
}
}  // namespace

/** @brief Creates an unallocated voice with no additional output parameters. */
MultiVoice::MultiVoice()
    : mChannelCount(0), mCallback(nullptr), mIsActive(false), mIsStart(false), mIsStarted(false),
      mIsPause(false), mSyncFlag(0), mpTvAdditionalParam(nullptr), mUpdateType(0),
      mpOutputReceiver(nullptr) {}

/**
 * @brief Creates an unallocated voice.
 * @param pAdditionalParam Storage for the TV output's additional parameters.
 */
MultiVoice::MultiVoice(OutputAdditionalParam* pAdditionalParam)
    : mChannelCount(0), mCallback(nullptr), mIsActive(false), mIsStart(false), mIsStarted(false),
      mIsPause(false), mSyncFlag(0), mpTvAdditionalParam(pAdditionalParam), mUpdateType(0),
      mpOutputReceiver(nullptr) {}

/** @brief Releases the renderer voices still held. */
MultiVoice::~MultiVoice() {
    FreeAllSdkVoice();
}

/**
 * @brief Acquires renderer voices for every wave channel.
 * @param channelCount Number of wave channels, clamped to [1, WaveChannelMax].
 * @param priority Voice priority.
 * @param callback Function notified when playback ends or the voice is dropped.
 * @param pCallbackArg Argument passed to callback.
 * @return Whether every channel got a renderer voice.
 */
bool MultiVoice::Alloc(int channelCount, int priority, VoiceCallback callback,
                       void* pCallbackArg) {
    channelCount = channelCount > 1 ? channelCount : 1;
    channelCount = channelCount < WaveChannelMax ? channelCount : WaveChannelMax;
    mChannelCount = 0;

    int allocCount = 0;

    for (; allocCount < channelCount; allocCount++) {
        if (!mVoice[allocCount].AllocVoice(priority)) {
            break;
        }
    }

    if (allocCount != channelCount) {
        for (int i = 0; i < allocCount; i++) {
            mVoice[i].Free();
        }

        return false;
    }

    mChannelCount = channelCount;
    InitParam(callback, pCallbackArg);
    mIsActive = true;
    return true;
}

/**
 * @brief Resets every parameter to its default.
 * @param callback Function notified when playback ends or the voice is dropped.
 * @param pCallbackArg Argument passed to callback.
 */
void MultiVoice::InitParam(VoiceCallback callback, void* pCallbackArg) {
    mSyncFlag = 0;
    mBiquadType = 0;
    _1eb = 0;
    mIsStart = false;
    mIsStarted = false;
    mIsPause = false;
    mIsPausing = false;
    mCallback = callback;
    mpCallbackArg = pCallbackArg;
    mpLastWaveBuffer = nullptr;
    mVolume = 1.0f;
    mPitch = 1.0f;
    mPanMode = PanMode_Dual;
    mPanCurve = PanCurve_Sqrt;
    mLpfFreq = 1.0f;
    mBiquadValue = 0.0f;
    mVoiceMode = VoiceMode_Mono;
    mTvParam.volume = 1.0f;
    mTvParam.mixMode = MixMode_Pan;
    mTvParam.pan = 0.0f;
    mTvParam.span = 0.0f;
    mTvParam.mainSend = 0.0f;

    for (int i = 0; i < AuxBus_Count; i++) {
        mTvParam.fxSend[i] = 0.0f;
    }

    if (mpTvAdditionalParam != nullptr) {
        mpTvAdditionalParam->Reset();
    }
}

/** @brief Releases the renderer voices and returns the voice to the manager. */
void MultiVoice::Free() {
    if (!mIsActive) {
        return;
    }

    FreeAllSdkVoice();
    mChannelCount = 0;
    MultiVoiceManager::GetInstance().FreeVoice(this);
    mIsActive = false;
}

/** @brief Requests playback to start on the next update. */
void MultiVoice::Start() {
    mIsStart = true;
    mIsPause = false;
    mSyncFlag |= UpdateFlag_Start;
}

/** @brief Stops playback. */
void MultiVoice::Stop() {
    if (mIsStarted) {
        StopAllSdkVoice();
        mIsStarted = false;
    }

    mIsPause = false;
    mIsPausing = false;
    mIsStart = false;
}

/** @brief Stops the renderer voice of every wave channel. */
void MultiVoice::StopAllSdkVoice() {
    for (int i = 0; i < mChannelCount; i++) {
        mVoice[i].SetState(VoiceState_Stop);
    }
}

/** @brief Detects the end of playback or a dropped renderer voice and notifies the owner. */
void MultiVoice::UpdateVoiceStatus() {
    if (!mIsActive || !mIsStarted) {
        return;
    }

    if (IsPlayFinished()) {
        if (mCallback != nullptr) {
            mCallback(this, VoiceCallbackStatus_FinishWave, mpCallbackArg);
        }

        mIsStart = false;
        mIsStarted = false;
        return;
    }

    for (int i = 0; i < mChannelCount; i++) {
        if (!mVoice[i].IsAvailable()) {
            FreeAllSdkVoice();
            mChannelCount = 0;
            mIsActive = false;
            MultiVoiceManager::GetInstance().FreeVoice(this);

            if (mCallback != nullptr) {
                mCallback(this, VoiceCallbackStatus_DropDsp, mpCallbackArg);
            }

            mIsStart = false;
            mIsStarted = false;
            return;
        }
    }
}

/**
 * @brief Checks whether the last wave buffer has been played.
 * @return True once the buffer marked last is neither waiting nor playing.
 */
bool MultiVoice::IsPlayFinished() const {
    return mChannelCount > 0 && mpLastWaveBuffer != nullptr &&
           mpLastWaveBuffer->status != WaveBuffer::Status_Wait &&
           mpLastWaveBuffer->status != WaveBuffer::Status_Play;
}

/**
 * @brief Requests playback to pause or resume on the next update.
 * @param isPause True to pause, false to resume.
 */
void MultiVoice::Pause(bool isPause) {
    if (mIsPause == isPause) {
        return;
    }

    mIsPause = isPause;
    mSyncFlag |= UpdateFlag_Pause;
}

/** @brief Pushes the parameters changed since the last call to the renderer voices. */
void MultiVoice::Calc() {
    if (!mIsStart) {
        return;
    }

    if (mSyncFlag & UpdateFlag_Src) {
        CalcSrc(false);
        mSyncFlag &= ~UpdateFlag_Src;
    }

    if (mSyncFlag & UpdateFlag_Ve) {
        CalcVe();
        mSyncFlag &= ~UpdateFlag_Ve;
    }

    if (mSyncFlag & UpdateFlag_Mix) {
        CalcMix();
        mSyncFlag &= ~UpdateFlag_Mix;
    }

    if (mSyncFlag & UpdateFlag_Lpf) {
        CalcLpf();
        mSyncFlag &= ~UpdateFlag_Lpf;
    }

    if (mSyncFlag & UpdateFlag_Biquad) {
        CalcBiquadFilter();
        mSyncFlag &= ~UpdateFlag_Biquad;
    }
}

/**
 * @brief Applies the pitch to every wave channel.
 * @param isInitialize Whether playback is being started.
 */
void MultiVoice::CalcSrc(bool isInitialize) {
    for (int i = 0; i < mChannelCount; i++) {
        mVoice[i].SetPitch(mPitch);
    }
}

/** @brief Applies the volume, scaled by the hardware output volume, to every wave channel. */
void MultiVoice::CalcVe() {
    float volume = mVolume;
    volume *= HardwareManager::GetInstance().GetOutputVolume();

    for (int i = 0; i < mChannelCount; i++) {
        mVoice[i].SetVolume(volume);
    }
}

/** @brief Recomputes the TV output gains of every wave channel. */
void MultiVoice::CalcMix() {
    for (int i = 0; i < mChannelCount; i++) {
        PreMixVolume preMixVolume;
        CalcPreMixVolume(&preMixVolume, mTvParam, mpTvAdditionalParam, i, OutputDevice_Main);

        OutputMix mix = {};
        CalcTvMix(&mix, preMixVolume);
        mVoice[i].SetTvMix(mix);
    }
}

/** @brief Applies the low-pass filter to every wave channel. */
void MultiVoice::CalcLpf() {
    u16 freq = Util::CalcLpfFreq(mLpfFreq);

    for (int i = 0; i < mChannelCount; i++) {
        if (freq >= LpfFreqMax) {
            mVoice[i].SetMonoFilter(false, 0);
        } else {
            mVoice[i].SetMonoFilter(true, freq);
        }
    }
}

/** @brief Applies the biquad filter to every wave channel. */
NOINLINE void MultiVoice::CalcBiquadFilter() {
    for (int i = 0; i < mChannelCount; i++) {
        const BiquadFilterCallback* pCallback =
            HardwareManager::GetInstance().GetBiquadFilterCallback(mBiquadType);

        if (pCallback == nullptr || mBiquadValue <= 0.0f) {
            mVoice[i].SetBiquadFilter(false, nullptr);
        } else {
            BiquadFilterCoefficients coefficients;
            pCallback->GetCoefficients(&coefficients, mBiquadType, mBiquadValue);
            mVoice[i].SetBiquadFilter(true, &coefficients);
        }
    }
}

/** @brief Starts, pauses or resumes the renderer voices and pushes their parameters. */
void MultiVoice::Update() {
    if (!mIsActive) {
        return;
    }

    VoiceStateRequest request = VoiceStateRequest_None;

    if (mSyncFlag & UpdateFlag_Start) {
        if (mIsStart && !mIsStarted) {
            CalcSrc(true);
            CalcMix();
            CalcVe();
            mIsStarted = true;
            request = VoiceStateRequest_Run;
            mSyncFlag &= ~(UpdateFlag_Start | UpdateFlag_Src | UpdateFlag_Mix);
        }
    }

    if (mIsStarted && (mSyncFlag & UpdateFlag_Pause) && mIsStart) {
        request = mIsPause ? VoiceStateRequest_Pause : VoiceStateRequest_Run;
        mIsPausing = mIsPause;
        mSyncFlag &= ~UpdateFlag_Pause;
    }

    for (int i = 0; i < mChannelCount; i++) {
        mVoice[i].UpdateParam();
    }

    if (request == VoiceStateRequest_Run) {
        RunAllSdkVoice();
    } else if (request == VoiceStateRequest_Pause) {
        PauseAllSdkVoice();
    }
}

/** @brief Starts the renderer voice of every wave channel. */
void MultiVoice::RunAllSdkVoice() {
    for (int i = 0; i < mChannelCount; i++) {
        mVoice[i].SetState(VoiceState_Play);
    }
}

/** @brief Pauses the renderer voice of every wave channel. */
void MultiVoice::PauseAllSdkVoice() {
    for (int i = 0; i < mChannelCount; i++) {
        mVoice[i].SetState(VoiceState_Pause);
    }
}

/**
 * @brief Sets the encoding of the wave data.
 * @param format Sample format.
 */
void MultiVoice::SetSampleFormat(SampleFormat format) {
    mFormat = format;

    for (int i = 0; i < mChannelCount; i++) {
        mVoice[i].SetSampleFormat(format);
    }
}

/**
 * @brief Sets the sample rate of the wave data.
 * @param sampleRate Sample rate in hertz.
 */
void MultiVoice::SetSampleRate(int sampleRate) {
    for (int i = 0; i < mChannelCount; i++) {
        mVoice[i].SetSampleRate(sampleRate);
    }
}

/**
 * @brief Sets the volume.
 * @param volume Linear gain; negative values are clamped to 0.
 */
void MultiVoice::SetVolume(float volume) {
    volume = volume < 0.0f ? 0.0f : volume;

    if (volume != mVolume) {
        mVolume = volume;
        mSyncFlag |= UpdateFlag_Ve;
    }
}

/**
 * @brief Sets the pitch.
 * @param pitch Frequency ratio.
 */
void MultiVoice::SetPitch(float pitch) {
    if (pitch != mPitch) {
        mPitch = pitch;
        mSyncFlag |= UpdateFlag_Src;
    }
}

/**
 * @brief Sets how stereo channels are panned.
 * @param mode Pan mode.
 */
void MultiVoice::SetPanMode(PanMode mode) {
    if (mode != mPanMode) {
        mPanMode = mode;
        mSyncFlag |= UpdateFlag_Mix;
    }
}

/**
 * @brief Sets the pan curve.
 * @param curve Pan curve.
 */
void MultiVoice::SetPanCurve(PanCurve curve) {
    if (curve != mPanCurve) {
        mPanCurve = curve;
        mSyncFlag |= UpdateFlag_Mix;
    }
}

/**
 * @brief Sets the low-pass filter.
 * @param lpfFreq Cutoff as a ratio of the maximum frequency.
 */
void MultiVoice::SetLpfFreq(float lpfFreq) {
    if (lpfFreq != mLpfFreq) {
        mLpfFreq = lpfFreq;
        mSyncFlag |= UpdateFlag_Lpf;
    }
}

/**
 * @brief Sets the biquad filter.
 * @param type Filter type.
 * @param value Filter strength, clamped to [0, 1].
 */
void MultiVoice::SetBiquadFilter(int type, float value) {
    value = value > 1.0f ? 1.0f : (value < 0.0f ? 0.0f : value);

    bool isUpdate = false;

    if (type != mBiquadType) {
        mBiquadType = type;
        isUpdate = true;
    }

    if (value != mBiquadValue) {
        mBiquadValue = value;
        isUpdate = true;
    }

    if (isUpdate) {
        mSyncFlag |= UpdateFlag_Biquad;
    }
}

/**
 * @brief Sets the priority of the voice and its renderer voices.
 * @param priority Voice priority.
 */
void MultiVoice::SetPriority(int priority) {
    mPriority = priority;
    MultiVoiceManager::GetInstance().ChangeVoicePriority(this);

    for (int i = 0; i < mChannelCount; i++) {
        mVoice[i].SetPriority(mPriority);
    }
}

/**
 * @brief Sets the output lines the voice is routed to.
 * @param outputLine Bit set of output lines.
 */
void MultiVoice::SetOutputLine(u32 outputLine) {
    if (outputLine != mOutputLineFlag) {
        mOutputLineFlag = outputLine;
        mSyncFlag |= UpdateFlag_Mix;
    }
}

/**
 * @brief Copies output parameters, flagging the mix for recalculation when anything changed.
 * @param rParam New parameters.
 * @param rOutputParam Parameters of the voice to update.
 */
void MultiVoice::SetOutputParamImpl(const OutputParam& rParam, OutputParam& rOutputParam) {
    float volume = rParam.volume < 0.0f ? 0.0f : rParam.volume;

    if (volume != rOutputParam.volume) {
        rOutputParam.volume = volume;
        mSyncFlag |= UpdateFlag_Mix;
    }

    if (rParam.mixMode != rOutputParam.mixMode) {
        rOutputParam.mixMode = rParam.mixMode;
        mSyncFlag |= UpdateFlag_Mix;
    }

    if (rOutputParam.mixMode == MixMode_MixParameter) {
        for (int ch = 0; ch < OutputParam::WaveChannelMax; ch++) {
            for (int i = 0; i < ChannelIndex_Count; i++) {
                if (rParam.mixParameter[ch].ch[i] != rOutputParam.mixParameter[ch].ch[i]) {
                    rOutputParam.mixParameter[ch].ch[i] = rParam.mixParameter[ch].ch[i];
                    mSyncFlag |= UpdateFlag_Mix;
                }
            }
        }
    }

    if (rOutputParam.mixMode == MixMode_Pan) {
        if (rParam.pan != rOutputParam.pan) {
            rOutputParam.pan = rParam.pan;
            mSyncFlag |= UpdateFlag_Mix;
        }

        if (rParam.span != rOutputParam.span) {
            rOutputParam.span = rParam.span;
            mSyncFlag |= UpdateFlag_Mix;
        }
    }

    if (rParam.mainSend != rOutputParam.mainSend) {
        rOutputParam.mainSend = rParam.mainSend;
        mSyncFlag |= UpdateFlag_Mix;
    }

    for (int i = 0; i < AuxBus_Count; i++) {
        if (rParam.fxSend[i] != rOutputParam.fxSend[i]) {
            rOutputParam.fxSend[i] = rParam.fxSend[i];
            mSyncFlag |= UpdateFlag_Mix;
        }
    }
}

/**
 * @brief Copies additional output parameters into the voice's own storage.
 * @param pAdditionalSend Sends of the buses past the default ones, or nullptr to keep them.
 * @param pBusMixVolumePacket Bus mix enable state, or nullptr to keep it.
 * @param pBusMixVolume Bus mix volumes, or nullptr to keep them.
 * @param pVolumeThroughModePacket Volume through modes, or nullptr to keep them.
 */
void MultiVoice::SetOutputAdditionalParamImpl(
    const ValueArray<float>* pAdditionalSend, const BusMixVolumePacket* pBusMixVolumePacket,
    const OutputBusMixVolume* pBusMixVolume,
    const VolumeThroughModePacket* pVolumeThroughModePacket) {
    if (mpTvAdditionalParam == nullptr) {
        return;
    }

    bool isAdditionalSendEnabled = mpTvAdditionalParam->IsAdditionalSendEnabled();

    if (pAdditionalSend != nullptr && isAdditionalSendEnabled) {
        int busCount = mpOutputReceiver->GetBusCount();

        for (int bus = DefaultBusCount; bus < busCount; bus++) {
            float send = pAdditionalSend->TryGetValue(bus - DefaultBusCount);

            if (send != mpTvAdditionalParam->TryGetAdditionalSend(bus)) {
                mpTvAdditionalParam->TrySetAdditionalSend(bus, send);
                mSyncFlag |= UpdateFlag_Mix;
            }
        }
    }

    bool isBusMixVolumeEnabled = mpTvAdditionalParam->IsBusMixVolumeEnabled();

    if (pBusMixVolume != nullptr && isBusMixVolumeEnabled && pBusMixVolumePacket != nullptr) {
        SetOutputBusMixVolumeImpl(*pBusMixVolumePacket, *pBusMixVolume,
                                  *mpTvAdditionalParam->GetBusMixVolumePacketAddr());
    }

    bool isVolumeThroughModeEnabled = mpTvAdditionalParam->IsVolumeThroughModeEnabled();

    if (pVolumeThroughModePacket != nullptr && isVolumeThroughModeEnabled) {
        SetOutputVolumeThroughModePacketImpl(
            *pVolumeThroughModePacket, *mpTvAdditionalParam->GetVolumeThroughModePacketAddr());
    }
}

/**
 * @brief Copies bus mix volumes, flagging the mix for recalculation when anything changed.
 * @param rPacket New bus enable state.
 * @param rBusMixVolume New bus volumes.
 * @param rOutputPacket Packet of the voice to update.
 */
void MultiVoice::SetOutputBusMixVolumeImpl(const BusMixVolumePacket& rPacket,
                                           const OutputBusMixVolume& rBusMixVolume,
                                           BusMixVolumePacket& rOutputPacket) {
    if (rPacket.IsUsed() != rOutputPacket.IsUsed()) {
        rOutputPacket.SetUsed(rPacket.IsUsed());
        mSyncFlag |= UpdateFlag_Mix;
    }

    if (!rOutputPacket.IsUsed()) {
        return;
    }

    int busCount = std::min(rPacket.GetBusCount(), rOutputPacket.GetBusCount());

    for (int i = 0; i < busCount; i++) {
        if (rPacket.IsEnabledForBus(i) != rOutputPacket.IsEnabledForBus(i)) {
            rOutputPacket.SetEnabledForBus(i, rPacket.IsEnabledForBus(i));
            mSyncFlag |= UpdateFlag_Mix;
        }
    }

    int count = mpOutputReceiver->GetBusCount() * mpOutputReceiver->GetChannelCount();
    OutputBusMixVolume& rOutputVolume = rOutputPacket.GetVolume();

    for (int ch = 0; ch < WaveChannelMax; ch++) {
        for (int i = 0; i < count; i++) {
            if (rBusMixVolume.volumes[ch][i] != rOutputVolume.volumes[ch][i]) {
                rOutputVolume.volumes[ch][i] = rBusMixVolume.volumes[ch][i];
                mSyncFlag |= UpdateFlag_Mix;
            }
        }
    }
}

/**
 * @brief Copies volume through modes, flagging the mix for recalculation when anything changed.
 * @param rPacket New modes.
 * @param rOutputPacket Packet of the voice to update.
 */
void MultiVoice::SetOutputVolumeThroughModePacketImpl(const VolumeThroughModePacket& rPacket,
                                                      VolumeThroughModePacket& rOutputPacket) {
    if (rPacket.IsUsed() != rOutputPacket.IsUsed()) {
        rOutputPacket.SetUsed(rPacket.IsUsed());

        if (!rOutputPacket.IsUsed()) {
            mSyncFlag |= UpdateFlag_Mix | UpdateFlag_Ve;
            return;
        }
    }

    float volume = rPacket.GetBinaryVolume() < 0.0f ? 0.0f : rPacket.GetBinaryVolume();

    if (volume != rOutputPacket.GetBinaryVolume()) {
        rOutputPacket.SetBinaryVolume(volume);
        mSyncFlag |= UpdateFlag_Mix | UpdateFlag_Ve;
    }

    int busCount = mpOutputReceiver->GetBusCount();

    for (int i = 0; i < busCount; i++) {
        u8 mode = rPacket.TryGetVolumeThroughMode(i);

        if (mode != rOutputPacket.TryGetVolumeThroughMode(i)) {
            rOutputPacket.TrySetVolumeThroughMode(i, mode);
            mSyncFlag |= UpdateFlag_Mix;
        }
    }
}

/**
 * @brief Sets the TV output parameters.
 * @param rParam New parameters.
 */
void MultiVoice::SetTvParam(const OutputParam& rParam) {
    SetOutputParamImpl(rParam, mTvParam);
}

/**
 * @brief Sets the TV output's additional parameters.
 * @param rParam New parameters.
 */
void MultiVoice::SetTvAdditionalParam(const OutputAdditionalParam& rParam) {
    bool isBusMixVolumeEnabled = rParam.IsBusMixVolumeEnabled();
    const ValueArray<float>* pAdditionalSend = rParam.GetAdditionalSendAddr();
    const BusMixVolumePacket* pBusMixVolumePacket = rParam.GetBusMixVolumePacketAddr();

    if (isBusMixVolumeEnabled) {
        SetOutputAdditionalParamImpl(pAdditionalSend, pBusMixVolumePacket,
                                     &rParam.GetBusMixVolume(),
                                     rParam.GetVolumeThroughModePacketAddr());
    } else {
        SetOutputAdditionalParamImpl(pAdditionalSend, pBusMixVolumePacket, nullptr,
                                     rParam.GetVolumeThroughModePacketAddr());
    }
}

/**
 * @brief Sets the TV output's additional parameters.
 * @param pAdditionalSend Sends of the buses past the default ones, or nullptr to keep them.
 * @param pBusMixVolumePacket Bus mix enable state, or nullptr to keep it.
 * @param pBusMixVolume Bus mix volumes, or nullptr to keep them.
 * @param pVolumeThroughModePacket Volume through modes, or nullptr to keep them.
 */
void MultiVoice::SetTvAdditionalParam(const ValueArray<float>* pAdditionalSend,
                                      const BusMixVolumePacket* pBusMixVolumePacket,
                                      const OutputBusMixVolume* pBusMixVolume,
                                      const VolumeThroughModePacket* pVolumeThroughModePacket) {
    SetOutputAdditionalParamImpl(pAdditionalSend, pBusMixVolumePacket, pBusMixVolume,
                                 pVolumeThroughModePacket);
}

/**
 * @brief Sets the receiver every wave channel is mixed into.
 *
 * The voice's own receiver is only updated when it has wave channels; the original assigns it
 * inside the loop.
 *
 * @param pReceiver Output receiver.
 */
void MultiVoice::SetOutputReceiver(OutputReceiver* pReceiver) {
    for (int i = 0; i < mChannelCount; i++) {
        mpOutputReceiver = pReceiver;
        mVoice[i].SetOutputReceiver(pReceiver);
    }
}

/**
 * @brief Computes the volume of one wave channel into every channel of every bus.
 * @param pPreMixVolume Receives the volumes.
 * @param rParam Output parameters.
 * @param pAdditionalParam Additional output parameters, or nullptr.
 * @param channelIndex Wave channel.
 * @param device Output device.
 */
void MultiVoice::CalcPreMixVolume(PreMixVolume* pPreMixVolume, const OutputParam& rParam,
                                  const OutputAdditionalParam* pAdditionalParam,
                                  int channelIndex, OutputDevice device) {
    const OutputMode mode = HardwareManager::GetInstance().GetOutputMode(device);
    const HardwareManager& rHardwareManager = HardwareManager::GetInstance();

    float frontLeft;
    float frontRight;
    float rearLeft;
    float rearRight;
    float frontCenter;
    float lfe;

    if (rParam.mixMode == MixMode_Pan) {
        Util::PanInfo panInfo;

        if (rHardwareManager.IsCompatiblePanCurveEnabled()) {
            panInfo.isCompatibleMode = true;
        }

        switch (mPanCurve) {
        case PanCurve_Sqrt:
            panInfo.curve = Util::PanCurve_Sqrt;
            break;
        case PanCurve_Sqrt0Db:
            panInfo.curve = Util::PanCurve_Sqrt;
            panInfo.centerZeroFlag = true;
            break;
        case PanCurve_Sqrt0DbClamp:
            panInfo.curve = Util::PanCurve_Sqrt;
            panInfo.centerZeroFlag = true;
            panInfo.zeroClampFlag = true;
            break;
        case PanCurve_SinCos:
            panInfo.curve = Util::PanCurve_SinCos;
            break;
        case PanCurve_SinCos0Db:
            panInfo.curve = Util::PanCurve_SinCos;
            panInfo.centerZeroFlag = true;
            break;
        case PanCurve_SinCos0DbClamp:
            panInfo.curve = Util::PanCurve_SinCos;
            panInfo.centerZeroFlag = true;
            panInfo.zeroClampFlag = true;
            break;
        case PanCurve_Linear:
            panInfo.curve = Util::PanCurve_Linear;
            break;
        case PanCurve_Linear0Db:
            panInfo.curve = Util::PanCurve_Linear;
            panInfo.centerZeroFlag = true;
            break;
        case PanCurve_Linear0DbClamp:
            panInfo.curve = Util::PanCurve_Linear;
            panInfo.centerZeroFlag = true;
            panInfo.zeroClampFlag = true;
            break;
        default:
            panInfo.curve = Util::PanCurve_Sqrt;
            break;
        }

        float left = 0.0f;
        float right = 0.0f;
        float front = 0.0f;
        float rear = 0.0f;

        switch (mode) {
        case OutputMode_Stereo:
        case OutputMode_Surround:
            if (mChannelCount >= 2 && mPanMode == PanMode_Balance) {
                if (channelIndex == 0) {
                    left = Util::CalcPanRatio(rParam.pan, panInfo, mode);
                } else if (channelIndex == 1) {
                    right = Util::CalcPanRatio(0.0f - rParam.pan, panInfo, mode);
                }
            } else if (mVoiceMode != VoiceMode_Mono && mPanMode != PanMode_Dual &&
                       !HardwareManager::GetInstance().IsCompatiblePanCurveEnabled()) {
                if (mVoiceMode == VoiceMode_StereoLeft) {
                    left = Util::CalcPanRatio(rParam.pan, panInfo, mode);
                } else {
                    right = Util::CalcPanRatio(0.0f - rParam.pan, panInfo, mode);
                }
            } else {
                float pan = rParam.pan;

                if ((channelIndex == 0 && mChannelCount == 2) ||
                    mVoiceMode == VoiceMode_StereoLeft) {
                    pan -= 1.0f;
                } else if ((channelIndex == 1 && mChannelCount == 2) ||
                           mVoiceMode == VoiceMode_StereoRight) {
                    pan += 1.0f;
                }

                left = Util::CalcPanRatio(pan, panInfo, mode);
                right = Util::CalcPanRatio(0.0f - pan, panInfo, mode);
            }

            break;
        case OutputMode_Monaural:
            left = Util::CalcPanRatio(0.0f, panInfo, OutputMode_Monaural);
            right = left;
            break;
        default:
            break;
        }

        if (mode == OutputMode_Monaural || mode == OutputMode_Stereo) {
            front = Util::CalcSurroundPanRatio(0.0f, panInfo);
            rear = Util::CalcSurroundPanRatio(2.0f, panInfo);
        } else if (mode == OutputMode_Surround) {
            front = Util::CalcSurroundPanRatio(rParam.span, panInfo);
            rear = Util::CalcSurroundPanRatio(2.0f - rParam.span, panInfo);
        }

        frontLeft = left * front;
        frontRight = right * front;
        rearLeft = left * rear;
        rearRight = right * rear;
        frontCenter = 0.0f;
        lfe = 0.0f;
    } else {
        const MixParameter& rMixParameter = rParam.mixParameter[channelIndex];
        frontLeft = rMixParameter.ch[ChannelIndex_FrontLeft];
        frontRight = rMixParameter.ch[ChannelIndex_FrontRight];
        rearLeft = rMixParameter.ch[ChannelIndex_RearLeft];
        rearRight = rMixParameter.ch[ChannelIndex_RearRight];
        frontCenter = rMixParameter.ch[ChannelIndex_FrontCenter];
        lfe = rMixParameter.ch[ChannelIndex_Lfe];
    }

    OutputMode endUserMode = HardwareManager::GetInstance().GetEndUserOutputMode(device);

    switch (endUserMode) {
    case OutputMode_Monaural:
        if (mode == OutputMode_Stereo) {
            float volume = (frontRight + frontLeft) * 0.5f;
            frontLeft = volume;
            frontRight = volume;
        } else if (mode == OutputMode_Surround) {
            float volume = ((rearLeft + frontLeft) + (rearRight + frontRight)) * 0.5f;
            frontLeft = volume;
            frontRight = volume;
            rearLeft = 0.0f;
            rearRight = 0.0f;
        }

        break;
    case OutputMode_Stereo:
        if (mode == OutputMode_Surround) {
            frontRight = rearRight + frontRight;
            rearRight = 0.0f;
            frontLeft = rearLeft + frontLeft;
            rearLeft = 0.0f;
        }

        break;
    default:
        break;
    }

    for (int i = 0; i < OutputMix::ChannelGainCount; i++) {
        int bus = i / mpOutputReceiver->GetChannelCount();
        float volume = 0.0f;

        if (bus < mpOutputReceiver->GetBusCount()) {
            if (pAdditionalParam != nullptr && pAdditionalParam->IsBusMixVolumeEnabled() &&
                pAdditionalParam->IsBusMixVolumeUsed() &&
                pAdditionalParam->IsBusMixVolumeEnabledForBus(bus)) {
                volume = pAdditionalParam->GetBusMixVolume(channelIndex, i);
            } else {
                switch (i % mpOutputReceiver->GetChannelCount()) {
                case ChannelIndex_FrontLeft:
                    volume = frontLeft;
                    break;
                case ChannelIndex_FrontRight:
                    volume = frontRight;
                    break;
                case ChannelIndex_RearLeft:
                    volume = rearLeft;
                    break;
                case ChannelIndex_RearRight:
                    volume = rearRight;
                    break;
                case ChannelIndex_FrontCenter:
                    volume = frontCenter;
                    break;
                case ChannelIndex_Lfe:
                    volume = lfe;
                    break;
                default:
                    volume = 0.0f;
                    break;
                }
            }
        }

        pPreMixVolume->volume[i] = volume;
    }
}

/**
 * @brief Computes the TV output gains of one wave channel.
 * @param pMix Receives the gains.
 * @param rPreMixVolume Volumes computed by CalcPreMixVolume().
 */
void MultiVoice::CalcTvMix(OutputMix* pMix, const PreMixVolume& rPreMixVolume) {
    CalcMixImpl(pMix, 0, mTvParam, mpTvAdditionalParam, rPreMixVolume);
}

/**
 * @brief Applies the bus send levels to the pre-mix volumes of one wave channel.
 * @param pMix Receives the gains.
 * @param outputDeviceIndex Output device the gains are for.
 * @param rParam Output parameters.
 * @param pAdditionalParam Additional output parameters, or nullptr.
 * @param rPreMixVolume Volumes computed by CalcPreMixVolume().
 */
void MultiVoice::CalcMixImpl(OutputMix* pMix, u32 outputDeviceIndex, const OutputParam& rParam,
                             const OutputAdditionalParam* pAdditionalParam,
                             const PreMixVolume& rPreMixVolume) {
    const int busCount = mpOutputReceiver->GetBusCount();
    float send[OutputMix::ChannelGainCount];

    for (int i = 0; i < busCount; i++) {
        send[i] = 0.0f;
    }

    u32 deviceFlag = 0;

    for (int i = 0; i < OutputLineCount; i++) {
        if (mOutputLineFlag & (1 << i)) {
            deviceFlag |= HardwareManager::GetInstance().GetOutputDeviceFlag(i);
        }
    }

    if (deviceFlag & (1 << outputDeviceIndex)) {
        for (int bus = 0; bus < busCount; bus++) {
            if (bus < DefaultBusCount) {
                float value = GetBusSend(rParam, bus);
                value = bus == MainBus ? value + 1.0f : value;

                if (mpOutputReceiver->IsSoundSendClampEnabled(bus)) {
                    send[bus] = ClampSend(value);
                } else {
                    send[bus] = std::max(value, 0.0f);
                }
            } else {
                if (pAdditionalParam == nullptr || !pAdditionalParam->IsAdditionalSendEnabled()) {
                    break;
                }

                if (mpOutputReceiver->IsSoundSendClampEnabled(bus)) {
                    send[bus] = ClampSend(pAdditionalParam->TryGetAdditionalSend(bus));
                } else {
                    send[bus] = std::max(pAdditionalParam->TryGetAdditionalSend(bus), 0.0f);
                }
            }
        }
    }

    for (int bus = 0; bus < busCount; bus++) {
        for (int ch = 0; ch < std::min(mpOutputReceiver->GetChannelCount(),
                                       static_cast<int>(ChannelIndex_Count));
             ch++) {
            int index = bus * mpOutputReceiver->GetChannelCount() + ch;
            float volume = rParam.volume * send[bus] * rPreMixVolume.volume[index];

            if (pAdditionalParam != nullptr && pAdditionalParam->IsVolumeThroughModeUsed() &&
                (pAdditionalParam->TryGetVolumeThroughMode(bus) & VolumeThroughMode_Binary) == 0) {
                volume *= pAdditionalParam->GetBinaryVolume();
            }

            pMix->channelGain[index] = volume;
        }
    }
}

/**
 * @brief Gets the renderer-side voice of one wave channel.
 * @param channel Channel index.
 * @return The channel's voice.
 */
const Voice* MultiVoice::detail_GetSdkVoice(int channel) const {
    return &mVoice[channel];
}

/**
 * @brief Gets the playback position.
 * @return Position in samples of the first wave channel, or 0 if none.
 */
size_t MultiVoice::GetCurrentPlayingSample() const {
    if (mChannelCount <= 0) {
        return 0;
    }

    return mVoice[0].GetPlayPosition();
}

/** @brief Gets the encoding of the wave data. @return Sample format. */
SampleFormat MultiVoice::GetFormat() const {
    return mFormat;
}

/** @brief Checks whether the voice is playing. @return True while the first channel plays. */
bool MultiVoice::IsRun() const {
    if (mChannelCount <= 0) {
        return false;
    }

    return mVoice[0].GetState() == VoiceState_Play;
}

/**
 * @brief Sets the sample interpolation type of every wave channel.
 * @param type Interpolation type.
 */
void MultiVoice::SetInterpolationType(u8 type) {
    for (int i = 0; i < mChannelCount; i++) {
        mVoice[i].SetInterpolationType(type);
    }
}

/**
 * @brief Sets how the wave channels are mixed.
 * @param mode Voice mode.
 */
void MultiVoice::SetVoiceMode(VoiceMode mode) {
    mVoiceMode = mode;
}

/** @brief Gets how the wave channels are mixed. @return Voice mode. */
MultiVoice::VoiceMode MultiVoice::GetVoiceMode() {
    return mVoiceMode;
}

/**
 * @brief Queues a wave buffer on one wave channel.
 * @param channel Channel index.
 * @param pBuffer Buffer to queue.
 * @param isLast Whether this is the last buffer of the sound.
 */
void MultiVoice::AppendWaveBuffer(int channel, WaveBuffer* pBuffer, bool isLast) {
    mVoice[channel].AppendWaveBuffer(pBuffer);

    if (isLast) {
        mpLastWaveBuffer = pBuffer;
    }
}

/**
 * @brief Sets the DSP ADPCM coefficients of one wave channel.
 * @param channel Channel index.
 * @param rParam Decoder coefficients.
 */
void MultiVoice::SetAdpcmParam(int channel, const audio::AdpcmParameter& rParam) {
    mVoice[channel].SetAdpcmParam(rParam);
}

/**
 * @brief Converts a sample count into a byte count.
 * @param frame Sample count.
 * @param format Sample format.
 * @return Size of frame samples in bytes, or 0 for an unknown format.
 */
size_t MultiVoice::FrameToByte(s64 frame, SampleFormat format) {
    switch (format) {
    case SampleFormat_DspAdpcm: {
        size_t nibbleCount = (frame / AdpcmFrameSampleCount) * AdpcmFrameNibbleCount;
        size_t rest = frame % AdpcmFrameSampleCount;

        if (rest != 0) {
            nibbleCount += rest + 2;
        }

        return nibbleCount / 2;
    }
    case SampleFormat_PcmS16:
        return frame * sizeof(s16);
    case SampleFormat_PcmS8:
        return frame;
    default:
        return 0;
    }
}

/**
 * @brief Decodes DSP ADPCM data up to an offset to obtain the decoder state there.
 * @param pContext Decoder state, advanced to offset.
 * @param rParam Decoder coefficients.
 * @param offset Sample offset to decode up to.
 * @param pData DSP ADPCM data.
 */
void MultiVoice::CalcOffsetAdpcmParam(AdpcmContext* pContext, const audio::AdpcmParameter& rParam,
                                      s64 offset, const void* pData) {
    s16 samples[AdpcmFrameSampleCount] = {};

    for (s64 i = 0; i < offset; i += AdpcmFrameSampleCount) {
        DecodeDspAdpcm(i, *pContext, rParam, pData, AdpcmFrameSampleCount, samples);
    }
}
}  // namespace nn::atk::detail::driver
