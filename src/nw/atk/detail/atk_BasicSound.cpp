#include <nn/atk/detail/atk_BasicSound.h>

#include <cstring>
#include <nn/atk/atk_BasicSoundPlayer.h>
#include <nn/atk/atk_ChannelMixVolume.h>
#include <nn/atk/atk_DriverCommand.h>
#include <nn/atk/atk_ExternalSoundPlayer.h>
#include <nn/atk/atk_FinalMix.h>
#include <nn/atk/atk_OutputAdditionalParam.h>
#include <nn/atk/atk_Sound3DEngine.h>
#include <nn/atk/atk_SoundActor.h>
#include <nn/atk/atk_SoundHandle.h>
#include <nn/atk/atk_SoundPlayer.h>
#include <nn/atk/atk_Util.h>
#include <nn/util.h>

namespace nn::atk::detail {
namespace {
/**
 * @brief Allocates a driver command of a given type.
 * @tparam T Command structure.
 * @param rManager Command manager the command is allocated from.
 * @return Uninitialized command storage.
 */
template <typename T>
T* AllocCommand(CommandManager& rManager) {
    return static_cast<T*>(rManager.AllocMemory(sizeof(T), true));
}
}  // namespace

u32 BasicSound::g_LastInstanceId;

/**
 * @brief Creates a sound with neutral parameters that is not yet initialized.
 */
BasicSound::BasicSound()
    : mFadeVolume(),
      mPauseFadeVolume(),
      mMuteFadeVolume(),
      mState(State_Constructed),
      m_SetupTick(0),
      mExtMoveVolume(),
      mOutputAdditionalParam(),
      mUserParam(nullptr),
      mUserParamSize(0),
      mSoundStopCallback(nullptr) {
    ResetAmbientParam();
    ResetActorParam();
}

/**
 * @brief Resets the sound for a new playback and binds its driver player to an output.
 * @param pOutputReceiver Output the sound plays to.
 * @return Always true.
 */
bool BasicSound::Initialize(OutputReceiver* pOutputReceiver) {
    mInstanceId = g_LastInstanceId++;
    mPlayerHeap = nullptr;
    mGeneralHandle = nullptr;
    mTempGeneralHandle = nullptr;
    mSoundPlayer = nullptr;
    mSoundActor = nullptr;
    mExternalSoundPlayer = nullptr;
    m_pSoundArchive = nullptr;
    mAmbientInfo.paramUpdateCallback = nullptr;
    mAmbientInfo.argUpdateCallback = nullptr;
    mAmbientInfo.argAllocator = nullptr;
    mAmbientInfo.arg = nullptr;
    mAmbientInfo.argSize = 0;
    mPlayerState = PlayerState_Init;
    mPauseState = PauseState_Normal;
    mMuteState = MuteState_Normal;
    mPauseMode = PauseMode_Default;
    mStartFlag = false;
    mStartedFlag = false;
    mAutoStopFlag = false;
    mFadeOutFlag = false;
    mPlayerAvailableFlag = false;
    mUnPauseFlag = false;
    mAutoStopCounter = 0;
    mUpdateCounter = 0;
    mPlayingCounter = 0;
    m_Id = 0xffffffff;
    mFadeVolume.InitValue(1.0f);
    mPauseFadeVolume.InitValue(1.0f);
    mMuteFadeVolume.InitValue(1.0f);
    mInitVolume = 1.0f;
    mPitch = 1.0f;
    mLpfFreq = 0.0f;
    mBiquadFilterType = -1;
    mBiquadFilterValue = 0.0f;
    mOutputLineFlag = 1;
    mExtMoveVolume.InitValue(1.0f);
    mMixMode = MixMode_Pan;
    mPan = 0.0f;
    mSurroundPan = 0.0f;
    for (int i = 0; i < DefaultBusCount; i++) {
        mSend[i] = 0.0f;
    }

    for (int i = 0; i < OutputDevice_Count; i++) {
        mOutputParam[i].volume = 1.0f;
        mOutputParam[i].pan = 0.0f;
        mOutputParam[i].span = 0.0f;
        mOutputParam[i].mainSend = 0.0f;
        for (int bus = 0; bus < AuxBus_Count; bus++) {
            mOutputParam[i].fxSend[bus] = 0.0f;
        }

        if (mOutputAdditionalParam[i] != nullptr) {
            mOutputAdditionalParam[i]->Reset();
        }
    }

    ResetAmbientParam();
    ResetActorParam();

    if (mUserParamSize != 0) {
        std::memset(mUserParam, 0, mUserParamSize);
    }

    driver::BasicSoundPlayer* pPlayer = GetBasicSoundPlayerHandle();
    DriverCommand& rCommandManager = DriverCommand::GetInstance();
    auto* pCommand = AllocCommand<DriverCommandPlayerInit>(rCommandManager);
    pCommand->type = DriverCommandId_PlayerInit;
    pCommand->player = pPlayer;
    pCommand->pOutputReceiver = pOutputReceiver;
    pCommand->availableFlag = &mPlayerAvailableFlag;
    rCommandManager.PushCommand(pCommand);

    pPlayer->ClearEvent();
    pOutputReceiver->AddReferenceCount(1);
    mOutputReceiver = pOutputReceiver;
    mState = State_Initialized;
    return true;
}

/**
 * @brief Sets the player and ambient priorities without re-sorting the player.
 * @param priority Player priority.
 * @param ambientPriority Priority added by the ambient callback.
 */
void BasicSound::SetPriority(int priority, int ambientPriority) {
    mPlayerPriority = priority;
    mAmbientParam.priority = ambientPriority;
}

/**
 * @brief Gets the player and ambient priorities.
 * @param pPriority Receives the player priority; may be nullptr.
 * @param pAmbientPriority Receives the ambient priority; may be nullptr.
 */
void BasicSound::GetPriority(int* pPriority, int* pAmbientPriority) const {
    if (pPriority != nullptr) {
        *pPriority = mPlayerPriority;
    }

    if (pAmbientPriority != nullptr) {
        *pAmbientPriority = mAmbientParam.priority;
    }
}

/**
 * @brief Asks the driver player to forget that it was finalized for lack of resources.
 */
void BasicSound::ClearIsFinalizedForCannotAllocatedResourceFlag() {
    DriverCommand& rCommandManager = DriverCommand::GetInstance();
    auto* pCommand = AllocCommand<DriverCommandPlayerClearResourceFlag>(rCommandManager);
    pCommand->type = DriverCommandId_PlayerClearResourceFlag;
    pCommand->player = GetBasicSoundPlayerHandle();
    rCommandManager.PushCommand(pCommand);
}

/**
 * @brief Detaches the sound from its handles and players and stops the driver player.
 */
void BasicSound::Finalize() {
    if (mState != State_Initialized) {
        return;
    }

    m_Id = 0xffffffff;

    if (mGeneralHandle != nullptr) {
        mGeneralHandle->DetachSound();
    }

    if (mTempGeneralHandle != nullptr) {
        mTempGeneralHandle->DetachSound();
    }

    if (IsAttachedTempSpecialHandle()) {
        DetachTempSpecialHandle();
    }

    if (mSoundPlayer != nullptr) {
        mSoundPlayer->detail_RemoveSound(this);
    }

    if (mExternalSoundPlayer != nullptr) {
        mExternalSoundPlayer->RemoveSound(this);
    }

    if (mAmbientInfo.argAllocator != nullptr) {
        mAmbientInfo.argAllocator->detail_FreeAmbientArg(mAmbientInfo.arg, this);
        mAmbientInfo.arg = nullptr;
    }

    if (mStartedFlag) {
        DriverCommand& rCommandManager = DriverCommand::GetInstance();
        auto* pCommand = AllocCommand<DriverCommandPlayer>(rCommandManager);
        pCommand->type = DriverCommandId_PlayerStop;
        pCommand->player = GetBasicSoundPlayerHandle();
        pCommand->flag = mFadeOutFlag;
        rCommandManager.PushCommand(pCommand);
        mStartedFlag = false;
    }

    mPlayerAvailableFlag = false;
    mPlayerState = PlayerState_Stop;

    {
        DriverCommand& rCommandManager = DriverCommand::GetInstance();
        auto* pCommand = AllocCommand<DriverCommandPlayer>(rCommandManager);
        pCommand->type = DriverCommandId_PlayerFinalize;
        pCommand->player = GetBasicSoundPlayerHandle();
        rCommandManager.PushCommand(pCommand);
    }

    mOutputReceiver = nullptr;

    if (mSoundStopCallback != nullptr) {
        mSoundStopCallback();
        mSoundStopCallback = nullptr;
    }

    mFadeOutFlag = false;
    mState = State_Finalized;
}

/**
 * @brief Sets the id of the sound.
 * @param id Sound id.
 */
void BasicSound::SetId(u32 id) {
    m_Id = id;
}

/**
 * @brief Tests whether a general handle refers to the sound.
 * @return Whether a general handle is attached.
 */
bool BasicSound::IsAttachedGeneralHandle() {
    return mGeneralHandle != nullptr;
}

/** @brief Detaches the general handle from the sound. */
void BasicSound::DetachGeneralHandle() {
    mGeneralHandle->DetachSound();
}

/**
 * @brief Tests whether a temporary general handle refers to the sound.
 * @return Whether a temporary general handle is attached.
 */
bool BasicSound::IsAttachedTempGeneralHandle() {
    return mTempGeneralHandle != nullptr;
}

/** @brief Detaches the temporary general handle from the sound. */
void BasicSound::DetachTempGeneralHandle() {
    mTempGeneralHandle->DetachSound();
}

/** @brief Requests that the prepared sound starts on the next update. */
void BasicSound::StartPrepared() {
    mStartFlag = true;
}

/**
 * @brief Stops the sound, optionally fading it out.
 * @param fadeFrames Fade-out length in frames; zero or less stops immediately.
 */
void BasicSound::Stop(int fadeFrames) {
    if (fadeFrames < 1 || mPauseState == PauseState_Paused) {
        Finalize();
        return;
    }

    if (!mStartFlag && !mStartedFlag) {
        Finalize();
        return;
    }

    int frames = static_cast<int>(fadeFrames * mFadeVolume.GetValue());
    mFadeVolume.SetTarget(0.0f, frames);
    SetPlayerPriority(0);
    mAutoStopFlag = false;
    mFadeOutFlag = true;
    mPauseState = PauseState_Normal;
    mMuteState = MuteState_Normal;
    mUnPauseFlag = false;
    mPauseMode = PauseMode_Default;
}

/**
 * @brief Changes the player priority and re-sorts the owning player.
 * @param priority New player priority.
 */
void BasicSound::SetPlayerPriority(int priority) {
    mPlayerPriority = priority;

    if (mSoundPlayer != nullptr) {
        mSoundPlayer->detail_SortPriorityList(this);
    }

    OnUpdatePlayerPriority();
}

/** @brief Stops the sound immediately. */
void BasicSound::ForceStop() {
    mFadeOutFlag = true;
    Finalize();
}

/**
 * @brief Pauses or resumes the sound with the default pause mode.
 * @param flag True to pause, false to resume.
 * @param fadeFrames Transition length in frames.
 */
void BasicSound::Pause(bool flag, int fadeFrames) {
    Pause(flag, fadeFrames, PauseMode_Default);
}

/**
 * @brief Pauses or resumes the sound.
 * @param flag True to pause, false to resume.
 * @param fadeFrames Transition length in frames.
 * @param mode Pause policy.
 */
void BasicSound::Pause(bool flag, int fadeFrames, PauseMode mode) {
    if (flag) {
        switch (mPauseState) {
        case PauseState_Normal:
        case PauseState_Pausing:
        case PauseState_Unpausing: {
            int frames = static_cast<int>(fadeFrames * mPauseFadeVolume.GetValue());
            if (frames < 1) {
                frames = 1;
            }

            mPauseFadeVolume.SetTarget(0.0f, frames);
            mUnPauseFlag = false;
            mPauseState = PauseState_Pausing;
            break;
        }
        default:
            return;
        }
    } else {
        switch (mPauseState) {
        case PauseState_Pausing:
        case PauseState_Paused:
        case PauseState_Unpausing: {
            int frames = static_cast<int>(fadeFrames * (1.0f - mPauseFadeVolume.GetValue()));
            if (frames < 1) {
                frames = 1;
            }

            mPauseFadeVolume.SetTarget(1.0f, frames);
            mPauseState = PauseState_Unpausing;
            mUnPauseFlag = true;
            break;
        }
        default:
            return;
        }
    }

    mPauseMode = mode;
}

/**
 * @brief Mutes or unmutes the sound.
 * @param flag True to mute, false to unmute.
 * @param fadeFrames Transition length in frames.
 */
void BasicSound::Mute(bool flag, int fadeFrames) {
    if (flag) {
        switch (mMuteState) {
        case MuteState_Normal:
        case MuteState_Muting:
        case MuteState_Unmuting: {
            int frames = static_cast<int>(fadeFrames * mMuteFadeVolume.GetValue());
            if (frames < 1) {
                frames = 1;
            }

            mMuteFadeVolume.SetTarget(0.0f, frames);
            mMuteState = MuteState_Muting;
            break;
        }
        default:
            return;
        }
    } else {
        switch (mMuteState) {
        case MuteState_Muting:
        case MuteState_Muted:
        case MuteState_Unmuting: {
            int frames = static_cast<int>(fadeFrames * (1.0f - mMuteFadeVolume.GetValue()));
            if (frames < 1) {
                frames = 1;
            }

            mMuteFadeVolume.SetTarget(1.0f, frames);
            mMuteState = MuteState_Unmuting;
            break;
        }
        default:
            return;
        }
    }
}

/**
 * @brief Stops the sound automatically after a number of updates.
 * @param frames Updates before the sound stops; zero or less disables the auto stop.
 */
void BasicSound::SetAutoStopCounter(int frames) {
    mAutoStopCounter = frames;
    mAutoStopFlag = frames > 0;
}

/**
 * @brief Fades the sound in from silence, if it has not been updated yet.
 * @param frames Fade-in length in frames.
 */
void BasicSound::FadeIn(int frames) {
    if (mFadeOutFlag) {
        return;
    }

    if (mUpdateCounter == 0) {
        mFadeVolume.InitValue(0.0f);
        mFadeVolume.SetTarget(1.0f, frames);
    }
}

/**
 * @brief Tests whether the sound is pausing or paused.
 * @return Whether a pause is in effect.
 */
bool BasicSound::IsPause() const {
    return mPauseState == PauseState_Pausing || mPauseState == PauseState_Paused;
}

/**
 * @brief Tests whether the sound is muting or muted.
 * @return Whether a mute is in effect.
 */
bool BasicSound::IsMute() const {
    return mMuteState == MuteState_Muting || mMuteState == MuteState_Muted;
}

/**
 * @brief Advances the sound by one frame: starts, fades, pauses and stops it, and sends its
 * parameters to the driver player.
 */
void BasicSound::Update() {
    driver::BasicSoundPlayer* pPlayer = GetBasicSoundPlayerHandle();

    if (mPlayerAvailableFlag && GetBasicSoundPlayerHandle()->IsPlayFinished()) {
        Finalize();
        return;
    }

    if (pPlayer->IsFinalizedForCannotAllocateResource()) {
        ClearIsFinalizedForCannotAllocatedResourceFlag();
        Finalize();
        return;
    }

    if (!IsPrepared()) {
        return;
    }

    if (mAutoStopFlag) {
        if (mAutoStopCounter == 0) {
            if (mPauseState == PauseState_Normal || mPauseState == PauseState_Unpausing) {
                Stop(0);
                return;
            }
        } else {
            mAutoStopCounter--;
        }
    }

    bool isPlayerStarted = false;
    if (!mStartedFlag) {
        if (!mStartFlag) {
            return;
        }

        if (!IsPrepared()) {
            return;
        }

        isPlayerStarted = true;
        mPlayerState = PlayerState_Play;
    }

    if (mPlayerState == PlayerState_Play) {
        if (mUpdateCounter != 0xffffffff) {
            mUpdateCounter++;
        }

        if (mPauseState != PauseState_Paused && mPlayingCounter != 0xffffffff) {
            mPlayingCounter++;
        }
    }

    bool isPauseFadeFinished = mPauseFadeVolume.IsFinished();
    switch (mPauseState) {
    case PauseState_Pausing:
        mPauseFadeVolume.Update();
        break;
    case PauseState_Unpausing:
        mPauseFadeVolume.Update();
        [[fallthrough]];
    case PauseState_Normal:
        UpdateMoveValue();
        break;
    default:
        break;
    }

    if (mAmbientInfo.argUpdateCallback != nullptr) {
        mAmbientInfo.argUpdateCallback->detail_UpdateAmbientArg(mAmbientInfo.arg, this);
    }

    if (mAmbientInfo.paramUpdateCallback != nullptr) {
        SoundAmbientParam ambientParam;
        if (mUpdateCounter != 0) {
            ambientParam.volume = mAmbientParam.volume;
            ambientParam.pitch = mAmbientParam.pitch;
            ambientParam.lpf = mAmbientParam.lpf;
            ambientParam.biquadFilterValue = mAmbientParam.biquadFilterValue;
            ambientParam.biquadFilterType = mAmbientParam.biquadFilterType;
            ambientParam.userData = mAmbientParam.userData;
            ambientParam.priority = mAmbientParam.priority;
            ambientParam.outputLineFlag = mAmbientParam.outputLineFlag;
            ambientParam.tvParam = mAmbientParam.tvParam;
        } else {
            ambientParam.userData = 0;
        }

        mAmbientInfo.paramUpdateCallback->detail_UpdateAmbientParam(mAmbientInfo.arg, m_Id,
                                                                    &ambientParam);

        mAmbientParam.volume = ambientParam.volume;
        mAmbientParam.pitch = ambientParam.pitch;
        mAmbientParam.lpf = ambientParam.lpf;
        mAmbientParam.biquadFilterValue = ambientParam.biquadFilterValue;
        mAmbientParam.biquadFilterType = ambientParam.biquadFilterType;
        mAmbientParam.userData = ambientParam.userData;
        mAmbientParam.priority = ambientParam.priority;
        mAmbientParam.outputLineFlag = ambientParam.outputLineFlag;
        mAmbientParam.tvParam = ambientParam.tvParam;
    }

    if (mSoundActor != nullptr) {
        mActorParam = mSoundActor->detail_GetActorParam();
    }

    UpdateParam();

    if (mFadeOutFlag && mFadeVolume.IsFinished()) {
        mFadeOutFlag = false;
        Finalize();
        return;
    }

    DriverCommand& rCommandManager = DriverCommand::GetInstance();

    if (isPlayerStarted) {
        auto* pCommand = AllocCommand<DriverCommandPlayer>(rCommandManager);
        pCommand->type = DriverCommandId_PlayerStart;
        pCommand->player = GetBasicSoundPlayerHandle();
        rCommandManager.PushCommand(pCommand);

        mStartFlag = false;
        mStartedFlag = true;
    }

    // The pause fade state was sampled before this frame's fade update; refresh it in this mode.
    if (mPauseMode == PauseMode_PauseImmediately) {
        isPauseFadeFinished = mPauseFadeVolume.IsFinished();
    }

    switch (mPauseState) {
    case PauseState_Pausing:
        if (isPauseFadeFinished) {
            auto* pCommand = AllocCommand<DriverCommandPlayer>(rCommandManager);
            pCommand->type = DriverCommandId_PlayerPause;
            pCommand->player = GetBasicSoundPlayerHandle();
            pCommand->flag = true;
            rCommandManager.PushCommand(pCommand);

            mPauseState = PauseState_Paused;
        }
        break;
    case PauseState_Unpausing:
        if (mPauseFadeVolume.IsFinished()) {
            mPauseState = PauseState_Normal;
        }
        break;
    default:
        break;
    }

    if (mUnPauseFlag) {
        auto* pCommand = AllocCommand<DriverCommandPlayer>(rCommandManager);
        pCommand->type = DriverCommandId_PlayerPause;
        pCommand->player = GetBasicSoundPlayerHandle();
        pCommand->flag = false;
        rCommandManager.PushCommand(pCommand);

        mUnPauseFlag = false;
    }

    switch (mMuteState) {
    case MuteState_Muting:
        if (mMuteFadeVolume.IsFinished()) {
            mMuteState = MuteState_Muted;
        }
        break;
    case MuteState_Unmuting:
        if (mMuteFadeVolume.IsFinished()) {
            mMuteState = MuteState_Normal;
        }
        break;
    default:
        break;
    }
}

/**
 * @brief Combines every parameter layer and sends the result to the driver player.
 */
void BasicSound::UpdateParam() {
    OnUpdateParam();

    f32 volume = CalculateVolume();
    f32 pitch = CalculatePitch();
    f32 lpfFreq = CalculateLpfFrequency();
    u32 outputLineFlag = CalculateOutLineFlag();
    int biquadFilterType;
    f32 biquadFilterValue;
    CalculateBiquadFilter(&biquadFilterType, &biquadFilterValue);
    OutputParam tvParam;
    CalculateOutputParam(&tvParam, OutputDevice_Main);

    DriverCommand& rCommandManager = DriverCommand::GetInstance();
    auto* pParamCommand = AllocCommand<DriverCommandPlayerParam>(rCommandManager);
    pParamCommand->type = DriverCommandId_PlayerParam;
    pParamCommand->player = GetBasicSoundPlayerHandle();
    pParamCommand->volume = volume;
    pParamCommand->pitch = pitch;
    pParamCommand->lpfFreq = lpfFreq;
    pParamCommand->biquadFilterType = biquadFilterType;
    pParamCommand->biquadFilterValue = biquadFilterValue;
    pParamCommand->outputLineFlag = outputLineFlag;
    pParamCommand->tvParam = tvParam;
    rCommandManager.PushCommand(pParamCommand);

    if (mOutputAdditionalParam[OutputDevice_Main] == nullptr) {
        return;
    }

    if (mOutputAdditionalParam[OutputDevice_Main]->IsAdditionalSendEnabled()) {
        ValueArray<f32>* pSends =
            mOutputAdditionalParam[OutputDevice_Main]->GetAdditionalSendAddr();
        SoundPlayer* pPlayer = mSoundPlayer;
        for (int i = 0; i < pSends->GetCount(); i++) {
            int bus = i + DefaultBusCount;
            f32 playerSend = pPlayer != nullptr ? pPlayer->GetSend(bus) : 0.0f;

            auto* pCommand = AllocCommand<DriverCommandPlayerAdditionalSend>(rCommandManager);
            pCommand->type = DriverCommandId_PlayerAdditionalSend;
            pCommand->player = GetBasicSoundPlayerHandle();
            pCommand->bus = bus;
            pCommand->send = playerSend + pSends->GetValue(i);
            rCommandManager.PushCommand(pCommand);
        }
    }

    if (mOutputAdditionalParam[OutputDevice_Main]->IsBusMixVolumeEnabled()) {
        bool isBusMixVolumeUsed = mOutputAdditionalParam[OutputDevice_Main]->IsBusMixVolumeUsed();

        auto* pUsedCommand = AllocCommand<DriverCommandPlayerBusMixVolumeUsed>(rCommandManager);
        pUsedCommand->type = DriverCommandId_PlayerBusMixVolumeUsed;
        pUsedCommand->player = GetBasicSoundPlayerHandle();
        pUsedCommand->isUsed = isBusMixVolumeUsed;
        rCommandManager.PushCommand(pUsedCommand);

        if (isBusMixVolumeUsed) {
            OutputBusMixVolume busMixVolume;
            CalculateOutputBusMixVolume(&busMixVolume, OutputDevice_Main);

            auto* pVolumeCommand = AllocCommand<DriverCommandPlayerBusMixVolume>(rCommandManager);
            pVolumeCommand->type = DriverCommandId_PlayerBusMixVolume;
            pVolumeCommand->player = GetBasicSoundPlayerHandle();
            pVolumeCommand->volume = busMixVolume;
            rCommandManager.PushCommand(pVolumeCommand);

            for (int i = 0; i < mOutputAdditionalParam[OutputDevice_Main]
                                    ->GetBusMixVolumePacketAddr()
                                    ->GetBusCount();
                 i++) {
                auto* pCommand =
                    AllocCommand<DriverCommandPlayerBusMixVolumeEnabled>(rCommandManager);
                pCommand->type = DriverCommandId_PlayerBusMixVolumeEnabled;
                pCommand->player = GetBasicSoundPlayerHandle();
                pCommand->bus = i;
                pCommand->isEnabled =
                    mOutputAdditionalParam[OutputDevice_Main]->IsBusMixVolumeEnabledForBus(i);
                rCommandManager.PushCommand(pCommand);
            }
        }
    }

    if (mOutputAdditionalParam[OutputDevice_Main]->IsVolumeThroughModeEnabled()) {
        auto* pUsedCommand =
            AllocCommand<DriverCommandPlayerVolumeThroughModeUsed>(rCommandManager);
        pUsedCommand->type = DriverCommandId_PlayerVolumeThroughModeUsed;
        pUsedCommand->player = GetBasicSoundPlayerHandle();
        pUsedCommand->isVolumeThroughModeUsed = IsVolumeThroughModeUsed();
        rCommandManager.PushCommand(pUsedCommand);

        if (pUsedCommand->isVolumeThroughModeUsed) {
            f32 binaryVolume = mInitVolume;
            VolumeThroughModePacket* pPacket =
                mOutputAdditionalParam[OutputDevice_Main]->GetVolumeThroughModePacketAddr();

            auto* pVolumeCommand = AllocCommand<DriverCommandPlayerBinaryVolume>(rCommandManager);
            pVolumeCommand->type = DriverCommandId_PlayerBinaryVolume;
            pVolumeCommand->player = GetBasicSoundPlayerHandle();
            pVolumeCommand->volume = binaryVolume;
            rCommandManager.PushCommand(pVolumeCommand);

            for (int i = 0; i < pPacket->GetBusCount(); i++) {
                auto* pCommand =
                    AllocCommand<DriverCommandPlayerVolumeThroughMode>(rCommandManager);
                pCommand->type = DriverCommandId_PlayerVolumeThroughMode;
                pCommand->player = GetBasicSoundPlayerHandle();
                pCommand->bus = i;
                pCommand->mode = pPacket->GetVolumeThroughMode(i);
                rCommandManager.PushCommand(pCommand);
            }
        }
    }
}

/** @brief Advances the stop fade, the mute fade and the volume transition by one frame. */
void BasicSound::UpdateMoveValue() {
    mFadeVolume.Update();
    mMuteFadeVolume.Update();
    mExtMoveVolume.Update();
}

/**
 * @brief Combines every volume layer of the sound.
 * @return Final linear volume.
 */
f32 BasicSound::CalculateVolume() const {
    if (mMuteState == MuteState_Muted) {
        return 0.0f;
    }

    f32 volume = IsVolumeThroughModeUsed() ? 1.0f : mInitVolume;
    volume *= mSoundPlayer->GetVolume();
    volume *= mExtMoveVolume.GetValue();
    volume *= mFadeVolume.GetValue();
    volume *= mPauseFadeVolume.GetValue();
    volume *= mMuteFadeVolume.GetValue();
    volume *= mAmbientParam.volume;
    volume *= mActorParam.volume;
    return volume;
}

/**
 * @brief Combines every pitch layer of the sound.
 * @return Final pitch ratio.
 */
f32 BasicSound::CalculatePitch() const {
    return mPitch * mAmbientParam.pitch * mActorParam.pitch;
}

/**
 * @brief Combines every low-pass filter layer of the sound.
 * @return Final low-pass filter offset.
 */
f32 BasicSound::CalculateLpfFrequency() const {
    return mLpfFreq + mAmbientParam.lpf + mSoundPlayer->GetLowPassFilterFrequency() +
           mActorParam.lpf;
}

/**
 * @brief Picks the output lines of the sound.
 * @return Ambient output lines, or the sound's own when the ambient callback set none.
 */
u32 BasicSound::CalculateOutLineFlag() const {
    u32 outputLineFlag = mOutputLineFlag;
    if (mAmbientParam.outputLineFlag != 0xffffffff) {
        outputLineFlag = mAmbientParam.outputLineFlag;
    }

    return outputLineFlag;
}

/**
 * @brief Picks the biquad filter of the first layer that sets one: sound, player, actor, then the
 * ambient callback.
 * @param pType Receives the filter type.
 * @param pValue Receives the filter strength.
 */
void BasicSound::CalculateBiquadFilter(int* pType, f32* pValue) const {
    int type;
    f32 value;
    if (mBiquadFilterType != -1) {
        type = mBiquadFilterType;
        value = mBiquadFilterValue;
    } else if (mSoundPlayer->GetBiquadFilterType() != -1) {
        type = mSoundPlayer->GetBiquadFilterType();
        value = mSoundPlayer->GetBiquadFilterValue();
    } else if (mActorParam.biquadFilterType != -1) {
        type = mActorParam.biquadFilterType;
        value = mActorParam.biquadFilterValue;
    } else {
        type = mAmbientParam.biquadFilterType;
        value = mAmbientParam.biquadFilterValue;
    }

    *pType = type;
    *pValue = value;
}

/**
 * @brief Combines every layer of the output parameters of one device.
 * @param pParam Receives the parameters.
 * @param device Output device.
 */
void BasicSound::CalculateOutputParam(OutputParam* pParam, OutputDevice device) const {
    switch (device) {
    case OutputDevice_Main: {
        f32 actorVolume = mActorParam.tvVolume;
        f32 actorPan = mActorParam.tvPan;
        *pParam = mOutputParam[device];
        ApplyCommonParam(*pParam);

        const SoundPlayer* pPlayer = mSoundPlayer;
        pParam->volume *= pPlayer->GetOutputVolume() * mAmbientParam.tvParam.volume * actorVolume;
        pParam->pan += actorPan + mAmbientParam.tvParam.pan;
        pParam->span += mAmbientParam.tvParam.span;
        pParam->mainSend += pPlayer->GetOutputMainSend();
        for (int i = 0; i < AuxBus_Count; i++) {
            pParam->fxSend[i] += pPlayer->GetOutputEffectSend(static_cast<AuxBus>(i)) +
                                 mAmbientParam.tvParam.additionalOutputParams[i];
        }
        break;
    }
    default:
        NN_UNEXPECTED_DEFAULT;
    }
}

/**
 * @brief Gets the bus mix volumes of one device.
 * @param pVolume Receives the volumes.
 * @param device Output device.
 */
void BasicSound::CalculateOutputBusMixVolume(OutputBusMixVolume* pVolume,
                                             OutputDevice device) const {
    std::memcpy(pVolume, &mOutputAdditionalParam[device]->GetBusMixVolume(),
                sizeof(OutputBusMixVolume));
}

/**
 * @brief Tests whether any bus of the main output uses a volume through mode.
 * @return Whether a through mode is set.
 */
bool BasicSound::IsVolumeThroughModeUsed() const {
    if (mOutputAdditionalParam[OutputDevice_Main] == nullptr) {
        return false;
    }

    if (!mOutputAdditionalParam[OutputDevice_Main]->IsVolumeThroughModeEnabled()) {
        return false;
    }

    for (int i = 0; i < mOutputReceiver->GetBusCount(); i++) {
        VolumeThroughModePacket* pPacket =
            mOutputAdditionalParam[OutputDevice_Main]->GetVolumeThroughModePacketAddr();
        if (pPacket->TryGetVolumeThroughMode(i) != 0) {
            return true;
        }
    }

    return false;
}

/**
 * @brief Adds the device-independent pan, send and mix mode settings to output parameters.
 * @param rParam Parameters to update.
 */
void BasicSound::ApplyCommonParam(OutputParam& rParam) const {
    rParam.pan += mPan;
    rParam.span += mSurroundPan;
    rParam.mixMode = mMixMode;
    rParam.mainSend += mSend[0];
    for (int i = 0; i < AuxBus_Count; i++) {
        rParam.fxSend[i] += mSend[i + 1];
    }
}

/**
 * @brief Associates the heap the sound's data was loaded into.
 * @param pHeap Player heap.
 */
void BasicSound::AttachPlayerHeap(PlayerHeap* pHeap) {
    mPlayerHeap = pHeap;
}

/**
 * @brief Clears the player heap association.
 * @param pHeap Player heap being detached.
 */
void BasicSound::DetachPlayerHeap(PlayerHeap* pHeap) {
    mPlayerHeap = nullptr;
}

/**
 * @brief Associates the owning sound player.
 * @param pPlayer Sound player.
 */
void BasicSound::AttachSoundPlayer(SoundPlayer* pPlayer) {
    mSoundPlayer = pPlayer;
}

/**
 * @brief Clears the sound player association.
 * @param pPlayer Sound player being detached.
 */
void BasicSound::DetachSoundPlayer(SoundPlayer* pPlayer) {
    mSoundPlayer = nullptr;
}

/**
 * @brief Associates the owning sound actor.
 * @param pActor Sound actor.
 */
void BasicSound::AttachSoundActor(SoundActor* pActor) {
    mSoundActor = pActor;
}

/**
 * @brief Clears the sound actor association.
 * @param pActor Sound actor being detached.
 */
void BasicSound::DetachSoundActor(SoundActor* pActor) {
    mSoundActor = nullptr;
}

/**
 * @brief Associates the external player limiting the sound.
 * @param pPlayer External sound player.
 */
void BasicSound::AttachExternalSoundPlayer(ExternalSoundPlayer* pPlayer) {
    mExternalSoundPlayer = pPlayer;
}

/**
 * @brief Clears the external player association.
 * @param pPlayer External sound player being detached.
 */
void BasicSound::DetachExternalSoundPlayer(ExternalSoundPlayer* pPlayer) {
    mExternalSoundPlayer = nullptr;
}

/**
 * @brief Gets the frames left in the stop fade.
 * @return Remaining frames.
 */
int BasicSound::GetRemainingFadeFrames() const {
    return mFadeVolume.GetRemainingCount();
}

/**
 * @brief Gets the frames left in the pause fade.
 * @return Remaining frames.
 */
int BasicSound::GetRemainingPauseFadeFrames() const {
    return mPauseFadeVolume.GetRemainingCount();
}

/**
 * @brief Gets the frames left in the mute fade.
 * @return Remaining frames.
 */
int BasicSound::GetRemainingMuteFadeFrames() const {
    return mMuteFadeVolume.GetRemainingCount();
}

/**
 * @brief Records every parameter layer and the combined result.
 * @param pValues Receives the values.
 */
void BasicSound::CalculateSoundParamCalculationValues(SoundParamCalculationValues* pValues) const {
    pValues->soundArchiveParam.volume = mInitVolume;

    const SoundPlayer* pPlayer = mSoundPlayer;

    pValues->soundPlayerParam.volume = pPlayer->GetVolume();
    pValues->soundPlayerParam.lpf = pPlayer->GetLowPassFilterFrequency();
    pValues->soundPlayerParam.biquadFilterType = pPlayer->GetBiquadFilterType();
    pValues->soundPlayerParam.biquadFilterValue = pPlayer->GetBiquadFilterValue();
    pValues->soundPlayerParam.outputVolume = pPlayer->GetOutputVolume();
    pValues->soundPlayerParam.outputMainSend = pPlayer->GetOutputMainSend();
    for (int i = 0; i < AuxBus_Count; i++) {
        pValues->soundPlayerParam.outputEffectSend[i] =
            pPlayer->GetOutputEffectSend(static_cast<AuxBus>(i));
    }

    pValues->sound3DParam.volume = mAmbientParam.volume;
    pValues->sound3DParam.pitch = mAmbientParam.pitch;
    pValues->sound3DParam.lpf = mAmbientParam.lpf;
    pValues->sound3DParam.biquadFilterType = mAmbientParam.biquadFilterType;
    pValues->sound3DParam.biquadFilterValue = mAmbientParam.biquadFilterValue;
    pValues->sound3DParam.outputLineFlag = mAmbientParam.outputLineFlag;
    pValues->sound3DParam.priority = mAmbientParam.priority;
    pValues->sound3DParam.outputParam.volume = mAmbientParam.tvParam.volume;
    pValues->sound3DParam.outputParam.pan = mAmbientParam.tvParam.pan;
    pValues->sound3DParam.outputParam.span = mAmbientParam.tvParam.span;
    for (int i = 0; i < AuxBus_Count; i++) {
        pValues->sound3DParam.outputParam.additionalOutputParams[i] =
            mAmbientParam.tvParam.additionalOutputParams[i];
    }

    pValues->soundActorParam.volume = mActorParam.volume;
    pValues->soundActorParam.pitch = mActorParam.pitch;
    pValues->soundActorParam.lpf = mActorParam.lpf;
    pValues->soundActorParam.outputVolume = mActorParam.tvVolume;
    pValues->soundActorParam.outputPan = mActorParam.tvPan;

    pValues->soundHandleParam.volume = mExtMoveVolume.GetValue();
    pValues->soundHandleParam.pitch = mPitch;
    pValues->soundHandleParam.lpf = mLpfFreq;
    pValues->soundHandleParam.biquadFilterType = mBiquadFilterType;
    pValues->soundHandleParam.biquadFilterValue = mBiquadFilterValue;
    pValues->soundHandleParam.outputLineFlag = mOutputLineFlag;
    pValues->soundHandleParam.mixMode = mMixMode;
    pValues->soundHandleParam.pan = mPan;
    pValues->soundHandleParam.surroundPan = mSurroundPan;
    pValues->soundHandleParam.mainSend = mSend[0];
    pValues->soundHandleParam.priority = mPlayerPriority;
    for (int i = 0; i < AuxBus_Count; i++) {
        pValues->soundHandleParam.effectSend[i] = mSend[i + 1];
    }

    pValues->soundHandleParam.outputVolume = mOutputParam[OutputDevice_Main].volume;
    pValues->soundHandleParam.outputPan = mOutputParam[OutputDevice_Main].pan;
    pValues->soundHandleParam.outputSurroundPan = mOutputParam[OutputDevice_Main].span;
    pValues->soundHandleParam.outputMainSend = mOutputParam[OutputDevice_Main].mainSend;
    for (int i = 0; i < AuxBus_Count; i++) {
        pValues->soundHandleParam.outputEffectSend[i] = mOutputParam[OutputDevice_Main].fxSend[i];
    }

    for (int i = 0; i < OutputParam::WaveChannelMax; i++) {
        pValues->soundHandleParam.outputMixParameter[i] =
            mOutputParam[OutputDevice_Main].mixParameter[i];
    }

    pValues->resultParam.volume = CalculateVolume();
    pValues->resultParam.pitch = CalculatePitch();
    pValues->resultParam.lpf = CalculateLpfFrequency();
    pValues->resultParam.outputLineFlag = CalculateOutLineFlag();
    pValues->resultParam.priority = GetPlayerPriority();
    CalculateBiquadFilter(&pValues->resultParam.biquadFilterType,
                          &pValues->resultParam.biquadFilterValue);
    for (int i = 0; i < OutputDevice_Count; i++) {
        CalculateOutputParam(&pValues->resultParam.outputParam[i], static_cast<OutputDevice>(i));
    }

    pValues->fadeVolumeParam.stopFadeVolume = mFadeVolume.GetValue();
    pValues->fadeVolumeParam.pauseFadeVolume = mPauseFadeVolume.GetValue();
    pValues->fadeVolumeParam.muteFadeVolume = mMuteFadeVolume.GetValue();
    pValues->fadeVolumeParam.isMuted = mMuteState == MuteState_Muted;
}

/**
 * @brief Sets the volume the sound was started with.
 * @param volume Linear volume; negative values are clamped to zero.
 */
void BasicSound::SetInitialVolume(f32 volume) {
    if (volume < 0.0f) {
        volume = 0.0f;
    }

    mInitVolume = volume;
}

/**
 * @brief Gets the volume the sound was started with.
 * @return Linear volume.
 */
f32 BasicSound::GetInitialVolume() const {
    return mInitVolume;
}

/**
 * @brief Moves the sound's volume to a new value.
 * @param volume Target linear volume; negative values are clamped to zero.
 * @param frames Transition length in frames.
 */
void BasicSound::SetVolume(f32 volume, int frames) {
    if (volume < 0.0f) {
        volume = 0.0f;
    }

    mExtMoveVolume.SetTarget(volume, frames);
}

/**
 * @brief Gets the sound's current volume.
 * @return Linear volume.
 */
f32 BasicSound::GetVolume() const {
    return mExtMoveVolume.GetValue();
}

/**
 * @brief Sets the pitch of the sound.
 * @param pitch Frequency ratio.
 */
void BasicSound::SetPitch(f32 pitch) {
    mPitch = pitch;
}

/**
 * @brief Gets the pitch of the sound.
 * @return Frequency ratio.
 */
f32 BasicSound::GetPitch() const {
    return mPitch;
}

/**
 * @brief Sets the low-pass filter offset of the sound.
 * @param freq Frequency offset.
 */
void BasicSound::SetLpfFreq(f32 freq) {
    mLpfFreq = freq;
}

/**
 * @brief Gets the low-pass filter offset of the sound.
 * @return Frequency offset.
 */
f32 BasicSound::GetLpfFreq() const {
    return mLpfFreq;
}

/**
 * @brief Sets the biquad filter of the sound.
 * @param type Filter type, or -1 to use the player's.
 * @param value Filter strength.
 */
void BasicSound::SetBiquadFilter(int type, f32 value) {
    mBiquadFilterType = type;
    mBiquadFilterValue = value;
}

/**
 * @brief Gets the biquad filter of the sound.
 * @param pType Receives the filter type; may be nullptr.
 * @param pValue Receives the filter strength; may be nullptr.
 */
void BasicSound::GetBiquadFilter(int* pType, f32* pValue) const {
    if (pType != nullptr) {
        *pType = mBiquadFilterType;
    }

    if (pValue != nullptr) {
        *pValue = mBiquadFilterValue;
    }
}

/**
 * @brief Sets the output lines of the sound.
 * @param lineFlag Output line flags.
 */
void BasicSound::SetOutputLine(u32 lineFlag) {
    mOutputLineFlag = lineFlag;
}

/**
 * @brief Gets the output lines of the sound.
 * @return Output line flags.
 */
u32 BasicSound::GetOutputLine() const {
    return mOutputLineFlag;
}

/** @brief Resets the output lines to the player's default. */
void BasicSound::ResetOutputLine() {
    mOutputLineFlag = mSoundPlayer->GetDefaultOutputLine();
}

/**
 * @brief Sets how the sound is mixed to the output channels.
 * @param mode Mix mode.
 */
void BasicSound::SetMixMode(MixMode mode) {
    mMixMode = mode;
}

/**
 * @brief Gets how the sound is mixed to the output channels.
 * @return Mix mode.
 */
MixMode BasicSound::GetMixMode() {
    return mMixMode;
}

/**
 * @brief Sets the pan of the sound.
 * @param pan Pan offset.
 */
void BasicSound::SetPan(f32 pan) {
    mPan = pan;
}

/**
 * @brief Gets the pan of the sound.
 * @return Pan offset.
 */
f32 BasicSound::GetPan() const {
    return mPan;
}

/**
 * @brief Sets the surround pan of the sound.
 * @param pan Surround pan offset.
 */
void BasicSound::SetSurroundPan(f32 pan) {
    mSurroundPan = pan;
}

/**
 * @brief Gets the surround pan of the sound.
 * @return Surround pan offset.
 */
f32 BasicSound::GetSurroundPan() const {
    return mSurroundPan;
}

/**
 * @brief Sets the main send of the sound.
 * @param send Send level offset.
 */
void BasicSound::SetMainSend(f32 send) {
    mSend[0] = send;
}

/**
 * @brief Gets the main send of the sound.
 * @return Send level offset.
 */
f32 BasicSound::GetMainSend() const {
    return mSend[0];
}

/**
 * @brief Sets an aux bus send of the sound.
 * @param bus Aux bus.
 * @param send Send level offset.
 */
void BasicSound::SetFxSend(AuxBus bus, f32 send) {
    mSend[bus + 1] = send;
}

/**
 * @brief Gets an aux bus send of the sound.
 * @param bus Aux bus.
 * @return Send level offset.
 */
f32 BasicSound::GetFxSend(AuxBus bus) const {
    return mSend[bus + 1];
}

/**
 * @brief Sets the send of any bus: the main and aux buses, then the additional buses.
 * @param bus Bus index; the main send is bus 0.
 * @param send Send level.
 */
void BasicSound::SetSend(int bus, f32 send) {
    if (bus < DefaultBusCount) {
        mSend[bus] = send;
    } else {
        // The result is unused; the original only queried it (presumably for an assertion).
        if (mOutputAdditionalParam[OutputDevice_Main] != nullptr) {
            mOutputAdditionalParam[OutputDevice_Main]->IsAdditionalSendEnabled();
        }

        ValueArray<f32>* pSends =
            mOutputAdditionalParam[OutputDevice_Main]->GetAdditionalSendAddr();
        if (mOutputReceiver != nullptr) {
            pSends->SetValue(bus - DefaultBusCount, send);
        }
    }
}

/**
 * @brief Sets the send of an additional bus of one device.
 * @param device Output device.
 * @param bus Bus index, counting the main and aux buses.
 * @param send Send level.
 */
void BasicSound::SetOutputAdditionalSend(OutputDevice device, int bus, f32 send) {
    // The result is unused; the original only queried it (presumably for an assertion).
    if (mOutputAdditionalParam[device] != nullptr) {
        mOutputAdditionalParam[device]->IsAdditionalSendEnabled();
    }

    ValueArray<f32>* pSends = mOutputAdditionalParam[device]->GetAdditionalSendAddr();
    if (mOutputReceiver != nullptr) {
        pSends->SetValue(bus - DefaultBusCount, send);
    }
}

/**
 * @brief Gets the send of any bus.
 * @param bus Bus index; the main send is bus 0.
 * @return Send level.
 */
f32 BasicSound::GetSend(int bus) const {
    if (bus < DefaultBusCount) {
        return mSend[bus];
    }

    return mOutputAdditionalParam[OutputDevice_Main]->GetAdditionalSendAddr()->GetValue(
        bus - DefaultBusCount);
}

/**
 * @brief Gets the send of an additional bus of one device.
 * @param device Output device.
 * @param bus Bus index, counting the main and aux buses.
 * @return Send level.
 */
f32 BasicSound::GetOutputAdditionalSend(OutputDevice device, int bus) const {
    return mOutputAdditionalParam[device]->GetAdditionalSendAddr()->GetValue(bus - DefaultBusCount);
}

/**
 * @brief Sets the volume through mode of a bus of the main output.
 * @param bus Bus index.
 * @param mode Through mode.
 */
void BasicSound::SetVolumeThroughMode(int bus, u8 mode) {
    VolumeThroughModePacket* pPacket =
        mOutputAdditionalParam[OutputDevice_Main]->GetVolumeThroughModePacketAddr();
    if (mOutputReceiver != nullptr) {
        pPacket->SetVolumeThroughMode(bus, mode);
    }
}

/**
 * @brief Sets the volume through mode of a bus of one device.
 * @param device Output device.
 * @param bus Bus index.
 * @param mode Through mode.
 */
void BasicSound::SetOutputVolumeThroughMode(OutputDevice device, int bus, u8 mode) {
    VolumeThroughModePacket* pPacket =
        mOutputAdditionalParam[device]->GetVolumeThroughModePacketAddr();
    if (mOutputReceiver != nullptr) {
        pPacket->SetVolumeThroughMode(bus, mode);
    }
}

/**
 * @brief Gets the volume through mode of a bus of the main output.
 * @param bus Bus index.
 * @return Through mode.
 */
u8 BasicSound::GetVolumeThroughMode(int bus) {
    return mOutputAdditionalParam[OutputDevice_Main]
        ->GetVolumeThroughModePacketAddr()
        ->GetVolumeThroughMode(bus);
}

/**
 * @brief Gets the volume through mode of a bus of one device.
 * @param device Output device.
 * @param bus Bus index.
 * @return Through mode.
 */
u8 BasicSound::GetOutputVolumeThroughMode(OutputDevice device, int bus) const {
    return mOutputAdditionalParam[device]->GetVolumeThroughModePacketAddr()->GetVolumeThroughMode(
        bus);
}

/**
 * @brief Gets the number of buses of the output receiver.
 * @return Bus count, or 0 without a receiver.
 */
int BasicSound::GetSendBusCount() {
    if (mOutputReceiver == nullptr) {
        return 0;
    }

    return mOutputReceiver->GetBusCount();
}

/**
 * @brief Gets the number of channels of the output receiver.
 * @return Channel count, or 0 without a receiver.
 */
int BasicSound::GetSendChannelCount() {
    if (mOutputReceiver == nullptr) {
        return 0;
    }

    return mOutputReceiver->GetChannelCount();
}

/**
 * @brief Sets the additional parameters of one device for the sound and its driver player.
 * @param device Output device.
 * @param pParam Parameters of the sound.
 * @param pPlayerParam Parameters of the driver player.
 */
void BasicSound::SetOutputAdditionalParamAddr(OutputDevice device, OutputAdditionalParam* pParam,
                                              OutputAdditionalParam* pPlayerParam) {
    mOutputAdditionalParam[device] = pParam;
    GetBasicSoundPlayerHandle()->SetTvAdditionalParam(pPlayerParam);
}

/**
 * @brief Sets the volume of one device.
 * @param device Output device.
 * @param volume Linear volume.
 */
void BasicSound::SetOutputVolume(OutputDevice device, f32 volume) {
    mOutputParam[device].volume = volume;
}

/**
 * @brief Sets the bus mix volumes of one source channel of one device.
 * @param device Output device.
 * @param srcChannel Source channel.
 * @param srcBus Bus whose mix buffers receive the volumes.
 * @param volume Volume of every output channel.
 */
void BasicSound::SetOutputBusMixVolume(OutputDevice device, int srcChannel, int srcBus,
                                       ChannelMixVolume volume) {
    if (mOutputReceiver == nullptr) {
        return;
    }

    int volumeChannelCount = volume.GetChannelCount();
    int receiverChannelCount = mOutputReceiver->GetChannelCount();
    int channelCount =
        receiverChannelCount < volumeChannelCount ? receiverChannelCount : volumeChannelCount;

    for (int i = 0; i < channelCount; i++) {
        f32 channelVolume = volume.GetChannelVolume(i);
        mOutputAdditionalParam[device]->SetBusMixVolume(
            srcChannel, Util::GetOutputReceiverMixBufferIndex(mOutputReceiver, srcBus, i),
            channelVolume);
    }
}

/**
 * @brief Sets the mix parameters of one source channel of one device.
 * @param device Output device.
 * @param channel Source channel.
 * @param param Gains to every output channel.
 */
void BasicSound::SetOutputChannelMixParameter(OutputDevice device, u32 channel,
                                              MixParameter param) {
    MixParameter& rMixParameter = mOutputParam[device].mixParameter[channel];
    for (int i = 0; i < ChannelIndex_Count; i++) {
        rMixParameter.ch[i] = param.ch[i];
    }
}

/**
 * @brief Sets the pan of one device.
 * @param device Output device.
 * @param pan Pan offset.
 */
void BasicSound::SetOutputPan(OutputDevice device, f32 pan) {
    mOutputParam[device].pan = pan;
}

/**
 * @brief Sets the surround pan of one device.
 * @param device Output device.
 * @param span Surround pan offset.
 */
void BasicSound::SetOutputSurroundPan(OutputDevice device, f32 span) {
    mOutputParam[device].span = span;
}

/**
 * @brief Sets the main send of one device.
 * @param device Output device.
 * @param send Send level.
 */
void BasicSound::SetOutputMainSend(OutputDevice device, f32 send) {
    mOutputParam[device].mainSend = send;
}

/**
 * @brief Sets an aux bus send of one device.
 * @param device Output device.
 * @param bus Aux bus.
 * @param send Send level.
 */
void BasicSound::SetOutputFxSend(OutputDevice device, AuxBus bus, f32 send) {
    mOutputParam[device].fxSend[static_cast<int>(bus)] = send;
}

/**
 * @brief Enables or disables the bus mix volume of one bus of one device.
 * @param device Output device.
 * @param bus Bus index.
 * @param isEnabled Whether the bus uses its bus mix volume.
 */
void BasicSound::SetOutputBusMixVolumeEnabled(OutputDevice device, int bus, bool isEnabled) {
    if (mOutputReceiver == nullptr) {
        return;
    }

    OutputAdditionalParam* pParam = mOutputAdditionalParam[device];
    pParam->SetBusMixVolumeEnabledForBus(bus, isEnabled);
    pParam->SetBusMixVolumeUsed(false);

    // The original tests the requested bus on every iteration instead of bus i.
    for (int i = 0; i < pParam->GetBusMixVolumePacketAddr()->GetBusCount(); i++) {
        if (pParam->IsBusMixVolumeEnabledForBus(bus)) {
            pParam->SetBusMixVolumeUsed(true);
            return;
        }
    }
}

/**
 * @brief Gets the volume of one device.
 * @param device Output device.
 * @return Linear volume.
 */
f32 BasicSound::GetOutputVolume(OutputDevice device) const {
    return mOutputParam[device].volume;
}

/**
 * @brief Gets the mix parameters of one source channel of one device.
 * @param device Output device.
 * @param channel Source channel.
 * @return Gains to every output channel.
 */
MixParameter BasicSound::GetOutputChannelMixParameter(OutputDevice device, u32 channel) const {
    return mOutputParam[device].mixParameter[channel];
}

/**
 * @brief Gets the pan of one device.
 * @param device Output device.
 * @return Pan offset.
 */
f32 BasicSound::GetOutputPan(OutputDevice device) const {
    return mOutputParam[device].pan;
}

/**
 * @brief Gets the surround pan of one device.
 * @param device Output device.
 * @return Surround pan offset.
 */
f32 BasicSound::GetOutputSurroundPan(OutputDevice device) const {
    return mOutputParam[device].span;
}

/**
 * @brief Gets the main send of one device.
 * @param device Output device.
 * @return Send level.
 */
f32 BasicSound::GetOutputMainSend(OutputDevice device) const {
    return mOutputParam[device].mainSend;
}

/**
 * @brief Gets an aux bus send of one device.
 * @param device Output device.
 * @param bus Aux bus.
 * @return Send level.
 */
f32 BasicSound::GetOutputFxSend(OutputDevice device, AuxBus bus) const {
    return mOutputParam[device].fxSend[static_cast<int>(bus)];
}

/**
 * @brief Tests whether one bus of one device uses its bus mix volume.
 * @param device Output device.
 * @param bus Bus index.
 * @return Whether the bus mix volume is enabled; false without a receiver.
 */
bool BasicSound::IsOutputBusMixVolumeEnabled(OutputDevice device, int bus) const {
    if (mOutputReceiver == nullptr) {
        return false;
    }

    return mOutputAdditionalParam[device]->IsBusMixVolumeEnabledForBus(bus);
}

/**
 * @brief Gets the bus mix volumes of one source channel of one device.
 * @param device Output device.
 * @param srcChannel Source channel.
 * @param srcBus Bus index.
 * @return Volume of every output channel.
 */
ChannelMixVolume BasicSound::GetOutputBusMixVolume(OutputDevice device, int srcChannel,
                                                   int srcBus) const {
    ChannelMixVolume volume;
    if (mOutputReceiver == nullptr) {
        return volume;
    }

    int channelCount = mOutputReceiver->GetChannelCount();
    volume.SetChannelCount(channelCount);
    for (int i = 0; i < channelCount; i++) {
        volume.SetChannelVolume(
            i, mOutputAdditionalParam[device]->GetBusMixVolume(srcChannel, srcBus));
    }

    return volume;
}

/**
 * @brief Sends a new pan mode to the driver player.
 * @param mode Pan mode.
 */
void BasicSound::SetPanMode(PanMode mode) {
    DriverCommand& rCommandManager = DriverCommand::GetInstance();
    auto* pCommand = AllocCommand<DriverCommandPlayerPanParam>(rCommandManager);
    pCommand->type = DriverCommandId_PlayerPanMode;
    pCommand->player = GetBasicSoundPlayerHandle();
    pCommand->panMode = mode;
    rCommandManager.PushCommand(pCommand);
}

/**
 * @brief Sends a new pan curve to the driver player.
 * @param curve Pan curve.
 */
void BasicSound::SetPanCurve(PanCurve curve) {
    DriverCommand& rCommandManager = DriverCommand::GetInstance();
    auto* pCommand = AllocCommand<DriverCommandPlayerPanParam>(rCommandManager);
    pCommand->type = DriverCommandId_PlayerPanCurve;
    pCommand->player = GetBasicSoundPlayerHandle();
    pCommand->panCurve = curve;
    rCommandManager.PushCommand(pCommand);
}

/**
 * @brief Copies the ambient callbacks and a private copy of their argument.
 * @param rAmbientInfo Ambient information; its allocator provides the argument copy.
 */
void BasicSound::SetAmbientInfo(const AmbientInfo& rAmbientInfo) {
    void* pArg = rAmbientInfo.argAllocator->detail_AllocAmbientArg(rAmbientInfo.argSize);
    if (pArg == nullptr) {
        return;
    }

    std::memcpy(pArg, rAmbientInfo.arg, rAmbientInfo.argSize);
    mAmbientInfo = rAmbientInfo;
    mAmbientInfo.arg = pArg;
}

/**
 * @brief Asks the ambient callback for the priority a sound would start with.
 * @param rAmbientInfo Ambient information.
 * @param soundId Sound id.
 * @return Ambient priority, or 0 without a callback.
 */
int BasicSound::GetAmbientPriority(const AmbientInfo& rAmbientInfo, u32 soundId) {
    if (rAmbientInfo.paramUpdateCallback == nullptr) {
        return 0;
    }

    return rAmbientInfo.paramUpdateCallback->detail_GetAmbientPriority(rAmbientInfo.arg, soundId);
}

/**
 * @brief Records when the sound was set up.
 * @param rTick Setup time.
 */
void BasicSound::SetSetupTick(const os::Tick& rTick) {
    m_SetupTick = rTick;
}

/**
 * @brief Records the archive the sound was started from.
 * @param pArchive Sound archive.
 */
void BasicSound::SetSoundArchive(const SoundArchive* pArchive) {
    m_pSoundArchive = pArchive;
}
}  // namespace nn::atk::detail
