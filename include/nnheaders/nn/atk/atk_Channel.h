#pragma once

#include <nn/atk/atk_CurveAdshr.h>
#include <nn/atk/atk_CurveLfo.h>
#include <nn/atk/atk_HardwareManager.h>
#include <nn/atk/atk_OutputParam.h>
#include <nn/atk/atk_SoundStartable.h>
#include <nn/atk/atk_StreamTrack.h>
#include <nn/atk/atk_WaveInfo.h>
#include <cstddef>

namespace nn::atk::detail {
class OutputAdditionalParam;
}  // namespace nn::atk::detail

namespace nn::atk::detail::driver {
class alignas(8) Channel {
  public:
    static const int LfoCount = 4;

    enum ChannelCallbackStatus : int {
        ChannelCallbackStatus_Stopped,
        ChannelCallbackStatus_Drop,
        ChannelCallbackStatus_Finish,
        ChannelCallbackStatus_Cancel,
    };
    using ChannelCallback = void (*)(Channel*, ChannelCallbackStatus, void*);
    static Channel* AllocChannel(int channelCount, int priority, ChannelCallback callback, void* pArgument);
    void SetUpdateType(UpdateType updateType);
    void SetOutputReceiver(OutputReceiver* pReceiver);
    void Start(const WaveInfo& rWaveInfo, int length, long sampleOffset, bool isLastWave);
    static void FreeChannel(Channel* pChannel);
    static void DetachChannel(Channel* pChannel);
    void Stop();
    void Release();
    void NoteOff();
    void UpdateSweep(int count);
    void SetSweepParam(float sweepPitch, int sweepTime, bool autoUpdate);
    void SetBiquadFilter(int type, float value);
    void SetTvAdditionalParam(const OutputAdditionalParam& rParam);
    void CallChannelCallback(ChannelCallbackStatus status);

    /** @brief Tests whether the channel is playing. @return Whether the channel is active. */
    bool IsActive() const { return mActiveFlag; }
    /** @brief Tests whether the channel is paused. @return Whether the channel is paused. */
    bool IsPause() const { return mPauseFlag; }
    /**
     * @brief Pauses or resumes the channel's voice.
     * @param isPause Whether to pause.
     */
    void Pause(bool isPause) {
        mPauseFlag = isPause;
        mVoice->Pause(isPause);
    }
    /** @brief Tests whether the envelope is releasing. @return Whether the release phase started. */
    bool IsRelease() const { return mEnvelope.IsRelease(); }
    /** @brief Tests whether sweep updates automatically. @return Whether sweep is auto-updated. */
    bool IsAutoUpdateSweep() const { return mAutoSweep; }
    /** @brief Gets the remaining note length. @return Remaining length in ticks. */
    int GetLength() const { return mLength; }
    /** @brief Sets the remaining note length. @param length Length in ticks. */
    void SetLength(int length) { mLength = length; }
    /** @brief Gets the next channel in the owning track's list. @return Next channel, or null. */
    Channel* GetNextTrackChannel() const { return mNextTrackChannel; }
    /** @brief Gets the driver voice playing the channel. @return Voice, or null when none is attached. */
    const MultiVoice* GetVoice() const { return mVoice; }
    /** @brief Sets the next channel in the owning track's list. @param pChannel Next channel. */
    void SetNextTrackChannel(Channel* pChannel) { mNextTrackChannel = pChannel; }
    /** @brief Gets the exclusive note group. @return Group identifier. */
    u8 GetKeyGroupId() const { return mKeyGroup; }
    /** @brief Sets the played key. @param key MIDI-style played key, in [0, 127]. */
    void SetKey(u8 key) { mKey = key; }
    /** @brief Sets the user volume. @param volume Linear gain. */
    void SetUserVolume(float volume) { mUserVolume = volume; }
    /** @brief Sets the user pitch ratio. @param ratio Playback rate multiplier. */
    void SetUserPitchRatio(float ratio) { mUserPitchRatio = ratio; }
    /** @brief Sets the user pitch. @param pitch Pitch offset in semitones. */
    void SetUserPitch(float pitch) { mUserPitch = pitch; }
    /** @brief Sets the user low-pass filter frequency. @param freq Relative cutoff. */
    void SetUserLpfFreq(float freq) { mUserLpfFreq = freq; }
    /** @brief Sets the output line flags. @param line Output line bit flags. */
    void SetOutputLine(int line) { mOutputLine = line; }
    /** @brief Sets the TV output parameters. @param rParam Parameters to copy. */
    void SetTvParam(const OutputParam& rParam) { mTvParam = rParam; }
    /**
     * @brief Sets one LFO's parameters.
     * @param rParam LFO curve parameters.
     * @param index LFO index, in [0, LfoCount).
     */
    void SetLfoParam(const CurveLfoParam& rParam, int index) { mLfo[index].mParameter = rParam; }
    /**
     * @brief Sets one LFO's modulation target.
     * @param target Target parameter.
     * @param index LFO index, in [0, LfoCount).
     */
    void SetLfoModType(u8 target, int index) { mLfoTarget[index] = target; }
    /**
     * @brief Fades the channel in or out of silence.
     * @param isSilence Whether to fade to silence.
     * @param fadeFrames Fade length in frames.
     */
    void SetSilence(bool isSilence, int fadeFrames) {
        mSilenceVolume.SetTarget(isSilence ? 0 : SilenceVolumeMax, static_cast<u16>(fadeFrames));
    }
    /** @brief Sets whether release keeps its priority. @param fix Whether to fix the priority. */
    void SetReleasePriorityFix(bool fix) { mReleasePriorityFix = fix; }
    /** @brief Sets the pan mode. @param mode Pan mode. */
    void SetPanMode(int mode) { mPanMode = mode; }
    /** @brief Sets the pan curve. @param curve Pan curve. */
    void SetPanCurve(int curve) { mPanCurve = curve; }

