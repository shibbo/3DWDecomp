#pragma once

#include "Library/Nerve/NerveStateBase.hpp"
#include <basis/seadTypes.h>
#include <math/seadVector.h>

class TargetFinder;
struct FlyerStateParam;
struct FlyerStateFindPlayerParam;

class FlyerStateFindPlayer : public al::ActorStateBase {
public:
    FlyerStateFindPlayer(al::LiveActor* pHost, sead::Vector3f* pFront,
                         TargetFinder* pTargetFinder, const FlyerStateParam* pParam,
                         const FlyerStateFindPlayerParam* pFindParam);

    bool _20;
    bool _21;
    u8 _22[0x48 - 0x22];
};

static_assert(sizeof(FlyerStateFindPlayer) == 0x48);
