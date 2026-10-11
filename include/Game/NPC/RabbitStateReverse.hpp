#pragma once

#include "Library/Nerve/NerveStateBase.hpp"

class Rabbit;

/**
 * @brief Rabbit state for turning around on its route.
 * @note Only what reconstructed code needs is declared so far.
 */
class RabbitStateReverse : public al::HostStateBase<Rabbit> {
public:
    RabbitStateReverse(Rabbit* pRabbit);

    void init() override;
    void appear() override;
    void setNoStop();
    void exeReverseStop();
    void exeReverse();
};

static_assert(sizeof(RabbitStateReverse) == 0x20);
