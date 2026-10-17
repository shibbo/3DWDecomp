#include "LuigiBros/Utility/Utility.hpp"

#include "LuigiBros/Emulator/cafe.hpp"

namespace Vessel { namespace Utility {

/** @brief Initializes an empty timing sample accumulator. */
CTickWatcher::CTickWatcher()
    : mStart(0), mElapsed(0), mCount(0), mTotal(0), mMaximum(0), mMinimum(~0UL) {}

/** @brief Destroys the timing accumulator. */
CTickWatcher::~CTickWatcher() = default;

/** @brief Clears all samples and restores the minimum sentinel. */
void CTickWatcher::Reset() {
    mStart = 0;
    mElapsed = 0;
    mCount = 0;
    mTotal = 0;
    mMaximum = 0;
    mMinimum = ~0UL;
}

/** @brief Records the start tick for the next sample. */
void CTickWatcher::Enter() {
    mStart = OSGetTick();
}

/** @brief Accumulates the elapsed ticks and updates the sample extrema. */
void CTickWatcher::Leave() {
    mElapsed = OSGetTick() - mStart;
    mTotal += mElapsed;
    ++mCount;
    if (mMaximum < mElapsed) {
        mMaximum = mElapsed;
    }
    if (mMinimum > mElapsed) {
        mMinimum = mElapsed;
    }
}

/**
 * @brief Clears samples when their count exceeds the reporting threshold.
 * @param count Maximum sample count before resetting.
 * @param pLabel Report label, unused on this platform.
 */
void CTickWatcher::Report(unsigned long count, const char* pLabel) {
    if (mCount > count) {
        Reset();
    }
}

/** @brief Gets the average sample duration. @return Duration in milliseconds, or zero with no samples. */
unsigned long CTickWatcher::GetAverage() const {
    return OSTicksToMilliseconds(mCount ? mTotal / mCount : 0);
}

/** @brief Gets the longest sample duration. @return Maximum duration in milliseconds. */
unsigned long CTickWatcher::GetMaximum() const {
    return OSTicksToMilliseconds(mMaximum);
}

/** @brief Gets the shortest sample duration. @return Minimum ticks converted to milliseconds. */
unsigned long CTickWatcher::GetMinimum() const {
    return OSTicksToMilliseconds(mMinimum);
}

/** @brief Initializes the interval checker with zero ticks and samples. */
CTickChecker::CTickChecker() : mPrevious(0), mElapsed(0), mCount(0) {}

/** @brief Destroys the interval checker. */
CTickChecker::~CTickChecker() = default;

/** @brief Clears the previous tick, elapsed interval, and sample count. */
void CTickChecker::Reset() {
    mPrevious = 0;
    mElapsed = 0;
    mCount = 0;
}

/** @brief Measures the interval since the previous check and increments the count. */
void CTickChecker::Check() {
    unsigned long tick = OSGetTick();
    mElapsed = tick - mPrevious;
    mPrevious = tick;
    ++mCount;
}

/** @brief Platform stub for interval reporting. @param pLabel Unused report label. */
void CTickChecker::Report(const char* pLabel) {}

} }
