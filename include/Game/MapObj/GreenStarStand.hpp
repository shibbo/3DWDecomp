#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class GreenStarStand : public al::LiveActor {
public:
    GreenStarStand(const char*);
    ~GreenStarStand() override;
    void init(const al::ActorInitInfo&) override;
    void switchOn();
    void exeBeforeWait();
    void exeAppear();
    void exeAfterWait();
};
