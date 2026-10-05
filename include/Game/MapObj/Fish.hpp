#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class Fish : public al::LiveActor {
public:
    explicit Fish(const char* pName);
    ~Fish() override;
    void init(const al::ActorInitInfo& rInfo) override;
};
