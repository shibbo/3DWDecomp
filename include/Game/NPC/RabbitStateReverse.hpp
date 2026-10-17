#pragma once
#include "Library/Nerve/NerveStateBase.hpp"
class Rabbit;
class RabbitStateReverse : public al::HostStateBase<Rabbit> {
public:
    explicit RabbitStateReverse(Rabbit* pRabbit);
    ~RabbitStateReverse() override;
    void init() override;
    void appear() override;
    void setNoStop();
    void exeReverseStop();
    void exeReverse();
    /** @brief Keeps the no-stop marker nerve idle. */
    void exeNoStop() {}
};
