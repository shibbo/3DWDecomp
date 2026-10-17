#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include <math/seadQuat.h>
#include <math/seadVector.h>

class ActorStateSupportFreeze;
namespace al { class JointAimInfo; class JointSpringControllerHolder; }

class SyumockRotate : public al::LiveActor {
public:
    SyumockRotate(const char* pName);
    void init(const al::ActorInitInfo& rInfo) override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) override;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;
    void control() override;
    void exeRotate();
    void exeReaction();
    void exeSupportFreeze();
    void updateMove();

private:
    al::JointAimInfo* mEyeInfo = nullptr;
    ActorStateSupportFreeze* mSupportFreeze;
    al::JointSpringControllerHolder* mJointSpring;
    sead::Quatf mBaseQuat = sead::Quatf::unit;
    sead::Vector3f mOffset = sead::Vector3f::zero;
    sead::Vector3f mSide = sead::Vector3f::ex;
    sead::Vector3f mCenter = sead::Vector3f::zero;
    sead::Vector3f mRotateAxis = sead::Vector3f::ex;
    f32 mAngle = 0.0f;
    f32 mSpeed = 0.5f;
    s32 mReactionStep = 600;
};
static_assert(sizeof(SyumockRotate) == 0x1b0);
