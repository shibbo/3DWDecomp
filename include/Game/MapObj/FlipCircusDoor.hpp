#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class FlipCircusDoorA : public al::LiveActor {
public:
    explicit FlipCircusDoorA(const char* pName);
    ~FlipCircusDoorA() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void kill() override;
    void exeWait();
};
