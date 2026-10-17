#pragma once
#include "Library/Nerve/NerveStateBase.hpp"
#include "Raidon/RaidonActor.hpp"
class ActorStateSupportStroke;
namespace al {
class HitSensor;
class SensorMsg;
class ScreenPointer;
class ScreenPointTarget;
} // namespace al
class RaidonEndState : public al::HostStateBase<RaidonActor> {
public:
    RaidonEndState(const char* pName, RaidonActor* pActor, ActorStateSupportStroke* pStroke);
    void appear() override;
    void kill() override;
    void control() override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther);
    bool receiveMsg(const al::SensorMsg* pMessage, al::HitSensor* pOther, al::HitSensor* pSelf);
    bool receiveMsgScreenPoint(const al::SensorMsg* pMessage, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget);
    /** @brief Starts directly in the waiting phase on appearance. */
    void enableWaitStart() { mStartWaiting = true; }

private:
    ActorStateSupportStroke* mStroke;
    unsigned char mUnknown28[0x14];
    bool mStartWaiting;
};
static_assert(sizeof(RaidonEndState) == 0x40);
