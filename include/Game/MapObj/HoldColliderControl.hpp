#pragma once

namespace al {
class LiveActor;
class HitSensor;
}

class HoldColliderControl {
public:
    HoldColliderControl();
    void init(al::LiveActor* pActor);
    void start();
    void update(al::HitSensor* pPlayer, al::HitSensor* pHeld);

private:
    al::LiveActor* mActor = nullptr;
    int mStep = 0;
};
