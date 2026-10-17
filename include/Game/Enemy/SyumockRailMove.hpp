#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include <math/seadMatrix.h>
#include <math/seadVector.h>

class ActorStateSupportFreeze;
class EnemyAttachItem;
namespace al { class JointAimInfo; class JointSpringControllerHolder; }

class SyumockRailMove : public al::LiveActor {
public:
    SyumockRailMove(const char* pName);
    void init(const al::ActorInitInfo& rInfo) override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) override;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;
    void control() override;
    void exeMove();
    bool isHitHeadCylinder(al::HitSensor* pOther, al::HitSensor* pSelf) const;
    bool isHitHeadCylinder(sead::Vector3f* pReflect, al::HitSensor* pOther, al::HitSensor* pSelf) const;
    bool isHitSensorsSingleMode(al::HitSensor* pSelf, al::HitSensor* pOther);
    void exeReaction();
    void exeSupportFreeze();
    void updateMove();

private:
    ActorStateSupportFreeze* mSupportFreeze = nullptr;
    sead::Vector3f mClippingCenter = sead::Vector3f::zero;
    sead::Matrix34f mItemMtx = sead::Matrix34f::ident;
    al::JointAimInfo* mEyeInfo = nullptr;
    al::JointSpringControllerHolder* mJointSpring;
    sead::Vector3f mSide = sead::Vector3f::ex;
    f32 mSpeed = 10.0f;
    EnemyAttachItem* mItem = nullptr;
    s32 mReactionStep = 600;
    bool mInWater = false;
    bool mIsPlacedKinopioBrigade = false;
    bool mIsSingleMode = false;
};
static_assert(sizeof(SyumockRailMove) == 0x1c0);
