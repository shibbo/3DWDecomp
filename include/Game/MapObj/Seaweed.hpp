#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class ActorMicRumbler;
class Seaweed : public al::LiveActor {
public:
    explicit Seaweed(const char*);
    ~Seaweed() override;
    void init(const al::ActorInitInfo&) override;
    void control() override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    bool receiveMsgScreenPoint(const al::SensorMsg*, al::ScreenPointer*, al::ScreenPointTarget*) override;
    void exeWait();
    void exeReaction();
    void exeReactionAfter();
private:
    ActorMicRumbler* mMicRumbler = nullptr;
    int mReactionCooldown = 0;
};
