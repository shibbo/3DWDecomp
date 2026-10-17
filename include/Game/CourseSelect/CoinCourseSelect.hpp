#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class CoinCourseSelect : public al::LiveActor {
public:
    explicit CoinCourseSelect(const char* pName);
    ~CoinCourseSelect() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void control() override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSender, al::HitSensor* pReceiver) override;
    void appear() override;

    int getObjectId() const { return mObjectId; }

private:
    int mObjectId = -1;
    sead::Quatf mBaseQuat = sead::Quatf::unit;
};

static_assert(sizeof(CoinCourseSelect) == 0x158);
