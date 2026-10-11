#pragma once

#include <basis/seadTypes.h>

#include "Project/Audio/IUseAudioKeeper.hpp"

namespace al {
class ActorInitInfo;

/**
 * Plays the sound effects that make up the "Chika Chika" BGM in sync with the beat.
 */
class ChikaChikaBgmSequencer : public IUseAudioKeeper {
public:
    ChikaChikaBgmSequencer();

    void init(ActorInitInfo& rInfo);
    void update(s32 frame, s32 measure);
    void updateSceneStop();
    void stopAll();
    void setBeatCount(bool enabled) { mIsBeatCount = enabled; }

    AudioKeeper* getAudioKeeper() const override { return mAudioKeeper; }

private:
    AudioKeeper* mAudioKeeper = nullptr;
    bool mIsPlaying = false;
    s32 mStopDelayFrame = -1;
    bool mIsBeatCount = false;
};

static_assert(sizeof(ChikaChikaBgmSequencer) == 0x20);
}  // namespace al
