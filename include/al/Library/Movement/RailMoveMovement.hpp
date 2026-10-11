#pragma once

#include "Library/Movement/MoveType.hpp"
#include "Library/Nerve/NerveStateBase.hpp"

namespace al {
class ActorInitInfo;

class RailMoveMovement : public HostStateBase<LiveActor> {
public:
    RailMoveMovement(LiveActor* pHost, const ActorInitInfo& rInfo);

    float getSpeed() const { return mSpeed; }
    void setSpeed(float speed) { mSpeed = speed; }
    void exeMove();
    void exeStandby();

private:
    f32 mSpeed = 10.0f;
    MoveType mMoveType = MoveType::Loop;
    s32 mWaitTime = -1;
};

static_assert(sizeof(RailMoveMovement) == 0x30);

RailMoveMovement* tryCreateRailMoveMovement(LiveActor* pHost, const ActorInitInfo& rInfo);
}  // namespace al
