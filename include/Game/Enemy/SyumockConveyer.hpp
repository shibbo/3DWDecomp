#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include <math/seadVector.h>
class ActorStateSupportFreeze;
namespace al { class ConveyerKeyKeeper; class JointAimInfo; class JointSpringControllerHolder; }

class SyumockConveyer : public al::LiveActor {
public:
    SyumockConveyer(const char* pName);
    void init(const al::ActorInitInfo& rInfo) override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) override;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;
    void control() override;
    void setConveyerKeyKeeper(const al::ConveyerKeyKeeper* pKeeper, f32 maxCoord);
    void setTransByCoord(f32 coord, bool forward, bool reset);
    void exeMove();
    void exeReaction();
    void exeSupportFreeze();
    void exeEnd();
private:
    const al::ConveyerKeyKeeper* mKeys = nullptr;
    ActorStateSupportFreeze* mSupportFreeze = nullptr;
    al::JointAimInfo* mEyeInfo = nullptr;
    al::JointSpringControllerHolder* mJointSpring;
    sead::Vector3f mSide = sead::Vector3f::ex;
    f32 mCoord = 0.0f;
    f32 mMaxCoord = 0.0f;
    s32 mReactionStep = 600;
};
static_assert(sizeof(SyumockConveyer) == 0x180);
