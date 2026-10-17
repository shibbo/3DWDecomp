#pragma once

#include "container/seadOffsetList.h"
#include "thread/seadCriticalSection.h"

namespace aal {
class IAudioFrameProcess {
public:
    virtual ~IAudioFrameProcess() {}

    virtual void audioFrameProcess_() = 0;

    sead::ListNode mListNode;
};

class AudioFrameProcessMgr {
public:
    using ProcessList = sead::OffsetList<IAudioFrameProcess>;

    static AudioFrameProcessMgr* instance() { return sInstance; }

    AudioFrameProcessMgr();
    ~AudioFrameProcessMgr();

    void clearProcess();
    bool addProcess(IAudioFrameProcess* pProcess);
    void removeProcess(IAudioFrameProcess* pProcess);

private:
    bool registerAudioFrameCallback_();
    bool unregisterAudioFrameCallback_();
    void audioFrameProcess_();
    static void audioFrameCallback_(uintptr_t arg);

    static AudioFrameProcessMgr* sInstance;

    ProcessList mProcessList;
    sead::CriticalSection mCriticalSection;
    bool mIsCallbackRegistered = false;
};

static_assert(sizeof(AudioFrameProcessMgr) == 0x60);
}  // namespace aal
