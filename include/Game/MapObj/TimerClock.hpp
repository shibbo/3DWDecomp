#pragma once
#include "Library/LiveActor/LiveActor.hpp"

class TimerClockNumber;

class TimerClock : public al::LiveActor {
public:
    TimerClock(const char* pName);
    ~TimerClock() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void initAfterPlacement() override;
    void appearBySwitch();
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) override;
    bool isEnableMsgItemGet(const al::SensorMsg* pMsg) const;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;
    void exeWait();

private:
    int mTime = 10;
    TimerClockNumber* mNumber;
    bool mIsPlacementInRouteDokan = false;
};
