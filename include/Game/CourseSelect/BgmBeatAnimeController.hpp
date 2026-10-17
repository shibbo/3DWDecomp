#pragma once

#include "Library/Nerve/NerveExecutor.hpp"

namespace al { class LiveActor; }

class BgmBeatAnimeController : public al::NerveExecutor {
public:
    explicit BgmBeatAnimeController(al::LiveActor* pActor);
    void init(const char* pActionName, bool isIgnoreSkl, bool isIgnoreMts, bool isIgnoreMcl);
    void initAnimeInfo();
    void reset();
    void update();
    bool isPlayingJunctionAnime() const;
    void exeWait();
    void exeWaitSyncBeatBySyncAnime();
    void setActionFrameRateWithSklAnimeCheck(float rate);
    void setAnimeFrame();
    void exeSyncBeat();
    void exeWaitSyncBeatByWaitAnime();
    void exeWaitSyncBeatLightStart();
    void exeWaitSyncBeatLight();
    void exeSyncBeatLight();

private:
    float calcAnimeFrame(int beatIndex) const;
    void advanceBeatIndices();
    void readBeatIndices();
    bool isSyncBgm() const;
    static bool isSklEnabled(const al::LiveActor* pActor, bool isIgnore);
    static bool isMtsEnabled(const al::LiveActor* pActor, bool isIgnore);
    static bool isMclEnabled(const al::LiveActor* pActor, bool isIgnore);

    al::LiveActor* mpActor;
    const char* mpActionName = nullptr;
    unsigned char _20[8];
    int mSklBeatCount = 1;
    int mSklBeatIndex = 0;
    int mMtsBeatCount = 1;
    int mMtsBeatIndex = 0;
    int mMclBeatCount = 1;
    int mMclBeatIndex = 0;
    bool mIsIgnoreSkl = false;
    bool mIsIgnoreMts = false;
    bool mIsIgnoreMcl = false;
    bool mIsBeatTriggered = false;
    float mBeatAdvance = 1.0f / 30.0f;
    float mFinalBeatAdvance = 1.0f / 30.0f;
    float mFrameRateScale = 1.0f;
    float mFinalFrameRateScale = 1.0f;
    float mBeatRate = 0.0f;
    int mJunctionStepCount = 0;
};

static_assert(sizeof(BgmBeatAnimeController) == 0x60);
