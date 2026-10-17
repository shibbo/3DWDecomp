#pragma once

#include <nn/atk/atk_EffectAux.h>
#include <nn/atk/atk_Global.h>
#include <nn/atk/atk_HardwareManager.h>
#include <nn/atk/atk_SoundStartable.h>
#include <nn/audio.h>
#include <nn/time.h>

namespace nn {
class Result;
}  // namespace nn

namespace nn::atk {
namespace detail {
struct SoundInstanceConfig;
class ThreadInfoRecorder;
}  // namespace detail
class AudioRendererPerformanceReader;
class EffectBase;
struct SoundThreadUpdateProfile;
template <typename T>
class AtkProfileReader;
enum WarningId : int;
class IWarningCallbackInfo {};

/** @brief Arguments handed to the user warning callback. */
struct WarningCallbackArg {
    WarningId warningId;
    IWarningCallbackInfo* pInfo;
};

using WarningCallback = void (*)(WarningCallbackArg arg);

class SoundSystem {
public:
    enum FsPriority {
        FsPriority_RealTime,
        FsPriority_Normal,
        FsPriority_Low
    };

    /** @brief State of the user circular buffer sink. */
    enum CircularBufferSinkState {
        CircularBufferSinkState_Invalid,
        CircularBufferSinkState_Started,
        CircularBufferSinkState_Stopped
    };

    using SoundThreadUserCallback = void (*)(uintptr_t arg);

    struct SoundSystemParam {
        SoundSystemParam();

        s32 soundThreadPriority;
        size_t soundThreadStackSize;
        size_t soundThreadCommandBufferSize;
        int soundThreadCommandQueueCount;
        size_t voiceCommandBufferSize;
        s32 taskThreadPriority;
        size_t taskThreadStackSize;
        size_t taskThreadCommandBufferSize;
        int taskThreadCommandQueueCount;
        FsPriority taskThreadFsPriority;
        bool enableNwRenderer;
        u32 nwVoiceSynthesizeBufferCount;
        int rendererSampleRate;
        int effectCount;
        int voiceCountMax;
        int voiceCommandWaveBufferPacketCount;
        bool enableProfiler;
        bool enableDetailSoundThreadProfile;
        bool enableRecordingFinalOutputs;
        bool enableCircularBufferSink;
        int recordingAudioFrameCount;
        int soundThreadCoreNumber;
        int taskThreadCoreNumber;
        bool enableAdditionalEffectBus;
        bool enableAdditionalSubMix;
        bool enableTaskThread;
        bool enableSoundThread;
        bool enableMemoryPoolManagement;
        bool enableCircularBufferSinkBufferManagement;
        bool enableEffect;
        bool enableSubMix;
        bool enableStereoMode;
        bool enableVoiceDrop;
        bool enableCompatibleDownMixSetting;
        bool enableCompatibleLowPassFilter;
        bool enableUnusedEffectChannelMuting;
        bool enableCompatibleBusVolume;
        bool enableCompatiblePanCurve;
        bool enableUserThreadRendering;
        bool enableCustomSubMix;
        int subMixCount;
        int subMixTotalChannelCount;
        int mixBufferCount;
        int busCountMax;
        bool enableMemoryPoolAttachCheck;
        bool enableBusMixVolume;
        bool enableVolumeThroughMode;
        bool enableHighQualityVoiceResampler;
    };
    static_assert(sizeof(SoundSystemParam) == 0x98);

    /** @brief Memory regions the sound system is initialized with. */
    struct InitializeBufferSet {
        uintptr_t workMem;
        size_t workMemSize;
        uintptr_t memoryPoolMem;
        size_t memoryPoolMemSize;
        uintptr_t circularBufferSinkMem;
        size_t circularBufferSinkMemSize;
    };
    static_assert(sizeof(InitializeBufferSet) == 0x30);

