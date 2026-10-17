#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class TestKitaharaRouteBazookaEntrance;
class TestKitaharaRouteBazookaPartsGroup;
class TestKitaharaRouteBazookaEntranceGroup;
class TestKitaharaRouteBazookaRider;
class TestKitaharaRouteBazooka : public al::LiveActor {
public:
    TestKitaharaRouteBazooka(const char*);
    ~TestKitaharaRouteBazooka() override;
    void init(const al::ActorInitInfo&) override;
    void active();
    void deactive();
    virtual bool isBindStart(al::HitSensor*, al::HitSensor*);
    virtual bool startBind(TestKitaharaRouteBazookaEntrance*, al::HitSensor*, al::HitSensor*);
    virtual bool cancelBind(al::HitSensor*);
    bool damagePuppet(al::HitSensor*);
private:
    TestKitaharaRouteBazookaPartsGroup* mParts = nullptr;
    TestKitaharaRouteBazookaEntranceGroup* mEntrances = nullptr;
    TestKitaharaRouteBazookaRider** mRiders = nullptr;
    int mRiderCount = 8;
    TestKitaharaRouteBazookaEntrance* mStartEntrance = nullptr;
};
