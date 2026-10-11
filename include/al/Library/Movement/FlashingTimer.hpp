#pragma once

#include <basis/seadTypes.h>

namespace al {
class FlashingTimer {
public:
    FlashingTimer(s32 duration, s32 startHurry, s32 blinkVisibleFrames,
                  s32 hurryBlinkVisibleFrames);

    void update();

    bool isHurryStart() const;
    s32 getLastTime() const;

    s32 getTime() const { return mLastTime; }
    void start() { mLastTime = mDuration; }
    bool isVisible() const { return mIsVisible; }
    bool wasVisible() const { return mIsPrevVisible; }

private:
    s32 mLastTime = -1;
    s32 mDuration;
    s32 mStartHurry;
    s32 mBlinkVisibleFrames;
    s32 mHurryBlinkVisibleFrames;
    bool mIsVisible = false;
    bool mIsPrevVisible = false;
};

static_assert(sizeof(FlashingTimer) == 0x18);
}  // namespace al
