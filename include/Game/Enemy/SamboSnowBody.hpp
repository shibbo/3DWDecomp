#pragma once

#include <basis/seadTypes.h>

#include "Library/LiveActor/LiveActor.hpp"

class BallSnow;
class ActorStateSupportFreeze;

/** @brief One stacked snow ball of a snow Pokey, linked to the head that controls it. */
class SamboSnowBody : public al::LiveActor {
public:
    explicit SamboSnowBody(const char* pName);

    void init(const al::ActorInitInfo& rInfo) override;
    void killComplete(bool flag) override;
    void control() override;
    void reappear() override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) override;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer, al::ScreenPointTarget* pTarget) override;
    void exeWait();
    void exeBlowDown();
    void exeFireDown();
    void exeSupportFreeze();
    bool isAttacked();
    void requestBlowDown();
    void requestSupportFreeze(const al::LiveActor* pActor);
    void requestEndSupportFreeze();

    BallSnow* mSnowBall = nullptr;
    bool* mHeadIsStacked = nullptr;                    // 0x150
    bool* mHeadIsRequestAttack = nullptr;              // 0x158
    bool* mHeadIsRequestBlowDown;            // 0x160
    al::LiveActor** mHeadSupportFreezeActor = nullptr;  // 0x168

private:
    sead::Vector3f mBlowVelocity = sead::Vector3f::zero;
    ActorStateSupportFreeze* mFreeze = nullptr;
};
static_assert(sizeof(SamboSnowBody) == 0x188);
