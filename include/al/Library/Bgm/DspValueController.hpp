#pragma once

#include <basis/seadTypes.h>

namespace al {
class DspLinearValueController {
public:
    DspLinearValueController(f32 value);

    void init(f32 value);
    void update();
    void changeTarget(f32 target, s32 frames);

    f32 getValue() const { return mValue; }
    f32 getTarget() const { return mTarget; }

private:
    f32 mValue;
    f32 mTarget;
    f32 mStep;
};

static_assert(sizeof(DspLinearValueController) == 0xc);

class DspSinValueController {
public:
    DspSinValueController(f32 frameRate, f32 value);

    void init(f32 value);
    void update();
    void changeTarget(f32 target, s32 frames);
    void changeFreq(f32 freq);

private:
    f32 mFrameRate;
    f32 mFreq;
    f32 mPhaseStep;
    f32 mPhase;
    f32 mValue;
    DspLinearValueController* mAmplitude;
};

static_assert(sizeof(DspSinValueController) == 0x20);
}  // namespace al
