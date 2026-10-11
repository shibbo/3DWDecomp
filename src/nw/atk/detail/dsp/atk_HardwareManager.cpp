#include <nn/atk/detail/dsp/atk_HardwareManager.h>

#include <nn/atk/atk_BiquadFilterPresets.h>
#include <nn/atk/atk_DeviceOutRecorder.h>
#include <nn/atk/atk_DriverCommand.h>
#include <nn/atk/atk_EffectAux.h>
#include <nn/atk/atk_MultiVoiceManager.h>
#include <nn/atk/atk_SoundThread.h>
#include <nn/atk/atkfnd_ScopedLock.h>
#include <nn/diag.h>
#include <cstring>

// Release-build forms of the SDK abort macros: the message strings are compiled out.
#define NN_ABORT()                                                                                 \
    do {                                                                                           \
        ::nn::diag::detail::AbortImpl("", "", "", 0);                                              \
        __builtin_unreachable();                                                                   \
    } while (false)

#define NN_ABORT_UNLESS(condition)                                                                 \
    do {                                                                                           \
        if (!(condition)) {                                                                        \
            NN_ABORT();                                                                            \
        }                                                                                          \
    } while (false)

#define NN_ABORT_UNLESS_RESULT_SUCCESS(expression)                                                 \
    do {                                                                                           \
        ::nn::Result abortResult = (expression);                                                   \
        if (abortResult.IsFailure()) {                                                             \
            ::nn::diag::detail::AbortImpl("", "", "", 0, &abortResult, "");                        \
            __builtin_unreachable();                                                               \
        }                                                                                          \
    } while (false)

namespace nn::audio {
R_DEFINE_NAMESPACE_RESULT_MODULE(153);
R_DEFINE_ERROR_RESULT(AudioRendererUpdateRejected, 41);
R_DEFINE_ERROR_RESULT(AudioRendererRenderingOverload, 42);
} // namespace nn::audio

namespace nn::atk::detail::driver {
namespace {
/** Input mix-buffer index for each output channel of a circular buffer sink. */
const s8 CircularBufferSinkChannelIndexTable[ChannelIndex_Count] = {0, 1, 2, 3, 4, 5};
/** Input mix-buffer index for each output channel of the main device sink. */
const s8 DeviceSinkChannelIndexTable[ChannelIndex_Count] = {0, 1, 4, 5, 2, 3};

const u32 AudioFrameMilliSeconds = 5;
const u32 RendererSampleRateDefault = 48000;
const size_t MemoryPoolAlignment = 0x1000;
const size_t PerformanceFrameBufferAlignment = 64;
const size_t SubMixBufferAlignment = 8;

const int DownMixCoefficientCount = 16;
const int DownMixFrontLeftToLeft = 0;
const int DownMixFrontRightToRight = 3;

const f32 SendVolumeMin = -128.0f;
const f32 SendVolumeMax = 128.0f;

BiquadFilterLpf BiquadFilterInstanceLpf;
BiquadFilterHpf BiquadFilterInstanceHpf;
BiquadFilterBpf512 BiquadFilterInstanceBpf512;
BiquadFilterBpf1024 BiquadFilterInstanceBpf1024;
BiquadFilterBpf2048 BiquadFilterInstanceBpf2048;
BiquadFilterLpfNw4fCompatible48k BiquadFilterInstanceLpfNw4fCompatible48k;
BiquadFilterHpfNw4fCompatible48k BiquadFilterInstanceHpfNw4fCompatible48k;
BiquadFilterBpf512Nw4fCompatible48k BiquadFilterInstanceBpf512Nw4fCompatible48k;
BiquadFilterBpf1024Nw4fCompatible48k BiquadFilterInstanceBpf1024Nw4fCompatible48k;
BiquadFilterBpf2048Nw4fCompatible48k BiquadFilterInstanceBpf2048Nw4fCompatible48k;

/**
 * @brief Round a size up to a power-of-two alignment.
 * @param size Size to round.
 * @param alignment Power-of-two alignment.
 * @return Smallest multiple of alignment not below size.
 */
inline size_t AlignUp(size_t size, size_t alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}

/**
 * @brief Round an address up to a power-of-two alignment.
 * @param pAddress Address to round.
 * @param alignment Power-of-two alignment.
 * @return First aligned address not below pAddress.
 */
inline void* AlignUp(void* pAddress, size_t alignment) {
    return reinterpret_cast<void*>(AlignUp(reinterpret_cast<uintptr_t>(pAddress), alignment));
}

/**
 * @brief Advance an address by a byte count.
 * @param pAddress Base address.
 * @param offset Byte offset.
 * @return pAddress plus offset.
 */
inline void* AddOffset(void* pAddress, size_t offset) {
    return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(pAddress) + offset);
}

/**
 * @brief Clamp a bus or channel send volume to its valid range.
 * @param volume Requested volume.
 * @return Volume limited to [SendVolumeMin, SendVolumeMax].
 */
inline f32 ClampSendVolume(f32 volume) {
    if (volume > SendVolumeMax) {
        return SendVolumeMax;
    }

    return volume < SendVolumeMin ? SendVolumeMin : volume;
}

/**
 * @brief Ask the driver to resynchronize every voice with changed global parameters.
 * @param syncFlag AllVoicesSyncFlag bits describing what changed.
 */
inline void PushAllVoicesSyncCommand(u32 syncFlag) {
    DriverCommand& rCommandManager = DriverCommand::GetInstance();
    auto* pCommand = reinterpret_cast<DriverCommandAllVoicesSync*>(
        rCommandManager.AllocMemory(sizeof(DriverCommandAllVoicesSync), true));
    pCommand->type = DriverCommandAllVoicesSync::Id;
    pCommand->syncFlag = syncFlag;
    rCommandManager.PushCommand(pCommand);
}

/** @brief Hold the sound thread's critical section for this scope. */
class SoundThreadLock {
  public:
    /** @brief Acquire the sound thread lock. */
    SoundThreadLock() { SoundThread::GetInstance().Lock(); }
    /** @brief Release the sound thread lock. */
    ~SoundThreadLock() { SoundThread::GetInstance().Unlock(); }
};
} // namespace

