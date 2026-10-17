#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include <container/seadPtrArray.h>
#include <math/seadQuat.h>
namespace al {
class JointSpringController;
template <class T> class DeriveActorGroup;
} // namespace al
class KoopaFireBall;
class KoopaLastDemoEndActorHolder;
class KoopaLastDefendeBlockPow : public al::LiveActor {
  public:
    explicit KoopaLastDefendeBlockPow(const char *pName);
    ~KoopaLastDefendeBlockPow() override;
    void init(const al::ActorInitInfo &) override;
    void appear() override;
    void control() override;
    void attackSensor(al::HitSensor *, al::HitSensor *) override;
    bool receiveMsg(const al::SensorMsg *, al::HitSensor *, al::HitSensor *) override;
    bool receiveMsgScreenPoint(const al::SensorMsg *, al::ScreenPointer *,
                               al::ScreenPointTarget *) override;
    void startLastPowDamage();
    void startEndDemo();
    void exeWait();
    void exePowDamage();
    void exeLastPowDamage();
    void exeDemo();
    void exeShout();
    void exeAttackCurtainStart();
    void exeAttackCurtain();
    void exeAttackCurtainEnd();

  private:
    sead::Quatf mAimQuat = sead::Quatf::unit;
    sead::Quatf mHomeQuat = sead::Quatf::unit;
    sead::Quatf mLeftQuat = sead::Quatf::unit;
    sead::Quatf mRightQuat = sead::Quatf::unit;
    const sead::Matrix34f *mWristMtx;
    al::DeriveActorGroup<KoopaFireBall> *mFireballs = nullptr;
    KoopaLastDemoEndActorHolder *mDemoHolder = nullptr;
    sead::PtrArray<al::JointSpringController> mSprings;
};
static_assert(sizeof(KoopaLastDefendeBlockPow) == 0x1b0);
