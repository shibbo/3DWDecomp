#pragma once
#include "Library/Nerve/NerveStateBase.hpp"
class Rabbit;
class RabbitStateSwoon : public al::HostStateBase<Rabbit> {
public:
    /** @brief Sets the first swoon action. @param pAction Action name. */
    void setStartActionName(const char* pAction) { mStartAction = pAction; }
    /** @brief Skips the sustained swoon after support freeze. */
    void setAfterSupportFreeze() { mSkipSwoon = true; }
    explicit RabbitStateSwoon(Rabbit* pRabbit);
    ~RabbitStateSwoon() override;
    void init() override;
    void appear() override;
    void kill() override;
    void exeSwoonStart();
    void exeSwoon();
    void exeSwoonEnd();
    void exeTurn();

private:
    const char* mStartAction;
    bool mSkipSwoon;
};