    static size_t GetRequiredMemSize(const SoundSystemParam& rParam);
    static detail::SoundInstanceConfig GetSoundInstanceConfig(const SoundSystemParam& rParam);
    static size_t GetRequiredMemSizeForCircularBufferSink(const SoundSystemParam& rParam);
    static size_t GetRequiredMemSizeForMemoryPool(const SoundSystemParam& rParam);
    static bool detail_InitializeSoundSystem(Result* pResult, const SoundSystemParam& rParam,
                                             InitializeBufferSet& rBufferSet);
    static void detail_InitializeDriverCommandManager(const SoundSystemParam& rParam,
                                                      uintptr_t workMem, size_t workMemSize,
                                                      uintptr_t memoryPoolMem,
                                                      size_t memoryPoolMemSize);
    static bool Initialize(const SoundSystemParam& rParam, uintptr_t workMem, size_t workMemSize);
    static bool Initialize(Result* pResult, const SoundSystemParam& rParam, uintptr_t workMem,
                           size_t workMemSize);
    static void SetupInitializeBufferSet(InitializeBufferSet* pOutBufferSet,
                                         const SoundSystemParam& rParam,
                                         const InitializeBufferSet& rBufferSet);
    static bool Initialize(const SoundSystemParam& rParam, uintptr_t workMem, size_t workMemSize,
                           uintptr_t memoryPoolMem, size_t memoryPoolMemSize);
    static bool Initialize(Result* pResult, const SoundSystemParam& rParam, uintptr_t workMem,
                           size_t workMemSize, uintptr_t memoryPoolMem, size_t memoryPoolMemSize);
    static bool Initialize(const SoundSystemParam& rParam, InitializeBufferSet& rBufferSet);
    static bool Initialize(Result* pResult, const SoundSystemParam& rParam,
                           InitializeBufferSet& rBufferSet);
    static void Finalize();
    static void SetSoundThreadBeginUserCallback(SoundThreadUserCallback callback, uintptr_t arg);
    static void ClearSoundThreadBeginUserCallback();
    static void SetSoundThreadEndUserCallback(SoundThreadUserCallback callback, uintptr_t arg);
    static void ClearSoundThreadEndUserCallback();
    static bool IsInitialized();
    static void SuspendAudioRenderer(TimeSpan fadeTimes);
    static void ResumeAudioRenderer(TimeSpan fadeTimes);
    static void ExecuteRendering();
    static int GetAudioRendererRenderingTimeLimit();
    static void SetAudioRendererRenderingTimeLimit(int limitPercent);
    static void AttachMemoryPool(audio::MemoryPoolType* pPool, void* pMemory, size_t size);
    static void DetachMemoryPool(audio::MemoryPoolType* pPool);
    static void DumpMemory();
    static size_t GetAudioRendererBufferSize();
    static void SetupHardwareManagerParameterFromCurrentSetting(
        detail::driver::HardwareManager::HardwareManagerParameter* pParameter);
    static size_t GetRecorderBufferSize();
    static size_t GetUserCircularBufferSinkBufferSize();
    static size_t GetLowLevelVoiceAllocatorBufferSize();
    static size_t GetMultiVoiceManagerBufferSize();
    static detail::SoundInstanceConfig GetSoundInstanceConfig();
    static size_t GetChannelManagerBufferSize();
    static size_t GetSoundThreadCommandTotalBufferSize();
    static size_t GetTaskThreadCommandTotalBufferSize();
    static size_t GetDriverCommandBufferSize();
    static size_t GetAllocatableDriverCommandSize();
    static size_t GetAllocatedDriverCommandBufferSize();
    static int GetAllocatedDriverCommandCount();
    static void RegisterAudioRendererPerformanceReader(AudioRendererPerformanceReader& rReader);
    static bool AppendEffect(AuxBus bus, EffectBase* pEffect, void* pBuffer, size_t bufferSize);
    static bool AppendEffect(AuxBus bus, EffectBase* pEffect, void* pBuffer, size_t bufferSize,
                             OutputDevice device);
    static bool AppendEffect(AuxBus bus, EffectBase* pEffect, void* pBuffer, size_t bufferSize,
                             OutputDevice device, int subMixIndex);
    static bool AppendEffect(AuxBus bus, EffectAux* pEffect, void* pBuffer, size_t bufferSize);
    static bool AppendEffect(AuxBus bus, EffectAux* pEffect, void* pBuffer, size_t bufferSize,
                             OutputDevice device);
    static bool AppendEffect(AuxBus bus, EffectAux* pEffect, void* pBuffer, size_t bufferSize,
                             OutputDevice device, int subMixIndex);
    static bool AppendEffectToFinalMix(EffectAux* pEffect, void* pBuffer, size_t bufferSize);
    static bool AppendEffectToAdditionalSubMix(EffectAux* pEffect, void* pBuffer,
                                               size_t bufferSize);
    static size_t GetRequiredEffectAuxBufferSize(const EffectAux* pEffect);
    static bool RemoveEffect(AuxBus bus, EffectBase* pEffect);
    static bool RemoveEffect(AuxBus bus, EffectBase* pEffect, OutputDevice device);
    static bool RemoveEffect(AuxBus bus, EffectBase* pEffect, OutputDevice device,
                             int subMixIndex);
    static bool RemoveEffect(AuxBus bus, EffectAux* pEffect);
    static bool RemoveEffect(AuxBus bus, EffectAux* pEffect, OutputDevice device);
    static bool RemoveEffect(AuxBus bus, EffectAux* pEffect, OutputDevice device,
                             int subMixIndex);
    static bool RemoveEffectFromFinalMix(EffectAux* pEffect);
    static bool RemoveEffectFromAdditionalSubMix(EffectAux* pEffect);
    static void ClearEffect(AuxBus bus);
    static void ClearEffect(AuxBus bus, OutputDevice device);
    static void ClearEffect(AuxBus bus, OutputDevice device, int subMixIndex);
    static void ClearEffectFromFinalMix();
    static void ClearEffectFromAdditionalSubMix();
    static bool IsClearEffectFinished(AuxBus bus);
    static bool IsClearEffectFinished(AuxBus bus, OutputDevice device);
    static bool IsClearEffectFinished(AuxBus bus, OutputDevice device, int subMixIndex);
    static bool IsClearEffectFromFinalMixFinished();
    static bool IsClearEffectFromAdditionalSubMixFinished();
    static void SetAuxBusVolume(AuxBus bus, f32 volume, TimeSpan fadeTimes);
    static void SetAuxBusVolume(AuxBus bus, f32 volume, TimeSpan fadeTimes, int subMixIndex);
    static f32 GetAuxBusVolume(AuxBus bus);
    static f32 GetAuxBusVolume(AuxBus bus, int subMixIndex);
    static void SetMainBusChannelVolumeForAdditionalEffect(f32 volume, int srcChannel,
                                                           int dstChannel);
    static f32 GetMainBusChannelVolumeForAdditionalEffect(int srcChannel, int dstChannel);
    static void SetAuxBusChannelVolumeForAdditionalEffect(AuxBus bus, f32 volume, int srcChannel,
                                                          int dstChannel);
    static f32 GetAuxBusChannelVolumeForAdditionalEffect(AuxBus bus, int srcChannel,
                                                         int dstChannel);
    static void SetAllAuxBusChannelVolumeForAdditionalEffect(f32 volume, int srcChannel,
                                                             int dstChannel);
    static void SetAllBusChannelVolumeForAdditionalEffect(f32 volume, int srcChannel,
                                                          int dstChannel);
    static void VoiceCommandProcess(UpdateType updateType, u32 count);
    static void VoiceCommandProcess(u32 count);
    static void VoiceCommandUpdate();
    static size_t GetPerformanceFrameBufferSize();
    static int GetDroppedLowLevelVoiceCount();
    static void SetWarningCallback(WarningCallback callback);
    static void ClearWarningCallback();
    static void CallWarningCallback(WarningId id, IWarningCallbackInfo* pInfo);
    static void RegisterSoundThreadUpdateProfileReader(
        AtkProfileReader<SoundThreadUpdateProfile>& rReader);
    static void UnregisterSoundThreadUpdateProfileReader(
        AtkProfileReader<SoundThreadUpdateProfile>& rReader);
    static void RegisterSoundThreadInfoRecorder(detail::ThreadInfoRecorder& rRecorder);
    static void UnregisterSoundThreadInfoRecorder(detail::ThreadInfoRecorder& rRecorder);
    static size_t ReadCircularBufferSink(void* pBuffer, size_t bufferSize);
    static size_t GetCircularBufferSinkBufferSize();
    static int GetRendererSampleCount();
    static int GetRendererChannelCountMax();
    static void StopCircularBufferSink();
    static void StartCircularBufferSink();
    static CircularBufferSinkState GetCircularBufferSinkState();

