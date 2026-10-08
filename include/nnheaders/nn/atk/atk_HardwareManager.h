#pragma once

#include <nn/atk/atk_FinalMix.h>
#include <nn/atk/atk_Global.h>
#include <nn/atk/atk_LowLevelVoiceAllocator.h>
#include <nn/atk/atk_MoveValue.h>
#include <nn/atk/atk_SubMix.h>
#include <nn/audio.h>
#include <nn/os.h>
#include <nn/os/os_Mutex.h>
#include <nn/util/util_IntrusiveList.h>
#include <atomic>

namespace nn::atk {
class BiquadFilterCallback;
class DeviceOutRecorder;
class EffectAux;
} // namespace nn::atk

namespace nn::atk::detail::driver {
class HardwareManager : public Util::Singleton<HardwareManager> {
  public:
    class EffectAuxListScopedLock {
      public:
        EffectAuxListScopedLock();
        ~EffectAuxListScopedLock();
    };
    class EffectAuxListForFinalMixScopedLock {
      public:
        EffectAuxListForFinalMixScopedLock();
        ~EffectAuxListForFinalMixScopedLock();
    };
    class EffectAuxListForAdditionalSubMixScopedLock {
      public:
        EffectAuxListForAdditionalSubMixScopedLock();
        ~EffectAuxListForAdditionalSubMixScopedLock();
    };
    class SubMixListScopedLock {
      public:
        SubMixListScopedLock();
        ~SubMixListScopedLock();
    };
    struct HardwareManagerParameter {
        /**
         * @brief Construct the default renderer setup: one preset sub mix, 30 mix channels.
         * The feature switches the sound system always fills are left uninitialized.
         */
        HardwareManagerParameter()
            : rendererSampleRate(48000), userEffectCount(10), voiceCount(96),
              recordingAudioFrameCount(8), subMixCount(1), subMixTotalChannelCount(30),
              enableAdditionalEffectBus(false), enableAdditionalSubMix(false), enableSubMix(true),
              enableCompatibleDownMixSetting(false), _23(false),
              enableUnusedEffectChannelMuting(false), enableAutoEffectBusMute(false), _26(false),
              enableManualRendering(false), enableCustomSubMix(false),
              enableRenderingOverloadAbort(false), _2a(false) {}

        void SetSubMixParameter(bool enableStereoMode, bool enableEffect, bool enableSubMix,
                                bool enableAdditionalEffectBus, bool enableAdditionalSubMix,
                                bool enableCustomSubMix, int customSubMixCount, int customChannelCount);
        u32 rendererSampleRate;
        int userEffectCount;
        int voiceCount;
        int recordingAudioFrameCount;
        int subMixCount;
        int subMixTotalChannelCount;
        bool enableProfiler;
        bool enableAdditionalEffectBus;
        bool enableAdditionalSubMix;
        bool enableEffect;
        bool enableRecordingFinalOutputs;
        bool enableUserCircularBufferSink;
        bool enableSubMix;
        bool enableStereoMode;
        bool enableMemoryPoolAttachCheck;
        bool enableVoiceDrop;
        bool enableCompatibleDownMixSetting;
        bool _23;
        bool enableUnusedEffectChannelMuting;
        bool enableAutoEffectBusMute;
        bool _26;
        bool enableManualRendering;
        bool enableCustomSubMix;
        bool enableRenderingOverloadAbort;
        bool _2a;
    };

    using SubMixList =
        util::IntrusiveList<SubMix,
                            util::IntrusiveListMemberNodeTraits<SubMix, &SubMix::mLinkNode>>;

    /** @brief Hold the audio renderer lock of the singleton for this scope. */
    class AudioRendererLock {
      public:
        /** @brief Acquire the renderer lock. */
        AudioRendererLock() { GetInstance().LockAudioRenderer(); }
        /** @brief Release the renderer lock. */
        ~AudioRendererLock() { GetInstance().UnlockAudioRenderer(); }
    };

