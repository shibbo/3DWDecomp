#pragma once

#include "Library/Nerve/NerveStateBase.hpp"
#include <basis/seadTypes.h>
#include <math/seadVector.h>

class TargetFinder;
struct FlyerStateParam;
struct FlyerStateChaseParam;

class FlyerStateChase : public al::ActorStateBase {
public:
    FlyerStateChase(al::LiveActor* pHost, sead::Vector3f* pFront, TargetFinder* pTargetFinder,
                    const FlyerStateParam* pParam, const FlyerStateChaseParam* pChaseParam);

    bool _20;
    bool _21;
    u8 _22[0x48 - 0x22];
};

static_assert(sizeof(FlyerStateChase) == 0x48);
