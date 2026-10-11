#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class TestKitazonoBlockBrickBreak : public al::LiveActor {
public:
    TestKitazonoBlockBrickBreak(const char* pName);
    ~TestKitazonoBlockBrickBreak() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void control() override;
    void breakBlock(const sead::Vector3f& rTrans);
    void exeWait();
    void exeBreak();
};
