#pragma once

#include "Library/Nerve/NerveStateBase.hpp"
#include <math/seadVector.h>

namespace al {
struct ActorInitInfo;
class HitSensor;
class SensorMsg;
class ScreenPointer;
class ScreenPointTarget;
class RumbleCalculatorCosMultLinear;
} // namespace al
class ActorStateSupportStroke;

class NpcStateWaitParam {
public:
    NpcStateWaitParam(const char* pWaitAction, const char* pWaitAfterAction,
                      const char* pTurnAction, const char* pReactionAction,
                      const char* pMicReactionAction, const char* pStrokeAction,
                      const char* pTrampledAction, bool appearItem,
                      const sead::Vector3f* pItemOffset, bool restrictReaction);
    const char* mWaitAction;
    const char* mWaitAfterAction;
    const char* mTurnAction;
    const char* mReactionAction;
    const char* mMicReactionAction;
    const char* mStrokeAction;
    const char* mTrampledAction;
    bool mAppearItem;
    sead::Vector3f mItemOffset;
    bool mRestrictReaction;
};

class NpcStateTurnParam {
public:
    NpcStateTurnParam(float startAngle, float endAngle, float turnDegrees, float searchRadius,
                      bool isEnabled, bool onlyAfter, int endFrames);
    float mStartAngle;
    float mEndAngle;
    float mTurnDegrees;
    float mSearchRadius;
    bool mIsEnabled;
    bool mOnlyAfter;
    int mEndFrames;
};

class NpcStateRumbleParam {
public:
    NpcStateRumbleParam(int frames, float frequency, float angleOffset, float amplitude,
                        float scale);
    int mFrames;
    float mFrequency;
    float mAngleOffset;
    float mAmplitude;
    float mScale;
};

class NpcStateWait : public al::ActorStateBase {
public:
    NpcStateWait(al::LiveActor* pActor, const al::ActorInitInfo& rInitInfo,
                 const NpcStateWaitParam* pWaitParam, const NpcStateTurnParam* pTurnParam,
                 const NpcStateRumbleParam* pRumbleParam);
    ~NpcStateWait() override;
    void appear() override;
    void control() override;
    /** @brief Gets whether turning is disabled. @return whether turning is disabled. */
    bool isInvalidTurn() const { return mIsTurnDisabled; }
    void setWaitAfter();
    void setWait();
    /** @brief Reports whether searching turns are disabled. @return Whether the actor should face
     * its initial direction. */
    bool isTurnDisabled() const { return mIsTurnDisabled; }
    /** @brief Reports the alternate waiting phase. @return Whether the alternate phase is active.
     */
    bool isWaitAfter() const { return mIsWaitAfter; }
    /** @brief Changes waiting behavior. @param pParam Waiting parameters, kept alive by the caller.
     */
    void setWaitParam(const NpcStateWaitParam* pParam) { mWaitParam = pParam; }
    /** @brief Changes turning behavior. @param pParam Turning parameters, kept alive by the caller.
     */
    void setTurnParam(const NpcStateTurnParam* pParam) { mTurnParam = pParam; }
    void startWait();
    void invalidateTurn();
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther);
    bool receiveMsg(const al::SensorMsg* pMessage, al::HitSensor* pOther, al::HitSensor* pSelf);
    bool tryStartReaction();
    bool receiveMsgScreenPoint(const al::SensorMsg* pMessage, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget);
    void startTurnEnd();
    bool tryStartTurn(const NpcStateTurnParam* pParam);
    bool tryStartMicReaction();
    void exeWait();
    void exeStroke();
    void exeWaitAfter();
    void exeTurn();
    void exeTrampled();
    void exeReaction();
    void exeMicReaction();
    void exeTurnEnd();

private:
    ActorStateSupportStroke* mStrokeState;
    const NpcStateWaitParam* mWaitParam;
    const NpcStateTurnParam* mTurnParam;
    const NpcStateRumbleParam* mRumbleParam;
    al::RumbleCalculatorCosMultLinear* mRumbleCalculator;
    int mRumbleFrame;
    int mReactionCooldown;
    bool mIsWaitAfter;
    bool mIsTurnDisabled;
};
