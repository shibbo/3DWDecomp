#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>

#include "Library/Nerve/NerveStateBase.hpp"

namespace al {
class ActorInitInfo;
class HitSensor;
class LiveActor;
class RumbleCalculatorCosMultLinear;
class ScreenPointer;
class ScreenPointTarget;
class SensorMsg;
}  // namespace al

class ActorStateSupportStroke;
class NpcStateRumbleParam;
class NpcStateTurnParam;

/**
 * @brief Parameters of the NPC wait state: the action names it plays and how it reacts.
 */
class NpcStateWaitParam {
public:
    NpcStateWaitParam(const char* pWaitActionName, const char* pWaitAfterActionName,
                      const char* pTurnActionName, const char* pReactionActionName,
                      const char* pReactionMicActionName, const char* pTouchActionName,
                      const char* pTrampledActionName, bool isAppearItemOnStroke,
                      const sead::Vector3f* pItemOffset, bool isReactOnlyBlowDown);

    const char* mWaitActionName;         // 0x0
    const char* mWaitAfterActionName;    // 0x8
    const char* mTurnActionName;         // 0x10
    const char* mReactionActionName;     // 0x18
    const char* mReactionMicActionName;  // 0x20
    const char* mTouchActionName;        // 0x28
    const char* mTrampledActionName;     // 0x30
    bool mIsAppearItemOnStroke;          // 0x38
    sead::Vector3f mItemOffset;          // 0x3c
    bool mIsReactOnlyBlowDown;           // 0x48
};

static_assert(sizeof(NpcStateWaitParam) == 0x50);

/**
 * @brief NPC state that waits in place, turning to and reacting at the player.
 */
class NpcStateWait : public al::ActorStateBase {
public:
    NpcStateWait(al::LiveActor* pHost, const al::ActorInitInfo& rInfo,
                 const NpcStateWaitParam* pWaitParam, const NpcStateTurnParam* pTurnParam,
                 const NpcStateRumbleParam* pRumbleParam);

    void setWaitAfter();
    void setWait();
    void appear() override;
    void startWait();
    void control() override;
    void invalidateTurn();
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther);
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSelf, al::HitSensor* pOther);
    bool tryStartReaction();
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget);
    void exeWait();
    void exeWaitAfter();
    void exeTurn();
    void startTurnEnd();
    void exeTurnEnd();
    void exeReaction();
    void exeMicReaction();
    void exeStroke();
    void exeTrampled();

    /** @return Whether turning to the player has been disabled. */
    bool isInvalidTurn() const { return mIsInvalidTurn; }

private:
    bool tryStartMicReaction();

    ActorStateSupportStroke* mStateStroke = nullptr;   // 0x20
    const NpcStateWaitParam* mWaitParam;               // 0x28
    const NpcStateTurnParam* mTurnParam;               // 0x30
    const NpcStateRumbleParam* mRumbleParam;           // 0x38
    al::RumbleCalculatorCosMultLinear* mRumble = nullptr;  // 0x40
    s32 mRumbleFrame = -1;                             // 0x48
    s32 mReactionCooldown = 0;                         // 0x4c
    bool mIsWaitAfter = false;                         // 0x50
    bool mIsInvalidTurn = false;                       // 0x51
};

static_assert(sizeof(NpcStateWait) == 0x58);