    /** @brief Hold the submix list lock of the singleton for this scope (inlined form). */
    class SubMixListLock {
      public:
        /** @brief Acquire the submix list lock. */
        SubMixListLock() { GetInstance().m_SubMixListMutex.Lock(); }
        /** @brief Release the submix list lock. */
        ~SubMixListLock() { GetInstance().m_SubMixListMutex.Unlock(); }
    };

    /** @brief State of a circular buffer sink owned by the manager. */
    enum CircularBufferSinkState {
        CircularBufferSinkState_Invalid,
        CircularBufferSinkState_Started,
        CircularBufferSinkState_Stopped,
    };

    HardwareManager();

    void ResetParameters();
    audio::MemoryPoolState GetMemoryPoolState(audio::MemoryPoolType* pPool);
    void SetupAudioRendererParameter(audio::AudioRendererParameter* pParameter,
                                     const HardwareManagerParameter& rParameter) const;
    size_t GetRequiredMemSize(const HardwareManagerParameter& rParameter) const;
    size_t GetRequiredMemSizeForMemoryPool(int voiceCount) const;
    size_t GetRequiredRecorderWorkBufferSize(const HardwareManagerParameter& rParameter) const;
    size_t GetRequiredCircularBufferSinkWithMemoryPoolBufferSize(
        const HardwareManagerParameter& rParameter) const;
    size_t GetRequiredCircularBufferSinkBufferSize(
        const HardwareManagerParameter& rParameter) const;
    int GetChannelCountMax() const;
    bool RegisterRecorder(DeviceOutRecorder* pRecorder);
    void UnregisterRecorder(DeviceOutRecorder* pRecorder);
    void UpdateRecorder();
    size_t ReadRecordingCircularBufferSink(void* pBuffer, size_t bufferSize);
    audio::CircularBufferSinkType* AllocateRecordingCircularBufferSink();
    void FreeRecordingCircularBufferSink(audio::CircularBufferSinkType* pSink);
    void StartRecordingCircularBufferSink();
    void StopUserCircularBufferSink();
    void StartUserCircularBufferSink(bool isForceStart);
    size_t ReadUserCircularBufferSink(void* pBuffer, size_t bufferSize);
    void AttachMemoryPool(audio::MemoryPoolType* pPool, void* pAddress, size_t size,
                          bool waitAttach);
    Result RequestUpdateAudioRenderer();
    void DetachMemoryPool(audio::MemoryPoolType* pPool, bool waitDetach);
    void ExecuteAudioRendererRendering();
    void WaitAudioRendererEvent();
    int GetDroppedLowLevelVoiceCount() const;
    s64 GetElapsedAudioFrameCount() const;
    size_t GetRequiredPerformanceFramesBufferSize(const HardwareManagerParameter& rParameter) const;
    Result Initialize(void* pRendererBuffer, size_t rendererBufferSize, void* pVoiceBuffer,
                      size_t voiceBufferSize, void* pUserCircularBuffer,
                      size_t userCircularBufferSize, const HardwareManagerParameter& rParameter);
    void SetBiquadFilterCallback(int type, const BiquadFilterCallback* pCallback);
    void SetEndUserOutputMode(OutputMode mode);
    void UpdateEndUserOutputMode();
    void Finalize();
    void Update(int audioFrameCount);
    void UpdateEffect();
    void SuspendAudioRenderer();
    void ResumeAudioRenderer();
    bool TimedWaitAudioRendererEvent(nn::TimeSpan timeout);
    Result SetAudioRendererRenderingTimeLimit(int limitPercent);
    int GetAudioRendererRenderingTimeLimit();
    void PrepareReset();
    bool IsResetReady() const;
    void AddSubMix(SubMix* pSubMix);
    void RemoveSubMix(SubMix* pSubMix);
    SubMix* GetSubMix(int index);
    const SubMix* GetSubMix(int index) const;
    int GetSubMixCount() const;
    int GetChannelCount() const;
    f32 GetOutputVolume() const;
    void SetOutputDeviceFlag(u32 outputLineIndex, u8 flag);
    void SetSrcType(SampleRateConverterType type);
    size_t GetRequiredEffectAuxBufferSize(const EffectAux* pEffect) const;
    void SetAuxBusVolume(AuxBus bus, f32 volume, int fadeFrames, int subMixIndex);
    f32 GetAuxBusVolume(AuxBus bus, int subMixIndex) const;
    void SetMainBusChannelVolumeForAdditionalEffect(f32 volume, int sourceChannel,
                                                    int destinationChannel);
    f32 GetMainBusChannelVolumeForAdditionalEffect(int sourceChannel, int destinationChannel) const;
    void SetAuxBusChannelVolumeForAdditionalEffect(AuxBus bus, f32 volume, int sourceChannel,
                                                   int destinationChannel);
    f32 GetAuxBusChannelVolumeForAdditionalEffect(AuxBus bus, int sourceChannel,
                                                  int destinationChannel) const;
    static void FlushDataCache(void* pAddress, size_t size);
    void LockEffectAuxList();
    void UnlockEffectAuxList();
    void LockEffectAuxListForFinalMix();
    void UnlockEffectAuxListForFinalMix();
    void LockEffectAuxListForAdditionalSubMix();
    void UnlockEffectAuxListForAdditionalSubMix();
    void LockSubMixList();
    void UnlockSubMixList();
    void SetMasterVolume(f32 volume, int fadeFrames);
    void SetOutputMode(OutputMode mode, OutputDevice device);

