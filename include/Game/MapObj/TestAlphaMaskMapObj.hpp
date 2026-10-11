#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class TestAlphaMaskMapObj : public al::LiveActor {
public:
    TestAlphaMaskMapObj(const char* pName);
    ~TestAlphaMaskMapObj() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void exeWait();
    void exeAlphaMaskIn();
    void exeAlphaMaskOut();
    void exeAlphaMaskWait();
};
