#pragma once

#include <attributes.h>
#include <nn/atk/atk_Global.h>
#include <nn/atk/atk_MixParameter.h>
#include <nn/atk/atk_MoveValue.h>
#include <nn/atk/atk_OutputParam.h>
#include <nn/atk/atk_Sound3DEngine.h>
#include <nn/os.h>
#include <nn/util/util_IntrusiveList.h>
#include <nn/types.h>

namespace nn::atk {
class ChannelMixVolume;
class OutputReceiver;
class SoundArchive;
class SoundHandle;
class SoundActor;
class SoundPlayer;
typedef void (*SoundStopCallback)();
struct SoundAmbientParam;

/**
 * @brief Snapshot of every parameter that contributes to a sound's final output, grouped by the
 * layer that supplies it (see BasicSound::CalculateSoundParamCalculationValues).
 */
struct SoundParamCalculationValues {
    /** @brief Parameters from the sound archive. */
    struct SoundArchiveParam {
        f32 volume;
    };

    /** @brief Parameters of the owning sound player. */
    struct SoundPlayerParam {
        f32 volume;
        f32 lpf;
        int biquadFilterType;
        f32 biquadFilterValue;
        f32 outputVolume;
        f32 outputMainSend;
        f32 outputEffectSend[AuxBus_Count];
    };

    /** @brief Parameters from the ambient (3D) callback. */
    struct Sound3DParam {
        f32 volume;
        f32 pitch;
        f32 lpf;
        int biquadFilterType;
        f32 biquadFilterValue;
        u32 outputLineFlag;
        OutputAmbientParam outputParam;
        int priority;
    };

    /** @brief Parameters of the owning sound actor. */
    struct SoundActorParam {
        f32 volume;
        f32 pitch;
        f32 lpf;
        f32 outputVolume;
        f32 outputPan;
    };

    /** @brief Parameters set through the sound handle. */
    struct SoundHandleParam {
        f32 volume;
        f32 pitch;
        f32 lpf;
        int biquadFilterType;
        f32 biquadFilterValue;
        u32 outputLineFlag;
        f32 outputVolume;
        f32 outputPan;
        f32 outputSurroundPan;
        f32 outputMainSend;
        f32 outputEffectSend[AuxBus_Count];
        MixParameter outputMixParameter[OutputParam::WaveChannelMax];
        MixMode mixMode;
        f32 pan;
        f32 surroundPan;
        f32 mainSend;
        f32 effectSend[AuxBus_Count];
        int priority;
    };

    /** @brief Parameters after every layer was combined. */
    struct ResultParam {
        f32 volume;
        f32 pitch;
        f32 lpf;
        int biquadFilterType;
        f32 biquadFilterValue;
        u32 outputLineFlag;
        OutputParam outputParam[OutputDevice_Count];
        int priority;
    };

    /** @brief Current values of the stop, pause and mute fades. */
    struct FadeVolumeParam {
        f32 stopFadeVolume;
        f32 pauseFadeVolume;
        f32 muteFadeVolume;
        bool isMuted;
    };

    SoundArchiveParam soundArchiveParam;
    SoundPlayerParam soundPlayerParam;
    Sound3DParam sound3DParam;
    SoundActorParam soundActorParam;
    SoundHandleParam soundHandleParam;
    ResultParam resultParam;
    FadeVolumeParam fadeVolumeParam;
};
static_assert(sizeof(SoundParamCalculationValues) == 0x170, "SoundParamCalculationValues size");
} // namespace nn::atk
namespace nn::atk::detail {
class ExternalSoundPlayer;
class OutputAdditionalParam;
class PlayerHeap;
struct OutputBusMixVolume;
namespace driver {
class BasicSoundPlayer;
} // namespace driver

struct RuntimeTypeInfo {
    // parent identifies the immediate base type, or null for the root type.
    explicit RuntimeTypeInfo(const RuntimeTypeInfo* parent) : parent(parent) {}
    const RuntimeTypeInfo* parent;
};

/** @brief Parameters a sound actor applies to every sound it owns. */
struct SoundActorParam {
    f32 volume;
    f32 pitch;
    f32 tvVolume;
    f32 tvPan;
    f32 lpf;
    int biquadFilterType;
    f32 biquadFilterValue;
};
static_assert(sizeof(SoundActorParam) == 0x1c, "SoundActorParam size");

class BasicSound {
public:
    /** @brief Life cycle of the sound object. */
    enum State {
        State_Constructed,
        State_Initialized,
        State_Finalized,
        State_Destructed,
    };

