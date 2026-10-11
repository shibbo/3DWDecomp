#pragma once

#include "Library/Nerve/NerveStateBase.hpp"

class Rabbit;

/**
 * @brief Rabbit state for being knocked out after an attack.
 * @note Only what reconstructed code needs is declared so far.
 */
class RabbitStateSwoon : public al::HostStateBase<Rabbit> {
public:
    RabbitStateSwoon(Rabbit* pRabbit);

    void init() override;
    void appear() override;
    void kill() override;
    void exeSwoonStart();
    void exeSwoon();
    void exeSwoonEnd();
    void exeTurn();

    /**
     * @brief Sets the action played when the swoon starts.
     * @param pActionName Action name.
     */
    void setStartActionName(const char* pActionName) { mStartActionName = pActionName; }

    /** @brief Marks that the swoon follows a support freeze. */
    void setAfterSupportFreeze() { mIsAfterSupportFreeze = true; }

private:
    const char* mStartActionName;
    bool mIsAfterSupportFreeze;
};

static_assert(sizeof(RabbitStateSwoon) == 0x30);
