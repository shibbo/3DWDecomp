#pragma once

#include "Demo/StageStartEventBase.hpp"

namespace al { class WipeSimple; }
class DemoSkipLayout;
class DemoSceneActorHolder;

/** @brief Plays a stage opening cutscene with a skip prompt and fade. */
class StageStartEventDemo : public StageStartEventBase {
public:
    explicit StageStartEventDemo(const char* pName);
    /** @brief Destroys the event actor. */
    ~StageStartEventDemo() override = default;
    void init(const al::ActorInitInfo& rInfo) override;
    /** @brief Identifies a cutscene event. @return Cutscene event type, 1. */
    s64 getEventType() const override { return 1; }
    void startDemo() override;
    void endDemo() override;
    bool isEndDemo() const override;
    void exePlay();
    void exeFade();
    void exeEnd();

private:
    const char* mModelName = nullptr;
    DemoSceneActorHolder* mDemo = nullptr;
    al::WipeSimple* mWipe = nullptr;
    DemoSkipLayout* mSkipLayout = nullptr;
};
