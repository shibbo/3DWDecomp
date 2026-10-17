#pragma once

#include <basis/seadTypes.h>
#include <math/seadMatrix.h>

#include "Library/Nerve/NerveStateBase.hpp"

namespace al {
class ActorInitInfo;
class KeyPoseKeeper;
}  // namespace al

class GoalPole;
class GoalPoleWing;
class GoalPoleRunawayStep;

/**
 * @brief State of the runaway goal pole (逃げる), which flees from the players on a cloud.
 */
class GoalPoleStateRunaway : public al::NerveStateBase {
public:
    GoalPoleStateRunaway(GoalPole* pPole, const al::ActorInitInfo& rInfo,
                         const sead::Matrix34f* pBaseMtx);

    void start();
    void kill() override;
    void appearStep();
    void startMove();
    void updateWaitTrans();
    void addFloatTrans();
    void updateMoveTransAndRotate();
    void updateRotate();
    bool isMoveStop(float) const;
    void exeStandBy();
    void exeWait();
    void exeMove();
    void exeWaitNearPlayer();
    void exeStop();

private:
    GoalPole* mPole;
    al::KeyPoseKeeper* mKeyPose = nullptr;
    float mMoveStopDistance = 2000.0f;
    float mMoveRestartDistance = 1800.0f;
    s32 mWaitTime = 0;
    s32 mMoveTime = 60;
    s32 mMoveStep = 0;
    s32 _3c = 0;
    float mFloatAngle = 0.0f;
    sead::Vector3f mForward = sead::Vector3f::ez;
    GoalPoleWing* mWing = nullptr;
    GoalPoleRunawayStep* mStep = nullptr;
};

static_assert(sizeof(GoalPoleStateRunaway) == 0x60);
