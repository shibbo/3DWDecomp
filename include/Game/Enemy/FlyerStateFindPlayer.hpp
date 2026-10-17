#pragma once
#include "Library/Nerve/NerveStateBase.hpp"
#include <basis/seadTypes.h>
#include <math/seadVector.h>
class TargetFinder;
struct FlyerStateParam;
struct FlyerStateFindPlayerParam;

/** @brief Turns a flying enemy toward a newly discovered player. */
class FlyerStateFindPlayer : public al::ActorStateBase {
public:
    FlyerStateFindPlayer(al::LiveActor* pHost, sead::Vector3f* pFront,
                         TargetFinder* pTargetFinder, const FlyerStateParam* pParam,
                         const FlyerStateFindPlayerParam* pFindParam);
    /** @brief Destroys the player discovery state. */
    ~FlyerStateFindPlayer() override = default;
    void appear() override;
    void kill() override;
    void exeTurnPlayer();
    void exeFind();
    bool _20 = false; // Play the discovery action after turning.
    bool _21 = false; // Also animate the first sub-actor.
private:
    sead::Vector3f* mFront;
    TargetFinder* mTargetFinder;
    const FlyerStateParam* mParam;
    const FlyerStateFindPlayerParam* mFindParam;
};
static_assert(sizeof(FlyerStateFindPlayer) == 0x48);
