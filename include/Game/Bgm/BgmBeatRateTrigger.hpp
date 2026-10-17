#pragma once

#include <basis/seadTypes.h>

namespace al {
class LiveActor;
}  // namespace al

/**
 * @brief Fires a trigger on each beat of the currently playing BGM, shifted by a beat rate.
 * @note Minimal declaration: only what the reconstructed code uses is declared so far.
 */
class BgmBeatRateTrigger {
public:
    BgmBeatRateTrigger(al::LiveActor* pActor, f32 rate);

    void update();

    /** @brief Sets the beat rate at which the trigger fires. */
    void setRate(f32 rate) { mRate = rate; }

    /** @brief Checks whether the trigger fired during the last update. */
    bool isTrigger() const { return mIsTrigger; }

private:
    al::LiveActor* mActor;
    f32 mRate;
    bool mIsTrigger;
};

static_assert(sizeof(BgmBeatRateTrigger) == 0x10);
