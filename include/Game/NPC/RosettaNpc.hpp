#pragma once

#include <basis/seadTypes.h>

#include "Library/LiveActor/LiveActor.hpp"

namespace al {
class ActorInitInfo;
}  // namespace al

/**
 * @brief Rosalina (ロゼッタNPC), who waits at the goal pole of the course she unlocks.
 */
class RosettaNpc : public al::LiveActor {
public:
    RosettaNpc(const char* pName, const al::ActorInitInfo& rInfo);

    void startGoalPose(s32 catchNum);

private:
    u8 mUnreconstructed148[0xd0];
};

static_assert(sizeof(RosettaNpc) == 0x218);
