#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include <math/seadQuat.h>
namespace al { template <class T> class DeriveActorGroup; }
class KoopaFireBall;
class KoopaLastBeforeCheckPoint : public al::LiveActor {
public:
    explicit KoopaLastBeforeCheckPoint(const char* pName);
    ~KoopaLastBeforeCheckPoint() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void appear() override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) override;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer, al::ScreenPointTarget* pTarget) override;
    void exeAppear();
    void exeLand();
    void exeWait();
    void exeAttackStart();
    void exeAttackCircle();
    void exeAttackEnd();
    void exeDamage();
private:
    int mAttackSide = 0;
    sead::Vector3f mHomePos = {0.0f, 0.0f, 0.0f};
    sead::Quatf mAimQuat = sead::Quatf::unit;
    sead::Quatf mLeftQuat = sead::Quatf::unit;
    sead::Quatf mRightQuat = sead::Quatf::unit;
    al::DeriveActorGroup<KoopaFireBall>* mFireballs = nullptr;
};
static_assert(sizeof(KoopaLastBeforeCheckPoint) == 0x190);
