#pragma once

#include "Library/Nerve/NerveStateBase.hpp"
#include <math/seadVector.h>

class TargetFinder;
struct FlyerStateParam;
struct FlyerStateWanderParam;

class FlyerStateWander : public al::ActorStateBase {
public:
    FlyerStateWander(al::LiveActor* pHost, sead::Vector3f* pCenter,
                    TargetFinder* pTargetFinder, const FlyerStateParam* pParam,
                    const FlyerStateWanderParam* pWanderParam);
    /** @brief Destroys the wandering state. */
    ~FlyerStateWander() override = default;
    void appear() override;
    void exeWander();
    void exeWait();

private:
    int mStepDuration = 0;
    sead::Vector3f* mCenter;
    sead::Vector3f mWanderTarget = {0.0f, 0.0f, 0.0f};
    const FlyerStateParam* mParam;
    const FlyerStateWanderParam* mWanderParam;
    TargetFinder* mTargetFinder;
};

static_assert(sizeof(FlyerStateWander) == 0x58);
