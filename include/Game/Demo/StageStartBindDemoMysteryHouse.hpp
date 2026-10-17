#pragma once

#include "Demo/StageStartEventBase.hpp"

class WindowMessage;
class BindPuppeteerGroup;

/** @brief Holds players while the Mystery House introduction is displayed. */
class StageStartBindDemoMysteryHouse : public StageStartEventBase {
public:
    explicit StageStartBindDemoMysteryHouse(const char* pName);
    /** @brief Destroys the opening event. */
    ~StageStartBindDemoMysteryHouse() override = default;
    void init(const al::ActorInitInfo& rInfo) override;
    void kill() override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSender,
                    al::HitSensor* pReceiver) override;
    /** @brief Identifies a player-binding event. @return Event type 3. */
    s64 getEventType() const override { return 3; }
    /** @brief Activates the opening event. */
    void startDemo() override { appear(); }
    /** @brief Ends the opening event. */
    void endDemo() override { kill(); }
    bool isEndDemo() const override;
    bool isEnableOpenStartWipe() const override;
    void exeBindWait();
    void exeSound();
    void exeMessage();
    void exeBindEnd();

private:
    WindowMessage* mWindow = nullptr;
    BindPuppeteerGroup* mPuppeteers = nullptr;
};