    /** @brief Whether the driver player has been started. */
    enum PlayerState {
        PlayerState_Init,
        PlayerState_Play,
        PlayerState_Stop,
    };

    /** @brief Progress of a pause request. */
    enum PauseState {
        PauseState_Normal,
        PauseState_Pausing,
        PauseState_Paused,
        PauseState_Unpausing,
    };

    /** @brief Progress of a mute request. */
    enum MuteState {
        MuteState_Normal,
        MuteState_Muting,
        MuteState_Muted,
        MuteState_Unmuting,
    };

    /** @brief Number of sends stored on the sound itself (main send and the aux buses). */
    static const int DefaultBusCount = AuxBus_Count + 1;

    /**
     * @brief Gets the type information shared by every BasicSound.
     * @return Root type information.
     */
    BasicSound();

    NOINLINE static const RuntimeTypeInfo* GetRuntimeTypeInfoStatic() {
        static const RuntimeTypeInfo s_TypeInfo(nullptr);
        return &s_TypeInfo;
    }

    /**
     * @brief Gets the type information of this sound.
     * @return Type information of the most derived type.
     */
    virtual const RuntimeTypeInfo* GetRuntimeTypeInfo() const { return GetRuntimeTypeInfoStatic(); }

    /** @brief Marks the sound as destroyed. */
    virtual ~BasicSound() { mState = State_Destructed; }

    virtual bool Initialize(OutputReceiver* pOutputReceiver);
    virtual void Finalize();
    virtual bool IsPrepared() const = 0;
    virtual bool IsAttachedTempSpecialHandle() = 0;
    virtual void DetachTempSpecialHandle() = 0;
    virtual driver::BasicSoundPlayer* GetBasicSoundPlayerHandle() = 0;

    /** @brief Notifies the derived sound that the player priority changed. */
    virtual void OnUpdatePlayerPriority() {}

    virtual void UpdateMoveValue();

    /** @brief Lets the derived sound update its parameters before they are sent to the driver. */
    virtual void OnUpdateParam() {}

    class AmbientParamUpdateCallback;
    class AmbientArgUpdateCallback;
    class AmbientArgAllocator;

    /** @brief Callbacks and argument that drive a sound's ambient (3D) parameters. */
    class AmbientInfo {
    public:
        AmbientParamUpdateCallback* paramUpdateCallback;
        AmbientArgUpdateCallback* argUpdateCallback;
        AmbientArgAllocator* argAllocator;
        void* arg;
        size_t argSize;
    };

    class AmbientParamUpdateCallback {
    public:
        /**
         * @brief Destroys the ambient-parameter callback.
         */
        virtual ~AmbientParamUpdateCallback() = default;
        virtual void detail_UpdateAmbientParam(const void* pArg, u32 soundId, SoundAmbientParam* pParam) = 0;
        virtual int detail_GetAmbientPriority(const void* pArg, u32 soundId) = 0;
    };
    class AmbientArgAllocator {
    public:
        /**
         * @brief Destroys the ambient-argument allocator.
         */
        virtual ~AmbientArgAllocator() = default;
        virtual void* detail_AllocAmbientArg(size_t size) = 0;
        virtual void detail_FreeAmbientArg(void* pArg, const BasicSound* pSound) = 0;
    };

