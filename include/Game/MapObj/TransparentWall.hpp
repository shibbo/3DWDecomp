#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class TransparentWall : public al::LiveActor {
public:
    TransparentWall(const char* pName);
    virtual ~TransparentWall();
    virtual void init(const al::ActorInitInfo& rInfo);
    virtual bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf);
};
