#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class TeresaFakeObject : public al::LiveActor {
public:
    TeresaFakeObject(const char* pName);
    ~TeresaFakeObject() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void kill() override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
};
