#pragma once

#include "Library/Nerve/NerveStateBase.hpp"

/** @brief Wait state shared by the Bros enemies. */
class BrosStateWait : public al::ActorStateBase {
public:
    explicit BrosStateWait(al::LiveActor* pHost);

    /** @brief Destroys the waiting state. */
    ~BrosStateWait() override = default;
    void exeWait();
};

static_assert(sizeof(BrosStateWait) == 0x20);
