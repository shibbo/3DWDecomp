#pragma once

namespace Perf { namespace CPU {
class Tick {
public:
    /** @brief Destroys the timing scope; this platform performs no cleanup. */
    __attribute__((used)) ~Tick() {}
};
} }

class performance {
public:
    enum DISP : unsigned int { DISP_NONE, DISP_FRAME_RATE };
    void init(DISP display);
    float cpu();
    float gpu();
    void cpu_start();
    void cpu_end();
    void gpu_start();
    void gpu_end();
    void cpu_emu();
    void main();

private:
    float mCpu;
    float mGpu;
    float mCpuTotal;
    float mGpuTotal;
    float mSampleTime;
    unsigned int mFrameCount;
    bool mNeedsGpuAllocation;
    bool mResetSamples;
    bool mCpuRunning;
    DISP mDisplay;
    unsigned long mStartTick;
    unsigned long mCpuStartTick;
    unsigned long* mpGpuStart;
    unsigned long* mpGpuEnd;
    unsigned char _40[0xc];
    char mText[0x3c];
};
static_assert(sizeof(performance) == 0x88, "Performance monitor layout");
