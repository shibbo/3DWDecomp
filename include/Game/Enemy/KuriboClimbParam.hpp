#pragma once

#include <basis/seadTypes.h>

struct ActorStateSupportFreezeParam;
class TargetFinderParam;
class WalkerStateChaseParam;
class WalkerStateFindPlayerParam;
class WalkerStateJumpParam;
class WalkerStateWanderParam;
struct WalkerStateParam;

/** @brief Shared tuning values of the climbing Goombas (KuriboClimb, KuriboClimbRail). */
class KuriboClimbParam {
public:
    const TargetFinderParam* getParamTargetFinder() const;
    const WalkerStateParam* getParamBase() const;
    const WalkerStateWanderParam* getParamWander() const;
    const WalkerStateChaseParam* getParamChase() const;
    const WalkerStateFindPlayerParam* getParamFindPlayer() const;
    const WalkerStateJumpParam* getParamJumpFind() const;
    const ActorStateSupportFreezeParam* getParamSupportFreeze() const;
    f32 getPushPower() const;
    f32 getBodyAttackStartDistance() const;
};
