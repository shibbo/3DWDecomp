#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include <container/seadPtrArray.h>
namespace al {
class FlashingCtrl;
}
namespace {
class CoinCircle;
}
class CoinConcentricCircle : public al::LiveActor {
public:
  explicit CoinConcentricCircle(const char *);
  ~CoinConcentricCircle() override;
  void init(const al::ActorInitInfo &) override;
  void initAfterPlacement() override;
  void appear() override;
  void kill() override;
  void control() override;
  void calcHeightOffset(float);
  void exeAppear();
  void exeWait();
  bool isShared() const { return mShared; }
  float getHeightOffset() const { return mHeightOffset; }

private:
  sead::PtrArray<CoinCircle>::iterator getRingsEnd() const {
    return sead::PtrArray<CoinCircle>::iterator(mRings.size() + mRings.data());
  }
  sead::PtrArray<CoinCircle> mRings;
  al::FlashingCtrl *mFlashing = nullptr;
  int mTimer = 0;
  bool mHidden = false;
  int mAppearInterval = 0;
  int mAppearFrames = 0;
  int mFloorCheckType = 0;
  float mHeightOffset = 0.0f;
  float mHoverHeight = 100.0f;
  bool mShared = false;
};
static_assert(sizeof(CoinConcentricCircle) == 0x180);
