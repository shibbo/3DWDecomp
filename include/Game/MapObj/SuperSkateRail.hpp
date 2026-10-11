#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class SuperSkateRail : public al::LiveActor {
public:
    explicit SuperSkateRail(const char*);
    void init(const al::ActorInitInfo&) override;
    void attackSensor(al::HitSensor*, al::HitSensor*) override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    void control() override;
    sead::Vector3f getDirection();
    sead::Vector3f getReverseDirection();
    SuperSkateRail* getNextRail();
    SuperSkateRail* getPreviousRail();
    sead::Vector3f lerp(float);
    float getDistance();
    float getReverseDistance();
private:
    SuperSkateRail* mPrevious = nullptr;
    SuperSkateRail* mNext = nullptr;
    float mDistance = 0.0f;
};
static_assert(sizeof(SuperSkateRail) == 0x160);
