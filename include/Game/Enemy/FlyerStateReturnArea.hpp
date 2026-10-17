#pragma once
#include "Library/Nerve/NerveStateBase.hpp"
#include <basis/seadTypes.h>
#include <math/seadVector.h>
struct FlyerStateParam;
struct FlyerStateReturnAreaParam;

/** @brief Brakes, turns, and returns a flying enemy to its home area. */
class FlyerStateReturnArea : public al::ActorStateBase {
public:
    FlyerStateReturnArea(al::LiveActor* pHost, sead::Vector3f* pFront,
                         const sead::Vector3f& rReturnPos, const FlyerStateParam* pParam,
                         const FlyerStateReturnAreaParam* pReturnParam);
    /** @brief Destroys the return state. */
    ~FlyerStateReturnArea() override = default;
    void appear() override;
    void exeBrake();
    void exeReturnRotate();
    void exeReturn();
private:
    sead::Vector3f* mFront;
    sead::Vector3f mReturnPos;
    const FlyerStateParam* mParam;
    const FlyerStateReturnAreaParam* mReturnParam;
};
static_assert(sizeof(FlyerStateReturnArea) == 0x48);
