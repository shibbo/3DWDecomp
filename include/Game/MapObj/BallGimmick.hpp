#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class ActorStateSupportFreeze;
class BallGimmick : public al::LiveActor {
public:
    explicit BallGimmick(const char*);
    ~BallGimmick() override;
    void init(const al::ActorInitInfo&) override;
    void kill() override;
    void control() override;
    void attackSensor(al::HitSensor*, al::HitSensor*) override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    bool receiveMsgScreenPoint(const al::SensorMsg*, al::ScreenPointer*, al::ScreenPointTarget*) override;
    void exePreFall();
    void boundGround();
    void rotateBall();
    void boundWall();
    void exeFall();
    void exeRolling();
    void exeSupportFreeze();
private:
    sead::Vector3f mHomeTrans = sead::Vector3f::zero;
    sead::Vector3f mPreviousTrans = sead::Vector3f::zero;
    sead::Matrix34f mEffectMtx = sead::Matrix34f::ident;
    ActorStateSupportFreeze* mFreezeState = nullptr;
    sead::Vector3f mFreezeTrans;
    al::HitSensor* mCollideSensor = nullptr;
};
static_assert(sizeof(BallGimmick) == 0x1b0);