/** @brief Restore output modes, volumes and renderer settings to their defaults. */
void HardwareManager::ResetParameters() {
    m_SrcType = SampleRateConverterType_4Tap;

    for (int i = 0; i < OutputDevice_Count; i++) {
        m_OutputMode[i] = OutputMode_Count;
        m_EndUserOutputMode[i] = OutputMode_Count;
    }

    m_MasterVolume.InitValue(1.0f);
    m_VolumeForReset.InitValue(1.0f);

    audio::InitializeAudioRendererParameter(&m_RendererParameter);
    m_RendererParameter.sampleRate = RendererSampleRateDefault;
    m_RendererParameter.sampleCount = RendererSampleRateDefault * AudioFrameMilliSeconds / 1000;
    m_RendererParameter.mixBufferCount = 30;
    m_RendererParameter.subMixCount = 1;
    m_RendererParameter.voiceCount = 96;
    m_RendererParameter.sinkCount = 1;
    m_RendererParameter.effectCount = 14;
    m_RendererParameter.performanceFrameCount = 0;
    m_RendererParameter.isVoiceDropEnabled = false;

    NN_ABORT_UNLESS(audio::IsValidAudioRendererParameter(m_RendererParameter));
}

/**
 * @brief Query a memory pool's attach state under the renderer lock.
 * @param pPool Memory pool previously acquired from the renderer configuration.
 * @return Current client-side state of the pool.
 */
audio::MemoryPoolState HardwareManager::GetMemoryPoolState(audio::MemoryPoolType* pPool) {
    AudioRendererLock lock;
    return audio::GetMemoryPoolState(pPool);
}

/**
 * @brief Derive the renderer creation parameters from the manager parameters.
 * @param pParameter Destination renderer parameters.
 * @param rParameter Manager parameters to translate.
 */
void HardwareManager::SetupAudioRendererParameter(
    audio::AudioRendererParameter* pParameter, const HardwareManagerParameter& rParameter) const {
    audio::InitializeAudioRendererParameter(pParameter);
    pParameter->sampleRate = rParameter.rendererSampleRate;
    pParameter->sampleCount = rParameter.rendererSampleRate * AudioFrameMilliSeconds / 1000;
    pParameter->mixBufferCount = rParameter.subMixTotalChannelCount;
    pParameter->subMixCount = rParameter.subMixCount;
    pParameter->voiceCount = rParameter.voiceCount;
    pParameter->sinkCount = 1;
    pParameter->effectCount = rParameter.userEffectCount + 4;
    pParameter->performanceFrameCount = 0;
    pParameter->isVoiceDropEnabled = rParameter.enableVoiceDrop;

    if (rParameter.enableProfiler) {
        pParameter->performanceFrameCount = 3;
    }

    if (!rParameter.enableEffect) {
        pParameter->effectCount = 0;
    }

    if (rParameter.enableRecordingFinalOutputs) {
        pParameter->sinkCount++;
    }

    if (rParameter.enableUserCircularBufferSink) {
        pParameter->sinkCount++;
    }

    if (rParameter.enableManualRendering) {
        pParameter->renderingDevice = audio::AudioRendererRenderingDevice_Cpu;
        pParameter->executionMode = audio::AudioRendererExecutionMode_ManualExecution;
    }
}

/**
 * @brief Compute the work memory Initialize needs for the renderer and mixers.
 * @param rParameter Manager parameters the memory is sized for.
 * @return Required size in bytes.
 */
size_t HardwareManager::GetRequiredMemSize(const HardwareManagerParameter& rParameter) const {
    bool isEffectEnabled = rParameter.enableEffect;
    audio::AudioRendererParameter parameter;
    SetupAudioRendererParameter(&parameter, rParameter);

    size_t size = audio::GetAudioRendererWorkBufferSize(parameter);
    size += audio::GetAudioRendererConfigWorkBufferSize(parameter);
    size += FinalMix::GetRequiredMemorySize(isEffectEnabled);

    if (rParameter.enableSubMix) {
        int busCount = rParameter.enableEffect ? AuxBus_Count + 1 : 1;
        int channelCount = rParameter.enableStereoMode ? 2 : ChannelIndex_Count;
        bool isAdditionalSubMixEnabled = rParameter.enableAdditionalSubMix;
        size += SubMix::GetRequiredMemorySize(busCount, channelCount, 1, channelCount,
                                              isEffectEnabled, true);

        if (isAdditionalSubMixEnabled) {
            size += SubMix::GetRequiredMemorySize(1, channelCount, 1, channelCount,
                                                  isEffectEnabled, true);
            size += SubMixBufferAlignment;
        }

        if (rParameter.enableAdditionalEffectBus) {
            size += SubMix::GetRequiredMemorySize(AuxBus_Count, 2, 1, channelCount,
                                                  isEffectEnabled, true);
            size += SubMixBufferAlignment;
        }
    }

    return size;
}

/**
 * @brief Compute the memory-pool backed memory needed for low-level voices.
 * @param voiceCount Number of low-level voices.
 * @return Required size in bytes.
 */
size_t HardwareManager::GetRequiredMemSizeForMemoryPool(int voiceCount) const {
    return LowLevelVoiceAllocator::GetRequiredMemSize(voiceCount);
}

/**
 * @brief Compute the memory needed to record the final output.
 * @param rParameter Manager parameters the memory is sized for.
 * @return Size of the sink buffer (with its memory pool slack) plus a read buffer.
 */
size_t HardwareManager::GetRequiredRecorderWorkBufferSize(
    const HardwareManagerParameter& rParameter) const {
    return GetRequiredCircularBufferSinkWithMemoryPoolBufferSize(rParameter) +
           GetRequiredCircularBufferSinkBufferSize(rParameter);
}

/**
 * @brief Compute a circular buffer sink's size including memory pool alignment slack.
 * @param rParameter Manager parameters the memory is sized for.
 * @return Required size in bytes.
 */
size_t HardwareManager::GetRequiredCircularBufferSinkWithMemoryPoolBufferSize(
    const HardwareManagerParameter& rParameter) const {
    return AlignUp(GetRequiredCircularBufferSinkBufferSize(rParameter), MemoryPoolAlignment) +
           MemoryPoolAlignment;
}

/**
 * @brief Compute the sample buffer size of a circular buffer sink.
 * @param rParameter Manager parameters the memory is sized for.
 * @return Required size in bytes.
 */
