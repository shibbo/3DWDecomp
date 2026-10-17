#include "aal/components/aalAudioFrameProcessMgr.h"

#include <nn/atk/atk_SoundThread.h>

namespace aal {
AudioFrameProcessMgr* AudioFrameProcessMgr::sInstance = nullptr;

/**
 * Constructs the manager and registers it as the global instance.
 */
AudioFrameProcessMgr::AudioFrameProcessMgr() {
    mProcessList.initOffset(offsetof(IAudioFrameProcess, mListNode));
    sInstance = this;
}

/**
 * Removes all processes and unregisters the global instance.
 */
AudioFrameProcessMgr::~AudioFrameProcessMgr() {
    clearProcess();
    sInstance = nullptr;
}

/**
 * Removes all processes and unregisters the sound frame callback.
 */
void AudioFrameProcessMgr::clearProcess() {
    mCriticalSection.lock();
    mProcessList.clear();

    if (mIsCallbackRegistered) {
        unregisterAudioFrameCallback_();
    }

    mCriticalSection.unlock();
}

/**
 * Adds a process, registering the sound frame callback if needed.
 * @param pProcess Process to add.
 * @return False if the process is already in a list.
 */
bool AudioFrameProcessMgr::addProcess(IAudioFrameProcess* pProcess) {
    if (mProcessList.isNodeLinked(pProcess)) {
        return false;
    }

    mCriticalSection.lock();

    if (!mIsCallbackRegistered) {
        registerAudioFrameCallback_();
    }

    mProcessList.pushBack(pProcess);
    mCriticalSection.unlock();
    return true;
}

/**
 * Registers the sound frame callback with the atk sound thread.
 * @return Always true.
 */
bool AudioFrameProcessMgr::registerAudioFrameCallback_() {
    nn::atk::detail::driver::SoundThread::GetInstance().RegisterSoundFrameUserCallback(
        audioFrameCallback_, reinterpret_cast<uintptr_t>(this));
    mIsCallbackRegistered = true;
    return true;
}

/**
 * Removes a process, unregistering the sound frame callback when none are left.
 * @param pProcess Process to remove.
 */
void AudioFrameProcessMgr::removeProcess(IAudioFrameProcess* pProcess) {
    if (!mProcessList.isNodeLinked(pProcess)) {
        return;
    }

    mCriticalSection.lock();
    mProcessList.erase(pProcess);

    if (mProcessList.size() == 0 && mIsCallbackRegistered) {
        unregisterAudioFrameCallback_();
    }

    mCriticalSection.unlock();
}

/**
 * Clears the sound frame callback of the atk sound thread.
 * @return Always true.
 */
bool AudioFrameProcessMgr::unregisterAudioFrameCallback_() {
    nn::atk::detail::driver::SoundThread::GetInstance().ClearSoundFrameUserCallback();
    mIsCallbackRegistered = false;
    return true;
}

/**
 * Runs every registered process.
 */
void AudioFrameProcessMgr::audioFrameProcess_() {
    mCriticalSection.lock();

    for (auto& process : mProcessList) {
        process.audioFrameProcess_();
    }

    mCriticalSection.unlock();
}

/**
 * Sound frame callback that runs the processes of a manager.
 * @param arg Manager passed at registration.
 */
void AudioFrameProcessMgr::audioFrameCallback_(uintptr_t arg) {
    if (arg) {
        reinterpret_cast<AudioFrameProcessMgr*>(arg)->audioFrameProcess_();
    }
}
}  // namespace aal
