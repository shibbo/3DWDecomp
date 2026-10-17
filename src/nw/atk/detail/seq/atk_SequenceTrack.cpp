#include <attributes.h>
#include <nn/atk/detail/seq/atk_SequenceTrack.h>

namespace nn::atk::detail::driver {
namespace {
constexpr u32 SoundFrameIntervalMsec = 5;
constexpr u32 ParseCountMax = 10000;
constexpr u8 BiquadFilterTypeInherit = 255;
constexpr int OutputLineInherit = -1;
constexpr int KeyGroupReleaseValue = 126;
constexpr int HoldMax = 127;

/**
 * @brief Stops a channel at once, reports it as stopped and returns it to the channel pool.
 * @param pChannel Channel to stop.
 */
ALWAYS_INLINE void StopAndFreeChannel(Channel* pChannel) {
    pChannel->Stop();
    pChannel->CallChannelCallback(Channel::ChannelCallbackStatus_Stopped);
    Channel::FreeChannel(pChannel);
}
}  // namespace

/**
 * @brief Sets the index of this track inside its sequence player.
 * @param playerTrackNo Track index, in [0, 16).
 */
void SequenceTrack::SetPlayerTrackNo(int playerTrackNo) {
    mPlayerTrackNo = playerTrackNo;
}

/**
 * @brief Constructs a closed track with no player and no channels.
 */
SequenceTrack::SequenceTrack() : mOpenFlag(false), mPlayer(nullptr), mChannelList(nullptr) {
    InitParam();
}

/**
 * @brief Resets every sequence and output parameter to its default.
 */
NOINLINE void SequenceTrack::InitParam() {
    mExtVolume = 1.0f;
    mExtPitch = 1.0f;
    mPanRange = 1.0f;
    mTvParam.volume = 1.0f;
    mTvParam.mixMode = MixMode_Pan;
    mTvParam.pan = 0.0f;
    mTvParam.span = 0.0f;
    mTvParam.mainSend = 0.0f;

    for (int i = 0; i < AuxBus_Count; i++) {
        mTvParam.fxSend[i] = 0.0f;
    }

    mContext.mSequenceData = nullptr;
    mContext.mPosition = nullptr;
    mContext.mCondition = true;
    mContext.mNoteWait = true;
    mContext.mTie = false;
    mContext.mMono = false;
    mContext.mStackDepth = 0;
    mContext.mWait = 0;
    mContext.mMuted = false;
    mContext.mSilence = false;
    mWaitForNote = false;
    mPortamento = false;
    mParamDF = false;
    mParamB6 = 0;
    mProgram = 0;

    for (int i = 0; i < LfoCount; i++) {
        mLfo[i].AsCurveParam().Initialize();
        mLfoShape[i] = 0;
    }

    mPitchSweep = 0.0f;
    mVolume.InitValue(127);
    mVolume2.InitValue(127);
    mPan.InitValue(0);
    mSurroundPan.InitValue(0);
    mPitchBend.InitValue(0);
    mParamB3 = 127;
    mBendRange = 2;
    mParamDC = 0;
    mTranspose = 0;
    mPriority = 64;
    mPortamentoKey = 60;
    mPortamentoTime = 0;
    mAttack = -1;
    mDecay = -1;
    mSustain = -1;
    mRelease = -1;
    mHold = 255;
    mParamB4 = 0;
    mParamDB = 127;
    mParamD9 = 0;
    mParamDA = 0;
    mParamDE = 0;
    mParamD8 = 0.0f;
    mParamB5 = 0.0f;
    mOutputLine = OutputLineInherit;

    for (int i = 0; i < VariableCount; i++) {
        mTrackVariable[i] = -1;
    }

    mForceMute = false;
}

/**
 * @brief Destroys the track, releasing and detaching all of its channels.
 */
SequenceTrack::~SequenceTrack() {
    Close();
}

/**
 * @brief Releases and detaches every channel, then closes the track.
 */
void SequenceTrack::Close() {
    ReleaseAllChannel(-1);
    FreeAllChannel();
    mOpenFlag = false;
}

/**
 * @brief Sets the sequence data the track reads.
 * @param data Start of the sequence data.
 * @param offset Byte offset of the track's first command.
 */
void SequenceTrack::SetSeqData(const void* data, int offset) {
    mContext.mSequenceData = static_cast<const u8*>(data);
    mContext.mPosition = mContext.mSequenceData + offset;
}

/**
 * @brief Opens the track so it starts parsing.
 */
void SequenceTrack::Open() {
    mWaitForNote = false;
    mContext.mStackDepth = 0;
    mContext.mWait = 0;
    mOpenFlag = true;
}

/**
 * @brief Releases every active channel of the track.
 * @param release Release rate to apply first, or a negative value to keep each channel's own.
 */
void SequenceTrack::ReleaseAllChannel(int release) {
    UpdateChannelParam();

    Channel* pChannel = mChannelList;
    while (pChannel != nullptr) {
        if (pChannel->IsActive()) {
            if (release >= 0) {
                pChannel->GetEnvelope().SetRelease(static_cast<u8>(release));
            }

            pChannel->Release();
        }

        pChannel = pChannel->GetNextTrackChannel();
    }
}

/**
 * @brief Detaches every channel from the track without stopping it.
 */
void SequenceTrack::FreeAllChannel() {
    Channel* pChannel = mChannelList;
    while (pChannel != nullptr) {
        Channel::DetachChannel(pChannel);
        pChannel = pChannel->GetNextTrackChannel();
    }

    mChannelList = nullptr;
}

/**
 * @brief Counts down every channel's note length and advances manual sweeps by one tick.
 */
void SequenceTrack::UpdateChannelLength() {
    if (!mOpenFlag) {
        return;
    }

    Channel* pChannel = mChannelList;
    while (pChannel != nullptr) {
        if (pChannel->GetLength() > 0) {
            pChannel->SetLength(pChannel->GetLength() - 1);
        }

        UpdateChannelRelease(pChannel);

        if (!pChannel->IsAutoUpdateSweep()) {
            pChannel->UpdateSweep(1);
        }

        pChannel = pChannel->GetNextTrackChannel();
    }
}

/**
 * @brief Ends a channel's note once its length ran out, unless the damper holds it.
 * @param pChannel Channel to check.
 */
void SequenceTrack::UpdateChannelRelease(Channel* pChannel) {
    if (pChannel->GetLength() == 0 && !pChannel->IsRelease() && !mParamDF) {
        pChannel->NoteOff();
    }
}

/**
 * @brief Advances the track by one tick, parsing commands until it has to wait.
 * @param doNoteOn Whether parsed notes start channels.
 * @return 1 while the track keeps playing, 0 if it is closed, or -1 when its sequence ended.
 */
int SequenceTrack::ParseNextTick(bool doNoteOn) {
    if (!mOpenFlag) {
        return 0;
    }

    mVolume.Update();
    mVolume2.Update();
    mPan.Update();
    mSurroundPan.Update();
    mPitchBend.Update();

    if (mWaitForNote) {
        if (mChannelList != nullptr) {
            return 1;
        }

        mWaitForNote = false;
    }

    if (mContext.mWait > 0) {
        mContext.mWait--;
        if (mContext.mWait > 0) {
            return 1;
        }
    }

    if (mContext.mPosition != nullptr) {
        u32 count = 0;
        while (mContext.mWait == 0 && !mWaitForNote) {
            if (count++ >= ParseCountMax) {
                break;
            }

            if (Parse(doNoteOn) == 1) {
                return -1;
            }
        }
    }

    return 1;
}

/**
 * @brief Stops every channel at once and returns them to the channel pool.
 */
void SequenceTrack::StopAllChannel() {
    Channel* pChannel = mChannelList;
    while (pChannel != nullptr) {
        Channel* pNext = pChannel->GetNextTrackChannel();
        StopAndFreeChannel(pChannel);
        pChannel = pNext;
    }

    mChannelList = nullptr;
}

/**
 * @brief Pushes the track's current volume, pitch, pan, filter, LFO and send state to its channels.
 */
void SequenceTrack::UpdateChannelParam() {
    if (!mOpenFlag) {
        return;
    }

    if (mChannelList == nullptr) {
        return;
    }

    int volumeProduct = mVolume.GetValue() * mVolume2.GetValue() * mPlayer->mVolume.GetValue();
    float volume = static_cast<float>(volumeProduct) / (127.0f * 127.0f * 127.0f);
    volume = mPlayer->mBaseVolume * (mExtVolume * (volume * volume));

    float pitch = mPitchBend.GetValue() / 128.0f * mBendRange;
    float pitchRatio = mPlayer->mBasePitch * mExtPitch;
    float lpfFreq = mParamD8 + mPlayer->mBaseLpfFreq;

    s8 biquadType = mParamB4;
    float biquadValue = mParamB5;
    if (mPlayer->mBaseBiquadType != BiquadFilterTypeInherit) {
        biquadType = mPlayer->mBaseBiquadType;
        biquadValue = mPlayer->mBaseBiquadValue;
    }

    int outputLine = mPlayer->mBaseOutputLine;
    if (mOutputLine != OutputLineInherit) {
        outputLine = mOutputLine;
    }

    float pan = mPan.GetValue() / 63.0f;
    pan = pan > 1.0f ? 1.0f : (pan < -1.0f ? -1.0f : pan);
    pan = mPlayer->mPanRange * (mPanRange * pan);

    float span = mSurroundPan.GetValue() / 63.0f;
    span = span > 2.0f ? 2.0f : (span < 0.0f ? 0.0f : span);

    float mainSend = mParamDB / 127.0f - 1.0f;
    float fxSend[AuxBus_Count] = {mParamD9 / 127.0f, mParamDA / 127.0f, mParamDE / 127.0f};

    OutputParam tvParam = mPlayer->mTvParam;
    tvParam.volume = mTvParam.volume * tvParam.volume;

    for (int ch = 0; ch < OutputParam::WaveChannelMax; ch++) {
        for (int i = 0; i < ChannelIndex_Count; i++) {
            tvParam.mixParameter[ch].ch[i] =
                mTvParam.mixParameter[ch].ch[i] * tvParam.mixParameter[ch].ch[i];
        }
    }

    tvParam.pan = pan + tvParam.pan + mTvParam.pan;
    tvParam.span = span + tvParam.span + mTvParam.span;
    tvParam.mainSend = mainSend + (mTvParam.mainSend + tvParam.mainSend);

    for (int i = 0; i < AuxBus_Count; i++) {
        tvParam.fxSend[i] = fxSend[i] + (mTvParam.fxSend[i] + tvParam.fxSend[i]);
    }

    Channel* pChannel = mChannelList;
    while (pChannel != nullptr) {
        pChannel->SetUserVolume(volume);
        pChannel->SetUserPitch(pitch);
        pChannel->SetUserPitchRatio(pitchRatio);
        pChannel->SetUserLpfFreq(lpfFreq);
        pChannel->SetBiquadFilter(biquadType, biquadValue);
        pChannel->SetOutputLine(outputLine);

        for (int i = 0; i < LfoCount; i++) {
            pChannel->SetLfoParam(mLfo[i].AsCurveParam(), i);
            pChannel->SetLfoModType(mLfoShape[i], i);
        }

        pChannel->SetTvParam(tvParam);

        if (mPlayer->mTvAdditionalParam != nullptr) {
            pChannel->SetTvAdditionalParam(*mPlayer->mTvAdditionalParam);
        }

        pChannel = pChannel->GetNextTrackChannel();
    }
}

/**
 * @brief Pauses or resumes every active channel of the track.
 * @param isPause Whether to pause.
 */
void SequenceTrack::PauseAllChannel(bool isPause) {
    Channel* pChannel = mChannelList;
    while (pChannel != nullptr) {
        if (pChannel->IsActive() && isPause != pChannel->IsPause()) {
            pChannel->Pause(isPause);
        }

        pChannel = pChannel->GetNextTrackChannel();
    }
}

/**
 * @brief Prepends a channel to the track's channel list.
 * @param pChannel Channel to add.
 */
void SequenceTrack::AddChannel(Channel* pChannel) {
    pChannel->SetNextTrackChannel(mChannelList);
    mChannelList = pChannel;
}

/**
 * @brief Counts the channels the track currently owns.
 * @return Number of channels in the list.
 */
int SequenceTrack::GetChannelCount() const {
    int count = 0;

    Channel* pChannel = mChannelList;
    while (pChannel != nullptr) {
        count++;
        pChannel = pChannel->GetNextTrackChannel();
    }

    return count;
}

/**
 * @brief Removes a dropped channel from its track after notifying the player.
 * @param pDropChannel Channel that stopped.
 * @param status Why the channel stopped.
 * @param pUserData Owning SequenceTrack.
 */
void SequenceTrack::ChannelCallbackFunc(Channel* pDropChannel, Channel::ChannelCallbackStatus status,
                                        void* pUserData) {
    SequenceTrack* pTrack = static_cast<SequenceTrack*>(pUserData);

    if (pTrack->mPlayer != nullptr) {
        pTrack->mPlayer->ChannelCallback(pDropChannel);
    }

    if (pTrack->mChannelList == pDropChannel) {
        pTrack->mChannelList = pDropChannel->GetNextTrackChannel();
        return;
    }

    Channel* pChannel = pTrack->mChannelList;
    while (pChannel->GetNextTrackChannel() != nullptr) {
        if (pChannel->GetNextTrackChannel() == pDropChannel) {
            pChannel->SetNextTrackChannel(pDropChannel->GetNextTrackChannel());
            return;
        }

        pChannel = pChannel->GetNextTrackChannel();
    }
}

/**
 * @brief Mutes or unmutes the track, unless it is force-muted.
 * @param mute Mute mode; stopping and releasing modes also end the playing channels.
 */
void SequenceTrack::SetMute(SequenceMute mute) {
    if (mForceMute) {
        return;
    }

    switch (mute) {
    case SequenceMute_Off:
        mContext.mMuted = false;
        break;
    case SequenceMute_Stop:
        StopAllChannel();
        mContext.mMuted = true;
        break;
    case SequenceMute_Release:
        ReleaseAllChannel(-1);
        FreeAllChannel();
        mContext.mMuted = true;
        break;
    case SequenceMute_NoStop:
        mContext.mMuted = true;
        break;
    }
}

/**
 * @brief Mutes the track permanently, ignoring later SetMute calls.
 */
void SequenceTrack::ForceMute() {
    mForceMute = true;
    mContext.mMuted = true;
}

/**
 * @brief Fades the track's channels in or out of silence.
 * @param isSilence Whether to fade to silence.
 * @param fadeTimes Fade length in milliseconds.
 */
void SequenceTrack::SetSilence(bool isSilence, int fadeTimes) {
    mContext.mSilence = isSilence;

    Channel* pChannel = mChannelList;
    while (pChannel != nullptr) {
        pChannel->SetSilence(isSilence,
                             (fadeTimes + SoundFrameIntervalMsec - 1) / SoundFrameIntervalMsec);
        pChannel = pChannel->GetNextTrackChannel();
    }
}

/**
 * @brief Sets the track's biquad filter.
 * @param type Filter type.
 * @param value Filter strength.
 */
void SequenceTrack::SetBiquadFilter(int type, float value) {
    mParamB4 = type;
    mParamB5 = value;
}

/**
 * @brief Selects the bank the track plays notes from.
 * @param bankIndex Bank index.
 */
void SequenceTrack::SetBankIndex(int bankIndex) {
    mParamB6 = bankIndex;
}

/**
 * @brief Sets the key transposition.
 * @param transpose Semitones added to every key.
 */
void SequenceTrack::SetTranspose(s8 transpose) {
    mTranspose = transpose;
}

/**
 * @brief Sets the velocity range notes are scaled by.
 * @param velocityRange Maximum velocity, in [0, 127].
 */
void SequenceTrack::SetVelocityRange(u8 velocityRange) {
    mParamB3 = velocityRange;
}

/**
 * @brief Sets the output line flags.
 * @param outputLine Output line bit flags, or -1 to inherit the player's.
 */
void SequenceTrack::SetOutputLine(int outputLine) {
    mOutputLine = outputLine;
}

/**
 * @brief Sets one gain of the TV mix matrix.
 * @param srcChannel Source wave channel.
 * @param dstChannel Destination output channel.
 * @param param Gain.
 */
void SequenceTrack::SetTvMixParameter(u32 srcChannel, int dstChannel, float param) {
    mTvParam.mixParameter[srcChannel].ch[dstChannel] = param;
}

/**
 * @brief Gets a track variable.
 * @param index Variable index, in [0, VariableCount).
 * @return Variable value.
 */
s16 SequenceTrack::GetTrackVariable(int index) const {
    return mTrackVariable[index];
}

/**
 * @brief Sets a track variable.
 * @param index Variable index, in [0, VariableCount).
 * @param value New value.
 */
void SequenceTrack::SetTrackVariable(int index, s16 value) {
    mTrackVariable[index] = value;
}

/**
 * @brief Gets a pointer to a track variable.
 * @param index Variable index.
 * @return Variable pointer, or nullptr if the index is out of range.
 */
s16* SequenceTrack::GetVariablePtr(int index) {
    return index < VariableCount ? &mTrackVariable[index] : nullptr;
}

/**
 * @brief Plays a note on the track, reusing a tied or monophonic channel when possible.
 * @param key Key to play.
 * @param velocity Note velocity, in [0, 127] before the velocity range is applied.
 * @param length Note length in ticks, or -1 for no limit.
 * @param tieFlag Whether the note ties into the previous one.
 * @return Channel playing the note, or nullptr if none could be allocated.
 */
Channel* SequenceTrack::NoteOn(int key, int velocity, int length, bool tieFlag) {
    const SequenceSoundPlayer* pPlayer = mPlayer;
    Channel* pChannel = nullptr;

    velocity = velocity * mParamB3 / 127;

    if (tieFlag) {
        pChannel = mChannelList;
        if (pChannel != nullptr) {
            pChannel->SetKey(key);
            pChannel->SetVelocityVolume(Bank::CalcChannelVelocityVolume(velocity));
        }
    }

    if (mContext.mMono) {
        pChannel = mChannelList;
        if (pChannel != nullptr) {
            if (pChannel->IsRelease()) {
                StopAndFreeChannel(pChannel);
                pChannel = nullptr;
            } else {
                pChannel->SetKey(key);
                pChannel->SetVelocityVolume(Bank::CalcChannelVelocityVolume(velocity));
                pChannel->SetLength(length);
            }
        }
    }

    if (pChannel == nullptr) {
        NoteOnInfo info = {
            static_cast<int>(mProgram),
            key,
            velocity,
            tieFlag ? -1 : length,
            mParamDC,
            pPlayer->mPriority + mPriority,
            ChannelCallbackFunc,
            this,
            mPlayer->mOutputReceiver,
            mPlayer->mUpdateType,
        };

        pChannel = mPlayer->NoteOn(mParamB6, info);
        if (pChannel == nullptr) {
            return nullptr;
        }

        if (pChannel->GetKeyGroupId() != 0) {
            Channel* pOther = mChannelList;
            while (pOther != nullptr) {
                if (pOther->GetKeyGroupId() == pChannel->GetKeyGroupId()) {
                    pOther->GetEnvelope().SetRelease(KeyGroupReleaseValue);
                    pOther->Release();
                }

                pOther = pOther->GetNextTrackChannel();
            }
        }

        AddChannel(pChannel);
    }

    if (mAttack >= 0) {
        pChannel->GetEnvelope().SetAttack(static_cast<u8>(mAttack));
    }

    if (mDecay >= 0) {
        pChannel->GetEnvelope().SetDecay(static_cast<u8>(mDecay));
    }

    if (mSustain >= 0) {
        pChannel->GetEnvelope().SetSustain(static_cast<u8>(mSustain));
    }

    if (mRelease >= 0) {
        pChannel->GetEnvelope().SetRelease(static_cast<u8>(mRelease));
    }

    if (mHold <= HoldMax) {
        pChannel->GetEnvelope().SetHold(mHold);
    }

    float sweepPitch = mPitchSweep;
    if (mPortamento) {
        sweepPitch += mPortamentoKey - key;
    }

    if (mPortamentoTime == 0) {
        pChannel->SetSweepParam(sweepPitch, length, false);
    } else {
        int sweepTime = mPortamentoTime * mPortamentoTime;
        sweepTime = static_cast<int>(sweepTime * (sweepPitch >= 0.0f ? sweepPitch : -sweepPitch));
        sweepTime >>= 5;
        sweepTime *= SoundFrameIntervalMsec;
        pChannel->SetSweepParam(sweepPitch, sweepTime, true);
    }

    mPortamentoKey = key;

    pChannel->SetSilence(mContext.mSilence, 0);
    pChannel->SetReleasePriorityFix(mPlayer->mReleasePriorityFix);
    pChannel->SetPanMode(mPlayer->mPanMode);
    pChannel->SetPanCurve(mPlayer->mPanCurve);
    return pChannel;
}
}  // namespace nn::atk::detail::driver
