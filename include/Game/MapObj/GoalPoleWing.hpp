#pragma once

#include "Library/Obj/PartsModel.hpp"

class GoalPoleWing : public al::PartsModel {
public:
    explicit GoalPoleWing(al::LiveActor* pParent);
    ~GoalPoleWing() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void exeAppear();
    void exeRunaway();

private:
    al::LiveActor* mParent;
};