size_t HardwareManager::GetRequiredCircularBufferSinkBufferSize(
    const HardwareManagerParameter& rParameter) const {
    audio::AudioRendererParameter parameter;
    SetupAudioRendererParameter(&parameter, rParameter);

    return audio::GetRequiredBufferSizeForCircularBufferSink(
        &parameter, GetChannelCountMax(), rParameter.recordingAudioFrameCount,
        audio::SampleFormat_PcmInt16);
}

/**
 * @brief Get the number of channels the final mix renders.
 * @return Two in stereo mode, otherwise six.
 */
int HardwareManager::GetChannelCountMax() const {
    return m_IsStereoModeEnabled ? 2 : ChannelIndex_Count;
}

/**
 * @brief Attach the recorder that receives the final output.
 * @param pRecorder Recorder to attach.
 * @return Whether the recorder was attached; false when one is already attached.
 */
bool HardwareManager::RegisterRecorder(DeviceOutRecorder* pRecorder) {
    SoundThreadLock lock;

    if (m_pRecorder != nullptr) {
        return false;
    }

    m_pRecorder = pRecorder;
    return true;
}

/**
 * @brief Detach the recorder that receives the final output.
 * @param pRecorder Recorder to detach; unused.
 */
void HardwareManager::UnregisterRecorder(DeviceOutRecorder* pRecorder) {
    SoundThreadLock lock;
    m_pRecorder = nullptr;
}

/** @brief Hand newly rendered final output to the attached recorder. */
void HardwareManager::UpdateRecorder() {
    if (m_pRecorder == nullptr) {
        return;
    }

    if (m_RecordingCircularBufferSinkState != CircularBufferSinkState_Started) {
        return;
    }

    if (m_IsRecordingCircularBufferSinkAllocated) {
        return;
    }

    void* pBuffer = m_pRecordingReadBuffer;
    size_t readSize = ReadRecordingCircularBufferSink(pBuffer, m_RecordingBufferSize);
    if (readSize == 0) {
        return;
    }

    m_pRecorder->RecordSamples(static_cast<const s16*>(pBuffer), readSize / sizeof(s16));
}

/**
 * @brief Read rendered samples from the recording sink.
 * @param pBuffer Destination buffer.
 * @param bufferSize Capacity of pBuffer in bytes.
 * @return Number of bytes read; zero while the sink is not started.
 */
size_t HardwareManager::ReadRecordingCircularBufferSink(void* pBuffer, size_t bufferSize) {
    if (m_RecordingCircularBufferSinkState != CircularBufferSinkState_Started) {
        return 0;
    }

    AudioRendererLock lock;
    return audio::ReadCircularBufferSink(&m_RecordingCircularBufferSink, pBuffer, bufferSize);
}

/**
 * @brief Hand the recording sink to the caller, bypassing the recorder.
 * @return The sink, or nullptr when it is not available or already handed out.
 */
audio::CircularBufferSinkType* HardwareManager::AllocateRecordingCircularBufferSink() {
    if (m_RecordingCircularBufferSinkState == CircularBufferSinkState_Invalid) {
        return nullptr;
    }

    if (m_IsRecordingCircularBufferSinkAllocated) {
        return nullptr;
    }

    m_IsRecordingCircularBufferSinkAllocated = true;
    return &m_RecordingCircularBufferSink;
}

/**
 * @brief Return the recording sink handed out by AllocateRecordingCircularBufferSink.
 * @param pSink Sink to return; unused.
 */
void HardwareManager::FreeRecordingCircularBufferSink(audio::CircularBufferSinkType* pSink) {
    m_IsRecordingCircularBufferSinkAllocated = false;
}

/** @brief Add the recording sink to the final mix. */
void HardwareManager::StartRecordingCircularBufferSink() {
    {
        AudioRendererLock lock;
        NN_ABORT_UNLESS_RESULT_SUCCESS(audio::AddCircularBufferSink(
            &m_Config, &m_RecordingCircularBufferSink, m_FinalMix.GetFinalMix(),
            CircularBufferSinkChannelIndexTable, GetChannelCountMax(), m_pRecordingBuffer,
            m_RecordingBufferSize, audio::SampleFormat_PcmInt16));
    }

    m_RecordingCircularBufferSinkState = CircularBufferSinkState_Started;
}

/** @brief Remove the user circular buffer sink from the final mix. */
void HardwareManager::StopUserCircularBufferSink() {
    if (m_UserCircularBufferSinkState != CircularBufferSinkState_Started) {
        return;
    }

    {
        AudioRendererLock lock;
        audio::RemoveCircularBufferSink(&m_Config, &m_UserCircularBufferSink,
                                        m_FinalMix.GetFinalMix());
    }

    m_UserCircularBufferSinkState = CircularBufferSinkState_Stopped;
}

/**
 * @brief Add the user circular buffer sink to the final mix.
 * @param isForceStart Start even when the sink was not stopped before.
 */
void HardwareManager::StartUserCircularBufferSink(bool isForceStart) {
    if (!isForceStart && m_UserCircularBufferSinkState != CircularBufferSinkState_Stopped) {
        return;
    }

    {
        AudioRendererLock lock;
        NN_ABORT_UNLESS_RESULT_SUCCESS(audio::AddCircularBufferSink(
            &m_Config, &m_UserCircularBufferSink, m_FinalMix.GetFinalMix(),
            CircularBufferSinkChannelIndexTable, GetChannelCountMax(), m_pUserCircularBuffer,
            m_UserCircularBufferSize, audio::SampleFormat_PcmInt16));
    }

    m_UserCircularBufferSinkState = CircularBufferSinkState_Started;
}

/**
 * @brief Read rendered samples from the user circular buffer sink.
 * @param pBuffer Destination buffer.
 * @param bufferSize Capacity of pBuffer in bytes.
 * @return Number of bytes read; zero while the sink is not started.
 */
size_t HardwareManager::ReadUserCircularBufferSink(void* pBuffer, size_t bufferSize) {
    if (m_UserCircularBufferSinkState != CircularBufferSinkState_Started) {
        return 0;
    }

    AudioRendererLock lock;
    return audio::ReadCircularBufferSink(&GetInstance().m_UserCircularBufferSink, pBuffer,
                                         bufferSize);
}

/**
 * @brief Register memory with the renderer and request it to be attached.
 * @param pPool Memory pool to acquire.
 * @param pAddress Start of the memory, aligned for memory pools.
 * @param size Size of the memory, aligned for memory pools.
 * @param waitAttach Poll until the renderer reports the pool attached.
 */
