#pragma once

#include "Library/Nerve/NerveStateBase.hpp"

namespace al {
class HitSensor;
class ScreenPointer;
class ScreenPointTarget;
class SensorMsg;
}  // namespace al

class ActorStateSupportStroke;
class RaidonActor;

/// Plessie after every player got off at the goal.
class RaidonEndState : public al::NerveStateBase {
public:
    RaidonEndState(const char* pName, RaidonActor* pHost, ActorStateSupportStroke* pStroke);

    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther);
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf);
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget);

private:
    u8 _11[0x40 - 0x11];
};

static_assert(sizeof(RaidonEndState) == 0x40);
