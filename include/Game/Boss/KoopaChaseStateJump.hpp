#pragma once
#include "Library/Nerve/NerveStateBase.hpp"
#include <math/seadVector.h>
namespace al { class ActorInitInfo; class ParabolicPathMovement; }
class KoopaChase;

class KoopaChaseStateJump : public al::HostStateBase<KoopaChase> {
public:
    KoopaChaseStateJump(KoopaChase* pHost, const al::ActorInitInfo& rInfo);
    void startJump(const sead::Vector3f& rTarget);
    void startWarp(const sead::Vector3f& rTarget);
    void exeJumpStart();
    void exeJump();
    void exeJumpEnd();

private:
    sead::Vector3f mStartPosition;
    sead::Vector3f mTargetPosition;
    al::ParabolicPathMovement* mMovement;
    bool mIsWarp = false;
};
static_assert(sizeof(KoopaChaseStateJump) == 0x48);