    class AmbientArgUpdateCallback {
    public:
        virtual ~AmbientArgUpdateCallback() {}
        virtual void detail_UpdateAmbientArg(void* pArg, const BasicSound* pSound) = 0;
    };

    void AttachExternalSoundPlayer(ExternalSoundPlayer* pPlayer);
    void DetachExternalSoundPlayer(ExternalSoundPlayer* pPlayer);
    void DetachSoundActor(SoundActor* pActor);
    void Pause(bool pause, int fadeFrames, PauseMode mode);
    /**
     * @brief Gets the actor owning this sound.
     * @return Associated actor, or null.
     */
    SoundActor* GetSoundActor() const { return mSoundActor; }
    /**
     * @brief Combines player and ambient priority.
     * @return Priority clamped to [0, 127].
     */
    int GetPlayerPriority() const {
        int priority = mPlayerPriority + mAmbientParam.priority;
        if (priority < 0) {
            priority = 0;
        }
        return priority < 127 ? priority : 127;
    }
    /**
     * @brief Gets the external player's intrusive linkage.
     * @return Node initialized by the sound constructor.
     */
    util::IntrusiveListNode& GetExternalPlayerNode() { return mExternalPlayerNode; }
    /**
     * @brief Gets the owning sound player's sound-list linkage.
     * @return Node initialized by the sound constructor.
     */
    util::IntrusiveListNode& GetSoundPlayerPlayNode() { return mSoundPlayerPlayNode; }
    /**
     * @brief Gets the owning sound player's priority-list linkage.
     * @return Node initialized by the sound constructor.
     */
    util::IntrusiveListNode& GetSoundPlayerPriorityNode() { return mSoundPlayerPriorityNode; }
    void Update();
    void AttachSoundPlayer(SoundPlayer* pPlayer);
    void DetachSoundPlayer(SoundPlayer* pPlayer);
    void Stop(int fadeFrames);
    void Pause(bool flag, int fadeFrames);
    void SetVolume(f32 volume, int frames);
    void SetPitch(f32 pitch);
    void SetPan(f32 pan);
    bool IsAttachedGeneralHandle();
    void DetachGeneralHandle();
    bool IsAttachedTempGeneralHandle();
    void DetachTempGeneralHandle();
    void CalculateSoundParamCalculationValues(SoundParamCalculationValues* pValues) const;
    bool IsPause() const;
    f32 GetVolume() const;
    void SetSurroundPan(f32 pan);
    void SetMainSend(f32 send);
    void SetFxSend(AuxBus bus, f32 send);
    void SetLpfFreq(f32 freq);
    void StartPrepared();
    void FadeIn(int frames);
    void SetOutputLine(u32 lineFlag);
    void SetOutputFxSend(OutputDevice device, AuxBus bus, f32 send);
    void SetBiquadFilter(int type, f32 value);
    void SetMixMode(MixMode mode);
    void SetOutputChannelMixParameter(OutputDevice device, u32 channel, MixParameter param);
    static int GetAmbientPriority(const AmbientInfo& rAmbientInfo, u32 soundId);
    void SetInitialVolume(f32 volume);
    void SetPanMode(PanMode mode);
    void SetPanCurve(PanCurve curve);
    void AttachSoundActor(SoundActor* pActor);
    void SetPlayerPriority(int priority);
    void SetSoundArchive(const SoundArchive* pArchive);
    void SetSetupTick(const os::Tick& rTick);
    void ResetOutputLine();

