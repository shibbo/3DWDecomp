#pragma once

class screenfilter {
public:
    screenfilter();
    ~screenfilter();
    void Init();
    void Normal();
    void NFrame(float* pWeights);
    void MotionBlur(float first, float second);
    void Dark(unsigned int mode, float amount);
    void Contrast(unsigned int mode, float contrast, float offset);
    void Fade(float amount);
    void OnePause();
};
