#pragma once
#include "Player/Normal/PlayerModel.hpp"
class PlayerRetargettingSelector;
class ActorStateSupportStroke;
class RosettaNpc : public PlayerModel {
public:
    RosettaNpc(const char* pName, const al::ActorInitInfo& rInfo);
    ~RosettaNpc() override;
    void control() override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMessage, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMessage, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;
    bool tryStartGuard();
    void exeWait();
    void exeGoalPose();
    void exeBarrier();
    void exeBarrierLoop();
    void exeGuard();
    void startGoalPose(int rank);

private:
    PlayerRetargettingSelector* mSelector;
    ActorStateSupportStroke* mStroke;
    sead::Vector3f mInitialFront;
    bool mJointFlag;
    sead::Quatf mQuat;
    int mGoalRank;
    int mGuardCooldown;
};
static_assert(sizeof(RosettaNpc) == 0x218);
