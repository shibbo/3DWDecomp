#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class GroupClippingDummyTarget : public al::LiveActor {
public:
    explicit GroupClippingDummyTarget(const char* pName);
    ~GroupClippingDummyTarget() override;
    void init(const al::ActorInitInfo& rInfo) override;
};
