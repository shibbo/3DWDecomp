#pragma once

#include <basis/seadTypes.h>

#include "Library/Nerve/NerveStateBase.hpp"

namespace al {
class ActorInitInfo;
class HitSensor;
}  // namespace al
struct WalkerStateParam;
class WalkerStateRouteDokanMoveParam;

/** @brief Walker state that moves the host through route pipes (route dokan). */
class WalkerStateRouteDokanMove : public al::ActorStateBase {
public:
    WalkerStateRouteDokanMove(al::LiveActor* pHost, const al::ActorInitInfo& rInfo,
                              const WalkerStateParam* pParam,
                              const WalkerStateRouteDokanMoveParam* pMoveParam);

    bool tryStart(al::HitSensor* pSelf, al::HitSensor* pOther);
    bool isMove() const;
    bool isEject() const;

private:
    u8 _20[0x48];
};

static_assert(sizeof(WalkerStateRouteDokanMove) == 0x68);
