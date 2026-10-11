#pragma once

#include <basis/seadTypes.h>
#include <math/seadMatrix.h>

#include "Library/Nerve/NerveStateBase.hpp"

namespace al {
class ActorInitInfo;
}  // namespace al

class GoalPole;

/**
 * @brief State of the runaway goal pole (逃げる), which flees from the players on a cloud.
 */
class GoalPoleStateRunaway : public al::NerveStateBase {
public:
    GoalPoleStateRunaway(GoalPole* pPole, const al::ActorInitInfo& rInfo,
                         const sead::Matrix34f* pBaseMtx);

    void appearStep();

private:
    u8 mUnreconstructed18[0x48];
};

static_assert(sizeof(GoalPoleStateRunaway) == 0x60);
