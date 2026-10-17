#pragma once

#include "Project/Audio/IUseAudioKeeper.hpp"

namespace al {
class AudioDirector;
}

class PlayerBigBgmController : public al::IUseAudioKeeper {
public:
    explicit PlayerBigBgmController(al::AudioDirector* pDirector);
    void update();
    void notifyStart();
    void notifyEnd();
    void notifyGet();
    al::AudioKeeper* getAudioKeeper() const override;

private:
    al::AudioKeeper* mAudioKeeper = nullptr;
    s32 _10 = 0;
    s32 mPlayerCount = 0;
    s32 mTimer = 0;
    bool mIsStartRequested = false;
    bool mIsActive = false;
    s32 mSituationIndex = 0;
};

static_assert(sizeof(PlayerBigBgmController) == 0x28);