void HardwareManager::AttachMemoryPool(audio::MemoryPoolType* pPool, void* pAddress, size_t size,
                                       bool waitAttach) {
    {
        AudioRendererLock lock;
        audio::AcquireMemoryPool(&m_Config, pPool, pAddress, size);
        audio::RequestAttachMemoryPool(pPool);
    }

    if (waitAttach && m_IsMemoryPoolAttachCheckEnabled && !m_IsManualRenderingEnabled) {
        while (GetMemoryPoolState(pPool) != audio::MemoryPoolState_Attached) {
            os::SleepThread(TimeSpan::FromMilliSeconds(AudioFrameMilliSeconds));
        }
    } else {
        NN_ABORT_UNLESS_RESULT_SUCCESS(RequestUpdateAudioRenderer());
    }
}

/**
 * @brief Send the pending configuration to the renderer.
 * @return Update result; a rendering overload is ignored unless configured to abort.
 */
Result HardwareManager::RequestUpdateAudioRenderer() {
    AudioRendererLock lock;
    Result result = audio::RequestUpdateAudioRenderer(m_RendererHandle, &m_Config);
    m_RendererUpdateCount++;

    if (result.IsFailure()) {
        if (audio::ResultAudioRendererRenderingOverload::Includes(result)) {
            NN_ABORT_UNLESS(!m_IsRenderingOverloadAbortEnabled);

            result = ResultSuccess();
        } else if (audio::ResultAudioRendererUpdateRejected::Includes(result)) {
            NN_ABORT();
        }
    }

    return result;
}

/**
 * @brief Request a memory pool detach, wait for it and release the pool.
 * @param pPool Memory pool previously attached with AttachMemoryPool.
 * @param waitDetach Poll for the detach instead of driving renderer updates.
 */
void HardwareManager::DetachMemoryPool(audio::MemoryPoolType* pPool, bool waitDetach) {
    {
        AudioRendererLock lock;
        audio::RequestDetachMemoryPool(pPool);
    }

    if (waitDetach && m_IsMemoryPoolAttachCheckEnabled && !m_IsManualRenderingEnabled) {
        while (GetMemoryPoolState(pPool) != audio::MemoryPoolState_Detached) {
            os::SleepThread(TimeSpan::FromMilliSeconds(AudioFrameMilliSeconds));
        }
    } else if (m_IsManualRenderingEnabled) {
        SoundThreadLock soundThreadLock;
        AudioRendererLock lock;

        while (GetMemoryPoolState(pPool) != audio::MemoryPoolState_Detached) {
            NN_ABORT_UNLESS_RESULT_SUCCESS(RequestUpdateAudioRenderer());
            ExecuteAudioRendererRendering();
            WaitAudioRendererEvent();
        }
    } else {
        while (GetMemoryPoolState(pPool) != audio::MemoryPoolState_Detached) {
            NN_ABORT_UNLESS_RESULT_SUCCESS(RequestUpdateAudioRenderer());
            WaitAudioRendererEvent();
        }
    }

    AudioRendererLock lock;
    audio::ReleaseMemoryPool(&m_Config, pPool);

    if (m_IsManualRenderingEnabled) {
        SoundThreadLock soundThreadLock;
        ExecuteAudioRendererRendering();
    }
}

/** @brief Render one audio frame when the renderer runs in manual execution mode. */
void HardwareManager::ExecuteAudioRendererRendering() {
    NN_ABORT_UNLESS_RESULT_SUCCESS(audio::ExecuteAudioRendererRendering(m_RendererHandle));
}

/** @brief Block until the renderer signals that a frame was rendered. */
void HardwareManager::WaitAudioRendererEvent() {
    os::WaitSystemEvent(&m_RendererEvent.m_SystemEventType);
}

/**
 * @brief Get the number of low-level voices the renderer dropped.
 * @return Dropped voice count.
 */
int HardwareManager::GetDroppedLowLevelVoiceCount() const {
    return m_VoiceAllocator.GetDroppedVoiceCount();
}

/**
 * @brief Get the number of audio frames the renderer has processed.
 * @return Elapsed audio frame count.
 */
s64 HardwareManager::GetElapsedAudioFrameCount() const {
    return audio::GetAudioRendererElapsedFrameCount(&m_Config);
}

/**
 * @brief Compute the buffer size for renderer performance frames.
 * @param rParameter Manager parameters the memory is sized for.
 * @return Required size in bytes.
 */
size_t HardwareManager::GetRequiredPerformanceFramesBufferSize(
    const HardwareManagerParameter& rParameter) const {
    audio::AudioRendererParameter parameter;
    SetupAudioRendererParameter(&parameter, rParameter);

    return AlignUp(audio::GetRequiredBufferSizeForPerformanceFrames(parameter),
                   PerformanceFrameBufferAlignment);
}

/**
 * @brief Open the renderer and build the final mix, sinks and built-in submixes.
 * @param pRendererBuffer Work memory sized by GetRequiredMemSize.
 * @param rendererBufferSize Size of pRendererBuffer; unused.
 * @param pVoiceBuffer Memory-pool memory sized by GetRequiredMemSizeForMemoryPool.
 * @param voiceBufferSize Size of pVoiceBuffer; unused.
 * @param pUserCircularBuffer Memory for the user circular buffer sink.
 * @param userCircularBufferSize Size of pUserCircularBuffer; unused.
 * @param rParameter Manager parameters.
 * @return Success, the renderer's open failure, or an already-initialized error.
 */
