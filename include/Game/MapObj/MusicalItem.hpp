#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class MusicalItem : public al::LiveActor {
public:
    MusicalItem(const char* pName);
    ~MusicalItem() override;
    void init(const al::ActorInitInfo&) override;
    void makeActorAppeared() override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    void exeWait();
private:
    int mInstrumentType;
};
