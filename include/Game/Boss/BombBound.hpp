#pragma once

#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"

class BombStateExplosion;

class BombBound : public al::LiveActor {
public:
    /** @brief How a launched bomb moves before it settles. */
    enum Type {
        Type_Roll = 0,
        Type_Bound = 1,
    };

    explicit BombBound(const char* pName);
    void init(const al::ActorInitInfo& rInfo) override;
    void makeActorAppeared() override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool canKicked(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) const;
    bool canKickedSM(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) const;
    bool canBoomerangHit() const;
    bool goExplosion();
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;
    void control() override;
    void reload(bool isBlink);
    void resetInner();
    void launch(Type type, const sead::Vector3f& rVelocity, bool isStartFuse);
    void thrown(Type type, const sead::Vector3f& rVelocity, bool isStartFuse);
    void vanish();
    bool isExplodingOrKicked() const;
    void exeBound();
    void exeRoll();
    void endRoll();
    void exeKicked();
    void exeStop();
    void exeExplosion();
    void exeBreak();
    void exePlaced();
    void endPlaced();

    /** @brief Waits in the generator until launched. */
    void exeWait() {}

private:
    BombStateExplosion* mStateExplosion = nullptr;
    BombStateExplosion* mStateBreak = nullptr;
    al::HitSensor* mKickerSensor = nullptr;
    s32 mControlUserId = -1;
    s32 mFuseTimer = 0;
    Type mType = Type_Roll;
    f32 mSpeedH = 10.0f;
    sead::Vector3f mSmokePos = sead::Vector3f::zero;
    s32 mCollideStartStep = 10;
    bool mIsSingleMode = false;
    bool mIsFuseEnabled = true;
};
static_assert(sizeof(BombBound) == 0x188);
