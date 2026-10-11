#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class ZigzagBuildingCover : public al::LiveActor {
public:
    ZigzagBuildingCover(const char* pName);
    virtual ~ZigzagBuildingCover();
    virtual void init(const al::ActorInitInfo& rInfo);
    virtual bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf);
};