Result HardwareManager::Initialize(void* pRendererBuffer, size_t rendererBufferSize,
                                   void* pVoiceBuffer, size_t voiceBufferSize,
                                   void* pUserCircularBuffer, size_t userCircularBufferSize,
                                   const HardwareManagerParameter& rParameter) {
    if (m_IsInitialized) {
        return result::detail::ConstructResult(721);
    }

    m_IsEffectEnabled = rParameter.enableEffect;
    m_IsSubMixEnabled = rParameter.enableSubMix;
    m_IsAdditionalEffectBusEnabled = rParameter.enableAdditionalEffectBus;
    m_IsAdditionalSubMixEnabled = rParameter.enableAdditionalSubMix;
    m_IsStereoModeEnabled = rParameter.enableStereoMode;
    m_IsRenderingOverloadAbortEnabled = rParameter.enableRenderingOverloadAbort;

    SetupAudioRendererParameter(&m_RendererParameter, rParameter);
    NN_ABORT_UNLESS(audio::IsValidAudioRendererParameter(m_RendererParameter));

    size_t rendererWorkSize = audio::GetAudioRendererWorkBufferSize(m_RendererParameter);
    m_pRendererWorkBuffer = pRendererBuffer;
    Result result = audio::OpenAudioRenderer(&m_RendererHandle, &m_RendererEvent,
                                             m_RendererParameter, pRendererBuffer,
                                             rendererWorkSize);
    if (result.IsFailure()) {
        m_pRendererWorkBuffer = nullptr;
        return result;
    }

    m_RendererUpdateCount = 0;

    void* pConfigBuffer = AddOffset(pRendererBuffer, rendererWorkSize);
    size_t configWorkSize = audio::GetAudioRendererConfigWorkBufferSize(m_RendererParameter);
    void* pCurrent = AddOffset(pConfigBuffer, configWorkSize);
    m_pConfigWorkBuffer = pConfigBuffer;
    audio::InitializeAudioRendererConfig(&m_Config, m_RendererParameter, pConfigBuffer,
                                         configWorkSize);

    bool isEffectEnabled = m_IsEffectEnabled;
    size_t finalMixSize = FinalMix::GetRequiredMemorySize(isEffectEnabled);
    m_FinalMix.Initialize(&m_Config, GetChannelCountMax(), isEffectEnabled, pCurrent, finalMixSize);

    if (!rParameter.enableManualRendering) {
        audio::AddDeviceSink(&m_Config, &m_DeviceSink, m_FinalMix.GetFinalMix(),
                             DeviceSinkChannelIndexTable, GetChannelCountMax(), "MainAudioOut");

        if (rParameter.enableCompatibleDownMixSetting) {
            // Down-mix to stereo by passing the front left and right channels straight through.
            audio::DeviceSinkType::DownMixParameter downMixParameter;
            for (int i = 0; i < DownMixCoefficientCount; i++) {
                bool isPassThrough = i == DownMixFrontLeftToLeft || i == DownMixFrontRightToRight;
                downMixParameter.coefficient[i] = isPassThrough ? 1.0f : 0.0f;
            }

            audio::SetDownMixParameter(&m_DeviceSink, &downMixParameter);
            audio::SetDownMixParameterEnabled(&m_DeviceSink, true);
        } else {
            audio::SetDownMixParameterEnabled(&m_DeviceSink, false);
        }
    }

    pCurrent = AddOffset(pCurrent, finalMixSize);

    if (rParameter.enableRecordingFinalOutputs) {
        void* pBuffer = AlignUp(pCurrent, MemoryPoolAlignment);
        m_RecordingBufferSize = audio::GetRequiredBufferSizeForCircularBufferSink(
            &m_RendererParameter, GetChannelCountMax(), rParameter.recordingAudioFrameCount,
            audio::SampleFormat_PcmInt16);
        size_t poolSize = AlignUp(m_RecordingBufferSize, MemoryPoolAlignment);
        m_pRecordingBuffer = pBuffer;
        AttachMemoryPool(&m_RecordingMemoryPool, pBuffer, poolSize, false);
        StartRecordingCircularBufferSink();

        m_pRecordingReadBuffer = AddOffset(pBuffer, poolSize);
        pCurrent = AddOffset(m_pRecordingReadBuffer, m_RecordingBufferSize);
    }

    if (rParameter.enableUserCircularBufferSink) {
        void* pBuffer = AlignUp(pUserCircularBuffer, MemoryPoolAlignment);
        m_UserCircularBufferSize = audio::GetRequiredBufferSizeForCircularBufferSink(
            &m_RendererParameter, GetChannelCountMax(), rParameter.recordingAudioFrameCount,
            audio::SampleFormat_PcmInt16);
        m_pUserCircularBuffer = pBuffer;
        AttachMemoryPool(&m_UserMemoryPool, pBuffer,
                         AlignUp(m_UserCircularBufferSize, MemoryPoolAlignment), false);
        StartUserCircularBufferSink(true);
    }

    _956 = rParameter._23;
    m_IsMemoryPoolAttachCheckEnabled = rParameter.enableMemoryPoolAttachCheck;
    m_IsAutoEffectBusMuteEnabled = rParameter.enableAutoEffectBusMute;
    _a65 = rParameter._26;
    m_IsManualRenderingEnabled = rParameter.enableManualRendering;
    _958 = rParameter._2a;

    NN_ABORT_UNLESS_RESULT_SUCCESS(RequestUpdateAudioRenderer());

    size_t voiceAllocatorSize = LowLevelVoiceAllocator::GetRequiredMemSize(rParameter.voiceCount);
    m_VoiceAllocator.Initialize(rParameter.voiceCount, pVoiceBuffer, voiceAllocatorSize);
    audio::StartAudioRenderer(m_RendererHandle);
    m_RendererSuspendCount = 0;

    std::memset(m_BiquadFilterCallbackTable, 0, sizeof(m_BiquadFilterCallbackTable));
    m_BiquadFilterCallbackTable[1] = &BiquadFilterInstanceLpf;
    m_BiquadFilterCallbackTable[2] = &BiquadFilterInstanceHpf;
    m_BiquadFilterCallbackTable[3] = &BiquadFilterInstanceBpf512;
    m_BiquadFilterCallbackTable[4] = &BiquadFilterInstanceBpf1024;
    m_BiquadFilterCallbackTable[5] = &BiquadFilterInstanceBpf2048;
    m_BiquadFilterCallbackTable[6] = &BiquadFilterInstanceLpfNw4fCompatible48k;
    m_BiquadFilterCallbackTable[7] = &BiquadFilterInstanceHpfNw4fCompatible48k;
    m_BiquadFilterCallbackTable[8] = &BiquadFilterInstanceBpf512Nw4fCompatible48k;
    m_BiquadFilterCallbackTable[9] = &BiquadFilterInstanceBpf1024Nw4fCompatible48k;
    m_BiquadFilterCallbackTable[10] = &BiquadFilterInstanceBpf2048Nw4fCompatible48k;

    m_EndUserOutputMode[OutputDevice_Main] = OutputMode_Surround;
    SetOutputMode(OutputMode_Surround, OutputDevice_Main);

    m_OutputDeviceFlag[0] = 1;
    for (int i = 1; i < OutputLineCount; i++) {
        m_OutputDeviceFlag[i] = 0;
    }

    m_IsInitialized = true;

    if (m_IsSubMixEnabled) {
        int busCount = rParameter.enableEffect ? AuxBus_Count + 1 : 1;
        int channelCount = GetChannelCountMax();
        bool isUnusedEffectChannelMuted = rParameter.enableUnusedEffectChannelMuting;
        bool isAdditionalSubMixEnabled = rParameter.enableAdditionalSubMix;

        size_t subMixSize = SubMix::GetRequiredMemorySize(busCount, channelCount, 1, channelCount,
                                                          isEffectEnabled, true);
        m_SubMix[0].Initialize(busCount, channelCount, 1, channelCount, isEffectEnabled, true,
                               pCurrent, subMixSize);
        m_SubMix[0].SetMuteUnusedEffectChannel(isUnusedEffectChannelMuted);
        pCurrent = AddOffset(pCurrent, subMixSize);

        if (isAdditionalSubMixEnabled) {
            pCurrent = AlignUp(pCurrent, SubMixBufferAlignment);
            subMixSize = SubMix::GetRequiredMemorySize(1, channelCount, 1, channelCount,
                                                       isEffectEnabled, true);
            m_SubMix[2].Initialize(1, channelCount, 1, channelCount, isEffectEnabled, true,
                                   pCurrent, subMixSize);
            m_SubMix[2].SetMuteUnusedEffectChannel(isUnusedEffectChannelMuted);
            pCurrent = AddOffset(pCurrent, subMixSize);

            m_SubMix[0].SetDestination(&m_SubMix[2]);
            m_SubMix[2].SetDestination(&m_FinalMix);
            for (int bus = 0; bus < busCount; bus++) {
                m_SubMix[0].SetSend(bus, 0, 1.0f);
            }

            m_SubMix[2].SetSend(0, 0, 1.0f);
        } else {
            m_SubMix[0].SetDestination(&m_FinalMix);
            for (int bus = 0; bus < busCount; bus++) {
                m_SubMix[0].SetSend(bus, 0, 1.0f);
            }
        }

        if (rParameter.enableAdditionalEffectBus) {
            pCurrent = AlignUp(pCurrent, SubMixBufferAlignment);
            subMixSize = SubMix::GetRequiredMemorySize(AuxBus_Count, 2, 1, channelCount,
                                                       isEffectEnabled, true);
            m_SubMix[1].Initialize(AuxBus_Count, 2, 1, channelCount, isEffectEnabled, true,
                                   pCurrent, subMixSize);
            m_SubMix[1].SetMuteUnusedEffectChannel(isUnusedEffectChannelMuted);
            m_SubMix[1].SetDestination(&m_FinalMix);
            for (int bus = 0; bus < AuxBus_Count; bus++) {
                m_SubMix[1].SetSend(bus, 0, 1.0f);
            }
        }
    }

    return ResultSuccess();
}

