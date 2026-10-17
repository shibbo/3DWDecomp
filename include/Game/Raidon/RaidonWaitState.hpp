#pragma once

#include "Library/Nerve/NerveStateBase.hpp"

namespace al {
class HitSensor;
class ScreenPointer;
class ScreenPointTarget;
class SensorMsg;
}  // namespace al

class ActorStateSupportStroke;
class RaidonBase;

/// Plessie waiting for players to get on.
class RaidonWaitState : public al::NerveStateBase {
public:
    RaidonWaitState(const char* pName, RaidonBase* pHost, ActorStateSupportStroke* pStroke);

    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf);
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget);

private:
    u8 _11[0x38 - 0x11];
};

static_assert(sizeof(RaidonWaitState) == 0x38);