    void SetPriority(int priority, int ambientPriority);
    void GetPriority(int* pPriority, int* pAmbientPriority) const;
    void ClearIsFinalizedForCannotAllocatedResourceFlag();
    void SetId(u32 id);
    void ForceStop();
    void Mute(bool flag, int fadeFrames);
    void SetAutoStopCounter(int frames);
    bool IsMute() const;
    void UpdateParam();
    f32 CalculateVolume() const;
    f32 CalculatePitch() const;
    f32 CalculateLpfFrequency() const;
    u32 CalculateOutLineFlag() const;
    void CalculateBiquadFilter(int* pType, f32* pValue) const;
    void CalculateOutputParam(OutputParam* pParam, OutputDevice device) const;
    void CalculateOutputBusMixVolume(OutputBusMixVolume* pVolume, OutputDevice device) const;
    bool IsVolumeThroughModeUsed() const;
    void ApplyCommonParam(OutputParam& rParam) const;
    void AttachPlayerHeap(PlayerHeap* pHeap);
    void DetachPlayerHeap(PlayerHeap* pHeap);
    int GetRemainingFadeFrames() const;
    int GetRemainingPauseFadeFrames() const;
    int GetRemainingMuteFadeFrames() const;
    f32 GetInitialVolume() const;
    f32 GetPitch() const;
    f32 GetLpfFreq() const;
    void GetBiquadFilter(int* pType, f32* pValue) const;
    u32 GetOutputLine() const;
    MixMode GetMixMode();
    f32 GetPan() const;
    f32 GetSurroundPan() const;
    f32 GetMainSend() const;
    f32 GetFxSend(AuxBus bus) const;
    void SetSend(int bus, f32 send);
    void SetOutputAdditionalSend(OutputDevice device, int bus, f32 send);
    f32 GetSend(int bus) const;
    f32 GetOutputAdditionalSend(OutputDevice device, int bus) const;
    void SetVolumeThroughMode(int bus, u8 mode);
    void SetOutputVolumeThroughMode(OutputDevice device, int bus, u8 mode);
    u8 GetVolumeThroughMode(int bus);
    u8 GetOutputVolumeThroughMode(OutputDevice device, int bus) const;
    int GetSendBusCount();
    int GetSendChannelCount();
    void SetOutputAdditionalParamAddr(OutputDevice device, OutputAdditionalParam* pParam,
                                      OutputAdditionalParam* pPlayerParam);
    void SetOutputVolume(OutputDevice device, f32 volume);
    void SetOutputBusMixVolume(OutputDevice device, int srcChannel, int srcBus,
                               ChannelMixVolume volume);
    void SetOutputPan(OutputDevice device, f32 pan);
    void SetOutputSurroundPan(OutputDevice device, f32 span);
    void SetOutputMainSend(OutputDevice device, f32 send);
    void SetOutputBusMixVolumeEnabled(OutputDevice device, int bus, bool isEnabled);
    f32 GetOutputVolume(OutputDevice device) const;
    MixParameter GetOutputChannelMixParameter(OutputDevice device, u32 channel) const;
    f32 GetOutputPan(OutputDevice device) const;
    f32 GetOutputSurroundPan(OutputDevice device) const;
    f32 GetOutputMainSend(OutputDevice device) const;
    f32 GetOutputFxSend(OutputDevice device, AuxBus bus) const;
    bool IsOutputBusMixVolumeEnabled(OutputDevice device, int bus) const;
    ChannelMixVolume GetOutputBusMixVolume(OutputDevice device, int srcChannel, int srcBus) const;
    void SetAmbientInfo(const AmbientInfo& rAmbientInfo);

    /** @brief Gets the archive the sound was started from. @return Sound archive, or nullptr. */
    const SoundArchive* GetSoundArchive() const { return m_pSoundArchive; }
    /** @brief Gets when the sound was set up. @return Tick passed to SetSetupTick. */
    const os::Tick& GetSetupTick() const { return m_SetupTick; }
    /**
     * @brief Sets the callback invoked when the sound stops.
     * @param callback Stop callback, or nullptr.
     */
    void SetSoundStopCallback(SoundStopCallback callback) { mSoundStopCallback = callback; }

