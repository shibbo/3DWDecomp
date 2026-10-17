#pragma once

#include "Demo/StageStartEventBase.hpp"

class DemoSceneCamera;

/** @brief Plays the opening stage camera and handles skipping it. */
class StageStartEventCamera : public StageStartEventBase {
public:
    explicit StageStartEventCamera(const char* pName);
    /** @brief Destroys the event actor. */
    ~StageStartEventCamera() override = default;
    void init(const al::ActorInitInfo& rInfo) override;
    /** @brief Identifies a camera event. @return Camera event type, 0. */
    s64 getEventType() const override { return 0; }
    void startDemo() override;
    void endDemo() override;
    bool isEndDemo() const override;
    /** @brief Allows movement during the event. @return True. */
    bool isEnableMovement() const override { return true; }
    void exePlay();
    void exeEnd();

private:
    DemoSceneCamera* mCamera = nullptr;
};
