#pragma once

#include <basis/seadTypes.h>

namespace al {
class LiveActor;
}

class BgmBeatRateTrigger {
public:
    BgmBeatRateTrigger(al::LiveActor* pActor, f32 beatRate);
    void update();

    /** @brief Set the fraction of a beat at which the trigger fires.
     * @param rate Fractional beat position. */
    void setRate(f32 rate) { mBeatRate = rate; }

    bool isTrigger() const { return mIsTrigger; }

private:
    al::LiveActor* mActor;
    f32 mBeatRate;
    bool mIsTrigger = false;
    bool mIsWaiting = false;
};

static_assert(sizeof(BgmBeatRateTrigger) == 0x10);
