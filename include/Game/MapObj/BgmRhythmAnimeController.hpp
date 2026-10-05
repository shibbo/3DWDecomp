#pragma once

#include "Library/Nerve/NerveExecutor.hpp"

namespace al { class LiveActor; }
struct BgmRhythmAnimeInfo;

class BgmRhythmAnimeController : public al::NerveExecutor {
public:
    BgmRhythmAnimeController(al::LiveActor* pActor, bool isNormal);
    void update();
    void exeWait();
    void updateRhythmInfo();
    void updateAnimeFrame();
    void exePrepareWait();
    void exeSyncRhythm();

    al::LiveActor* mActor;                   // 0x10
    bool mIsNormal;                         // 0x18
    const BgmRhythmAnimeInfo* mInfo = nullptr; // 0x20
    float mFrameMax = 0.0f;                  // 0x28
    float mLastFrame = 0.0f;                 // 0x2c
    bool _30 = false;
};
