#pragma once
#include "Library/Layout/LayoutActor.hpp"
namespace al {
class LiveActor;
class HitSensor;
class LayoutInitInfo;
} // namespace al
class WarpCube;
class CounterWarpCube : public al::LayoutActor {
public:
  CounterWarpCube(const al::LayoutInitInfo &rInfo, WarpCube *pWarpCube,
                  int requiredCount);
  void addCount(const al::LiveActor *pCollector, al::HitSensor *pSensor);
  void appear() override;
  void exeAppear();
  void exeWait();
  void exeAdd();
  void exeComplete();
  void exeEnd();

private:
  WarpCube *mWarpCube;
  int mCount = 0;
  int mRequiredCount;
};
