#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class KoopaLastFloor : public al::LiveActor {
public:
    explicit KoopaLastFloor(const char* pName);
    ~KoopaLastFloor() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void start();
    void kill() override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSelf,
                    al::HitSensor* pOther) override;
    void exeWait();
    void exeSign();

private:
    al::LiveActor* mBreakModel = nullptr;
};
