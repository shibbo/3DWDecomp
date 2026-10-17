#pragma once

#include <basis/seadTypes.h>

namespace al {
class LiveActor;
}

class BgmBeatPeriodicTrigger {
public:
    BgmBeatPeriodicTrigger(al::LiveActor* pActor, s32 divisionNum);
    void update();

    bool isTrigger() const { return mIsTrigger; }
    s32 getTriggerIndex() const { return mTriggerIndex; }

private:
    al::LiveActor* mActor;
    f32 mNextBeatRate;
    f32 mBeatRateStep;
    bool mIsTrigger = false;
    s32 mTriggerIndex = 0;
};

static_assert(sizeof(BgmBeatPeriodicTrigger) == 0x18);