/**
 * @brief Install the coefficient callback for a biquad filter type.
 * @param type Filter type; zero (no filter) and negative types are ignored.
 * @param pCallback Callback to install.
 */
void HardwareManager::SetBiquadFilterCallback(int type, const BiquadFilterCallback* pCallback) {
    if (type < 1) {
        return;
    }

    m_BiquadFilterCallbackTable[type] = pCallback;
}

/**
 * @brief Change the output mode of a device and resynchronize voices and effects.
 * @param mode New output mode.
 * @param device Output device to change.
 */
void HardwareManager::SetOutputMode(OutputMode mode, OutputDevice device) {
    if (m_OutputMode[device] == mode) {
        return;
    }

    m_OutputMode[device] = mode;
    PushAllVoicesSyncCommand(AllVoicesSyncFlag_OutputMode);

    if (!m_IsEffectEnabled) {
        return;
    }

    {
        SubMixListLock lock;
        for (auto& rSubMix : m_SubMixList) {
            rSubMix.OnChangeOutputMode();
        }
    }

    m_FinalMix.OnChangeOutputMode();
}

/**
 * @brief Record the output mode chosen by the end user.
 * @param mode Output mode from the system settings.
 */
void HardwareManager::SetEndUserOutputMode(OutputMode mode) {
    m_EndUserOutputMode[OutputDevice_Main] = mode;
}

/** @brief Apply the end user's output mode; nothing to do on this platform. */
void HardwareManager::UpdateEndUserOutputMode() {}

/** @brief Tear down mixers and renderer and restore the default parameters. */
void HardwareManager::Finalize() {
    if (!m_IsInitialized) {
        return;
    }

    m_VoiceAllocator.Finalize();
    audio::StopAudioRenderer(m_RendererHandle);

    if (m_IsEffectEnabled) {
        if (m_IsSubMixEnabled) {
            for (int bus = 0; bus < m_SubMix[0].GetBusCount(); bus++) {
                m_SubMix[0].ClearEffectImpl(bus);
            }

            if (m_IsAdditionalEffectBusEnabled) {
                for (int bus = 0; bus < m_SubMix[1].GetBusCount(); bus++) {
                    m_SubMix[1].ClearEffectImpl(bus);
                }
            }

            if (m_IsAdditionalSubMixEnabled) {
                m_SubMix[2].ClearEffectImpl(0);
            }

            m_IsEffectEnabled = false;
        }

        m_FinalMix.ClearEffectImpl(0);
    }

    m_SubMix[0].Finalize();
    m_SubMix[1].Finalize();
    m_SubMix[2].Finalize();
    m_SubMixList.clear();

    NN_ABORT_UNLESS_RESULT_SUCCESS(RequestUpdateAudioRenderer());
    m_FinalMix.Finalize(&m_Config);
    audio::CloseAudioRenderer(m_RendererHandle);
    os::DestroySystemEvent(&m_RendererEvent.m_SystemEventType);

    if (m_RecordingCircularBufferSinkState != CircularBufferSinkState_Invalid) {
        m_RecordingCircularBufferSinkState = CircularBufferSinkState_Invalid;
    }

    ResetParameters();

    m_UserCircularBufferSinkState = CircularBufferSinkState_Invalid;
    m_IsMemoryPoolAttachCheckEnabled = true;
    m_IsInitialized = false;
    m_IsSubMixEnabled = true;
    m_IsAdditionalEffectBusEnabled = false;
    m_IsAdditionalSubMixEnabled = false;
    m_IsStereoModeEnabled = false;
    _956 = false;
    m_IsRenderingOverloadAbortEnabled = false;
    m_IsAutoEffectBusMuteEnabled = false;
    _a65 = false;
    m_IsManualRenderingEnabled = false;
}

