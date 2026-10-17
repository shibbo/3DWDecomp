#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class PlayerKeyConfig;
class PlayerInput;
class TestAndoBowserJr : public al::LiveActor {
public:
    explicit TestAndoBowserJr(const char* pName);
    ~TestAndoBowserJr() override;
    void init(const al::ActorInitInfo& rInfo) override;
    bool isAssistPadConnected() const;
    void exeHide();
    void exeWait();
    void exeAttackSpin();
    void control() override;

private:
    PlayerKeyConfig* mKeyConfig;
    PlayerInput* mInput;
};
