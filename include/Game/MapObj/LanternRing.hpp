#pragma once

#include "Library/LiveActor/LiveActor.hpp"
#include <math/seadQuat.h>

class LanternRing : public al::LiveActor {
public:
    explicit LanternRing(const char* pName);
    ~LanternRing() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    void setPrevTrans();
    void setPrevQuat();
    void exeWait();

private:
    sead::Vector3f mPrevTrans{0.0f, 0.0f, 0.0f};
    sead::Quatf mPrevQuat = sead::Quatf::unit;
};

static_assert(sizeof(LanternRing) == 0x160);