/**
 * @brief Advance submixes and volume fades by the elapsed audio frames.
 * @param audioFrameCount Number of elapsed audio frames.
 */
void HardwareManager::Update(int audioFrameCount) {
    fnd::ScopedMutexLock lock(m_UpdateMutex);

    if (m_IsEffectEnabled && m_IsSubMixEnabled && m_IsAutoEffectBusMuteEnabled) {
        for (int bus = 1; bus <= AuxBus_Count; bus++) {
            m_SubMix[0].SetBusMute(bus, !m_SubMix[0].HasEffect(bus));
        }

        if (m_IsAdditionalEffectBusEnabled) {
            for (int bus = 1; bus < AuxBus_Count; bus++) {
                m_SubMix[1].SetBusMute(bus, !m_SubMix[1].HasEffect(bus));
            }
        }
    }

    {
        SubMixListLock subMixListLock;
        for (auto& rSubMix : m_SubMixList) {
            rSubMix.Update(audioFrameCount);
        }
    }

    if (!m_MasterVolume.IsFinished()) {
        m_MasterVolume.Update(audioFrameCount);
        MultiVoiceManager::GetInstance().UpdateAllVoicesSync(AllVoicesSyncFlag_Volume);
    }

    if (!m_VolumeForReset.IsFinished()) {
        m_VolumeForReset.Update(audioFrameCount);
        MultiVoiceManager::GetInstance().UpdateAllVoicesSync(AllVoicesSyncFlag_Volume);
    }
}

/** @brief Feed rendered samples to the auxiliary effects of every mixer. */
void HardwareManager::UpdateEffect() {
    if (!m_IsEffectEnabled) {
        return;
    }

    {
        SubMixListLock lock;
        for (auto& rSubMix : m_SubMixList) {
            rSubMix.UpdateEffectAux();
        }
    }

    m_FinalMix.UpdateEffectAux();
}

/** @brief Stop the renderer; nests with ResumeAudioRenderer. */
void HardwareManager::SuspendAudioRenderer() {
    AudioRendererLock lock;

    if (m_RendererSuspendCount == 0) {
        audio::StopAudioRenderer(m_RendererHandle);
    }

    m_RendererSuspendCount++;
}

/** @brief Restart the renderer once every suspension was resumed. */
void HardwareManager::ResumeAudioRenderer() {
    if (m_RendererSuspendCount == 0) {
        return;
    }

    AudioRendererLock lock;

    m_RendererSuspendCount--;
    if (m_RendererSuspendCount == 0) {
        NN_ABORT_UNLESS_RESULT_SUCCESS(audio::StartAudioRenderer(m_RendererHandle));
    }
}

/**
 * @brief Wait for the renderer to signal a rendered frame, with a timeout.
 * @param timeout Maximum time to wait.
 * @return Whether the event was signaled before the timeout.
 */
bool HardwareManager::TimedWaitAudioRendererEvent(nn::TimeSpan timeout) {
    return os::TimedWaitSystemEvent(&m_RendererEvent.m_SystemEventType, timeout);
}

/**
 * @brief Limit the renderer's processing time per frame.
 * @param limitPercent Limit as a percentage of the frame time.
 * @return Result of the SDK call.
 */
Result HardwareManager::SetAudioRendererRenderingTimeLimit(int limitPercent) {
    AudioRendererLock lock;
    return audio::SetAudioRendererRenderingTimeLimit(m_RendererHandle, limitPercent);
}

/**
 * @brief Get the renderer's processing time limit per frame.
 * @return Limit as a percentage of the frame time.
 */
int HardwareManager::GetAudioRendererRenderingTimeLimit() {
    AudioRendererLock lock;
    return audio::GetAudioRendererRenderingTimeLimit(m_RendererHandle);
}

/** @brief Fade the output out ahead of a reset. */
void HardwareManager::PrepareReset() {
    m_VolumeForReset.SetTarget(0.0f, 3);
}

/**
 * @brief Check whether the manager may be reset.
 * @return Always true.
 */
bool HardwareManager::IsResetReady() const {
    return true;
}

/**
 * @brief Register a user submix for updates.
 * @param pSubMix Submix to append to the list.
 */
void HardwareManager::AddSubMix(SubMix* pSubMix) {
    SubMixListLock lock;
    m_SubMixList.push_back(*pSubMix);
}

/**
 * @brief Unregister a user submix.
 * @param pSubMix Submix to remove; ignored when not registered.
 */
void HardwareManager::RemoveSubMix(SubMix* pSubMix) {
    SubMixListLock lock;

    auto it = m_SubMixList.begin();
    for (; it != m_SubMixList.end(); ++it) {
        if (&*it == pSubMix) {
            break;
        }
    }

    m_SubMixList.erase(it);
}

/**
 * @brief Get a submix by index.
 * @param index Built-in submix index, or position in the user submix list.
 * @return The submix.
 */
SubMix* HardwareManager::GetSubMix(int index) {
    if (!m_IsSubMixEnabled) {
        SubMixListLock lock;

        int i = 0;
        for (auto& rSubMix : m_SubMixList) {
            if (i == index) {
                return &rSubMix;
            }

            i++;
        }
    }

    return &m_SubMix[index];
}

/**
 * @brief Get a submix by index.
 * @param index Built-in submix index, or position in the user submix list.
 * @return The submix.
 */
const SubMix* HardwareManager::GetSubMix(int index) const {
    if (!m_IsSubMixEnabled) {
        SubMixListLock lock;

        int i = 0;
        for (const auto& rSubMix : m_SubMixList) {
            if (i == index) {
                return &rSubMix;
            }

            i++;
        }
    }

    return &m_SubMix[index];
}

/**
 * @brief Get the number of registered user submixes.
 * @return Submix count.
 */
int HardwareManager::GetSubMixCount() const {
    return m_SubMixList.size();
}

