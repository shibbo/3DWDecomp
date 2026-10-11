#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class StrongHipDropReactPoint : public al::LiveActor {
public:
    explicit StrongHipDropReactPoint(const char* pName);
    ~StrongHipDropReactPoint() override;
    void init(const al::ActorInitInfo& rInfo) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
};
