#pragma once

#include "Library/Nerve/NerveStateBase.hpp"

struct WalkerStateParam;

class WalkerStateFall : public al::ActorStateBase {
public:
    WalkerStateFall(al::LiveActor* pHost, const WalkerStateParam* pParam);
    /** @brief Destroys the falling state. */
    ~WalkerStateFall() override = default;
    void appear() override;
    void exeFall();
    void exeLand();

private:
    const WalkerStateParam* mParam;
};

static_assert(sizeof(WalkerStateFall) == 0x28);
