#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class GrassHigh : public al::LiveActor {
public:
    explicit GrassHigh(const char*);
    void init(const al::ActorInitInfo&) override;
    void attackSensor(al::HitSensor*, al::HitSensor*) override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    bool receiveMsgScreenPoint(const al::SensorMsg*, al::ScreenPointer*, al::ScreenPointTarget*) override;
    void exeWait();
    void exeReaction();
    void exeDestroy();
};
