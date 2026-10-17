#pragma once
#include "Library/Nerve/NerveStateBase.hpp"
namespace al { class SensorMsg; class HitSensor; }
class KinokoStateRunaway : public al::ActorStateBase {
public:
    KinokoStateRunaway(al::LiveActor*);
    void init() override;
    void appear() override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*);
    void move(float speed, float gravity, float velocityScale, float turnDegrees, float deceleration);
    void setSpeed(float speed) { mSpeed = speed; }
    void exeLand();
    void exeMove();
    void exeReaction();
private:
    float mSpeed = 6.0f;
    float mBaseSpeed = 6.0f;
};
