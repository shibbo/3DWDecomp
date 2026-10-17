#pragma once

#include <prim/seadSafeString.h>

namespace al {
struct ActorParamMove;
}

struct FlyerStateWanderParam {
    FlyerStateWanderParam(int stepRandomRange, int stepWander, int stepWait,
                         const char* pAction, const al::ActorParamMove* pMoveParam);

    int mStepRandomRange;
    int mStepWander;
    int mStepWait;
    sead::FixedSafeString<32> mAction;
    const al::ActorParamMove* mMoveParam;
};

static_assert(sizeof(FlyerStateWanderParam) == 0x50);
