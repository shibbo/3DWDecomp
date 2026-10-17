#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class Skatis;
class EnemyStateBlowDown;

class SkatisHead : public al::LiveActor {
public:
    explicit SkatisHead(const Skatis* pHost);
    void init(const al::ActorInitInfo& rInfo) override;
    void reset();
    void exeWait();
    void exeBlowDown();
    void startBlowDown(al::HitSensor* pOther, al::HitSensor* pSelf);
private:
    const Skatis* mHost;
    const sead::Matrix34f* mJointMtx = nullptr;
    EnemyStateBlowDown* mBlowDown = nullptr;
    f32 mAngle = 0.0f;
};
static_assert(sizeof(SkatisHead) == 0x168);
