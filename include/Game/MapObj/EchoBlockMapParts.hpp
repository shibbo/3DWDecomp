#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class EchoBlockMapParts : public al::LiveActor {
public:
    explicit EchoBlockMapParts(const char* pName);
    ~EchoBlockMapParts() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void exeWait();
};
