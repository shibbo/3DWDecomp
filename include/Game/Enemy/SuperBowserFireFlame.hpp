#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class SuperBowserFireFlame : public al::LiveActor {
public:
    explicit SuperBowserFireFlame(const char* pName);
    ~SuperBowserFireFlame() override;
    void init(const al::ActorInitInfo&) override;
    void attackSensor(al::HitSensor*, al::HitSensor*) override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    void updateCollider() override;
    void appear() override;
    void kill() override;
    void startAttack(const sead::Vector3f&, const sead::Vector3f&, const sead::Vector3f&, bool, float);
    void killForce();
    void exeWait();
    void exeAttack();
    void exeExplode();
    void exeLandStart();
    void exeLand();
    void exeLandEnd();
    float getSpeedH() const;
private:
    bool mUseGravity = false;
    float mGravity;
};
static_assert(sizeof(SuperBowserFireFlame) == 0x150);
