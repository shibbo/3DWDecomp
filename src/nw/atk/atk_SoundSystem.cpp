#include <nn/atk/atk_SoundSystem.h>

#include <nn/atk/atk_AudioRendererPerformanceReader.h>
#include <nn/atk/atk_ChannelManager.h>
#include <nn/atk/atk_CurveLfo.h>
#include <nn/atk/atk_DriverCommand.h>
#include <nn/atk/atk_MultiVoiceManager.h>
#include <nn/atk/atk_OutputAdditionalParam.h>
#include <nn/atk/atk_SoundThread.h>
#include <nn/atk/atk_TaskManager.h>
#include <nn/atk/atk_TaskThread.h>
#include <nn/atk/atk_VoiceCommand.h>
#include <nn/atk/detail/dsp/atk_HardwareManager.h>
#include <nn/atk/detail/seq/atk_SequenceSoundPlayer.h>
#include <nn/diag.h>
#include <nn/util.h>

// Release-build form of the SDK result abort macro: the message strings are compiled out.
#define NN_ABORT_UNLESS_RESULT_SUCCESS(expression)                                                 \
    do {                                                                                           \
        ::nn::Result abortResult = (expression);                                                   \
        if (abortResult.IsFailure()) {                                                             \
            ::nn::diag::detail::AbortImpl("", "", "", 0, &abortResult, "");                        \
            __builtin_unreachable();                                                               \
        }                                                                                          \
    } while (false)

