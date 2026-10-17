#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class InkPuddle;
class SuperBowserPlessieChaseFireFlameState;
class SuperBowserFireFlamePlessieChase : public al::LiveActor {
public:
    SuperBowserFireFlamePlessieChase(const char* pName, SuperBowserPlessieChaseFireFlameState* pState);
    ~SuperBowserFireFlamePlessieChase() override;
    void init(const al::ActorInitInfo&) override;
    void attackSensor(al::HitSensor*, al::HitSensor*) override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    void appear() override;
    void kill() override;
    void startAttack(const sead::Vector3f&, const sead::Vector3f&, const sead::Vector3f&, float, float, float, bool, float, bool);
    void killForce();
    void exeWait();
    void exeAttack();
    void exeExplode();
    void exeLandStart();
    void exeLand();
    void exeLandEnd();
private:
    bool mUseGravity = false;
    float mGravity;
    bool mIsFirstShot;
    InkPuddle* mInkPuddle = nullptr;
    SuperBowserPlessieChaseFireFlameState* mState;
    float mBaseHeight = 0.0f;
    float mDisappearOffset = 0.0f;
};
static_assert(sizeof(SuperBowserFireFlamePlessieChase) == 0x168);
