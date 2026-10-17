#pragma once
#include "Library/Nerve/NerveStateBase.hpp"
#include <basis/seadTypes.h>
#include <math/seadVector.h>
class TargetFinder;
struct FlyerStateParam;
struct FlyerStateChaseParam;

/** @brief Pursues a player while flying, then slows down. */
class FlyerStateChase : public al::ActorStateBase {
public:
    FlyerStateChase(al::LiveActor* pHost, sead::Vector3f* pFront, TargetFinder* pTargetFinder,
                    const FlyerStateParam* pParam, const FlyerStateChaseParam* pChaseParam);
    /** @brief Destroys the chase state. */
    ~FlyerStateChase() override = default;
    void appear() override;
    void exeChase();
    void exeChaseEnd();
    bool _20 = false; // Enable gradual acceleration.
    bool _21 = false; // Also animate the first sub-actor.
private:
    sead::Vector3f* mFront;
    const FlyerStateParam* mParam;
    const FlyerStateChaseParam* mChaseParam;
    TargetFinder* mTargetFinder;
};
static_assert(sizeof(FlyerStateChase) == 0x48);
