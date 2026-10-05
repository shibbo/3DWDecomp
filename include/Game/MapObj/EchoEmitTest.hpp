#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class EchoEmitTest : public al::LiveActor {
public:
    explicit EchoEmitTest(const char* pName);
    ~EchoEmitTest() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void exeWait();
};
