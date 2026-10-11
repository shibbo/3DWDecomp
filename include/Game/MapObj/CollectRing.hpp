#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class CollectRingHolder;
class CollectNumber;
class CollectRing : public al::LiveActor {
public:
    CollectRing(const char* pName);
    ~CollectRing() override;
    void init(const al::ActorInitInfo&) override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    void setHost(CollectRingHolder*);
    void appearNumberComplete(int);
    void appearNumber(int);
    void exeAppear();
    void exeWait();
private:
    CollectRingHolder* mHost = nullptr;
    CollectNumber* mNumber = nullptr;
    al::HitSensor* mCollector = nullptr;
};
