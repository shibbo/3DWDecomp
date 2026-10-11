#pragma once
#include "Library/Layout/LayoutActor.hpp"
namespace al { class LiveActor; class HitSensor; class LayoutInitInfo; }
class WarpCube;
class CounterWarpCube : public al::LayoutActor {
public:
    CounterWarpCube(const al::LayoutInitInfo& rInfo, WarpCube* pWarpCube, int requiredCount);
    void addCount(const al::LiveActor*, al::HitSensor*);
private:
    WarpCube* mWarpCube;
    int mCount;
    int mRequiredCount;
};
