#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class DemoTimerStageSwitchInfo;

/** @brief Applies scheduled stage-switch transitions during a demo. */
class DemoTimerStageSwitchController : public al::LiveActor {
public:
    explicit DemoTimerStageSwitchController(const char* pName);
    void init(const al::ActorInitInfo& rInfo) override;
    void appear() override;
    void prepare(bool isOn);
    void control() override;
    ~DemoTimerStageSwitchController() override;

private:
    int mInfoCount = 0;
    DemoTimerStageSwitchInfo** mInfos = nullptr;
    int mStep = 0;
    int mMaxStep = 0;
};
