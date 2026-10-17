#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class TestKitaharaRouteBazookaEntrance;
class TestKitaharaRouteBazookaRider : public al::LiveActor {
public:
    TestKitaharaRouteBazookaRider(const char*, int);
    void setMoveSpeed(float);
    void setOutSpeed(float);
    void setTriggerShoot(bool trigger) { mTriggerShoot = trigger; }
    bool isActive(int) const;
    void startBind(TestKitaharaRouteBazookaEntrance*, al::HitSensor*, al::HitSensor*);
    void tryCancelBind(al::HitSensor*);
    bool damage(al::HitSensor*);
private:
    u8 mUnreconstructed144[0x6c];
    bool mTriggerShoot;
    u8 mUnreconstructed1b1[0x4f];
};
static_assert(sizeof(TestKitaharaRouteBazookaRider) == 0x200);
