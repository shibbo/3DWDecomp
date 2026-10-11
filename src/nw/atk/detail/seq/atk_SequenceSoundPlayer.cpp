#include <nn/atk/detail/seq/atk_SequenceSoundPlayer.h>

#include <attributes.h>
#include <cstring>
#include <nn/atk/atk_DisposeCallbackManager.h>
#include <nn/atk/atk_SequenceSoundFileReader.h>
#include <nn/atk/atk_SoundPlayer.h>
#include <nn/atk/atk_SoundProfile.h>
#include <nn/atk/atk_TaskManager.h>
#include <nn/atk/atk_Util.h>

namespace nn::atk::detail::driver {
namespace {
// Archive load flags selecting which file kinds SoundArchiveLoader::LoadData() brings in.
const u32 LoadFlag_Seq = 1 << 0;
const u32 LoadFlag_Bank = 1 << 2;

// Release value used when skipping drops every sounding note at once.
const int SkipReleaseValue = 127;

// Duration of one sound-thread frame in milliseconds, plain and in 16.16 fixed point.
const u32 FrameMsec = 5;
const u64 FrameMsecFixed = FrameMsec << 16;
const float FixedOne = 65536.0f;

/**
 * @brief Applies an operation to every allocated track selected by a bit mask.
 * @param pPlayer Player owning the tracks.
 * @param trackBitFlag Bit n selects track n.
 * @param func Operation called with each selected, allocated track.
 */
template <typename Func>
ALWAYS_INLINE void ForEachTrack(SequenceSoundPlayer* pPlayer, u32 trackBitFlag, Func func) {
    for (int trackNo = 0; trackNo < SequenceSoundPlayer::TrackCountPerPlayer && trackBitFlag != 0;
         trackNo++, trackBitFlag >>= 1) {
        if ((trackBitFlag & 1) == 0) {
            continue;
        }

        SequenceTrack* pTrack = pPlayer->GetPlayerTrack(trackNo);
        if (pTrack != nullptr) {
            func(pTrack);
        }
    }
}

/**
 * @brief Looks up how long the renderer spent on one voice.
 * @param rVoice Driver voice of one wave channel.
 * @param rProfile Profile of the last audio frame.
 * @return Render time in ticks, or 0 when the voice was not profiled.
 */
u64 GetVoiceProcessTick(const Voice& rVoice, const SoundProfile& rProfile) {
    const LowLevelVoice* pLowLevelVoice = rVoice.GetLowLevelVoice();
    if (pLowLevelVoice == nullptr) {
        return 0;
    }

    u32 voiceId = pLowLevelVoice->GetVoiceId();
    for (u32 i = 0; i < rProfile.voiceProfileCount; i++) {
        if (rProfile.voiceIdTable[i] == voiceId) {
            return rProfile.voiceProfile[i].GetProcessTick();
        }
    }

    return 0;
}

/**
 * @brief Sums the render time of every wave channel of a channel's voice.
 * @param rChannel Sequence channel.
 * @param rProfile Profile of the last audio frame.
 * @return Render time in ticks.
 */
u64 GetChannelProcessTick(const Channel& rChannel, const SoundProfile& rProfile) {
    const MultiVoice* pVoice = rChannel.GetVoice();
    if (pVoice == nullptr) {
        return 0;
    }

    u64 processTick = 0;
    for (int channel = 0; channel < pVoice->GetChannelCount(); channel++) {
        processTick += GetVoiceProcessTick(pVoice->GetVoice(channel), rProfile);
    }

    return processTick;
}

/**
 * @brief Sums the render time of every channel a track is playing.
 * @param rTrack Sequence track.
 * @param rProfile Profile of the last audio frame.
 * @return Render time in ticks, or 0 when the track is closed.
 */
u64 GetTrackProcessTick(const SequenceTrack& rTrack, const SoundProfile& rProfile) {
    if (!rTrack.IsOpened()) {
        return 0;
    }

    u64 processTick = 0;
    for (const Channel* pChannel = rTrack.GetChannelList(); pChannel != nullptr;
         pChannel = pChannel->GetNextTrackChannel()) {
        processTick += GetChannelProcessTick(*pChannel, rProfile);
    }

    return processTick;
}
}  // namespace

vs16 SequenceSoundPlayer::m_GlobalVariable[GlobalVariableCount];
int SequenceSoundPlayer::m_SkipIntervalTickPerFrame = DefaultSkipIntervalTick;

/** @brief Clears the sequence variables shared by every player. */
void SequenceSoundPlayer::InitSequenceSoundPlayer() {
    for (int i = 0; i < GlobalVariableCount; i++) {
        m_GlobalVariable[i] = -1;
    }
}

/** @brief Creates an idle player with no tracks, banks or loader attached. */
SequenceSoundPlayer::SequenceSoundPlayer()
    : mReleasePriorityFix(false), mPanRange(1.0f), mTempoRatio(1.0f), mTickFraction(0.0f),
      mSkipTimeCounter(0.0f), mDelayCount(0), mPriority(64), mParamB0(48), mTempo(120),
      mVolume(127), mNoteOnCallback(nullptr), mSequenceUserprocCallback(nullptr),
      mSequenceUserprocCallbackArg(nullptr), mTickCounter(0), mIsInitialized(false),
      mIsRegisterPlayerCallback(false), mLoaderManager(nullptr), mLoader(nullptr) {
    for (int i = 0; i < PlayerVariableCount; i++) {
        mLocalVariable[i] = -1;
    }

    for (int i = 0; i < TrackCountPerPlayer; i++) {
        mTracks[i] = nullptr;
    }
}

/** @brief Stops playback and releases every resource the player holds. */
SequenceSoundPlayer::~SequenceSoundPlayer() {
    Finalize();
}

/**
 * @brief Resets the player for a new sound.
 * @param pReceiver Destination of the player's channels.
 */
void SequenceSoundPlayer::Initialize(OutputReceiver* pReceiver) {
    BasicSoundPlayer::Initialize(pReceiver);
    mStartedFlag = false;
    mPauseFlag = false;
    mReleasePriorityFix = false;
    SetActiveFlag(false);

    mSkipTimeCounter = 0.0f;
    mDelayCount = 0;
    mPanRange = 1.0f;
    mTempoRatio = 1.0f;
    mTickFraction = 0.0f;
    mSkipTickCounter = 0;
    mTickCounter = 0;
    mUpdateType = UpdateType_AudioFrame;

    mSequenceUserprocCallback = nullptr;
    mSequenceUserprocCallbackArg = nullptr;
    mVolume.InitValue(127);
    mPriority = 64;
    mParamB0 = 48;
    mTempo = 120;
    mNoteOnCallback = nullptr;

    for (int i = 0; i < PlayerVariableCount; i++) {
        mLocalVariable[i] = -1;
    }

    mIsRegisterPlayerCallback = false;
    mIsInitialized = true;

    for (int i = 0; i < TrackCountPerPlayer; i++) {
        mTracks[i] = nullptr;
    }
}

/** @brief Stops playback, detaches all data and returns the loader. */
void SequenceSoundPlayer::Finalize() {
    mFinishFlag = true;
    FinishPlayer();

    if (mActiveFlag) {
        DisposeCallbackManager::GetInstance().UnregisterDisposeCallback(this);
        SetActiveFlag(false);
    }

    for (int i = 0; i < BankCountMax; i++) {
        mBankFileReader[i].Finalize();
        mWaveArchiveFileReader[i].Finalize();
    }

    if (mIsInitialized) {
        BasicSoundPlayer::Finalize();
        mIsInitialized = false;
    }

    if (mLoader != nullptr) {
        mLoader->Finalize();
    }

    FreeLoader();
}

/** @brief Leaves the sound thread's update list and closes every track. */
void SequenceSoundPlayer::FinishPlayer() {
    if (mIsRegisterPlayerCallback) {
        SoundThread::GetInstance().UnregisterPlayerCallback(this);
        mIsRegisterPlayerCallback = false;
    }

    if (mStartedFlag) {
        mStartedFlag = false;
    }

    for (int trackNo = 0; trackNo < TrackCountPerPlayer; trackNo++) {
        CloseTrack(trackNo);
    }
}

/** @brief Queues release of the player heap used by the last load. */
void SequenceSoundLoader::Finalize() {
    mFreeTask.pHeap = mLoadTask.pHeap;
    TaskManager::GetInstance().AppendTask(&mFreeTask, TaskManager::TaskPriority_Normal);
}

/** @brief Hands the loader back to its manager. */
void SequenceSoundPlayer::FreeLoader() {
    if (mLoader == nullptr) {
        return;
    }

    mLoaderManager->Free(mLoader);
    mLoader = nullptr;
}

/**
 * @brief Allocates the requested tracks.
 * @param rArg Track allocator, track mask and note-on callback; finalizes the player when too few
 * tracks are left.
 */
void SequenceSoundPlayer::Setup(const SetupArg& rArg) {
    mNoteOnCallback = rArg.pCallback;

    int allocTrackCount = 0;
    for (u32 trackBitFlag = rArg.allocTracks; trackBitFlag != 0; trackBitFlag >>= 1) {
        allocTrackCount += trackBitFlag & 1;
    }

    if (allocTrackCount > rArg.pTrackAllocator->GetAllocatableTrackCount()) {
        Finalize();
        return;
    }

    u32 trackBitFlag = rArg.allocTracks;
    for (int trackNo = 0; trackBitFlag != 0; trackNo++, trackBitFlag >>= 1) {
        if ((trackBitFlag & 1) == 0) {
            continue;
        }

        SequenceTrack* pTrack = rArg.pTrackAllocator->AllocTrack(this);
        SetPlayerTrack(trackNo, pTrack);
    }

    mSequenceTrackAllocator = rArg.pTrackAllocator;
}

/**
 * @brief Installs a track.
 * @param trackNo Track slot, in [0, TrackCountPerPlayer).
 * @param pTrack Track to install.
 */
void SequenceSoundPlayer::SetPlayerTrack(int trackNo, SequenceTrack* pTrack) {
    if (trackNo >= TrackCountPerPlayer) {
        return;
    }

    mTracks[trackNo] = pTrack;
    pTrack->SetPlayerTrackNo(trackNo);
}

/** @brief Mutes tracks immediately. @param trackBitFlag Bit n selects track n. */
void SequenceSoundPlayer::ForceTrackMute(u32 trackBitFlag) {
    ForEachTrack(this, trackBitFlag, [](SequenceTrack* pTrack) { pTrack->ForceMute(); });
}

/**
 * @brief Gets a track.
 * @param trackNo Track slot.
 * @return The track, or nullptr when the slot is empty or out of range.
 */
SequenceTrack* SequenceSoundPlayer::GetPlayerTrack(int trackNo) {
    if (trackNo >= TrackCountPerPlayer) {
        return nullptr;
    }

    return mTracks[trackNo];
}

/** @brief Starts playback once the player is prepared. */
void SequenceSoundPlayer::Start() {
    mStartedFlag = true;
}

/** @brief Stops playback and closes every track. */
void SequenceSoundPlayer::Stop() {
    FinishPlayer();
}

/** @brief Pauses or resumes every track. @param isPause Whether to pause. */
void SequenceSoundPlayer::Pause(bool isPause) {
    mPauseFlag = isPause;

    for (int trackNo = 0; trackNo < TrackCountPerPlayer; trackNo++) {
        SequenceTrack* pTrack = GetPlayerTrack(trackNo);
        if (pTrack != nullptr) {
            pTrack->PauseAllChannel(isPause);
        }
    }
}

/**
 * @brief Schedules part of the sequence to be skipped.
 * @param offsetType Unit of the offset.
 * @param offset Amount to skip, in ticks or milliseconds.
 */
void SequenceSoundPlayer::Skip(StartOffsetType offsetType, int offset) {
    if (!mActiveFlag) {
        return;
    }

    switch (offsetType) {
    case StartOffsetType_Tick:
        mSkipTickCounter += offset;
        break;
    case StartOffsetType_MilliSeconds:
        mSkipTimeCounter += static_cast<float>(offset);
        break;
    }
}

/** @brief Scales the sequence tempo. @param tempoRatio Tempo multiplier. */
void SequenceSoundPlayer::SetTempoRatio(float tempoRatio) {
    mTempoRatio = tempoRatio;
}

/** @brief Scales the pan of every note. @param panRange Pan multiplier. */
void SequenceSoundPlayer::SetPanRange(float panRange) {
    mPanRange = panRange;
}

/** @brief Sets the channel priority of new notes. @param priority Priority, in [0, 127]. */
void SequenceSoundPlayer::SetChannelPriority(int priority) {
    mPriority = priority;
}

/** @brief Sets whether releasing notes keep their priority. @param fix Whether to keep it. */
void SequenceSoundPlayer::SetReleasePriorityFix(bool fix) {
    mReleasePriorityFix = fix;
}

/**
 * @brief Installs the handler of the sequence's user procedure command.
 * @param callback Handler, or nullptr to ignore the command.
 * @param pArg Context passed to the handler.
 */
void SequenceSoundPlayer::SetSequenceUserprocCallback(SequenceUserProcCallback callback,
                                                      void* pArg) {
    mSequenceUserprocCallback = callback;
    mSequenceUserprocCallbackArg = pArg;
}

/**
 * @brief Runs the user procedure handler for a track.
 * @param procId Procedure identifier from the sequence.
 * @param pTrack Track executing the command; its compare flag takes the handler's result.
 */
void SequenceSoundPlayer::CallSequenceUserprocCallback(u16 procId, SequenceTrack* pTrack) {
    if (mSequenceUserprocCallback == nullptr) {
        return;
    }

    SequenceUserProcCallbackParam param;
    param.localVariable = GetVariablePtr(0);
    param.globalVariable = GetVariablePtr(PlayerVariableCount);
    param.trackVariable = pTrack->GetVariablePtr(0);
    param.cmpFlag = pTrack->GetCmpFlag();

    mSequenceUserprocCallback(procId, &param, mSequenceUserprocCallbackArg);

    pTrack->SetCmpFlag(param.cmpFlag);
}

/**
 * @brief Gets a sequence variable.
 * @param varNo Local variables come first, then the global ones.
 * @return The variable, or nullptr when the number is out of range.
 */
vs16* SequenceSoundPlayer::GetVariablePtr(int varNo) {
    if (varNo < PlayerVariableCount) {
        return &mLocalVariable[varNo];
    }

    if (varNo < PlayerVariableCount + GlobalVariableCount) {
        return &m_GlobalVariable[varNo - PlayerVariableCount];
    }

    return nullptr;
}

/** @brief Reads a local variable. @param varNo Variable number. @return Its value. */
s16 SequenceSoundPlayer::GetLocalVariable(int varNo) const {
    return mLocalVariable[varNo];
}

/** @brief Reads a global variable. @param varNo Variable number. @return Its value. */
s16 SequenceSoundPlayer::GetGlobalVariable(int varNo) {
    return m_GlobalVariable[varNo];
}

/** @brief Writes a local variable. @param varNo Variable number. @param value New value. */
void SequenceSoundPlayer::SetLocalVariable(int varNo, s16 value) {
    mLocalVariable[varNo] = value;
}

/** @brief Writes a global variable. @param varNo Variable number. @param value New value. */
void SequenceSoundPlayer::SetGlobalVariable(int varNo, s16 value) {
    m_GlobalVariable[varNo] = value;
}

/**
 * @brief Mutes or unmutes tracks.
 * @param trackBitFlag Bit n selects track n.
 * @param mute Mute mode.
 */
void SequenceSoundPlayer::SetTrackMute(u32 trackBitFlag, SequenceMute mute) {
    ForEachTrack(this, trackBitFlag, [mute](SequenceTrack* pTrack) { pTrack->SetMute(mute); });
}

/**
 * @brief Fades tracks to or from silence.
 * @param trackBitFlag Bit n selects track n.
 * @param isSilence Whether to fade to silence.
 * @param fadeTimes Fade length in frames.
 */
void SequenceSoundPlayer::SetTrackSilence(unsigned long trackBitFlag, bool isSilence,
                                          int fadeTimes) {
    ForEachTrack(this, trackBitFlag, [isSilence, fadeTimes](SequenceTrack* pTrack) {
        pTrack->SetSilence(isSilence, fadeTimes);
    });
}

/**
 * @brief Sets the volume of tracks.
 * @param trackBitFlag Bit n selects track n.
 * @param volume Gain.
 */
void SequenceSoundPlayer::SetTrackVolume(u32 trackBitFlag, float volume) {
    ForEachTrack(this, trackBitFlag, [volume](SequenceTrack* pTrack) {
        pTrack->SetExtVolume(volume);
    });
}

/**
 * @brief Sets the pitch of tracks.
 * @param trackBitFlag Bit n selects track n.
 * @param pitch Ratio.
 */
void SequenceSoundPlayer::SetTrackPitch(u32 trackBitFlag, float pitch) {
    ForEachTrack(this, trackBitFlag, [pitch](SequenceTrack* pTrack) {
        pTrack->SetExtPitch(pitch);
    });
}

/**
 * @brief Sets the low-pass cutoff of tracks.
 * @param trackBitFlag Bit n selects track n.
 * @param lpfFreq Cutoff.
 */
void SequenceSoundPlayer::SetTrackLpfFreq(u32 trackBitFlag, float lpfFreq) {
    ForEachTrack(this, trackBitFlag, [lpfFreq](SequenceTrack* pTrack) {
        pTrack->SetLpfFreq(lpfFreq);
    });
}

/**
 * @brief Sets the biquad filter of tracks.
 * @param trackBitFlag Bit n selects track n.
 * @param type Filter type.
 * @param value Filter strength.
 */
void SequenceSoundPlayer::SetTrackBiquadFilter(u32 trackBitFlag, int type, float value) {
    ForEachTrack(this, trackBitFlag,
                 [type, value](SequenceTrack* pTrack) { pTrack->SetBiquadFilter(type, value); });
}

/**
 * @brief Switches tracks to another bank.
 * @param trackBitFlag Bit n selects track n.
 * @param bankIndex Bank slot.
 * @return Whether the bank slot holds a bank.
 */
bool SequenceSoundPlayer::SetTrackBankIndex(u32 trackBitFlag, int bankIndex) {
    if (!mBankFileReader[bankIndex].IsInitialized()) {
        return false;
    }

    ForEachTrack(this, trackBitFlag, [bankIndex](SequenceTrack* pTrack) {
        pTrack->SetBankIndex(bankIndex);
    });
    return true;
}

/**
 * @brief Transposes tracks.
 * @param trackBitFlag Bit n selects track n.
 * @param transpose Semitones.
 */
void SequenceSoundPlayer::SetTrackTranspose(u32 trackBitFlag, s8 transpose) {
    ForEachTrack(this, trackBitFlag, [transpose](SequenceTrack* pTrack) {
        pTrack->SetTranspose(transpose);
    });
}

/**
 * @brief Sets the velocity range of tracks.
 * @param trackBitFlag Bit n selects track n.
 * @param range Range.
 */
void SequenceSoundPlayer::SetTrackVelocityRange(u32 trackBitFlag, u8 range) {
    ForEachTrack(this, trackBitFlag, [range](SequenceTrack* pTrack) {
        pTrack->SetVelocityRange(range);
    });
}

/**
 * @brief Routes tracks to output lines.
 * @param trackBitFlag Bit n selects track n.
 * @param outputLine Lines.
 */
void SequenceSoundPlayer::SetTrackOutputLine(u32 trackBitFlag, u32 outputLine) {
    ForEachTrack(this, trackBitFlag, [outputLine](SequenceTrack* pTrack) {
        pTrack->SetOutputLine(outputLine);
    });
}

/**
 * @brief Routes tracks back to the player's output lines.
 * @param trackBitFlag Bit n selects track n.
 */
void SequenceSoundPlayer::ResetTrackOutputLine(u32 trackBitFlag) {
    ForEachTrack(this, trackBitFlag, [](SequenceTrack* pTrack) { pTrack->SetOutputLine(-1); });
}

/**
 * @brief Sets the TV volume of tracks.
 * @param trackBitFlag Bit n selects track n.
 * @param volume Gain.
 */
void SequenceSoundPlayer::SetTrackTvVolume(u32 trackBitFlag, float volume) {
    ForEachTrack(this, trackBitFlag, [volume](SequenceTrack* pTrack) {
        pTrack->GetTvParam().volume = volume;
    });
}

/**
 * @brief Sets the TV mix of one source channel of tracks.
 * @param trackBitFlag Bit n selects track n.
 * @param srcChannel Source wave channel.
 * @param rParam Gain into each output channel.
 */
void SequenceSoundPlayer::SetTrackChannelTvMixParameter(u32 trackBitFlag, u32 srcChannel,
                                                        const MixParameter& rParam) {
    for (int channel = 0; channel < ChannelIndex_Count; channel++) {
        float param = rParam.ch[channel];
        ForEachTrack(this, trackBitFlag, [srcChannel, channel, param](SequenceTrack* pTrack) {
            pTrack->SetTvMixParameter(srcChannel, channel, param);
        });
    }
}

/** @brief Sets the TV pan of tracks. @param trackBitFlag Bit n selects track n. @param pan Pan. */
void SequenceSoundPlayer::SetTrackTvPan(u32 trackBitFlag, float pan) {
    ForEachTrack(this, trackBitFlag, [pan](SequenceTrack* pTrack) {
        pTrack->GetTvParam().pan = pan;
    });
}

/**
 * @brief Sets the TV surround pan of tracks.
 * @param trackBitFlag Bit n selects track n.
 * @param span Pan.
 */
void SequenceSoundPlayer::SetTrackTvSurroundPan(u32 trackBitFlag, float span) {
    ForEachTrack(this, trackBitFlag, [span](SequenceTrack* pTrack) {
        pTrack->GetTvParam().span = span;
    });
}

/**
 * @brief Sets the TV main send of tracks.
 * @param trackBitFlag Bit n selects track n.
 * @param send Gain.
 */
void SequenceSoundPlayer::SetTrackTvMainSend(u32 trackBitFlag, float send) {
    ForEachTrack(this, trackBitFlag, [send](SequenceTrack* pTrack) {
        pTrack->GetTvParam().mainSend = send;
    });
}

/**
 * @brief Sets the TV effect send of tracks.
 * @param trackBitFlag Bit n selects track n.
 * @param bus Effect bus.
 * @param send Gain.
 */
void SequenceSoundPlayer::SetTrackTvFxSend(u32 trackBitFlag, AuxBus bus, float send) {
    ForEachTrack(this, trackBitFlag, [bus, send](SequenceTrack* pTrack) {
        pTrack->GetTvParam().fxSend[bus] = send;
    });
}

/**
 * @brief Finalizes the player when memory holding its sequence is released, and detaches
 * released banks.
 * @param pStart First byte of the released memory.
 * @param pEnd Last byte of the released memory.
 */
void SequenceSoundPlayer::InvalidateData(const void* pStart, const void* pEnd) {
    if (!mActiveFlag) {
        return;
    }

    for (int trackNo = 0; trackNo < TrackCountPerPlayer; trackNo++) {
        const SequenceTrack* pTrack = GetPlayerTrack(trackNo);
        if (pTrack == nullptr) {
            continue;
        }

        const void* pSequenceData = pTrack->GetSequenceData();
        if (pStart <= pSequenceData && pSequenceData <= pEnd) {
            Finalize();
            break;
        }
    }

    for (int i = 0; i < BankCountMax; i++) {
        const void* pBankFile = mBankFileReader[i].GetBankFileAddress();
        if (pStart <= pBankFile && pBankFile <= pEnd) {
            mBankFileReader[i].Finalize();
        }
    }
}

/**
 * @brief Gets a track.
 * @param trackNo Track slot.
 * @return The track, or nullptr when the slot is empty or out of range.
 */
const SequenceTrack* SequenceSoundPlayer::GetPlayerTrack(int trackNo) const {
    if (trackNo >= TrackCountPerPlayer) {
        return nullptr;
    }

    return mTracks[trackNo];
}

/** @brief Closes a track and returns it to the allocator. @param trackNo Track slot. */
void SequenceSoundPlayer::CloseTrack(int trackNo) {
    SequenceTrack* pTrack = GetPlayerTrack(trackNo);
    if (pTrack == nullptr) {
        return;
    }

    pTrack->Close();
    mSequenceTrackAllocator->FreeTrack(mTracks[trackNo]);
    mTracks[trackNo] = nullptr;
}

/** @brief Pushes the current track parameters to every playing channel. */
void SequenceSoundPlayer::UpdateChannelParam() {
    for (int trackNo = 0; trackNo < TrackCountPerPlayer; trackNo++) {
        SequenceTrack* pTrack = GetPlayerTrack(trackNo);
        if (pTrack != nullptr) {
            pTrack->UpdateChannelParam();
        }
    }
}

/**
 * @brief Advances every track by one tick.
 * @param doNoteOn Whether note events start channels.
 * @return Whether every track has finished.
 */
bool SequenceSoundPlayer::ParseNextTick(bool doNoteOn) {
    bool isActive = false;
    mVolume.Update();

    for (int trackNo = 0; trackNo < TrackCountPerPlayer; trackNo++) {
        SequenceTrack* pTrack = GetPlayerTrack(trackNo);
        if (pTrack == nullptr) {
            continue;
        }

        pTrack->UpdateChannelLength();
        if (pTrack->ParseNextTick(doNoteOn) < 0) {
            CloseTrack(trackNo);
        }

        if (pTrack->IsOpened()) {
            isActive = true;
        }
    }

    return !isActive;
}

/**
 * @brief Advances loading and playback by one sound-thread frame.
 * @param frame Elapsed time, in frames.
 */
void SequenceSoundPlayer::Update(int frame) {
    switch (mResState) {
    case ResState_ReceiveLoadRequest:
        if (!TryAllocLoader()) {
            return;
        }

        mLoader->Initialize(mLoaderArg);
        [[fallthrough]];
    case ResState_AppendLoadTask: {
        if (!mLoader->TryWait()) {
            return;
        }

        if (!mLoader->IsLoadSuccess()) {
            mFinishFlag = true;
            FinishPlayer();
            return;
        }

        const SequenceSoundLoader::Data& rData = mLoader->GetData();
        PrepareArg arg;
        arg.seqFile = rData.seqFile;
        arg.seqOffset = mStartInfo.seqOffset;
        arg.delayTime = mStartInfo.delayTime;
        arg.delayCount = mStartInfo.delayCount;
        arg.updateType = mStartInfo.updateType;

        for (int i = 0; i < BankCountMax; i++) {
            arg.bankFiles[i] = rData.bankFiles[i];
            arg.warcFiles[i] = rData.warcFiles[i];
            arg.warcIsIndividuals[i] = rData.warcIsIndividuals[i];
        }
        PrepareForPlayerHeap(arg);

        Skip(mStartInfo.startOffsetType, mStartInfo.startOffset);
        break;
    }
    default:
        break;
    }

    if (mDelayCount > 0) {
        mDelayCount--;
        return;
    }

    if (!mActiveFlag || !mStartedFlag) {
        return;
    }

    if (mSkipTickCounter != 0 || mSkipTimeCounter > 0.0f) {
        SkipTick();
    } else if (!mPauseFlag) {
        UpdateTick(frame);
    }

    UpdateChannelParam();
}

/** @brief Takes a loader from the manager. @return Whether a loader was available. */
bool SequenceSoundPlayer::TryAllocLoader() {
    if (mLoaderManager == nullptr) {
        return false;
    }

    SequenceSoundLoader* pLoader = mLoaderManager->Alloc();
    if (pLoader == nullptr) {
        return false;
    }

    mLoader = pLoader;
    mResState = ResState_AppendLoadTask;
    return true;
}

/**
 * @brief Resets both tasks and installs the archive context for a new load.
 * @param rArg Archive, player and items to load; referenced objects must outlive the tasks.
 */
void SequenceSoundLoader::Initialize(const Arg& rArg) {
    WaitTasks();

    mLoadTask.Initialize();
    mLoadTask.arg = rArg;
    mLoadTask.pHeap = nullptr;
    mLoadTask.pHeapDataManager = &mHeapDataManager;

    mFreeTask.Initialize();
    mFreeTask.arg = rArg;
    mFreeTask.pHeap = nullptr;
    mFreeTask.pHeapDataManager = &mHeapDataManager;
}

/**
 * @brief Starts an idle load once a player heap is available.
 * @return Whether loading has ended.
 */
bool SequenceSoundLoader::TryWait() {
    if (!mLoadTask.TryAllocPlayerHeap()) {
        return false;
    }

    if (mLoadTask.mState == 3 || mLoadTask.mState == 4) {
        return true;
    }

    if (mLoadTask.mState == 0) {
        TaskManager::GetInstance().AppendTask(&mLoadTask, TaskManager::TaskPriority_Normal);
    }

    return false;
}

/**
 * @brief Attaches loaded sequence data and makes the player active.
 * @param rArg Sequence, bank and wave archive files and start parameters.
 */
void SequenceSoundPlayer::PrepareForPlayerHeap(const PrepareArg& rArg) {
    if (mActiveFlag) {
        FinishPlayer();
    }

    SequenceTrack* pSeqTrack = GetPlayerTrack(0);
    if (pSeqTrack == nullptr) {
        Finalize();
        return;
    }

    SequenceSoundFileReader reader(rArg.seqFile);
    pSeqTrack->SetSeqData(reader.GetSequenceData(), rArg.seqOffset);
    pSeqTrack->Open();

    for (int i = 0; i < BankCountMax; i++) {
        mBankFileReader[i].Initialize(rArg.bankFiles[i]);
        mWaveArchiveFileReader[i].Initialize(rArg.warcFiles[i], rArg.warcIsIndividuals[i]);
    }

    mResState = ResState_Assigned;

    // An explicit frame count wins over a delay given in milliseconds.
    mDelayCount = rArg.delayCount != 0 ? rArg.delayCount : rArg.delayTime / FrameMsec;

    SetActiveFlag(true);
    DisposeCallbackManager::GetInstance().RegisterDisposeCallback(this);
    mIsRegisterPlayerCallback = true;
    mUpdateType = rArg.updateType;
}

/** @brief Fast-forwards through pending skip time without playing notes. */
void SequenceSoundPlayer::SkipTick() {
    for (int trackNo = 0; trackNo < TrackCountPerPlayer; trackNo++) {
        SequenceTrack* pTrack = GetPlayerTrack(trackNo);
        if (pTrack == nullptr) {
            continue;
        }

        pTrack->ReleaseAllChannel(SkipReleaseValue);
        pTrack->FreeAllChannel();
    }

    int skipCount = 0;
    while (mSkipTickCounter != 0 || mSkipTimeCounter * CalcTickPerMsec() >= 1.0f) {
        if (skipCount >= m_SkipIntervalTickPerFrame) {
            return;
        }

        if (mSkipTickCounter != 0) {
            mSkipTickCounter--;
        } else {
            float tickPerMsec = CalcTickPerMsec();
            mSkipTimeCounter -= 1.0f / tickPerMsec;
        }

        if (ParseNextTick(false)) {
            FinishPlayer();
            mFinishFlag = true;
            return;
        }

        skipCount++;
        mTickCounter++;
    }

    mSkipTimeCounter = 0.0f;
}

/**
 * @brief Plays the ticks that fall within the elapsed time.
 * @param frame Elapsed time, in frames.
 */
void SequenceSoundPlayer::UpdateTick(int frame) {
    float tickPerMsec = CalcTickPerMsec();
    if (tickPerMsec == 0.0f) {
        return;
    }

    u64 restTime = static_cast<u64>(frame) * FrameMsecFixed;
    u64 nextTime = static_cast<u64>(mTickFraction * FixedOne / tickPerMsec);

    while (nextTime < restTime) {
        restTime -= nextTime;

        if (ParseNextTick(true)) {
            FinishPlayer();
            mFinishFlag = true;
            return;
        }

        mTickCounter++;

        tickPerMsec = CalcTickPerMsec();
        if (tickPerMsec == 0.0f) {
            return;
        }

        nextTime = static_cast<u64>(FixedOne / tickPerMsec);
    }

    nextTime -= restTime;
    mTickFraction = tickPerMsec * static_cast<float>(nextTime) / FixedOne;
}

/**
 * @brief Starts a note through the player's note-on callback.
 * @param bankIndex Bank slot.
 * @param rInfo Note parameters.
 * @return The started channel, or nullptr.
 */
Channel* SequenceSoundPlayer::NoteOn(u8 bankIndex, const NoteOnInfo& rInfo) {
    return mNoteOnCallback->NoteOn(this, bankIndex, rInfo);
}

/**
 * @brief Attaches resident sequence data, makes the player active and joins the sound thread.
 * @param rArg Sequence, bank and wave archive files and start parameters.
 */
void SequenceSoundPlayer::Prepare(const PrepareArg& rArg) {
    if (mActiveFlag) {
        FinishPlayer();
    }

    SequenceTrack* pSeqTrack = GetPlayerTrack(0);
    if (pSeqTrack == nullptr) {
        Finalize();
        return;
    }

    SequenceSoundFileReader reader(rArg.seqFile);
    pSeqTrack->SetSeqData(reader.GetSequenceData(), rArg.seqOffset);
    pSeqTrack->Open();

    for (int i = 0; i < BankCountMax; i++) {
        mBankFileReader[i].Initialize(rArg.bankFiles[i]);
        mWaveArchiveFileReader[i].Initialize(rArg.warcFiles[i], rArg.warcIsIndividuals[i]);
    }

    mResState = ResState_Assigned;

    // An explicit frame count wins over a delay given in milliseconds.
    mDelayCount = rArg.delayCount != 0 ? rArg.delayCount : rArg.delayTime / FrameMsec;

    SetActiveFlag(true);
    DisposeCallbackManager::GetInstance().RegisterDisposeCallback(this);
    SoundThread::GetInstance().RegisterPlayerCallback(this);
    mIsRegisterPlayerCallback = true;
    mUpdateType = rArg.updateType;
}

/**
 * @brief Requests the sequence data to be loaded on the task thread before playing.
 * @param rInfo Start parameters applied once loading ends.
 * @param rArg Archive context and items to load.
 */
void SequenceSoundPlayer::RequestLoad(const StartInfo& rInfo,
                                      const SequenceSoundLoader::Arg& rArg) {
    mStartInfo = rInfo;
    mLoaderArg = rArg;
    mResState = ResState_ReceiveLoadRequest;
    mDelayCount = 0;

    SoundThread::GetInstance().RegisterPlayerCallback(this);
    mIsRegisterPlayerCallback = true;
}

/**
 * @brief Sums the render time of every channel the player is playing.
 * @param rProfile Profile of the last audio frame.
 * @return Render time in ticks.
 */
u64 SequenceSoundPlayer::GetProcessTick(const SoundProfile& rProfile) {
    u64 processTick = 0;

    for (int trackNo = 0; trackNo < TrackCountPerPlayer; trackNo++) {
        SequenceTrack* pTrack = GetPlayerTrack(trackNo);
        if (pTrack == nullptr) {
            continue;
        }

        processTick += GetTrackProcessTick(*pTrack, rProfile);
    }

    return processTick;
}

/**
 * @brief Attaches banks for MIDI playback and makes the player active.
 * @param ppBankFiles Bank files, one per bank slot.
 * @param ppWarcFiles Wave archive files, one per bank slot.
 * @param pWarcIsIndividuals Whether each wave archive is loaded per wave.
 */
void SequenceSoundPlayer::PrepareForMidi(const void** ppBankFiles, const void** ppWarcFiles,
                                         bool* pWarcIsIndividuals) {
    for (int i = 0; i < BankCountMax; i++) {
        mBankFileReader[i].Initialize(ppBankFiles[i]);
        mWaveArchiveFileReader[i].Initialize(ppWarcFiles[i], pWarcIsIndividuals[i]);
    }

    mResState = ResState_Assigned;

    if (mActiveFlag) {
        return;
    }

    DisposeCallbackManager::GetInstance().RegisterDisposeCallback(this);
    mIsRegisterPlayerCallback = true;
    SetActiveFlag(true);
    SoundThread::GetInstance().RegisterPlayerCallback(this);
}

/** @brief Limits how many ticks skipping may parse per frame. @param tick Ticks per frame. */
void SequenceSoundPlayer::SetSkipIntervalTick(int tick) {
    m_SkipIntervalTickPerFrame = tick;
}

/** @brief Gets how many ticks skipping may parse per frame. @return Ticks per frame. */
int SequenceSoundPlayer::GetSkipIntervalTick() {
    return m_SkipIntervalTickPerFrame;
}

/**
 * @brief Collects pointers to the items a sequence needs.
 * @param pSoundArchive Archive holding the items.
 * @param pSoundDataManager Resident data to search first.
 * @param pSequenceInfo Sequence item.
 * @param pBankItemInfos Bank items, one per bank slot.
 * @param pPlayer Player whose heap receives the data.
 */
SequenceSoundLoader::LoadInfo::LoadInfo(const SoundArchive* pSoundArchive,
                                        const SoundDataManager* pSoundDataManager,
                                        LoadItemInfo* pSequenceInfo, LoadItemInfo* pBankItemInfos,
                                        SoundPlayer* pPlayer)
    : pArchive(pSoundArchive), pDataManager(pSoundDataManager), pSeqInfo(pSequenceInfo),
      pSoundPlayer(pPlayer) {
    for (int i = 0; i < BankCountMax; i++) {
        pBankInfos[i] = &pBankItemInfos[i];
    }
}

/** @brief Waits for outstanding tasks before destroying the embedded loader resources. */
SequenceSoundLoader::~SequenceSoundLoader() {
    WaitTasks();
}

/** @brief Resets the loading task to idle with no resolved files. */
void SequenceSoundLoader::DataLoadTask::Initialize() {
    mState = 0;
    // Clears every file entry, leaving the structure's tail padding alone.
    std::memset(&result, 0, offsetof(Data, warcIsIndividuals) + sizeof(result.warcIsIndividuals));

    succeeded = false;
}

/** @brief Resets the player-heap release task to idle. */
void SequenceSoundLoader::FreePlayerHeapTask::Initialize() {
    mState = 0;
}

/** @brief Acquires a player heap if none is assigned. @return Whether a heap is available. */
bool SequenceSoundLoader::DataLoadTask::TryAllocPlayerHeap() {
    if (pHeap == nullptr) {
        pHeap = arg.pSoundPlayer->detail_AllocPlayerHeap();
        if (pHeap == nullptr) {
            return false;
        }
    }

    return true;
}

/** @brief Checks whether either task is still running. @return Whether a task is unfinished. */
bool SequenceSoundLoader::IsInUse() {
    return !os::TryWaitEvent(&mLoadTask.mCompletionEvent) ||
           !os::TryWaitEvent(&mFreeTask.mCompletionEvent);
}

/**
 * @brief Loads the sequence, its banks and any missing wave archives into the player heap.
 * @param rLogger Unused task profiling destination in this implementation.
 */
void SequenceSoundLoader::DataLoadTask::Execute(TaskProfileLogger& rLogger) {
    pHeapDataManager->Initialize(arg.pArchive);

    if (arg.seqInfo.address == nullptr && arg.seqInfo.itemId != SoundArchive::InvalidId) {
        SoundArchive::ItemId id = arg.seqInfo.itemId;
        if (!pHeapDataManager->LoadData(id, pHeap, LoadFlag_Seq, 0)) {
            pHeap->SetLoadFinished();
            succeeded = false;
            return;
        }

        arg.seqInfo.address = pHeapDataManager->detail_GetFileAddressByItemId(id);
    }

    LoadItemInfo warcInfos[BankCountMax];
    bool isLoadIndividuals[BankCountMax] = {};
    bool isSuccess = true;

    for (int i = 0; i < BankCountMax; i++) {
        LoadItemInfo& rBankInfo = arg.bankInfos[i];

        if (rBankInfo.address == nullptr && rBankInfo.itemId != SoundArchive::InvalidId) {
            SoundArchive::ItemId id = rBankInfo.itemId;
            if (!pHeapDataManager->LoadData(id, pHeap, LoadFlag_Bank, 0)) {
                isSuccess = false;
                break;
            }

            rBankInfo.address = pHeapDataManager->detail_GetFileAddressByItemId(id);
        }

        if (rBankInfo.itemId == SoundArchive::InvalidId || rBankInfo.address == nullptr) {
            continue;
        }

        Util::WaveArchiveLoadStatus status =
            Util::GetWaveArchiveOfBank(warcInfos[i], isLoadIndividuals[i], rBankInfo.address,
                                       *arg.pArchive, *arg.pDataManager);
        if (status != Util::WaveArchiveLoadStatus_NotYet &&
            status != Util::WaveArchiveLoadStatus_Error) {
            continue;
        }

        if (!pHeapDataManager->detail_LoadWaveArchiveByBankFile(rBankInfo.address, pHeap)) {
            isSuccess = false;
            break;
        }

        Util::GetWaveArchiveOfBank(warcInfos[i], isLoadIndividuals[i], rBankInfo.address,
                                   *arg.pArchive, *pHeapDataManager);
    }

    if (isSuccess) {
        result.seqFile = arg.seqInfo.address;

        for (int i = 0; i < BankCountMax; i++) {
            result.bankFiles[i] = arg.bankInfos[i].address;
            result.warcFiles[i] = warcInfos[i].address;
            result.warcIsIndividuals[i] = isLoadIndividuals[i];
        }
    }

    pHeap->SetLoadFinished();
    succeeded = isSuccess;
}

/**
 * @brief Clears and returns the assigned player heap, then finalizes its data manager.
 * @param rLogger Unused task profiling destination in this implementation.
 */
void SequenceSoundLoader::FreePlayerHeapTask::Execute(TaskProfileLogger& rLogger) {
    if (pHeap != nullptr) {
        pHeap->Clear();
        arg.pSoundPlayer->detail_FreePlayerHeap(pHeap);
    }

    pHeapDataManager->Finalize();
}
}  // namespace nn::atk::detail::driver
