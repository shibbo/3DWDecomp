#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include <math/seadMatrix.h>
#include <math/seadVector.h>

/** @brief Hammer carried and thrown by a Hammer Bro. */
class HammerBrosHammer : public al::LiveActor {
public:
    explicit HammerBrosHammer(const char* pName);
    void init(const al::ActorInitInfo& rInfo) override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) override;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;
    void attach(const sead::Matrix34f* pMtx, const sead::Vector3f& rTrans,
                const sead::Vector3f& rRotate, const char* pAction);
    void shoot(const al::LiveActor* pHost, const sead::Vector3f& rVelocity);
    void exeWait();
    void exeMove();
private:
    const al::LiveActor* mHost = nullptr;
    const sead::Matrix34f* mFollowMtx = nullptr;
    sead::Vector3f mOffset = sead::Vector3f::zero;
    sead::Vector3f mRotate = sead::Vector3f::zero;
};
static_assert(sizeof(HammerBrosHammer) == 0x170);
