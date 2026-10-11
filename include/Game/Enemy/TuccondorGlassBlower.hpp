#pragma once

#include <math/seadVector.h>

namespace al {
class LiveActor;
class SensorMsg;
class ScreenPointer;
class ScreenPointTarget;
}
class TuccondorGlass;

class TuccondorGlassBlower {
public:
    TuccondorGlassBlower(al::LiveActor* pHost, TuccondorGlass* pGlass);
    bool tryRequestGlassBlow(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                             al::ScreenPointTarget* pTarget);
    void update();

private:
    al::LiveActor* mHost;
    TuccondorGlass* mGlass;
    sead::Vector3f mJointRotation = sead::Vector3f::zero;
    int mShakeTimer = 0;
};

static_assert(sizeof(TuccondorGlassBlower) == 0x20);
