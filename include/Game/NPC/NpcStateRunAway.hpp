#pragma once
#include "NPC/NpcStateRunAwayParam.hpp"

#include "Library/Nerve/NerveStateBase.hpp"
#include <math/seadVector.h>

class NpcTargetFinder;
class NpcStateParam;
class NpcStateRunAwayParam;

class NpcStateRunAway : public al::ActorStateBase {
public:
    /** @brief Gets the threat position. @return the threat position. */
    const sead::Vector3f& getThreatPos() const { return mTargetPosition; }
    /** @brief Gets whether avoidance overrides running away. @return whether avoidance overrides running away. */
    bool isRunStraight() const { return mIsAvoiding; }
    NpcStateRunAway(al::LiveActor* pActor, sead::Vector3f* pDirection,
                  NpcTargetFinder* pTargetFinder, const NpcStateParam* pStateParam,
                  const NpcStateRunAwayParam* pRunAwayParam, bool requireVisibleTarget,
                  bool* pIsGrounded);
    ~NpcStateRunAway() override;
    void appear() override;
    void exeStart();
    void exeEnd();
    void updateDirectionPtr();

private:
    sead::Vector3f* mDirection;
    sead::Vector3f mTargetPosition;
    const NpcStateParam* mStateParam;
    const NpcStateRunAwayParam* mRunAwayParam;
    bool mRequireVisibleTarget;
    bool* mIsGrounded;
    int mLostTargetFrames;
    NpcTargetFinder* mTargetFinder;
    float mTurnOffset;
    int mAvoidFrames;
    sead::Vector3f mAvoidPosition;
    bool mIsAvoiding;
};
