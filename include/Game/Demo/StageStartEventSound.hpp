#pragma once

#include "Demo/StageStartEventBase.hpp"

/** @brief Plays a stage-entry sound before enabling the opening wipe and stage music. */
class StageStartEventSound : public StageStartEventBase {
public:
    explicit StageStartEventSound(const char* pName);
    /** @brief Destroys the event actor. */
    ~StageStartEventSound() override = default;
    void init(const al::ActorInitInfo& rInfo) override;
    /** @brief Identifies a sound event. @return Sound event type, 2. */
    s64 getEventType() const override { return 2; }
    void startDemo() override;
    void endDemo() override;
    bool isEndDemo() const override;
    bool isEnableOpenStartWipe() const override;
    void exePlay();
    void exeEnd();

private:
    const char* mSoundName = nullptr;
    int mDuration = 0;
};
