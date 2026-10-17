#pragma once
#include "Enemy/AllDeadWatcher.hpp"
class DisasterBlockDeadWatcher : public AllDeadWatcher {
public:
    DisasterBlockDeadWatcher(const char*);
    ~DisasterBlockDeadWatcher() override;
    void init(const al::ActorInitInfo&) override;
    void exeWatch() override;
    void exeWatchPartial() override;
};
static_assert(sizeof(DisasterBlockDeadWatcher) == 0x168);
