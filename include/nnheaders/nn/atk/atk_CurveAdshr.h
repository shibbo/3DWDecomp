#pragma once

#include <nn/types.h>

namespace nn::atk::detail {
class CurveAdshr {
  public:
    CurveAdshr();
    void Initialize(float decibels);
    void SetAttack(int attack);
    void SetHold(int hold);
    void SetDecay(int decay);
    void SetSustain(int sustain);
    void SetRelease(int release);
    void Reset(float decibels);
    float GetValue() const;
    void Update(int step);
    static s16 CalcDecibelSquare(int value);
    static float CalcRelease(int release);
    static const s16 DecibelSquareTable[128];

    /** @brief Tests whether the envelope reached its release phase. @return Whether releasing. */
    bool IsRelease() const { return mState == Release; }

  private:
    enum State { Attack, Hold, Decay, Sustain, Release };
    State mState;
    float mValue;
    float mDecay;
    float mRelease;
    float mAttack;
    u16 mHold;
    u16 mHoldRemaining;
    u8 mSustain;
};
static_assert(sizeof(CurveAdshr) == 0x1c, "CurveAdshr size");
} // namespace nn::atk::detail
