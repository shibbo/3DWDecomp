#include "LuigiBros/Vessel/performance.hpp"
#include "LuigiBros/Vessel/VesselMemory.hpp"
#include "LuigiBros/Emulator/cafe.hpp"
#include <cstdio>

/** @brief Initializes timing and allocates GPU sample storage when needed. @param display Display mode. */
void performance::init(DISP display) {
    if (mNeedsGpuAllocation) {
        mpGpuStart = static_cast<unsigned long*>(VESSEL_malloc(8UL, 16));
        mpGpuEnd = static_cast<unsigned long*>(VESSEL_malloc(8UL, 16));
    }
    mFrameCount = 0;
    unsigned long tick = OSGetTick();
    mCpuRunning = false;
    mStartTick = tick;
    mNeedsGpuAllocation = false;
    mResetSamples = true;
    mDisplay = display;
    mText[0] = 0;
}

/** @brief Requests a fresh sampling window. @return Last CPU utilization value. */
float performance::cpu() {
    mResetSamples = true;
    return mCpu;
}

/** @brief Requests a fresh sampling window. @return Last GPU utilization value. */
float performance::gpu() {
    mResetSamples = true;
    return mGpu;
}

/** @brief Starts measuring CPU execution time. */
void performance::cpu_start() {
    mCpuRunning = true;
    mCpuStartTick = OSGetTick();
}

/** @brief Accumulates CPU time when a measurement is active. */
void performance::cpu_end() {
    if (mCpuRunning) {
        mCpuTotal += static_cast<float>(OSTicksToMicroseconds(OSDiffTick(OSGetTick(), mCpuStartTick)));
        mCpuRunning = false;
    }
}

/** @brief Samples the beginning GPU cycle after allocation. */
void performance::gpu_start() {
    if (!mNeedsGpuAllocation) {
        GX2SampleTopGPUCycle(mpGpuStart);
    }
}

/** @brief Samples the ending GPU cycle after allocation. */
void performance::gpu_end() {
    if (!mNeedsGpuAllocation) {
        GX2SampleBottomGPUCycle(mpGpuEnd);
    }
}

/** @brief Counts emulator frames and refreshes the diagnostic text once per second. */
void performance::cpu_emu() {
    unsigned long elapsed = OSDiffTick(OSGetTick(), mStartTick);
    ++mFrameCount;
    if (OSTicksToSeconds(elapsed) != 0) {
        if (mDisplay == DISP_FRAME_RATE) {
            // Preserve the original varargs widths and legacy format string.
            const char* pFormat = "Frame rate=%d (%df/%ds)\n[cpu %3.1f%% / gpu %3.1f%%]\n";
            std::sprintf(mText, pFormat,
                         mFrameCount / OSTicksToSeconds(elapsed), mFrameCount,
                         OSTicksToSeconds(elapsed), cpu(), gpu());
        }
        mStartTick = OSGetTick();
        mFrameCount = 0;
    }
}

/** @brief Updates CPU/GPU averages and handles requests to reset the sample window. */
void performance::main() {
    mSampleTime += 166.0f;
    mCpu = mCpuTotal / mSampleTime;
    if (!mNeedsGpuAllocation) {
        unsigned long start = *mpGpuStart;
        long elapsed = static_cast<long>(*mpGpuEnd - start);
        if (elapsed > 0) {
            mGpuTotal += static_cast<float>(elapsed) * 100.0f / 27.0f;
            mGpu = mGpuTotal / mSampleTime;
        }
    }
    if (mResetSamples) {
        mCpuTotal = 0.0f;
        mGpuTotal = 0.0f;
        mSampleTime = 0.0f;
        mResetSamples = false;
    }
}
