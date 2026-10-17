#pragma once

#include <basis/seadTypes.h>
#include <math/seadQuat.h>
#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"

namespace al {
template <class T>
class DeriveActorGroup;
class JointAimInfo;
class ParabolicPath;
}  // namespace al
class ActorStateSupportFreeze;
class MeraWanwanTrack;
class TargetFinder;

/** @brief Burning chain chomp ball (MeraWanwan) that rolls after the player and leaves fire tracks. */
class MeraWanwan : public al::LiveActor {
public:
    explicit MeraWanwan(const char* pName);

    void makeActorAppeared() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool isEnablePush() const;
    bool isEnableAttack() const;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    bool isEnableBreak(const al::SensorMsg* pMsg) const;
    bool isEnableBlow(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                      al::HitSensor* pSelf) const;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;
    bool isEnableSupportFreeze() const;
    void control() override;
    void setFindDistance(f32 distance);
    void appearByBossGorobon();
    void forceDead();
    void deleteTrack();
    void exeHide();
    void exeAppear();
    bool trySink();
    void exeAppearLand();
    void updateVelocity();
    void exeChase();
    void emitTrack();
    void exeFall();
    void exeLand();
    void exeBlow();
    void exeSink();
    void exeSinkHide();
    void exeSupportFreeze();
    void exeDead();

private:
    al::DeriveActorGroup<MeraWanwanTrack>* mTrackGroup = nullptr;  // 0x148
    TargetFinder* mTargetFinder = nullptr;
    al::ParabolicPath* mParabolicPath = nullptr;
    al::JointAimInfo* mJointAimInfoL = nullptr;
    al::JointAimInfo* mJointAimInfoR = nullptr;
    ActorStateSupportFreeze* mStateSupportFreeze = nullptr;
    sead::Quatf mBodyQuat = sead::Quatf::unit;  // 0x178
    sead::Vector3f mFrontDir = sead::Vector3f::ez;
    sead::Vector3f mBlowDir = sead::Vector3f::ez;
    sead::Vector3f mMoment = sead::Vector3f::zero;
    sead::Vector3f mPrevTrans;  // 0x1ac
    MeraWanwanTrack* mLastTrack = nullptr;
    f32 mAppearRange = 1500.0f;
    s32 mAirTime = 0;
    bool mIsAppearFromLink = false;  // 0x1c8

public:
    bool _1C9 = true;  ///< Whether the MeraWanwan hides again after sinking instead of dying.

private:
    sead::Vector3f mInitTrans = sead::Vector3f::zero;  // 0x1cc
    sead::Quatf mInitQuat = sead::Quatf(1.0f, 0.0f, 0.0f, 0.0f);
    bool mIsRestored = false;  // 0x1e8
};
static_assert(sizeof(MeraWanwan) == 0x1f0);
