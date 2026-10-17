#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class TestGraphicObj : public al::LiveActor {
public:
    explicit TestGraphicObj(const char* pName);
    ~TestGraphicObj() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void exeWait();
};