namespace nn::atk {
namespace {
using HardwareManager = detail::driver::HardwareManager;
using HardwareManagerParameter = HardwareManager::HardwareManagerParameter;

/** Alignment of the thread stacks and of the memory pool carved from the work memory. */
const size_t MemoryAlignment = 4096;
/** Alignment of the audio renderer performance frame buffer. */
const size_t PerformanceFrameBufferAlignment = 64;
/** Number of performance frames kept per audio renderer frame. */
const size_t PerformanceFrameCount = 3;
/** Upper bound of SoundSystemParam::busCountMax. */
const int BusCountMaxLimit = 24;
/** Length of one audio frame in milliseconds. */
const int AudioFrameMilliSeconds = 5;

/**
 * @brief Round an address or a size up to a power of two.
 * @param value Value to round.
 * @param alignment Power-of-two alignment.
 * @return Smallest multiple of the alignment not below the value.
 */
inline uintptr_t AlignUp(uintptr_t value, size_t alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
}

/**
 * @brief Fill a hardware manager parameter from the user's sound system parameter.
 * @param pParameter Parameter to fill.
 * @param rParam Sound system parameter it is built from.
 */
inline void SetupHardwareManagerParameter(HardwareManagerParameter* pParameter,
                                          const SoundSystem::SoundSystemParam& rParam) {
    // Some HardwareManagerParameter field names are provisional: the byte at the
    // enableMemoryPoolAttachCheck offset receives enableSoundThread, and the one at the
    // enableRenderingOverloadAbort offset receives enableMemoryPoolAttachCheck.
    pParameter->rendererSampleRate = rParam.rendererSampleRate;
    pParameter->userEffectCount = rParam.effectCount;
    pParameter->enableProfiler = rParam.enableProfiler;
    pParameter->voiceCount = rParam.voiceCountMax;
    pParameter->enableEffect = rParam.enableEffect;
    pParameter->enableStereoMode = rParam.enableStereoMode;
    pParameter->enableRecordingFinalOutputs = rParam.enableRecordingFinalOutputs;
    pParameter->recordingAudioFrameCount = rParam.recordingAudioFrameCount;
    pParameter->enableUserCircularBufferSink = rParam.enableCircularBufferSink;
    pParameter->enableMemoryPoolAttachCheck = rParam.enableSoundThread;
    pParameter->enableVoiceDrop = rParam.enableVoiceDrop;
    pParameter->enableCompatibleDownMixSetting = rParam.enableCompatibleDownMixSetting;
    pParameter->_23 = rParam.enableCompatibleLowPassFilter;
    pParameter->enableUnusedEffectChannelMuting = rParam.enableUnusedEffectChannelMuting;
    pParameter->enableAutoEffectBusMute = rParam.enableCompatibleBusVolume;
    pParameter->_26 = rParam.enableCompatiblePanCurve;
    pParameter->enableManualRendering = rParam.enableUserThreadRendering;
    pParameter->enableRenderingOverloadAbort = rParam.enableMemoryPoolAttachCheck;
    pParameter->_2a = rParam.enableHighQualityVoiceResampler;

    int mixBufferCount =
        rParam.mixBufferCount == -1 ? rParam.subMixTotalChannelCount : rParam.mixBufferCount;
    pParameter->SetSubMixParameter(rParam.enableStereoMode, rParam.enableEffect,
                                   rParam.enableSubMix, rParam.enableAdditionalEffectBus,
                                   rParam.enableAdditionalSubMix, rParam.enableCustomSubMix,
                                   rParam.subMixCount, mixBufferCount);
}
}  // namespace

bool SoundSystem::g_IsStreamLoadWait = false;
bool SoundSystem::g_IsStreamOpenFailureHalt = false;
bool SoundSystem::g_IsInitialized = false;
bool SoundSystem::g_IsInitializedDriverCommandManager = false;
bool SoundSystem::g_IsTaskThreadEnabled = true;
bool SoundSystem::g_IsSoundThreadEnabled = true;
bool SoundSystem::g_IsManagingMemoryPool = true;
bool SoundSystem::g_IsProfilerEnabled = false;
bool SoundSystem::g_IsDetailSoundThreadProfilerEnabled = false;
bool SoundSystem::g_IsAdditionalEffectBusEnabled = false;
bool SoundSystem::g_IsAdditionalSubMixEnabled = false;
bool SoundSystem::g_IsEffectEnabled = true;
bool SoundSystem::g_IsRecordingEnabled = false;
bool SoundSystem::g_IsCircularBufferSinkEnabled = false;
bool SoundSystem::g_IsCircularBufferSinkWarningDisplayed = false;
bool SoundSystem::g_IsSubMixEnabled = true;
bool SoundSystem::g_IsPresetSubMixEnabled = true;
bool SoundSystem::g_IsStereoModeEnabled = false;
bool SoundSystem::g_IsVoiceDropEnabled = true;
bool SoundSystem::g_IsPreviousSdkVersionLowPassFilterCompatible = false;
bool SoundSystem::g_IsUnusedEffectChannelMutingEnabled = false;
bool SoundSystem::g_IsUserThreadRenderingEnabled = false;
bool SoundSystem::g_IsCustomSubMixEnabled = false;
bool SoundSystem::g_IsMemoryPoolAttachCheckEnabled = false;
bool SoundSystem::g_IsBusMixVolumeEnabled = false;
bool SoundSystem::g_IsVolumeThroughModeEnabled = false;
bool SoundSystem::g_IsHighQualityVoiceResamplerEnabled = false;
int SoundSystem::g_RendererSampleRate = 48000;
int SoundSystem::g_UserEffectCount = 10;
int SoundSystem::g_VoiceCountMax = 96;
int SoundSystem::g_CustomSubMixSubMixCount = 0;
int SoundSystem::g_CustomSubMixMixBufferCount = 0;
int SoundSystem::g_BusCountMax = 4;
uintptr_t SoundSystem::g_SoundThreadStackPtr = 0;
size_t SoundSystem::g_SoundThreadStackSize = 0;
size_t SoundSystem::g_SoundThreadCommandBufferSize = 0;
int SoundSystem::g_SoundThreadCommandQueueCount = 0;
uintptr_t SoundSystem::g_LoadThreadStackPtr = 0;
size_t SoundSystem::g_LoadThreadStackSize = 0;
size_t SoundSystem::g_TaskThreadCommandBufferSize = 0;
int SoundSystem::g_TaskThreadCommandQueueCount = 0;
SoundSystem::FsPriority SoundSystem::g_TaskThreadFsPriority = FsPriority_Normal;
uintptr_t SoundSystem::g_PerformanceFrameBuffer = 0;
size_t SoundSystem::g_PerformanceFrameBufferSize = 0;
audio::MemoryPoolType SoundSystem::g_MemoryPoolForSoundSystem;
WarningCallback SoundSystem::g_WarningCallback = nullptr;

/** @brief Construct the default parameter: sound and task threads, effects and one sub mix. */
SoundSystem::SoundSystemParam::SoundSystemParam()
    : soundThreadPriority(4), soundThreadStackSize(0x4000), soundThreadCommandBufferSize(0x20000),
      soundThreadCommandQueueCount(32), taskThreadPriority(3), taskThreadStackSize(0x4000),
      taskThreadCommandBufferSize(0x2000), taskThreadCommandQueueCount(32),
      taskThreadFsPriority(FsPriority_Normal), rendererSampleRate(48000), effectCount(10),
      voiceCountMax(96), voiceCommandWaveBufferPacketCount(512), enableProfiler(false),
      enableDetailSoundThreadProfile(false), enableRecordingFinalOutputs(false),
      enableCircularBufferSink(false), recordingAudioFrameCount(8), soundThreadCoreNumber(0),
      taskThreadCoreNumber(0), enableAdditionalEffectBus(false), enableAdditionalSubMix(false),
      enableTaskThread(true), enableSoundThread(true), enableMemoryPoolManagement(true),
      enableCircularBufferSinkBufferManagement(true), enableEffect(true), enableSubMix(true),
      enableStereoMode(false), enableVoiceDrop(true), enableCompatibleDownMixSetting(false),
      enableCompatibleLowPassFilter(false), enableUnusedEffectChannelMuting(false),
      enableCompatibleBusVolume(false), enableCompatiblePanCurve(false),
      enableUserThreadRendering(false), enableCustomSubMix(false), subMixCount(0),
      subMixTotalChannelCount(0), mixBufferCount(-1), busCountMax(4),
      enableMemoryPoolAttachCheck(false), enableBusMixVolume(false),
      enableVolumeThroughMode(false), enableHighQualityVoiceResampler(false) {}

/**
 * @brief Compute the work memory needed to initialize the sound system.
 * @param rParam Parameter the sound system will be initialized with.
 * @return Required work memory size in bytes.
 */
size_t SoundSystem::GetRequiredMemSize(const SoundSystemParam& rParam) {
    detail::SoundInstanceConfig config = GetSoundInstanceConfig(rParam);

    size_t memSize = rParam.taskThreadStackSize;
    memSize += detail::driver::MultiVoiceManager::GetInstance().GetRequiredMemSize(
        rParam.voiceCountMax, config);
    memSize += rParam.soundThreadStackSize;

    {
        HardwareManagerParameter hardwareManagerParameter;
        SetupHardwareManagerParameter(&hardwareManagerParameter, rParam);

        if (rParam.enableProfiler) {
            memSize += HardwareManager::GetInstance().GetRequiredPerformanceFramesBufferSize(
                           hardwareManagerParameter) *
                           PerformanceFrameCount +
                       PerformanceFrameBufferAlignment;
        }

        memSize += MemoryAlignment;

        if (rParam.enableRecordingFinalOutputs) {
            memSize += HardwareManager::GetInstance().GetRequiredRecorderWorkBufferSize(
                hardwareManagerParameter);
        }

        memSize += HardwareManager::GetInstance().GetRequiredMemSize(hardwareManagerParameter);
    }

    memSize += detail::DriverCommand::GetInstance().GetRequiredMemSize(
        rParam.soundThreadCommandBufferSize, rParam.soundThreadCommandQueueCount);
    memSize += detail::DriverCommand::GetInstanceForTaskThread().GetRequiredMemSize(
        rParam.taskThreadCommandBufferSize, rParam.taskThreadCommandQueueCount);
    memSize += MemoryAlignment;

    if (rParam.enableCircularBufferSink && rParam.enableCircularBufferSinkBufferManagement) {
        memSize += GetRequiredMemSizeForCircularBufferSink(rParam);
    }

    if (rParam.enableMemoryPoolManagement) {
        memSize += AlignUp(GetRequiredMemSizeForMemoryPool(rParam), MemoryAlignment);
        memSize += MemoryAlignment;
    }

    return memSize;
}

/**
 * @brief Build the per-sound instance configuration a parameter selects.
 * @param rParam Sound system parameter.
 * @return Configuration with the bus count clamped to [1, 24].
 */
detail::SoundInstanceConfig SoundSystem::GetSoundInstanceConfig(const SoundSystemParam& rParam) {
    detail::SoundInstanceConfig config;
    config.enableBusMixVolume = rParam.enableBusMixVolume;
    config.enableVolumeThroughMode = rParam.enableVolumeThroughMode;

    int busCount = rParam.busCountMax > 1 ? rParam.busCountMax : 1;
    config.busCount = busCount < BusCountMaxLimit ? busCount : BusCountMaxLimit;
    return config;
}

/**
 * @brief Compute the memory needed by the user circular buffer sink.
 * @param rParam Parameter the sound system will be initialized with.
 * @return Required buffer size including its memory pool.
 */
size_t SoundSystem::GetRequiredMemSizeForCircularBufferSink(const SoundSystemParam& rParam) {
    HardwareManagerParameter hardwareManagerParameter;
    SetupHardwareManagerParameter(&hardwareManagerParameter, rParam);

    return HardwareManager::GetInstance().GetRequiredCircularBufferSinkWithMemoryPoolBufferSize(
        hardwareManagerParameter);
}

/**
 * @brief Compute the memory that must live in the sound system's memory pool.
 * @param rParam Parameter the sound system will be initialized with.
 * @return Required memory pool size in bytes.
 */
size_t SoundSystem::GetRequiredMemSizeForMemoryPool(const SoundSystemParam& rParam) {
    detail::SoundInstanceConfig config = GetSoundInstanceConfig(rParam);
    size_t channelManagerSize = detail::driver::ChannelManager::GetInstance().GetRequiredMemSize(
        rParam.voiceCountMax * 2, config);

    return HardwareManager::GetInstance().GetRequiredMemSizeForMemoryPool(rParam.voiceCountMax) +
           channelManagerSize;
}

/**
 * @brief Initialize every sound system component from prepared buffers.
 * @param pResult Receives the hardware initialization result; may be null.
 * @param rParam Sound system parameter.
 * @param rBufferSet Work memory, memory pool and circular buffer sink regions.
 * @return Whether the sound system is initialized.
 */
bool SoundSystem::detail_InitializeSoundSystem(Result* pResult, const SoundSystemParam& rParam,
                                               InitializeBufferSet& rBufferSet) {
    util::ReferSymbol("SDK MW+Nintendo+NintendoWare_Atk-10_4_0-Release");

    detail::SoundInstanceConfig config = GetSoundInstanceConfig(rParam);

    if (g_IsInitialized) {
        if (pResult != nullptr) {
            *pResult = ResultSuccess();
        }

        return true;
    }

    uintptr_t workMem = rBufferSet.workMem;
    uintptr_t memoryPoolMem = rBufferSet.memoryPoolMem;
    uintptr_t circularBufferSinkMem = rBufferSet.circularBufferSinkMem;

    if (!g_IsInitializedDriverCommandManager) {
        size_t soundThreadCommandSize = detail::DriverCommand::GetInstance().GetRequiredMemSize(
            rParam.soundThreadCommandBufferSize, rParam.soundThreadCommandQueueCount);
        size_t taskThreadCommandSize =
            detail::DriverCommand::GetInstanceForTaskThread().GetRequiredMemSize(
                rParam.taskThreadCommandBufferSize, rParam.taskThreadCommandQueueCount);
        detail_InitializeDriverCommandManager(rParam, workMem, soundThreadCommandSize,
                                              workMem + soundThreadCommandSize,
                                              taskThreadCommandSize);
        workMem += soundThreadCommandSize + taskThreadCommandSize;
    }

    HardwareManagerParameter hardwareManagerParameter;
    SetupHardwareManagerParameter(&hardwareManagerParameter, rParam);

    workMem = AlignUp(workMem, MemoryAlignment);

    g_VoiceCountMax = rParam.voiceCountMax;

    size_t hardwareManagerSize =
        HardwareManager::GetInstance().GetRequiredMemSize(hardwareManagerParameter);
    size_t memoryPoolSize =
        HardwareManager::GetInstance().GetRequiredMemSizeForMemoryPool(g_VoiceCountMax);

    if (rParam.enableRecordingFinalOutputs) {
        hardwareManagerSize += HardwareManager::GetInstance().GetRequiredRecorderWorkBufferSize(
            hardwareManagerParameter);
    }

    size_t circularBufferSinkSize;
    if (rParam.enableCircularBufferSink) {
        circularBufferSinkSize =
            HardwareManager::GetInstance().GetRequiredCircularBufferSinkWithMemoryPoolBufferSize(
                hardwareManagerParameter);
    } else {
        circularBufferSinkSize = 0;
        circularBufferSinkMem = 0;
    }

    Result result = HardwareManager::GetInstance().Initialize(
        reinterpret_cast<void*>(workMem), hardwareManagerSize,
        reinterpret_cast<void*>(memoryPoolMem), memoryPoolSize,
        reinterpret_cast<void*>(circularBufferSinkMem), circularBufferSinkSize,
        hardwareManagerParameter);
    if (pResult != nullptr) {
        *pResult = result;
    }

    if (result.IsFailure()) {
        if (g_IsInitializedDriverCommandManager) {
            detail::DriverCommand::GetInstance().Finalize();
            detail::DriverCommand::GetInstanceForTaskThread().Finalize();
            g_SoundThreadCommandBufferSize = 0;
            g_TaskThreadCommandBufferSize = 0;
        }

        return false;
    }

    workMem += hardwareManagerSize;
    memoryPoolMem += memoryPoolSize;

    uintptr_t performanceFrameBuffer;
    size_t performanceFrameBufferSize;
    if (rParam.enableProfiler) {
        performanceFrameBuffer = AlignUp(workMem, PerformanceFrameBufferAlignment);
        performanceFrameBufferSize =
            HardwareManager::GetInstance().GetRequiredPerformanceFramesBufferSize(
                hardwareManagerParameter) *
            PerformanceFrameCount;
        workMem = performanceFrameBuffer + performanceFrameBufferSize;
    } else {
        performanceFrameBuffer = 0;
        performanceFrameBufferSize = 0;
    }

    g_PerformanceFrameBuffer = performanceFrameBuffer;
    g_PerformanceFrameBufferSize = performanceFrameBufferSize;
    g_CustomSubMixMixBufferCount =
        rParam.mixBufferCount == -1 ? rParam.subMixTotalChannelCount : rParam.mixBufferCount;
    g_RendererSampleRate = rParam.rendererSampleRate;
    g_UserEffectCount = rParam.effectCount;
    g_CustomSubMixSubMixCount = rParam.subMixCount;
    g_IsProfilerEnabled = rParam.enableProfiler;
    g_IsDetailSoundThreadProfilerEnabled = rParam.enableDetailSoundThreadProfile;
    g_IsAdditionalEffectBusEnabled = rParam.enableAdditionalEffectBus;
    g_IsAdditionalSubMixEnabled = rParam.enableAdditionalSubMix;
    g_IsEffectEnabled = rParam.enableEffect;
    g_IsRecordingEnabled = rParam.enableRecordingFinalOutputs;
    g_IsCircularBufferSinkEnabled = rParam.enableCircularBufferSink;
    g_IsPresetSubMixEnabled = hardwareManagerParameter.enableSubMix;
    g_IsSubMixEnabled = rParam.enableSubMix;
    g_IsStereoModeEnabled = rParam.enableStereoMode;
    g_IsSoundThreadEnabled = rParam.enableSoundThread;
    g_IsVoiceDropEnabled = rParam.enableVoiceDrop;
    g_IsPreviousSdkVersionLowPassFilterCompatible = rParam.enableCompatibleLowPassFilter;
    g_IsUserThreadRenderingEnabled = rParam.enableUserThreadRendering;
    g_IsUnusedEffectChannelMutingEnabled = rParam.enableUnusedEffectChannelMuting;
    g_IsCustomSubMixEnabled = rParam.enableCustomSubMix;
    g_IsMemoryPoolAttachCheckEnabled = rParam.enableMemoryPoolAttachCheck;
    g_IsBusMixVolumeEnabled = rParam.enableBusMixVolume;
    g_IsVolumeThroughModeEnabled = rParam.enableVolumeThroughMode;
    g_IsHighQualityVoiceResamplerEnabled = rParam.enableHighQualityVoiceResampler;
    g_BusCountMax = GetSoundInstanceConfig(rParam).busCount;

    g_SoundThreadStackPtr = AlignUp(workMem, MemoryAlignment);
    g_SoundThreadStackSize = rParam.soundThreadStackSize;
    workMem = g_SoundThreadStackPtr + rParam.soundThreadStackSize;
    detail::driver::SoundThread::GetInstance().Initialize(
        reinterpret_cast<void*>(g_PerformanceFrameBuffer), g_PerformanceFrameBufferSize,
        g_IsProfilerEnabled, g_IsDetailSoundThreadProfilerEnabled,
        g_IsUserThreadRenderingEnabled);

    uintptr_t multiVoiceManagerMem = workMem + rParam.taskThreadStackSize;
    detail::TaskManager::GetInstance().Initialize(rParam.enableProfiler);
    g_LoadThreadStackPtr = workMem;
    g_LoadThreadStackSize = rParam.taskThreadStackSize;
    g_TaskThreadFsPriority = rParam.taskThreadFsPriority;

    // The size is computed once for a (compiled-out) buffer check and again for Initialize.
    size_t multiVoiceManagerSize =
        detail::driver::MultiVoiceManager::GetInstance().GetRequiredMemSize(g_VoiceCountMax,
                                                                            config);
    static_cast<void>(multiVoiceManagerSize);
    detail::driver::MultiVoiceManager::GetInstance().Initialize(
        reinterpret_cast<void*>(multiVoiceManagerMem),
        detail::driver::MultiVoiceManager::GetInstance().GetRequiredMemSize(g_VoiceCountMax,
                                                                            config),
        config);

    size_t channelManagerSize = detail::driver::ChannelManager::GetInstance().GetRequiredMemSize(
        g_VoiceCountMax * 2, config);
    detail::driver::ChannelManager::GetInstance().Initialize(
        reinterpret_cast<void*>(memoryPoolMem), channelManagerSize, g_VoiceCountMax * 2, config);

    detail::driver::SequenceSoundPlayer::InitSequenceSoundPlayer();

    if (rParam.enableTaskThread) {
        detail::TaskThread::GetInstance().Create(
            rParam.taskThreadPriority, reinterpret_cast<void*>(g_LoadThreadStackPtr),
            g_LoadThreadStackSize, rParam.taskThreadCoreNumber, 0,
            static_cast<atk::FsPriority>(g_TaskThreadFsPriority));
        g_IsTaskThreadEnabled = true;
    } else {
        g_IsTaskThreadEnabled = false;
    }

    detail::DriverCommand::GetInstance().FlushCommand(false);

    if (g_IsSoundThreadEnabled) {
        detail::driver::SoundThread::GetInstance().CreateSoundThread(
            rParam.soundThreadPriority, reinterpret_cast<void*>(g_SoundThreadStackPtr),
            g_SoundThreadStackSize, rParam.soundThreadCoreNumber, 0);
    }

    detail::CurveLfo::InitializeCurveTable();
    g_IsInitialized = true;
    return true;
}

/**
 * @brief Initialize the sound and task thread command queues once.
 * @param rParam Sound system parameter selecting the queue sizes.
 * @param workMem Start of the memory both command queues are placed in.
 * @param workMemSize Size of the sound thread command queue memory.
 * @param memoryPoolMem Start of the task thread command queue memory.
 * @param memoryPoolMemSize Size of the task thread command queue memory.
 */
void SoundSystem::detail_InitializeDriverCommandManager(const SoundSystemParam& rParam,
                                                        uintptr_t workMem, size_t workMemSize,
                                                        uintptr_t memoryPoolMem,
                                                        size_t memoryPoolMemSize) {
    if (g_IsInitializedDriverCommandManager) {
        return;
    }

    size_t soundThreadCommandSize = detail::DriverCommand::GetInstance().GetRequiredMemSize(
        rParam.soundThreadCommandBufferSize, rParam.soundThreadCommandQueueCount);
    uintptr_t taskThreadCommandMem = workMem + soundThreadCommandSize;
    size_t taskThreadCommandSize =
        detail::DriverCommand::GetInstanceForTaskThread().GetRequiredMemSize(
            rParam.taskThreadCommandBufferSize, rParam.taskThreadCommandQueueCount);

    detail::DriverCommand::GetInstance().Initialize(
        reinterpret_cast<void*>(workMem), soundThreadCommandSize,
        rParam.soundThreadCommandBufferSize, rParam.soundThreadCommandQueueCount);
    detail::DriverCommand::GetInstanceForTaskThread().Initialize(
        reinterpret_cast<void*>(taskThreadCommandMem), taskThreadCommandSize,
        rParam.taskThreadCommandBufferSize, rParam.taskThreadCommandQueueCount);

    g_SoundThreadCommandBufferSize = rParam.soundThreadCommandBufferSize;
    g_SoundThreadCommandQueueCount = rParam.soundThreadCommandQueueCount;
    g_TaskThreadCommandBufferSize = rParam.taskThreadCommandBufferSize;
    g_TaskThreadCommandQueueCount = rParam.taskThreadCommandQueueCount;
    g_IsInitializedDriverCommandManager = true;
}

/**
 * @brief Initialize the sound system in a single work memory block.
 * @param rParam Sound system parameter.
 * @param workMem Start of the work memory.
 * @param workMemSize Size of the work memory.
 * @return Whether the sound system is initialized.
 */
bool SoundSystem::Initialize(const SoundSystemParam& rParam, uintptr_t workMem,
                             size_t workMemSize) {
    InitializeBufferSet bufferSet = {workMem, workMemSize, 0, 0, 0, 0};
    InitializeBufferSet setupBufferSet;
    SetupInitializeBufferSet(&setupBufferSet, rParam, bufferSet);

    bool isSuccess = detail_InitializeSoundSystem(nullptr, rParam, setupBufferSet);
    if (isSuccess) {
        HardwareManager::GetInstance().AttachMemoryPool(
            &g_MemoryPoolForSoundSystem, reinterpret_cast<void*>(setupBufferSet.memoryPoolMem),
            setupBufferSet.memoryPoolMemSize, true);
    }

    return isSuccess;
}

/**
 * @brief Initialize the sound system in a single work memory block.
 * @param pResult Receives the hardware initialization result; may be null.
 * @param rParam Sound system parameter.
 * @param workMem Start of the work memory.
 * @param workMemSize Size of the work memory.
 * @return Whether the sound system is initialized.
 */
bool SoundSystem::Initialize(Result* pResult, const SoundSystemParam& rParam, uintptr_t workMem,
                             size_t workMemSize) {
    InitializeBufferSet bufferSet = {workMem, workMemSize, 0, 0, 0, 0};
    InitializeBufferSet setupBufferSet;
    SetupInitializeBufferSet(&setupBufferSet, rParam, bufferSet);

    bool isSuccess = detail_InitializeSoundSystem(pResult, rParam, setupBufferSet);
    if (isSuccess) {
        HardwareManager::GetInstance().AttachMemoryPool(
            &g_MemoryPoolForSoundSystem, reinterpret_cast<void*>(setupBufferSet.memoryPoolMem),
            setupBufferSet.memoryPoolMemSize, true);
    }

    return isSuccess;
}

/**
 * @brief Split user buffers into the regions the sound system initializes from.
 * @param pOutBufferSet Receives the regions to initialize from.
 * @param rParam Sound system parameter.
 * @param rBufferSet Buffers supplied by the user.
 */
void SoundSystem::SetupInitializeBufferSet(InitializeBufferSet* pOutBufferSet,
                                           const SoundSystemParam& rParam,
                                           const InitializeBufferSet& rBufferSet) {
    g_IsManagingMemoryPool = rParam.enableMemoryPoolManagement;

    if (g_IsManagingMemoryPool) {
        uintptr_t workMemEnd = rBufferSet.workMem + rBufferSet.workMemSize;
        size_t memoryPoolSize =
            AlignUp(GetRequiredMemSizeForMemoryPool(rParam), MemoryAlignment);
        uintptr_t memoryPoolMem = AlignUp(rBufferSet.workMem, MemoryAlignment);
        uintptr_t workMem = memoryPoolMem + memoryPoolSize;

        pOutBufferSet->workMem = workMem;
        pOutBufferSet->workMemSize = workMemEnd - workMem;
        pOutBufferSet->memoryPoolMem = memoryPoolMem;
        pOutBufferSet->memoryPoolMemSize = memoryPoolSize;
    } else {
        pOutBufferSet->workMem = rBufferSet.workMem;
        pOutBufferSet->workMemSize = rBufferSet.workMemSize;
        pOutBufferSet->memoryPoolMem = rBufferSet.memoryPoolMem;
        pOutBufferSet->memoryPoolMemSize = rBufferSet.memoryPoolMemSize;
    }

    if (rParam.enableCircularBufferSinkBufferManagement) {
        if (rParam.enableCircularBufferSink) {
            size_t circularBufferSinkSize = GetRequiredMemSizeForCircularBufferSink(rParam);
            uintptr_t circularBufferSinkMem = pOutBufferSet->workMem;
            pOutBufferSet->workMem = circularBufferSinkMem + circularBufferSinkSize;
            pOutBufferSet->workMemSize -= circularBufferSinkSize;
            pOutBufferSet->circularBufferSinkMem = circularBufferSinkMem;
            pOutBufferSet->circularBufferSinkMemSize = circularBufferSinkSize;
        } else {
            pOutBufferSet->circularBufferSinkMem = 0;
            pOutBufferSet->circularBufferSinkMemSize = 0;
        }
    } else {
        pOutBufferSet->circularBufferSinkMem = rBufferSet.circularBufferSinkMem;
        pOutBufferSet->circularBufferSinkMemSize = rBufferSet.circularBufferSinkMemSize;
    }
}

/**
 * @brief Initialize the sound system with a separate memory pool block.
 * @param rParam Sound system parameter.
 * @param workMem Start of the work memory.
 * @param workMemSize Size of the work memory.
 * @param memoryPoolMem Start of the memory pool memory.
 * @param memoryPoolMemSize Size of the memory pool memory.
 * @return Whether the sound system is initialized.
 */
bool SoundSystem::Initialize(const SoundSystemParam& rParam, uintptr_t workMem,
                             size_t workMemSize, uintptr_t memoryPoolMem,
                             size_t memoryPoolMemSize) {
    InitializeBufferSet bufferSet = {workMem, workMemSize, memoryPoolMem, memoryPoolMemSize, 0, 0};
    InitializeBufferSet setupBufferSet;
    SetupInitializeBufferSet(&setupBufferSet, rParam, bufferSet);

    return detail_InitializeSoundSystem(nullptr, rParam, setupBufferSet);
}

/**
 * @brief Initialize the sound system with a separate memory pool block.
 * @param pResult Receives the hardware initialization result; may be null.
 * @param rParam Sound system parameter.
 * @param workMem Start of the work memory.
 * @param workMemSize Size of the work memory.
 * @param memoryPoolMem Start of the memory pool memory.
 * @param memoryPoolMemSize Size of the memory pool memory.
 * @return Whether the sound system is initialized.
 */
bool SoundSystem::Initialize(Result* pResult, const SoundSystemParam& rParam, uintptr_t workMem,
                             size_t workMemSize, uintptr_t memoryPoolMem,
                             size_t memoryPoolMemSize) {
    InitializeBufferSet bufferSet = {workMem, workMemSize, memoryPoolMem, memoryPoolMemSize, 0, 0};
    InitializeBufferSet setupBufferSet;
    SetupInitializeBufferSet(&setupBufferSet, rParam, bufferSet);

    return detail_InitializeSoundSystem(pResult, rParam, setupBufferSet);
}

/**
 * @brief Initialize the sound system from a user buffer set.
 * @param rParam Sound system parameter.
 * @param rBufferSet Buffers supplied by the user.
 * @return Whether the sound system is initialized.
 */
bool SoundSystem::Initialize(const SoundSystemParam& rParam, InitializeBufferSet& rBufferSet) {
    InitializeBufferSet setupBufferSet;
    SetupInitializeBufferSet(&setupBufferSet, rParam, rBufferSet);

    bool isSuccess = detail_InitializeSoundSystem(nullptr, rParam, setupBufferSet);
    if (isSuccess && rParam.enableMemoryPoolManagement) {
        HardwareManager::GetInstance().AttachMemoryPool(
            &g_MemoryPoolForSoundSystem, reinterpret_cast<void*>(setupBufferSet.memoryPoolMem),
            setupBufferSet.memoryPoolMemSize, true);
    }

    return isSuccess;
}

/**
 * @brief Initialize the sound system from a user buffer set.
 * @param pResult Receives the hardware initialization result; may be null.
 * @param rParam Sound system parameter.
 * @param rBufferSet Buffers supplied by the user.
 * @return Whether the sound system is initialized.
 */
bool SoundSystem::Initialize(Result* pResult, const SoundSystemParam& rParam,
                             InitializeBufferSet& rBufferSet) {
    InitializeBufferSet setupBufferSet;
    SetupInitializeBufferSet(&setupBufferSet, rParam, rBufferSet);

    bool isSuccess = detail_InitializeSoundSystem(pResult, rParam, setupBufferSet);
    if (isSuccess && rParam.enableMemoryPoolManagement) {
        HardwareManager::GetInstance().AttachMemoryPool(
            &g_MemoryPoolForSoundSystem, reinterpret_cast<void*>(setupBufferSet.memoryPoolMem),
            setupBufferSet.memoryPoolMemSize, true);
    }

    return isSuccess;
}

/** @brief Stop the sound system threads and release every component. */
void SoundSystem::Finalize() {
    if (!g_IsInitialized) {
        return;
    }

    if (g_IsPresetSubMixEnabled) {
        u32 tag = detail::DriverCommand::GetInstance().FlushCommand(true);
        if (g_IsUserThreadRenderingEnabled) {
            detail::driver::SoundThread::GetInstance().FrameProcess(UpdateType_AudioFrame);
        }

        detail::DriverCommand::GetInstance().WaitCommandReply(tag);
    }

    if (g_IsTaskThreadEnabled) {
        detail::TaskManager::GetInstance().CancelAllTask();
        detail::TaskThread::GetInstance().Destroy();
        detail::TaskManager::GetInstance().Finalize();
    } else {
        g_IsTaskThreadEnabled = true;
    }

    if (g_IsSoundThreadEnabled) {
        detail::driver::SoundThread::GetInstance().Destroy();
    } else {
        g_IsSoundThreadEnabled = true;
    }

    detail::driver::ChannelManager::GetInstance().Finalize();

    if (g_IsManagingMemoryPool) {
        HardwareManager::GetInstance().DetachMemoryPool(&g_MemoryPoolForSoundSystem, false);
    } else {
        g_IsManagingMemoryPool = true;
    }

    detail::driver::MultiVoiceManager::GetInstance().Finalize();
    HardwareManager::GetInstance().Finalize();
    detail::driver::SoundThread::GetInstance().Finalize();

    if (g_IsInitializedDriverCommandManager) {
        detail::DriverCommand::GetInstance().Finalize();
        detail::DriverCommand::GetInstanceForTaskThread().Finalize();
        g_SoundThreadCommandBufferSize = 0;
        g_TaskThreadCommandBufferSize = 0;
        g_IsInitializedDriverCommandManager = false;
    }

    g_IsCircularBufferSinkEnabled = false;
    g_IsCircularBufferSinkWarningDisplayed = false;
    g_IsInitialized = false;
}

/**
 * @brief Register a function called when the sound thread starts a frame.
 * @param callback Function to call.
 * @param arg Argument passed to the function.
 */
void SoundSystem::SetSoundThreadBeginUserCallback(SoundThreadUserCallback callback,
                                                  uintptr_t arg) {
    detail::driver::SoundThread::GetInstance().RegisterThreadBeginUserCallback(callback, arg);
}

/** @brief Remove the sound thread frame start callback. */
void SoundSystem::ClearSoundThreadBeginUserCallback() {
    detail::driver::SoundThread::GetInstance().ClearThreadBeginUserCallback();
}

/**
 * @brief Register a function called when the sound thread finishes a frame.
 * @param callback Function to call.
 * @param arg Argument passed to the function.
 */
void SoundSystem::SetSoundThreadEndUserCallback(SoundThreadUserCallback callback, uintptr_t arg) {
    detail::driver::SoundThread::GetInstance().RegisterThreadEndUserCallback(callback, arg);
}

/** @brief Remove the sound thread frame end callback. */
void SoundSystem::ClearSoundThreadEndUserCallback() {
    detail::driver::SoundThread::GetInstance().ClearThreadEndUserCallback();
}

/**
 * @brief Check whether the sound system is initialized.
 * @return True between Initialize and Finalize.
 */
bool SoundSystem::IsInitialized() {
    return g_IsInitialized;
}

/**
 * @brief Suspend the audio renderer.
 * @param fadeTimes Fade-out time (unused).
 */
void SoundSystem::SuspendAudioRenderer(TimeSpan fadeTimes) {
    HardwareManager::GetInstance().SuspendAudioRenderer();
}

/**
 * @brief Resume the audio renderer.
 * @param fadeTimes Fade-in time (unused).
 */
void SoundSystem::ResumeAudioRenderer(TimeSpan fadeTimes) {
    HardwareManager::GetInstance().ResumeAudioRenderer();
}

/** @brief Render one audio frame when rendering is driven by the user thread. */
void SoundSystem::ExecuteRendering() {
    HardwareManager::GetInstance().ExecuteAudioRendererRendering();
}

/**
 * @brief Read the audio renderer processing time limit.
 * @return Limit in percent of an audio frame.
 */
int SoundSystem::GetAudioRendererRenderingTimeLimit() {
    return HardwareManager::GetInstance().GetAudioRendererRenderingTimeLimit();
}

/**
 * @brief Set the audio renderer processing time limit; aborts on failure.
 * @param limitPercent Limit in percent of an audio frame.
 */
void SoundSystem::SetAudioRendererRenderingTimeLimit(int limitPercent) {
    NN_ABORT_UNLESS_RESULT_SUCCESS(
        HardwareManager::GetInstance().SetAudioRendererRenderingTimeLimit(limitPercent));
}

/**
 * @brief Attach user memory to the audio renderer as a memory pool.
 * @param pPool Memory pool to attach.
 * @param pMemory Start of the memory.
 * @param size Size of the memory.
 */
void SoundSystem::AttachMemoryPool(audio::MemoryPoolType* pPool, void* pMemory, size_t size) {
    HardwareManager::GetInstance().AttachMemoryPool(pPool, pMemory, size, true);
}

/**
 * @brief Detach a memory pool from the audio renderer.
 * @param pPool Memory pool to detach.
 */
void SoundSystem::DetachMemoryPool(audio::MemoryPoolType* pPool) {
    HardwareManager::GetInstance().DetachMemoryPool(pPool, true);
}

/** @brief Print memory usage (compiled out in release builds). */
void SoundSystem::DumpMemory() {}

/**
 * @brief Compute the audio renderer work memory for the current settings.
 * @return Required size, or 0 before Initialize.
 */
size_t SoundSystem::GetAudioRendererBufferSize() {
    HardwareManagerParameter hardwareManagerParameter;
    SetupHardwareManagerParameterFromCurrentSetting(&hardwareManagerParameter);

    if (!g_IsInitialized) {
        return 0;
    }

    return HardwareManager::GetInstance().GetRequiredMemSize(hardwareManagerParameter);
}

/**
 * @brief Fill a hardware manager parameter from the current sound system settings.
 * @param pParameter Parameter to fill.
 */
void SoundSystem::SetupHardwareManagerParameterFromCurrentSetting(
    HardwareManagerParameter* pParameter) {
    pParameter->rendererSampleRate = g_RendererSampleRate;
    pParameter->userEffectCount = g_UserEffectCount;
    pParameter->enableProfiler = g_IsProfilerEnabled;
    pParameter->voiceCount = g_VoiceCountMax;
    pParameter->enableEffect = g_IsEffectEnabled;
    pParameter->enableRecordingFinalOutputs = g_IsRecordingEnabled;
    pParameter->enableUserCircularBufferSink = g_IsCircularBufferSinkEnabled;
    pParameter->enableStereoMode = g_IsStereoModeEnabled;
    pParameter->enableMemoryPoolAttachCheck = g_IsSoundThreadEnabled;
    pParameter->enableVoiceDrop = g_IsVoiceDropEnabled;
    pParameter->_23 = g_IsPreviousSdkVersionLowPassFilterCompatible;
    pParameter->enableUnusedEffectChannelMuting = g_IsUnusedEffectChannelMutingEnabled;
    pParameter->enableManualRendering = g_IsUserThreadRenderingEnabled;
    pParameter->SetSubMixParameter(g_IsStereoModeEnabled, g_IsEffectEnabled, g_IsSubMixEnabled,
                                   g_IsAdditionalEffectBusEnabled, g_IsAdditionalSubMixEnabled,
                                   g_IsCustomSubMixEnabled, g_CustomSubMixSubMixCount,
                                   g_CustomSubMixMixBufferCount);
    pParameter->enableRenderingOverloadAbort = g_IsMemoryPoolAttachCheckEnabled;
}

/**
 * @brief Compute the final output recorder work memory for the current settings.
 * @return Required size, or 0 when recording is disabled.
 */
size_t SoundSystem::GetRecorderBufferSize() {
    HardwareManagerParameter hardwareManagerParameter;
    SetupHardwareManagerParameterFromCurrentSetting(&hardwareManagerParameter);

    if (!g_IsInitialized || !g_IsRecordingEnabled) {
        return 0;
    }

    return HardwareManager::GetInstance().GetRequiredRecorderWorkBufferSize(
        hardwareManagerParameter);
}

/**
 * @brief Compute the user circular buffer sink memory for the current settings.
 * @return Required size, or 0 when the sink is disabled.
 */
size_t SoundSystem::GetUserCircularBufferSinkBufferSize() {
    HardwareManagerParameter hardwareManagerParameter;
    SetupHardwareManagerParameterFromCurrentSetting(&hardwareManagerParameter);

    if (!g_IsInitialized || !g_IsCircularBufferSinkEnabled) {
        return 0;
    }

    return HardwareManager::GetInstance().GetRequiredCircularBufferSinkWithMemoryPoolBufferSize(
        hardwareManagerParameter);
}

/**
 * @brief Compute the low-level voice allocator memory.
 * @return Required size, or 0 before Initialize.
 */
size_t SoundSystem::GetLowLevelVoiceAllocatorBufferSize() {
    if (!g_IsInitialized) {
        return 0;
    }

    return HardwareManager::GetInstance().GetRequiredMemSizeForMemoryPool(g_VoiceCountMax);
}

/**
 * @brief Compute the multi voice manager memory.
 * @return Required size, or 0 before Initialize.
 */
size_t SoundSystem::GetMultiVoiceManagerBufferSize() {
    if (!g_IsInitialized) {
        return 0;
    }

    return detail::driver::MultiVoiceManager::GetInstance().GetRequiredMemSize(
        g_VoiceCountMax, GetSoundInstanceConfig());
}

/**
 * @brief Build the per-sound instance configuration of the current settings.
 * @return Current configuration.
 */
detail::SoundInstanceConfig SoundSystem::GetSoundInstanceConfig() {
    detail::SoundInstanceConfig config;
    config.enableBusMixVolume = g_IsBusMixVolumeEnabled;
    config.enableVolumeThroughMode = g_IsVolumeThroughModeEnabled;
    config.busCount = g_BusCountMax;
    return config;
}

/**
 * @brief Compute the channel manager memory.
 * @return Required size, or 0 before Initialize.
 */
size_t SoundSystem::GetChannelManagerBufferSize() {
    if (!g_IsInitialized) {
        return 0;
    }

    return detail::driver::ChannelManager::GetInstance().GetRequiredMemSize(
        g_VoiceCountMax * 2, GetSoundInstanceConfig());
}

/**
 * @brief Compute the memory of the sound thread command queue.
 * @return Required size in bytes.
 */
size_t SoundSystem::GetSoundThreadCommandTotalBufferSize() {
    return detail::DriverCommand::GetInstance().GetRequiredMemSize(
        g_SoundThreadCommandBufferSize, g_SoundThreadCommandQueueCount);
}

/**
 * @brief Compute the memory of the task thread command queue.
 * @return Required size in bytes.
 */
size_t SoundSystem::GetTaskThreadCommandTotalBufferSize() {
    return detail::DriverCommand::GetInstance().GetRequiredMemSize(
        g_TaskThreadCommandBufferSize, g_TaskThreadCommandQueueCount);
}

/**
 * @brief Read the size of the driver command buffer.
 * @return Command buffer size in bytes.
 */
size_t SoundSystem::GetDriverCommandBufferSize() {
    return detail::DriverCommand::GetInstance().GetCommandBufferSize();
}

/**
 * @brief Read the largest driver command that can currently be allocated.
 * @return Allocatable size in bytes.
 */
size_t SoundSystem::GetAllocatableDriverCommandSize() {
    return detail::DriverCommand::GetInstance().GetAllocatableCommandSize();
}

/**
 * @brief Read the driver command buffer memory in use.
 * @return Allocated size in bytes.
 */
size_t SoundSystem::GetAllocatedDriverCommandBufferSize() {
    return detail::DriverCommand::GetInstance().GetAllocatedCommandBufferSize();
}

/**
 * @brief Read the number of driver commands in use.
 * @return Allocated command count.
 */
int SoundSystem::GetAllocatedDriverCommandCount() {
    return detail::DriverCommand::GetInstance().GetAllocatedCommandCount();
}

/**
 * @brief Register a reader of audio renderer performance frames.
 * @param rReader Reader to register.
 */
void SoundSystem::RegisterAudioRendererPerformanceReader(AudioRendererPerformanceReader& rReader) {
    detail::driver::SoundThread::GetInstance().RegisterAudioRendererPerformanceReader(rReader);
}

/**
 * @brief Append an effect to an aux bus of the main sub mix.
 * @param bus Aux bus.
 * @param pEffect Effect to append.
 * @param pBuffer Effect work buffer.
 * @param bufferSize Size of the work buffer.
 * @return Whether the effect was appended.
 */
bool SoundSystem::AppendEffect(AuxBus bus, EffectBase* pEffect, void* pBuffer,
                               size_t bufferSize) {
    return HardwareManager::GetInstance().GetSubMix(0)->AppendEffect(pEffect, bus + 1, pBuffer,
                                                                     bufferSize);
}

/**
 * @brief Append an effect to an aux bus of the main sub mix.
 * @param bus Aux bus.
 * @param pEffect Effect to append.
 * @param pBuffer Effect work buffer.
 * @param bufferSize Size of the work buffer.
 * @param device Output device (unused).
 * @return Whether the effect was appended.
 */
bool SoundSystem::AppendEffect(AuxBus bus, EffectBase* pEffect, void* pBuffer, size_t bufferSize,
                               OutputDevice device) {
    return HardwareManager::GetInstance().GetSubMix(0)->AppendEffect(pEffect, bus + 1, pBuffer,
                                                                     bufferSize);
}

/**
 * @brief Append an effect to an aux bus of a sub mix.
 * @param bus Aux bus.
 * @param pEffect Effect to append.
 * @param pBuffer Effect work buffer.
 * @param bufferSize Size of the work buffer.
 * @param device Output device (unused).
 * @param subMixIndex Sub mix to append to.
 * @return Whether the effect was appended.
 */
bool SoundSystem::AppendEffect(AuxBus bus, EffectBase* pEffect, void* pBuffer, size_t bufferSize,
                               OutputDevice device, int subMixIndex) {
    return HardwareManager::GetInstance().GetSubMix(subMixIndex)->AppendEffect(
        pEffect, bus + 1, pBuffer, bufferSize);
}

/**
 * @brief Append an aux effect to an aux bus of the main sub mix.
 * @param bus Aux bus.
 * @param pEffect Effect to append.
 * @param pBuffer Effect work buffer.
 * @param bufferSize Size of the work buffer.
 * @return Whether the effect was appended.
 */
bool SoundSystem::AppendEffect(AuxBus bus, EffectAux* pEffect, void* pBuffer, size_t bufferSize) {
    return HardwareManager::GetInstance().GetSubMix(0)->AppendEffect(pEffect, bus + 1, pBuffer,
                                                                     bufferSize);
}

/**
 * @brief Append an aux effect to an aux bus of the main sub mix.
 * @param bus Aux bus.
 * @param pEffect Effect to append.
 * @param pBuffer Effect work buffer.
 * @param bufferSize Size of the work buffer.
 * @param device Output device (unused).
 * @return Whether the effect was appended.
 */
bool SoundSystem::AppendEffect(AuxBus bus, EffectAux* pEffect, void* pBuffer, size_t bufferSize,
                               OutputDevice device) {
    return HardwareManager::GetInstance().GetSubMix(0)->AppendEffect(pEffect, bus + 1, pBuffer,
                                                                     bufferSize);
}

/**
 * @brief Append an aux effect to an aux bus of a sub mix.
 * @param bus Aux bus.
 * @param pEffect Effect to append.
 * @param pBuffer Effect work buffer.
 * @param bufferSize Size of the work buffer.
 * @param device Output device (unused).
 * @param subMixIndex Sub mix to append to.
 * @return Whether the effect was appended.
 */
bool SoundSystem::AppendEffect(AuxBus bus, EffectAux* pEffect, void* pBuffer, size_t bufferSize,
                               OutputDevice device, int subMixIndex) {
    return HardwareManager::GetInstance().GetSubMix(subMixIndex)->AppendEffect(
        pEffect, bus + 1, pBuffer, bufferSize);
}

/**
 * @brief Append an aux effect to the final mix.
 * @param pEffect Effect to append.
 * @param pBuffer Effect work buffer.
 * @param bufferSize Size of the work buffer.
 * @return Whether the effect was appended.
 */
bool SoundSystem::AppendEffectToFinalMix(EffectAux* pEffect, void* pBuffer, size_t bufferSize) {
    return HardwareManager::GetInstance().GetFinalMix().AppendEffect(pEffect, pBuffer,
                                                                     bufferSize);
}

/**
 * @brief Append an aux effect to the additional sub mix.
 * @param pEffect Effect to append.
 * @param pBuffer Effect work buffer.
 * @param bufferSize Size of the work buffer.
 * @return Whether the effect was appended.
 */
bool SoundSystem::AppendEffectToAdditionalSubMix(EffectAux* pEffect, void* pBuffer,
                                                 size_t bufferSize) {
    return HardwareManager::GetInstance().GetAdditionalSubMix().AppendEffect(pEffect, 0, pBuffer,
                                                                             bufferSize);
}

/**
 * @brief Compute the work buffer an aux effect needs.
 * @param pEffect Effect to query.
 * @return Required buffer size in bytes.
 */
size_t SoundSystem::GetRequiredEffectAuxBufferSize(const EffectAux* pEffect) {
    return HardwareManager::GetInstance().GetRequiredEffectAuxBufferSize(pEffect);
}

/**
 * @brief Remove an effect from an aux bus of the main sub mix.
 * @param bus Aux bus.
 * @param pEffect Effect to remove.
 * @return Whether the effect was removed.
 */
bool SoundSystem::RemoveEffect(AuxBus bus, EffectBase* pEffect) {
    return HardwareManager::GetInstance().GetSubMix(0)->RemoveEffect(pEffect, bus + 1);
}

/**
 * @brief Remove an effect from an aux bus of the main sub mix.
 * @param bus Aux bus.
 * @param pEffect Effect to remove.
 * @param device Output device (unused).
 * @return Whether the effect was removed.
 */
bool SoundSystem::RemoveEffect(AuxBus bus, EffectBase* pEffect, OutputDevice device) {
    return HardwareManager::GetInstance().GetSubMix(0)->RemoveEffect(pEffect, bus + 1);
}

/**
 * @brief Remove an effect from an aux bus of a sub mix.
 * @param bus Aux bus.
 * @param pEffect Effect to remove.
 * @param device Output device (unused).
 * @param subMixIndex Sub mix to remove from.
 * @return Whether the effect was removed.
 */
bool SoundSystem::RemoveEffect(AuxBus bus, EffectBase* pEffect, OutputDevice device,
                               int subMixIndex) {
    return HardwareManager::GetInstance().GetSubMix(subMixIndex)->RemoveEffect(pEffect, bus + 1);
}

/**
 * @brief Remove an aux effect from an aux bus of the main sub mix.
 * @param bus Aux bus.
 * @param pEffect Effect to remove.
 * @return Whether the effect was removed.
 */
bool SoundSystem::RemoveEffect(AuxBus bus, EffectAux* pEffect) {
    return HardwareManager::GetInstance().GetSubMix(0)->RemoveEffect(pEffect, bus + 1);
}

/**
 * @brief Remove an aux effect from an aux bus of the main sub mix.
 * @param bus Aux bus.
 * @param pEffect Effect to remove.
 * @param device Output device (unused).
 * @return Whether the effect was removed.
 */
bool SoundSystem::RemoveEffect(AuxBus bus, EffectAux* pEffect, OutputDevice device) {
    return HardwareManager::GetInstance().GetSubMix(0)->RemoveEffect(pEffect, bus + 1);
}

/**
 * @brief Remove an aux effect from an aux bus of a sub mix.
 * @param bus Aux bus.
 * @param pEffect Effect to remove.
 * @param device Output device (unused).
 * @param subMixIndex Sub mix to remove from.
 * @return Whether the effect was removed.
 */
bool SoundSystem::RemoveEffect(AuxBus bus, EffectAux* pEffect, OutputDevice device,
                               int subMixIndex) {
    return HardwareManager::GetInstance().GetSubMix(subMixIndex)->RemoveEffect(pEffect, bus + 1);
}

/**
 * @brief Remove an aux effect from the final mix.
 * @param pEffect Effect to remove.
 * @return Whether the effect was removed.
 */
bool SoundSystem::RemoveEffectFromFinalMix(EffectAux* pEffect) {
    return HardwareManager::GetInstance().GetFinalMix().RemoveEffect(pEffect);
}

/**
 * @brief Remove an aux effect from the additional sub mix.
 * @param pEffect Effect to remove.
 * @return Whether the effect was removed.
 */
bool SoundSystem::RemoveEffectFromAdditionalSubMix(EffectAux* pEffect) {
    return HardwareManager::GetInstance().GetAdditionalSubMix().RemoveEffect(pEffect, 0);
}

/**
 * @brief Remove every effect of an aux bus of the main sub mix.
 * @param bus Aux bus.
 */
void SoundSystem::ClearEffect(AuxBus bus) {
    HardwareManager::GetInstance().GetSubMix(0)->ClearEffect(bus + 1);
}

/**
 * @brief Remove every effect of an aux bus of the main sub mix.
 * @param bus Aux bus.
 * @param device Output device (unused).
 */
void SoundSystem::ClearEffect(AuxBus bus, OutputDevice device) {
    HardwareManager::GetInstance().GetSubMix(0)->ClearEffect(bus + 1);
}

/**
 * @brief Remove every effect of an aux bus of a sub mix.
 * @param bus Aux bus.
 * @param device Output device (unused).
 * @param subMixIndex Sub mix to clear.
 */
void SoundSystem::ClearEffect(AuxBus bus, OutputDevice device, int subMixIndex) {
    HardwareManager::GetInstance().GetSubMix(subMixIndex)->ClearEffect(bus + 1);
}

/** @brief Remove every effect of the final mix. */
void SoundSystem::ClearEffectFromFinalMix() {
    HardwareManager::GetInstance().GetFinalMix().ClearEffect();
}

/** @brief Remove every effect of the additional sub mix. */
void SoundSystem::ClearEffectFromAdditionalSubMix() {
    HardwareManager::GetInstance().GetAdditionalSubMix().ClearEffect(0);
}

/**
 * @brief Check whether an aux bus of the main sub mix has no effect left.
 * @param bus Aux bus.
 * @return True once the bus is cleared.
 */
bool SoundSystem::IsClearEffectFinished(AuxBus bus) {
    return !HardwareManager::GetInstance().GetSubMix(0)->HasEffect(bus + 1);
}

/**
 * @brief Check whether an aux bus of the main sub mix has no effect left.
 * @param bus Aux bus.
 * @param device Output device (unused).
 * @return True once the bus is cleared.
 */
bool SoundSystem::IsClearEffectFinished(AuxBus bus, OutputDevice device) {
    return !HardwareManager::GetInstance().GetSubMix(0)->HasEffect(bus + 1);
}

/**
 * @brief Check whether an aux bus of a sub mix has no effect left.
 * @param bus Aux bus.
 * @param device Output device (unused).
 * @param subMixIndex Sub mix to check.
 * @return True once the bus is cleared.
 */
bool SoundSystem::IsClearEffectFinished(AuxBus bus, OutputDevice device, int subMixIndex) {
    return !HardwareManager::GetInstance().GetSubMix(subMixIndex)->HasEffect(bus + 1);
}

/**
 * @brief Check whether the final mix has no effect left.
 * @return True once the final mix is cleared.
 */
bool SoundSystem::IsClearEffectFromFinalMixFinished() {
    return !HardwareManager::GetInstance().GetFinalMix().HasEffect(0);
}

/**
 * @brief Check whether the additional sub mix has no effect left.
 * @return True once the additional sub mix is cleared.
 */
bool SoundSystem::IsClearEffectFromAdditionalSubMixFinished() {
    return !HardwareManager::GetInstance().GetAdditionalSubMix().HasEffect(0);
}

/**
 * @brief Fade the volume of an aux bus of the main sub mix.
 * @param bus Aux bus.
 * @param volume Target volume.
 * @param fadeTimes Fade time.
 */
void SoundSystem::SetAuxBusVolume(AuxBus bus, f32 volume, TimeSpan fadeTimes) {
    SetAuxBusVolume(bus, volume, fadeTimes, 0);
}

/**
 * @brief Fade the volume of an aux bus of a sub mix.
 * @param bus Aux bus.
 * @param volume Target volume.
 * @param fadeTimes Fade time.
 * @param subMixIndex Sub mix owning the bus.
 */
void SoundSystem::SetAuxBusVolume(AuxBus bus, f32 volume, TimeSpan fadeTimes, int subMixIndex) {
    int fadeFrames = (static_cast<int>(fadeTimes.GetMilliSeconds()) + AudioFrameMilliSeconds - 1) /
                     AudioFrameMilliSeconds;

    detail::DriverCommand& rCommandManager = detail::DriverCommand::GetInstance();
    auto* pCommand = static_cast<detail::DriverCommandAuxBusVolume*>(
        rCommandManager.AllocMemory(sizeof(detail::DriverCommandAuxBusVolume), true));
    pCommand->type = detail::DriverCommandAuxBusVolume::Id;
    pCommand->bus = bus;
    pCommand->subMixIndex = subMixIndex;
    pCommand->volume = volume;
    pCommand->fadeFrames = fadeFrames;
    rCommandManager.PushCommand(pCommand);
}

/**
 * @brief Read the volume of an aux bus of the main sub mix.
 * @param bus Aux bus.
 * @return Current volume.
 */
f32 SoundSystem::GetAuxBusVolume(AuxBus bus) {
    return HardwareManager::GetInstance().GetAuxBusVolume(bus, 0);
}

/**
 * @brief Read the volume of an aux bus of a sub mix.
 * @param bus Aux bus.
 * @param subMixIndex Sub mix owning the bus.
 * @return Current volume.
 */
f32 SoundSystem::GetAuxBusVolume(AuxBus bus, int subMixIndex) {
    return HardwareManager::GetInstance().GetAuxBusVolume(bus, subMixIndex);
}

/**
 * @brief Set a main bus channel send to the additional effect bus.
 * @param volume Send volume.
 * @param srcChannel Source channel.
 * @param dstChannel Destination channel.
 */
void SoundSystem::SetMainBusChannelVolumeForAdditionalEffect(f32 volume, int srcChannel,
                                                             int dstChannel) {
    HardwareManager::GetInstance().SetMainBusChannelVolumeForAdditionalEffect(volume, srcChannel,
                                                                              dstChannel);
}

/**
 * @brief Read a main bus channel send to the additional effect bus.
 * @param srcChannel Source channel.
 * @param dstChannel Destination channel.
 * @return Send volume.
 */
f32 SoundSystem::GetMainBusChannelVolumeForAdditionalEffect(int srcChannel, int dstChannel) {
    return HardwareManager::GetInstance().GetMainBusChannelVolumeForAdditionalEffect(srcChannel,
                                                                                     dstChannel);
}

/**
 * @brief Set an aux bus channel send to the additional effect bus.
 * @param bus Aux bus.
 * @param volume Send volume.
 * @param srcChannel Source channel.
 * @param dstChannel Destination channel.
 */
void SoundSystem::SetAuxBusChannelVolumeForAdditionalEffect(AuxBus bus, f32 volume,
                                                            int srcChannel, int dstChannel) {
    HardwareManager::GetInstance().SetAuxBusChannelVolumeForAdditionalEffect(bus, volume,
                                                                             srcChannel,
                                                                             dstChannel);
}

/**
 * @brief Read an aux bus channel send to the additional effect bus.
 * @param bus Aux bus.
 * @param srcChannel Source channel.
 * @param dstChannel Destination channel.
 * @return Send volume.
 */
f32 SoundSystem::GetAuxBusChannelVolumeForAdditionalEffect(AuxBus bus, int srcChannel,
                                                           int dstChannel) {
    return HardwareManager::GetInstance().GetAuxBusChannelVolumeForAdditionalEffect(
        bus, srcChannel, dstChannel);
}

/**
 * @brief Set a channel send to the additional effect bus for every aux bus.
 * @param volume Send volume.
 * @param srcChannel Source channel.
 * @param dstChannel Destination channel.
 */
void SoundSystem::SetAllAuxBusChannelVolumeForAdditionalEffect(f32 volume, int srcChannel,
                                                               int dstChannel) {
    int busCount = HardwareManager::GetInstance().GetAuxBusCountForAdditionalEffect();
    for (int i = 0; i < busCount; i++) {
        SetAuxBusChannelVolumeForAdditionalEffect(static_cast<AuxBus>(i), volume, srcChannel,
                                                  dstChannel);
    }
}

/**
 * @brief Set a channel send to the additional effect bus for the main and every aux bus.
 * @param volume Send volume.
 * @param srcChannel Source channel.
 * @param dstChannel Destination channel.
 */
void SoundSystem::SetAllBusChannelVolumeForAdditionalEffect(f32 volume, int srcChannel,
                                                            int dstChannel) {
    SetMainBusChannelVolumeForAdditionalEffect(volume, srcChannel, dstChannel);
    SetAllAuxBusChannelVolumeForAdditionalEffect(volume, srcChannel, dstChannel);
}

/**
 * @brief Run sound frames on the calling thread until enough voice commands are pending.
 * @param updateType Kind of frame to process.
 * @param count Pending voice command count to reach.
 */
void SoundSystem::VoiceCommandProcess(UpdateType updateType, u32 count) {
    if (!IsInitialized()) {
        return;
    }

    detail::driver::SoundThread& rSoundThread = detail::driver::SoundThread::GetInstance();
    detail::LowLevelVoiceCommand& rVoiceCommand = detail::LowLevelVoiceCommand::GetInstance();
    while (static_cast<u32>(rVoiceCommand.GetPendingCommandCount()) < count) {
        rSoundThread.FrameProcess(updateType);
        rVoiceCommand.RecvCommandReply();
        rVoiceCommand.FlushCommand(true, false);
    }
}

/**
 * @brief Run audio frames on the calling thread until enough voice commands are pending.
 * @param count Pending voice command count to reach.
 */
void SoundSystem::VoiceCommandProcess(u32 count) {
    VoiceCommandProcess(UpdateType_AudioFrame, count);
}

/** @brief Update voices and effects from the user thread when there is no sound thread. */
void SoundSystem::VoiceCommandUpdate() {
    if (g_IsSoundThreadEnabled) {
        return;
    }

    detail::driver::SoundThread::GetInstance().UpdateLowLevelVoices();
    detail::driver::SoundThread::GetInstance().EffectFrameProcess();
    HardwareManager::GetInstance().RequestUpdateAudioRenderer();
}

/**
 * @brief Read the size of one audio renderer performance frame.
 * @return Frame size in bytes.
 */
size_t SoundSystem::GetPerformanceFrameBufferSize() {
    return g_PerformanceFrameBufferSize / PerformanceFrameCount;
}

/**
 * @brief Read the number of voices the renderer dropped.
 * @return Dropped low-level voice count.
 */
int SoundSystem::GetDroppedLowLevelVoiceCount() {
    return HardwareManager::GetInstance().GetDroppedLowLevelVoiceCount();
}

/**
 * @brief Register the function warnings are reported to.
 * @param callback Function to call.
 */
void SoundSystem::SetWarningCallback(WarningCallback callback) {
    g_WarningCallback = callback;
}

/** @brief Remove the warning callback. */
void SoundSystem::ClearWarningCallback() {
    g_WarningCallback = nullptr;
}

/**
 * @brief Report a warning to the registered callback, if any.
 * @param id Warning identifier.
 * @param pInfo Warning details.
 */
void SoundSystem::CallWarningCallback(WarningId id, IWarningCallbackInfo* pInfo) {
    if (g_WarningCallback != nullptr) {
        WarningCallbackArg arg;
        arg.warningId = id;
        arg.pInfo = pInfo;
        g_WarningCallback(arg);
    }
}

/**
 * @brief Register a reader of sound thread update profiles.
 * @param rReader Reader to register.
 */
void SoundSystem::RegisterSoundThreadUpdateProfileReader(
    AtkProfileReader<SoundThreadUpdateProfile>& rReader) {
    detail::driver::SoundThread::GetInstance().RegisterSoundThreadUpdateProfileReader(rReader);
}

/**
 * @brief Unregister a reader of sound thread update profiles.
 * @param rReader Reader to unregister.
 */
void SoundSystem::UnregisterSoundThreadUpdateProfileReader(
    AtkProfileReader<SoundThreadUpdateProfile>& rReader) {
    detail::driver::SoundThread::GetInstance().UnregisterSoundThreadUpdateProfileReader(rReader);
}

/**
 * @brief Register a recorder of sound thread information.
 * @param rRecorder Recorder to register.
 */
void SoundSystem::RegisterSoundThreadInfoRecorder(detail::ThreadInfoRecorder& rRecorder) {
    detail::driver::SoundThread::GetInstance().RegisterSoundThreadInfoRecorder(rRecorder);
}

/**
 * @brief Unregister a recorder of sound thread information.
 * @param rRecorder Recorder to unregister.
 */
void SoundSystem::UnregisterSoundThreadInfoRecorder(detail::ThreadInfoRecorder& rRecorder) {
    detail::driver::SoundThread::GetInstance().UnregisterSoundThreadInfoRecorder(rRecorder);
}

/**
 * @brief Read samples from the user circular buffer sink.
 * @param pBuffer Destination buffer.
 * @param bufferSize Size of the destination buffer.
 * @return Number of bytes read; 0 when the sink is disabled.
 */
size_t SoundSystem::ReadCircularBufferSink(void* pBuffer, size_t bufferSize) {
    if (!g_IsCircularBufferSinkEnabled) {
        if (!g_IsCircularBufferSinkWarningDisplayed) {
            g_IsCircularBufferSinkWarningDisplayed = true;
        }

        return 0;
    }

    return HardwareManager::GetInstance().ReadUserCircularBufferSink(pBuffer, bufferSize);
}

/**
 * @brief Read the size of the user circular buffer sink.
 * @return Buffer size in bytes.
 */
size_t SoundSystem::GetCircularBufferSinkBufferSize() {
    return HardwareManager::GetInstance().GetUserCircularBufferSinkBufferSize();
}

/**
 * @brief Read the number of samples rendered per audio frame.
 * @return Renderer sample count.
 */
int SoundSystem::GetRendererSampleCount() {
    return HardwareManager::GetInstance().GetRendererSampleCount();
}

/**
 * @brief Read the maximum number of renderer output channels.
 * @return Channel count.
 */
int SoundSystem::GetRendererChannelCountMax() {
    return HardwareManager::GetInstance().GetChannelCountMax();
}

/** @brief Stop the user circular buffer sink if it exists. */
void SoundSystem::StopCircularBufferSink() {
    HardwareManager& rHardwareManager = HardwareManager::GetInstance();
    if (rHardwareManager.GetUserCircularBufferSinkState() !=
        HardwareManager::CircularBufferSinkState_Invalid) {
        rHardwareManager.StopUserCircularBufferSink();
    }
}

/** @brief Start the user circular buffer sink if it exists. */
void SoundSystem::StartCircularBufferSink() {
    HardwareManager& rHardwareManager = HardwareManager::GetInstance();
    if (rHardwareManager.GetUserCircularBufferSinkState() !=
        HardwareManager::CircularBufferSinkState_Invalid) {
        rHardwareManager.StartUserCircularBufferSink(false);
    }
}

/**
 * @brief Read the state of the user circular buffer sink.
 * @return Current sink state.
 */
SoundSystem::CircularBufferSinkState SoundSystem::GetCircularBufferSinkState() {
    return static_cast<CircularBufferSinkState>(
        HardwareManager::GetInstance().GetUserCircularBufferSinkState());
}
}  // namespace nn::atk
