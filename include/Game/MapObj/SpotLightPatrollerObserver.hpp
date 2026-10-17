#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class SpotLightPatroller;
class SpotLightPatrollerObserver : public al::LiveActor {
public:
    SpotLightPatrollerObserver(const char*);
    ~SpotLightPatrollerObserver() override;
    void init(const al::ActorInitInfo&) override;
    void exePatrol();
    void exeAlert();
private:
    int mPatrollerCount = 0;
    SpotLightPatroller** mPatrollers = nullptr;
};
