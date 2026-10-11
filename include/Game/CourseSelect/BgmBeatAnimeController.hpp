#pragma once

#include <basis/seadTypes.h>

#include "Library/Nerve/NerveExecutor.hpp"

namespace al {
class LiveActor;
}

/**
 * @brief Drives an actor's skeletal / material animations in sync with the beat of the
 * currently playing BGM (one animation "beat" is 30 frames).
 */
class BgmBeatAnimeController : public al::NerveExecutor {
public:
    explicit BgmBeatAnimeController(al::LiveActor* pActor);

    void init(const char* pActionName, bool isSklAnimeDisabled, bool isMtsAnimeDisabled,
              bool isMclAnimeDisabled);
    void initAnimeInfo();
    void reset();
    void update();
    bool isPlayingJunctionAnime() const;

    void exeWait();
    void exeWaitSyncBeatBySyncAnime();
    void setActionFrameRateWithSklAnimeCheck(f32 frameRate);
    void setAnimeFrame();
    void exeSyncBeat();
    void exeWaitSyncBeatByWaitAnime();
    void exeWaitSyncBeatLightStart();
    void exeWaitSyncBeatLight();
    void exeSyncBeatLight();

    /** @return Whether a BGM beat was consumed during the last update. */
    bool isTriggerBeat() const { return mIsTriggerBeat; }

private:
    bool isEnableSyncBgm() const;
    bool isEnableSyncAnime() const;
    void advanceBeat();
    void updateBeatFromAnime();

    al::LiveActor* mActor;
    const char* mActionName = nullptr;
    u8 _20[8];
    s32 mSklBeatNum = 1;
    s32 mSklBeat = 0;
    s32 mMtsBeatNum = 1;
    s32 mMtsBeat = 0;
    s32 mMclBeatNum = 1;
    s32 mMclBeat = 0;
    bool mIsSklAnimeDisabled = false;
    bool mIsMtsAnimeDisabled = false;
    bool mIsMclAnimeDisabled = false;
    bool mIsTriggerBeat = false;
    f32 mBeatStep = 1.0f / 30.0f;
    f32 mNextBeatStep = 1.0f / 30.0f;
    f32 mFrameRateScale = 1.0f;
    f32 mNextFrameRateScale = 1.0f;
    f32 mBeatRate = 0.0f;
    s32 mJunctionBeatCount = 0;
};
