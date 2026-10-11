#pragma once

#include <math/seadMatrix.h>

#include "Library/LiveActor/LiveActor.hpp"

namespace al {
class HitSensor;
}  // namespace al

/** @brief One of the biting heads of a potted Piranha Plant (PackunFlowerWithPot). */
class PackunFlowerHead : public al::LiveActor {
public:
    explicit PackunFlowerHead(const char* pName);

    void init(const al::ActorInitInfo& rInfo) override;
    void start();
    bool isEnableAppear();

    void exeWait();
    void exeEatGo();
    void exeEat();
    void exeEatBack();

    /** @brief Sets the sensor of the actor currently holding the plant. */
    void setHolderSensor(al::HitSensor* pSensor) { mHolderSensor = pSensor; }

    /** @brief Sets the host joint matrix the head follows. */
    void setFollowMtx(const sead::Matrix34f* pMtx) { mFollowMtx = pMtx; }

private:
    al::HitSensor* mHolderSensor = nullptr;
    void* _150 = nullptr;
    const sead::Matrix34f* mFollowMtx = nullptr;
};

static_assert(sizeof(PackunFlowerHead) == 0x160);
