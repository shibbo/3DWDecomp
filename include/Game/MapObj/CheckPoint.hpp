#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class CheckPointStar;
class CheckPoint : public al::LiveActor {
public:
  explicit CheckPoint(const char *name);
  void cancel();
  void starDoneMoving();
  CheckPointStar *getStar() const { return mStar; }

private:
  u8 _144[0x1c];
  CheckPointStar *mStar;
  u8 _168[0x90];
};
static_assert(sizeof(CheckPoint) == 0x1f8);
