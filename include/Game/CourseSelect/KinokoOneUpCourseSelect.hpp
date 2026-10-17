#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class KinokoOneUpCourseSelect : public al::LiveActor {
public:
    explicit KinokoOneUpCourseSelect(const char* pName);
    ~KinokoOneUpCourseSelect() override;
    void init(const al::ActorInitInfo& rInfo) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSender, al::HitSensor* pReceiver) override;
    void appear() override;
    void exeWait();

private:
    int mOneUpId = -1;
};

static_assert(sizeof(KinokoOneUpCourseSelect) == 0x148);