/**
 * @brief Get the channel count of the main device's current output mode.
 * @return Number of output channels.
 */
int HardwareManager::GetChannelCount() const {
    switch (GetInstance().GetOutputMode(OutputDevice_Main)) {
    case OutputMode_Monaural:
        return 1;
    case OutputMode_Stereo:
        return 2;
    case OutputMode_Surround:
        return GetChannelCountMax();
    default:
        return 2;
    }
}

/**
 * @brief Get the overall output gain.
 * @return Master volume multiplied by the reset fade volume.
 */
f32 HardwareManager::GetOutputVolume() const {
    return m_MasterVolume.GetValue() * m_VolumeForReset.GetValue();
}

/**
 * @brief Set the routing flag of an output line.
 * @param outputLineIndex Output line, from 1 to 31; line 0 is fixed.
 * @param flag New flag value.
 */
void HardwareManager::SetOutputDeviceFlag(u32 outputLineIndex, u8 flag) {
    if (outputLineIndex < 1 || outputLineIndex >= OutputLineCount) {
        return;
    }

    if (m_OutputDeviceFlag[outputLineIndex] == flag) {
        return;
    }

    m_OutputDeviceFlag[outputLineIndex] = flag;
    PushAllVoicesSyncCommand(AllVoicesSyncFlag_OutputMode);
}

/**
 * @brief Fade the master volume.
 * @param volume Target volume; negative values are clamped to zero.
 * @param fadeTimes Fade duration in milliseconds.
 */
void HardwareManager::SetMasterVolume(f32 volume, int fadeTimes) {
    if (volume < 0.0f) {
        volume = 0.0f;
    }

    m_MasterVolume.SetTarget(volume,
                             (fadeTimes + AudioFrameMilliSeconds - 1) / AudioFrameMilliSeconds);

    if (fadeTimes == 0) {
        PushAllVoicesSyncCommand(AllVoicesSyncFlag_Volume);
    }
}

/**
 * @brief Change the sample rate converter used by voices.
 * @param type New converter type.
 */
void HardwareManager::SetSrcType(SampleRateConverterType type) {
    if (m_SrcType == type) {
        return;
    }

    m_SrcType = type;
    PushAllVoicesSyncCommand(AllVoicesSyncFlag_SrcType);
}

/**
 * @brief Compute the buffer size an auxiliary effect needs with the current renderer.
 * @param pEffect Effect to size.
 * @return Required size in bytes.
 */
size_t HardwareManager::GetRequiredEffectAuxBufferSize(const EffectAux* pEffect) const {
    return pEffect->GetRequiredMemSize(m_RendererParameter);
}

/**
 * @brief Fade the volume of an auxiliary bus.
 * @param bus Auxiliary bus.
 * @param volume Target volume, clamped to the send volume range.
 * @param fadeFrames Fade duration in audio frames.
 * @param subMixIndex Zero for the main submix, otherwise the additional effect submix.
 */
void HardwareManager::SetAuxBusVolume(AuxBus bus, f32 volume, int fadeFrames, int subMixIndex) {
    volume = ClampSendVolume(volume);

    if (subMixIndex != 0) {
        m_SubMix[1].SetBusVolume(bus + 1, volume, fadeFrames);
    } else {
        m_SubMix[0].SetBusVolume(bus + 1, volume, fadeFrames);
    }

    if (fadeFrames == 0) {
        m_SubMix[subMixIndex].UpdateBusMixVolume(bus + 1);
    }
}

/**
 * @brief Get the current volume of an auxiliary bus.
 * @param bus Auxiliary bus.
 * @param subMixIndex Zero for the main submix, otherwise the additional effect submix.
 * @return Current bus volume.
 */
f32 HardwareManager::GetAuxBusVolume(AuxBus bus, int subMixIndex) const {
    fnd::ScopedMutexLock lock(m_UpdateMutex);
    const SubMix& rSubMix = subMixIndex != 0 ? m_SubMix[1] : m_SubMix[0];
    return rSubMix.GetBusVolume(bus + 1);
}

/**
 * @brief Set a main-bus channel send of the additional effect submix.
 * @param volume Send volume, clamped to the send volume range.
 * @param sourceChannel Source channel.
 * @param destinationChannel Destination channel.
 */
void HardwareManager::SetMainBusChannelVolumeForAdditionalEffect(f32 volume, int sourceChannel,
                                                                 int destinationChannel) {
    volume = ClampSendVolume(volume);
    m_SubMix[1].SetSendImpl(0, sourceChannel, 0, destinationChannel, volume);
}

/**
 * @brief Get a main-bus channel send of the additional effect submix.
 * @param sourceChannel Source channel.
 * @param destinationChannel Destination channel.
 * @return Send volume.
 */
f32 HardwareManager::GetMainBusChannelVolumeForAdditionalEffect(int sourceChannel,
                                                                int destinationChannel) const {
    return m_SubMix[1].GetSendImpl(0, sourceChannel, 0, destinationChannel);
}

/**
 * @brief Set an auxiliary-bus channel send of the additional effect submix.
 * @param bus Auxiliary bus.
 * @param volume Send volume, clamped to the send volume range.
 * @param sourceChannel Source channel.
 * @param destinationChannel Destination channel.
 */
void HardwareManager::SetAuxBusChannelVolumeForAdditionalEffect(AuxBus bus, f32 volume,
                                                                int sourceChannel,
                                                                int destinationChannel) {
    volume = ClampSendVolume(volume);
    m_SubMix[1].SetSendImpl(bus + 1, sourceChannel, 0, destinationChannel, volume);
}

/**
 * @brief Get an auxiliary-bus channel send of the additional effect submix.
 * @param bus Auxiliary bus.
 * @param sourceChannel Source channel.
 * @param destinationChannel Destination channel.
 * @return Send volume.
 */
f32 HardwareManager::GetAuxBusChannelVolumeForAdditionalEffect(AuxBus bus, int sourceChannel,
                                                               int destinationChannel) const {
    return m_SubMix[1].GetSendImpl(bus + 1, sourceChannel, 0, destinationChannel);
}

/**
 * @brief Flush CPU caches for memory shared with the renderer; not needed on this platform.
 * @param pAddress Start of the memory.
 * @param size Size of the memory.
 */
void HardwareManager::FlushDataCache(void* pAddress, size_t size) {}
} // namespace nn::atk::detail::driver
