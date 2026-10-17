#pragma once

class DrcTouchPointer;

class DrcTouchEffectTraceTracker {
public:
    explicit DrcTouchEffectTraceTracker(int);
    bool tryPlayTraceEffect(DrcTouchPointer* pPointer);
    bool endPlayTraceEffect(DrcTouchPointer* pPointer);

private:
    DrcTouchPointer* mPointer = nullptr;
};
