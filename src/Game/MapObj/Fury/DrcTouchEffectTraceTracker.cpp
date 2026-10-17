#include "MapObj/Fury/DrcTouchEffectTraceTracker.hpp"

DrcTouchEffectTraceTracker::DrcTouchEffectTraceTracker(int) {}

bool DrcTouchEffectTraceTracker::tryPlayTraceEffect(DrcTouchPointer* pPointer) {
    if (!mPointer)
        mPointer = pPointer;
    return mPointer == pPointer;
}

bool DrcTouchEffectTraceTracker::endPlayTraceEffect(DrcTouchPointer* pPointer) {
    bool isPlaying = mPointer == pPointer;
    mPointer = nullptr;
    return isPlaying;
}
