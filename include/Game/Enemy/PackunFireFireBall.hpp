#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class ActorStateRouteDokanMove;
class PackunFireFireBall : public al::LiveActor {
public:
    explicit PackunFireFireBall(al::LiveActor* pHost);
    void init(const al::ActorInitInfo& rInfo) override;
    void appear() override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) override;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer, al::ScreenPointTarget* pTarget) override;
    void killBySwitch();
    void setParam(const sead::Matrix34f* pMtx, const sead::Quatf& rQuat, f32 offset);
    void exeMove();
    void exeRouteDokan();
    bool tryKill(s32 step);
private:
    al::LiveActor* mHost;
    ActorStateRouteDokanMove* mRouteState = nullptr;
    bool mIsDeflected = false;
    bool mCanHitHost = false;
    s32 mMoveStep = 0;
    s32 mCollisionDelay = 0;
    void* _168 = nullptr;
};
static_assert(sizeof(PackunFireFireBall) == 0x170);
