#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class BgmPlayObj : public al::LiveActor {
public:
    BgmPlayObj(const char* pName);
    virtual ~BgmPlayObj();
    virtual void init(const al::ActorInitInfo& rInfo);

    void start();
    void exeWait();
    void exePlaying();

    const char* mBgmPlayName = nullptr;  // 0x148
    int mStartDelayFrameNum = 0;        // 0x150
    int mFadeInFrameNum = 0;            // 0x154
    int mCurBgmFadeOutFrameNum = 0;     // 0x158
};
