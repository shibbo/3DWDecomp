#pragma once

#include "Library/Nerve/NerveStateBase.hpp"

namespace al {
class HitSensor;
class ScreenPointer;
class ScreenPointTarget;
class SensorMsg;
}  // namespace al

class ActorStateSupportStroke;
class RaidonSurf;

/// Surfing Plessie waiting for players to get on, reacting to them and to touch input.
/// @note Only what reconstructed code needs is declared so far.
class RaidonSurfWaitState : public al::NerveStateBase {
public:
    RaidonSurfWaitState(const char* pName, RaidonSurf* pHost, ActorStateSupportStroke* pStroke);

    void setReactionNerve(s32 step);
    bool trySetReactionNerve(const al::SensorMsg* pMsg);
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf);
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget);
    bool isReacting();

    /** @brief Makes the state start with the first seen demo reaction. */
    void setFirstSeen() { mIsFirstSeen = true; }

private:
    u8 _11[0x34 - 0x11];
    bool mIsFirstSeen;  // 0x34
    u8 _35[0x38 - 0x35];
};

static_assert(sizeof(RaidonSurfWaitState) == 0x38);
