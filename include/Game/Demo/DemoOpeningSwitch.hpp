#pragma once

#include "Library/LiveActor/LiveActor.hpp"

/** @brief Routes course-opening demo events to their stage switches. */
class DemoOpeningSwitch : public al::LiveActor {
public:
    explicit DemoOpeningSwitch(const char* pName);
    /** @brief Destroys the switch actor. */
    ~DemoOpeningSwitch() override = default;
    void init(const al::ActorInitInfo& rInfo) override;
    void onSwitch(int index);
    void exeWait();
};
