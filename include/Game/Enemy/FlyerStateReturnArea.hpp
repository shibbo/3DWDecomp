#pragma once

#include "Library/Nerve/NerveStateBase.hpp"
#include <basis/seadTypes.h>
#include <math/seadVector.h>

struct FlyerStateParam;
struct FlyerStateReturnAreaParam;

class FlyerStateReturnArea : public al::ActorStateBase {
public:
    FlyerStateReturnArea(al::LiveActor* pHost, sead::Vector3f* pFront,
                         const sead::Vector3f& rReturnPos, const FlyerStateParam* pParam,
                         const FlyerStateReturnAreaParam* pReturnParam);

    u8 _20[0x48 - 0x20];
};

static_assert(sizeof(FlyerStateReturnArea) == 0x48);