    /** @brief Access the renderer configuration. @return Mutable configuration owned by this manager. */
    nn::audio::AudioRendererConfig& GetAudioRendererConfig() { return m_Config; }
    /** @brief Access renderer creation settings. @return Read-only renderer parameters. */
    const audio::AudioRendererParameter& GetAudioRendererParameter() const { return m_RendererParameter; }
    /** @brief Read the atomic renderer update counter. @return Completed renderer update count. */
    u64 GetAudioRendererUpdateCount() const { return m_RendererUpdateCount.load(); }
    /** @brief Acquire exclusive access to the audio renderer. */
    void LockAudioRenderer() { m_RendererMutex.Lock(); }
    /** @brief Release exclusive access to the audio renderer. */
    void UnlockAudioRenderer() { m_RendererMutex.Unlock(); }
    /** @brief Evaluate the current master-volume fade. @return Current master gain. */
    f32 GetMasterVolume() const { return m_MasterVolume.GetValue(); }
    /**
     * @brief Read the output mode of an audio device.
     * @param device Valid output device index, less than OutputDevice_Count.
     * @return Mode currently selected for the device.
     */
    OutputMode GetOutputMode(OutputDevice device) const { return m_OutputMode[device]; }
    /**
     * @brief Read the output mode the end user selected for an audio device.
     * @param device Valid output device index, less than OutputDevice_Count.
     * @return Mode the final output is downmixed to.
     */
    OutputMode GetEndUserOutputMode(OutputDevice device) const {
        return m_EndUserOutputMode[device];
    }
    /**
     * @brief Look up the biquad filter callback registered for a filter type.
     * @param type Filter type, less than BiquadFilterCallbackCount.
     * @return Registered callback, or nullptr if none.
     */
    const BiquadFilterCallback* GetBiquadFilterCallback(int type) const {
        return m_BiquadFilterCallbackTable[type];
    }
    /**
     * @brief Read the output devices an output line is routed to.
     * @param outputLineIndex Output line, less than OutputLineCount.
     * @return Bit set of output devices.
     */
    u8 GetOutputDeviceFlag(int outputLineIndex) const { return m_OutputDeviceFlag[outputLineIndex]; }
    /** @brief Checks whether pan curves are computed the legacy way. @return True when enabled. */
    bool IsCompatiblePanCurveEnabled() const { return _a65; }
    /** @brief Checks whether sub mixes are in use. @return True when sub mixes are enabled. */
    bool IsSubMixEnabled() const { return m_IsSubMixEnabled; }
    /** @brief Access the final mix. @return Final mix owned by this manager. */
    FinalMix& GetFinalMix() { return m_FinalMix; }
    /** @brief Access the sub mix reserved for additional effects. @return Additional sub mix. */
    SubMix& GetAdditionalSubMix() { return m_SubMix[2]; }
    /** @brief Checks whether the manager is initialized. @return True after Initialize. */
    bool IsInitialized() const { return m_IsInitialized; }
    /** @brief Read the audio renderer frame length. @return Samples rendered per audio frame. */
    int GetRendererSampleCount() const { return m_RendererParameter.sampleCount; }
    /** @brief Read the user circular buffer size. @return Buffer size, or 0 before Initialize. */
    size_t GetUserCircularBufferSinkBufferSize() const {
        return m_IsInitialized ? m_UserCircularBufferSize : 0;
    }
    /** @brief Read the user circular buffer sink state. @return Current sink state. */
    CircularBufferSinkState GetUserCircularBufferSinkState() const {
        return m_UserCircularBufferSinkState;
    }
    /** @brief Number of aux buses usable with additional effects. @return Bus count. */
    int GetAuxBusCountForAdditionalEffect() const { return AuxBusCountForAdditionalEffect; }