    static bool g_IsStreamLoadWait;
    static bool g_IsStreamOpenFailureHalt;

private:
    static bool g_IsInitialized;
    static bool g_IsInitializedDriverCommandManager;
    static bool g_IsTaskThreadEnabled;
    static bool g_IsSoundThreadEnabled;
    static bool g_IsManagingMemoryPool;
    static bool g_IsProfilerEnabled;
    static bool g_IsDetailSoundThreadProfilerEnabled;
    static bool g_IsAdditionalEffectBusEnabled;
    static bool g_IsAdditionalSubMixEnabled;
    static bool g_IsEffectEnabled;
    static bool g_IsRecordingEnabled;
    static bool g_IsCircularBufferSinkEnabled;
    static bool g_IsCircularBufferSinkWarningDisplayed;
    static bool g_IsSubMixEnabled;
    static bool g_IsPresetSubMixEnabled;
    static bool g_IsStereoModeEnabled;
    static bool g_IsVoiceDropEnabled;
    static bool g_IsPreviousSdkVersionLowPassFilterCompatible;
    static bool g_IsUnusedEffectChannelMutingEnabled;
    static bool g_IsUserThreadRenderingEnabled;
    static bool g_IsCustomSubMixEnabled;
    static bool g_IsMemoryPoolAttachCheckEnabled;
    static bool g_IsBusMixVolumeEnabled;
    static bool g_IsVolumeThroughModeEnabled;
    static bool g_IsHighQualityVoiceResamplerEnabled;
    static int g_RendererSampleRate;
    static int g_UserEffectCount;
    static int g_VoiceCountMax;
    static int g_CustomSubMixSubMixCount;
    static int g_CustomSubMixMixBufferCount;
    static int g_BusCountMax;
    static uintptr_t g_SoundThreadStackPtr;
    static size_t g_SoundThreadStackSize;
    static size_t g_SoundThreadCommandBufferSize;
    static int g_SoundThreadCommandQueueCount;
    static uintptr_t g_LoadThreadStackPtr;
    static size_t g_LoadThreadStackSize;
    static size_t g_TaskThreadCommandBufferSize;
    static int g_TaskThreadCommandQueueCount;
    static FsPriority g_TaskThreadFsPriority;
    static uintptr_t g_PerformanceFrameBuffer;
    static size_t g_PerformanceFrameBufferSize;
    static audio::MemoryPoolType g_MemoryPoolForSoundSystem;
    static WarningCallback g_WarningCallback;
};
}  // namespace nn::atk
