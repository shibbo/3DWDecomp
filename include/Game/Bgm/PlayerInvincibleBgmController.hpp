#pragma once

#include "Project/Audio/IUseAudioKeeper.hpp"

namespace al {
class AudioDirector;
}

class PlayerInvincibleBgmController : public al::IUseAudioKeeper {
public:
    explicit PlayerInvincibleBgmController(al::AudioDirector* pDirector);
    void update();
    void pause();
    void resume();
    void notifyStart(bool isShared);
    void notifyEnd(bool isCancelRestart);
    void notifyGet();
    al::AudioKeeper* getAudioKeeper() const override;

private:
    al::AudioKeeper* mAudioKeeper = nullptr;
    s32 mPlayerCount = 0;
    bool mIsStartRequested = false;
    bool mIsPlaying = false;
    bool mIsPauseDisabled = false;
    s32 mSavedSamplePosition = -1;
    bool mIsCancelRestart = false;
};

static_assert(sizeof(PlayerInvincibleBgmController) == 0x20);
