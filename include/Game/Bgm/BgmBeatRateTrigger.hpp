#pragma once

#include <basis/seadTypes.h>

namespace al {
class LiveActor;
}

class BgmBeatRateTrigger {
public:
    BgmBeatRateTrigger(al::LiveActor* pActor, f32 beatRate);
    void update();

    bool isTrigger() const { return mIsTrigger; }

private:
    al::LiveActor* mActor;
    f32 mBeatRate;
    bool mIsTrigger = false;
    bool mIsWaiting = false;
};

static_assert(sizeof(BgmBeatRateTrigger) == 0x10);
