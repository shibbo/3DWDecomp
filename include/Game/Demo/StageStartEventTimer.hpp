#pragma once

#include "Demo/StageStartEventBase.hpp"

namespace al {
class LayoutActor;
class WipeSimple;
}
class StageTimer;

/** @brief Displays the initial stage timer and releases players after the opening wipe. */
class StageStartEventTimer : public StageStartEventBase {
public:
    explicit StageStartEventTimer(const char* pName);
    /** @brief Destroys the event actor. */
    ~StageStartEventTimer() override = default;
    void setStageTimer(StageTimer* pTimer);
    void init(const al::ActorInitInfo& rInfo) override;
    /** @brief Identifies a timer event. @return Timer event type, 4. */
    s64 getEventType() const override { return 4; }
    void startDemo() override;
    void endDemo() override;
    bool isEndDemo() const override;
    bool isEnableMovement() const override;
    void exeAppear();
    void exeMove();
    void exeWait();
    void exeEnd();

private:
    al::WipeSimple* mWipe;
    al::LayoutActor* mLayout;
    StageTimer* mStageTimer = nullptr;
    bool _160 = false;
};
