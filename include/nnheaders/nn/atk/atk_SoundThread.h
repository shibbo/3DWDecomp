#pragma once

#include <nn/atk/atk_SoundStartable.h>
#include <nn/os/os_Mutex.h>
#include <nn/types.h>
#include <nn/util/util_IntrusiveList.h>

namespace nn::atk {
class AudioRendererPerformanceReader;
struct SoundThreadUpdateProfile;
template <typename T>
class AtkProfileReader;
namespace detail {
class ThreadInfoRecorder;
}  // namespace detail
}  // namespace nn::atk

namespace nn::atk::detail::driver {
class SoundThread {
  public:
    class SoundFrameCallback {
      public:
        /** @brief Constructs an unregistered sound-frame callback. */
        SoundFrameCallback() = default;
        /** @brief Destroys the callback without changing its registration. */
        virtual ~SoundFrameCallback() = default;
        virtual void OnBeginSoundFrame() = 0;
        virtual void OnEndSoundFrame();

      private:
        util::IntrusiveListNode mNode;
    };
    /** @brief Player hooked into the sound thread's per-frame update. */
    class PlayerCallback {
      public:
        /** @brief Constructs an unregistered player callback. */
        PlayerCallback() = default;
        /** @brief Destroys the callback without changing its registration. */
        virtual ~PlayerCallback() = default;
        virtual void OnUpdateFrameSoundThread(int frame) = 0;
        virtual void OnUpdateFrameSoundThreadWithAudioFrameFrequency(int frame) = 0;
        virtual void OnShutdownSoundThread() = 0;

      private:
        util::IntrusiveListNode mNode;
    };
    typedef void (*SoundFrameUserCallback)(uintptr_t arg);
    typedef void (*ThreadUserCallback)(uintptr_t arg);

    static SoundThread& GetInstance();
    void Initialize(void* pPerformanceFrameBuffer, size_t performanceFrameBufferSize,
                    bool isProfilerEnabled, bool isDetailSoundThreadProfilerEnabled,
                    bool isUserThreadRenderingEnabled);
    bool CreateSoundThread(int priority, void* pStack, size_t stackSize, int coreNumber,
                           u32 affinityMask);
    void Destroy();
    void Finalize();
    void FrameProcess(UpdateType updateType);
    void UpdateLowLevelVoices();
    void EffectFrameProcess();
    void RegisterThreadBeginUserCallback(ThreadUserCallback callback, uintptr_t arg);
    void ClearThreadBeginUserCallback();
    void RegisterThreadEndUserCallback(ThreadUserCallback callback, uintptr_t arg);
    void ClearThreadEndUserCallback();
    void RegisterAudioRendererPerformanceReader(AudioRendererPerformanceReader& rReader);
    void RegisterSoundThreadUpdateProfileReader(
        AtkProfileReader<SoundThreadUpdateProfile>& rReader);
    void UnregisterSoundThreadUpdateProfileReader(
        AtkProfileReader<SoundThreadUpdateProfile>& rReader);
    void RegisterSoundThreadInfoRecorder(ThreadInfoRecorder& rRecorder);
    void UnregisterSoundThreadInfoRecorder(ThreadInfoRecorder& rRecorder);
    void ForceWakeup();
    void RegisterPlayerCallback(PlayerCallback* pCallback);
    void UnregisterPlayerCallback(PlayerCallback* pCallback);

    void RegisterSoundFrameUserCallback(SoundFrameUserCallback callback, uintptr_t arg);
    void ClearSoundFrameUserCallback();

    /** @brief Acquire the sound thread's critical section (recursive). */
    void Lock() { m_CriticalSection.Lock(); }
    /** @brief Release the sound thread's critical section. */
    void Unlock() { m_CriticalSection.Unlock(); }

  private:
    // Thread, frame-callback and profiling state preceding the lock await reconstruction.
    u8 _0[0x348];
    os::Mutex m_CriticalSection;
};
} // namespace nn::atk::detail::driver