    u32 GetId() const { return m_Id; }

private:
    /** @brief Restores the neutral ambient parameters. */
    void ResetAmbientParam() {
        mAmbientParam.volume = 1.0f;
        mAmbientParam.pitch = 1.0f;
        mAmbientParam.lpf = 0.0f;
        mAmbientParam.biquadFilterValue = 0.0f;
        mAmbientParam.biquadFilterType = -1;
        mAmbientParam.priority = 0;
        mAmbientParam.outputLineFlag = 0xffffffff;
        mAmbientParam.userData = 0;
        mAmbientParam.tvParam.volume = 1.0f;
        mAmbientParam.tvParam.pan = 0.0f;
        mAmbientParam.tvParam.span = 0.0f;
        for (int i = 0; i < AuxBus_Count; i++) {
            mAmbientParam.tvParam.additionalOutputParams[i] = 0.0f;
        }
    }

    /** @brief Restores the neutral actor parameters. */
    void ResetActorParam() {
        mActorParam.volume = 1.0f;
        mActorParam.pitch = 1.0f;
        mActorParam.tvVolume = 1.0f;
        mActorParam.tvPan = 0.0f;
        mActorParam.lpf = 0.0f;
        mActorParam.biquadFilterType = -1;
        mActorParam.biquadFilterValue = 0.0f;
    }

    /** @brief Ambient parameters as stored on the sound. */
    struct AmbientParam {
        f32 volume;
        f32 pitch;
        f32 lpf;
        f32 biquadFilterValue;
        int biquadFilterType;
        int priority;
        u32 outputLineFlag;
        u32 userData;
        OutputAmbientParam tvParam;
    };

    friend class nn::atk::SoundHandle;
    PlayerHeap* mPlayerHeap;
    SoundHandle* mGeneralHandle;
    SoundHandle* mTempGeneralHandle;
    SoundPlayer* mSoundPlayer;
    SoundActor* mSoundActor;
    ExternalSoundPlayer* mExternalSoundPlayer;
    const SoundArchive* m_pSoundArchive;
    AmbientInfo mAmbientInfo;
    AmbientParam mAmbientParam;
    SoundActorParam mActorParam;
    MoveValue<f32, int> mFadeVolume;
    MoveValue<f32, int> mPauseFadeVolume;
    MoveValue<f32, int> mMuteFadeVolume;
    bool mStartFlag;
    bool mStartedFlag;
    bool mAutoStopFlag;
    bool mFadeOutFlag;
    bool mPlayerAvailableFlag;
    bool mUnPauseFlag;
    PauseMode mPauseMode;
    u8 mPlayerPriority;
    s8 mBiquadFilterType;
    int mState;
    u8 mPlayerState;
    u8 mPauseState;
    u8 mMuteState;
    int mAutoStopCounter;
    u32 mUpdateCounter;
    u32 mPlayingCounter;
    u32 m_Id;
    u32 mInstanceId;
    os::Tick m_SetupTick;
    f32 mInitVolume;
    f32 mPitch;
    f32 mLpfFreq;
    f32 mBiquadFilterValue;
    u32 mOutputLineFlag;
    OutputReceiver* mOutputReceiver;
    MoveValue<f32, int> mExtMoveVolume;
    MixMode mMixMode;
    f32 mPan;
    f32 mSurroundPan;
    f32 mSend[DefaultBusCount];
    OutputParam mOutputParam[OutputDevice_Count];
    OutputAdditionalParam* mOutputAdditionalParam[OutputDevice_Count];
    void* mUserParam;
    size_t mUserParamSize;
    SoundStopCallback mSoundStopCallback;
    util::IntrusiveListNode mSoundPlayerPlayNode;
    util::IntrusiveListNode mSoundPlayerPriorityNode;
    util::IntrusiveListNode mExternalPlayerNode;

    static u32 g_LastInstanceId;
};
static_assert(sizeof(BasicSound) == 0x210);
} // namespace nn::atk::detail