  private:
    static const int BiquadFilterCallbackCount = 128;
    static const int OutputLineCount = 32;
    static const int SubMixCount = 3;
    static const int AuxBusCountForAdditionalEffect = 2;

    bool m_IsInitialized;
    audio::AudioRendererHandle m_RendererHandle;
    nn::audio::AudioRendererConfig m_Config;
    u8 _70[0x90 - 0x18 - sizeof(nn::audio::AudioRendererConfig)];
    os::SystemEvent m_RendererEvent;
    int m_RendererSuspendCount;
    std::atomic<u64> m_RendererUpdateCount;
    void* m_pRendererWorkBuffer;
    void* m_pConfigWorkBuffer;
    OutputMode m_OutputMode[OutputDevice_Count];
    OutputMode m_EndUserOutputMode[OutputDevice_Count];
    SampleRateConverterType m_SrcType;
    MoveValue<float, int> m_MasterVolume;
    MoveValue<float, int> m_VolumeForReset;
    const BiquadFilterCallback* m_BiquadFilterCallbackTable[BiquadFilterCallbackCount];
    u8 m_OutputDeviceFlag[OutputLineCount];
    LowLevelVoiceAllocator m_VoiceAllocator;
    FinalMix m_FinalMix;
    SubMix m_SubMix[SubMixCount];
    SubMixList m_SubMixList;
    os::Mutex m_SubMixListMutex;
    audio::AudioRendererParameter m_RendererParameter;
    audio::DeviceSinkType m_DeviceSink;
    u8 _900[0x950 - 0x900];
    bool m_IsEffectEnabled;
    bool m_IsSubMixEnabled;
    bool m_IsAdditionalEffectBusEnabled;
    bool m_IsAdditionalSubMixEnabled;
    bool m_IsStereoModeEnabled;
    bool m_IsMemoryPoolAttachCheckEnabled;
    bool _956;
    bool m_IsRenderingOverloadAbortEnabled;
    bool _958;
    os::Mutex m_RendererMutex;
    mutable os::Mutex m_UpdateMutex;
    os::Mutex m_EffectAuxListMutex;
    os::Mutex m_EffectAuxListForFinalMixMutex;
    os::Mutex m_EffectAuxListForAdditionalSubMixMutex;
    audio::CircularBufferSinkType m_RecordingCircularBufferSink;
    CircularBufferSinkState m_RecordingCircularBufferSinkState;
    audio::MemoryPoolType m_RecordingMemoryPool;
    void* m_pRecordingBuffer;
    bool m_IsRecordingCircularBufferSinkAllocated;
    void* m_pRecordingReadBuffer;
    size_t m_RecordingBufferSize;
    DeviceOutRecorder* m_pRecorder;
    audio::CircularBufferSinkType m_UserCircularBufferSink;
    audio::MemoryPoolType m_UserMemoryPool;
    void* m_pUserCircularBuffer;
    size_t m_UserCircularBufferSize;
    CircularBufferSinkState m_UserCircularBufferSinkState;
    bool m_IsAutoEffectBusMuteEnabled;
    bool _a65;
    bool m_IsManualRenderingEnabled;
};
} // namespace nn::atk::detail::driver