    /**
     * @brief Gets the channel's amplitude envelope.
     * @return Mutable envelope used to configure attack, hold, decay, sustain and release.
     */
    CurveAdshr& GetEnvelope() { return mEnvelope; }
    /**
     * @brief Sets the played key and the sample's reference key together.
     * @param key MIDI-style played key, in [0, 127].
     * @param originalKey Original sample key, in [0, 127].
     */
    void SetKey(u8 key, u8 originalKey) {
        mKey = key;
        mOriginalKey = originalKey;
    }
    /** @brief Sets velocity gain. @param volume Linear gain derived from note velocity. */
    void SetVelocityVolume(float volume) { mVelocityVolume = volume; }
    /** @brief Sets instrument gain. @param volume Linear gain from the velocity region. */
    void SetVolume(float volume) { mVolume = volume; }
    /** @brief Sets instrument pitch. @param pitch Playback rate multiplier from the velocity region. */
    void SetPitch(float pitch) { mPitch = pitch; }
    /** @brief Sets the note's pan offset. @param pan Signed pan offset after combining track and region pan.
     */
    void SetPan(float pan) { mPan = pan; }
    /** @brief Sets the exclusive note group. @param group Group identifier used for note exclusion. */
    void SetKeyGroup(u8 group) { mKeyGroup = group; }
    /** @brief Sets whether note-off is ignored. @param ignore Nonzero preserves playback after note-off. */
    void SetIgnoreNoteOff(u8 ignore) { mIgnoreNoteOff = ignore; }
    /** @brief Sets sample interpolation. @param type Interpolation mode stored in the bank region. */
    void SetInterpolationType(u8 type) { mInterpolationType = type; }

  private:
    static const u8 SilenceVolumeMax = 255;

    u8 _0[0x20];
    CurveAdshr mEnvelope;
    CurveLfo mLfo[LfoCount];
    u8 mLfoTarget[LfoCount];
    bool mPauseFlag;
    bool mActiveFlag;
    u8 _c2;
    bool mAutoSweep;
    bool mReleasePriorityFix;
    u8 mIgnoreNoteOff;
    u8 _c6[2];
    float mUserVolume;
    float mUserPitchRatio;
    float mUserLpfFreq;
    u8 _d4[4];
    int mOutputLine;
    OutputParam mTvParam;
    u8 _12c[0x138 - 0x12c];
    float mUserPitch;
    u8 _13c[0x148 - 0x13c];
    float mPan;
    u8 _14c[4];
    float mPitch;
    MoveValue<u8, u16> mSilenceVolume;
    u8 _15a[0x164 - 0x15a];
    int mLength;
    int mPanMode;
    int mPanCurve;
    u8 mKey;
    u8 mOriginalKey;
    u8 mKeyGroup;
    u8 mInterpolationType;
    float mVolume;
    float mVelocityVolume;
    u8 _17c[0x190 - 0x17c];
    MultiVoice* mVoice;
    Channel* mNextTrackChannel;
    u8 _1a0[0x400 - 0x1a0];
};
static_assert(sizeof(Channel) == 0x400, "Channel size");
} // namespace nn::atk::detail::driver
