#pragma once
#include "Library/Obj/PartsModel.hpp"
class GoalPole;
class GoalPoleFlag : public al::PartsModel {
public:
    explicit GoalPoleFlag(GoalPole*);
    void init(const al::ActorInitInfo&) override;
    void initAfterPlacement() override;
    void kill() override;
    void control() override;
    void updateFollowMtx();
    void appearBottom(int);
    void startUpward(float);
    void startLerp();
    void exeWait();
    void exeUpward();
    void exeReachTop();
    /** @brief Sets the Y rotation of the flag. @param rotateY Rotation in degrees. */
    void setRotateY(float rotateY) { mRotateY = rotateY; }
private:
    GoalPole* mPole;
    float mTargetHeight = 700.0f;
    sead::Matrix34f mFollowMtx;
    float mHeight = 700.0f;
    float mRotateY = 0.0f;
};
static_assert(sizeof(GoalPoleFlag) == 0x1d0);
