#pragma once
#include "NPC/NpcStateChaseParam.hpp"

#include "Library/Nerve/NerveStateBase.hpp"
#include <math/seadVector.h>

class NpcTargetFinder;
class NpcStateParam;
class NpcStateChaseParam;

class NpcStateChase : public al::ActorStateBase {
public:
    NpcStateChase(al::LiveActor* pActor, sead::Vector3f* pDirection,
                  NpcTargetFinder* pTargetFinder, const NpcStateParam* pStateParam,
                  const NpcStateChaseParam* pChaseParam, bool requireVisibleTarget,
                  bool* pIsGrounded);
    ~NpcStateChase() override;
    void appear() override;
    void exeStart();
    void exeEnd();

private:
    sead::Vector3f* mDirection;
    const NpcStateParam* mStateParam;
    const NpcStateChaseParam* mChaseParam;
    bool mRequireVisibleTarget;
    bool* mIsGrounded;
    int mLostTargetFrames;
    NpcTargetFinder* mTargetFinder;
};
