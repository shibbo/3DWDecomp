#pragma once
#include "Library/Nerve/NerveStateBase.hpp"
#include <math/seadVector.h>
class DarkBowser;
namespace al { class ActorInitInfo; }
/** @brief Fury Bowser's shell spin and turning movement state. */
class DarkBowserShellSpin : public al::HostStateBase<DarkBowser> {
public:
    DarkBowserShellSpin(DarkBowser* pHost, const al::ActorInitInfo& rInfo);
    ~DarkBowserShellSpin() override;
    void appear() override;
    void kill() override;
    void control() override;
    void exeBegin();
    void exeSpin();
    void calcTargetPos();
    void spinFront();
    void exeReverse();
    float getMaxSpeedH() const;
    float getMaxSpeedV() const;
private:
    sead::Vector3f mTargetPos = sead::Vector3f::zero;
    sead::Vector3f mMoveDir = sead::Vector3f::ez;
    sead::Vector3f mTurnDir = sead::Vector3f::zero;
    int mReverseCount = 0;
    bool mIsTracking = true;
    int mLevel = 0;
    int mTurnBoost = 0;
    sead::Vector3f mVelocity = sead::Vector3f::zero;
};
static_assert(sizeof(DarkBowserShellSpin) == 0x60);
