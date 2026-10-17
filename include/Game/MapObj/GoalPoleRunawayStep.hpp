#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class GoalPoleRunawayStep : public al::LiveActor {
public:
    GoalPoleRunawayStep(const sead::Matrix34f*);
    ~GoalPoleRunawayStep() override;
    void init(const al::ActorInitInfo&) override;
    void appear() override;
    void exeAppear();
    void exeWait();
private:
    const sead::Matrix34f* mFollowMtx;
};
