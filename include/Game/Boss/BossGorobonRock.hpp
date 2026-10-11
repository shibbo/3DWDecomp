#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"

namespace al {
class BreakModel;
}

/** @brief Rock thrown by Gorobon that rolls along the ground until it hits something. */
class BossGorobonRock : public al::LiveActor {
public:
    explicit BossGorobonRock(const char* pName);
    void init(const al::ActorInitInfo& rInfo) override;
    void appear() override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    void exeShot();
    void exeBrake();

private:
    sead::Vector3f mSideDir = sead::Vector3f::zero;  // 0x144
    al::BreakModel* mBreakModel = nullptr;           // 0x150
};
static_assert(sizeof(BossGorobonRock) == 0x158);
