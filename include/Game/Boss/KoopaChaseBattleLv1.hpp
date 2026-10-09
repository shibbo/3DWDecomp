#pragma once
#include "Boss/KoopaChaseBattle.hpp"
#include <math/seadQuat.h>
#include <math/seadVector.h>

namespace al {
class ActorInitInfo;
class CameraInfo;
class LiveActor;
}  // namespace al
class KoopaChase;
class KoopaChaseDemoInfo;
class KoopaChaseStateDemo;
class KoopaChaseStateFire;
class KoopaChaseStateJump;
class KoopaChaseStateProvocation;
class KoopaChaseStateWarp;

/// Battle logic of the first Bowser chase: running, throwing, fire breath, jumps and warps.
class KoopaChaseBattleLv1 : public KoopaChaseBattle {
public:
    KoopaChaseBattleLv1(KoopaChase* pHost, const al::ActorInitInfo& rInfo);

    ~KoopaChaseBattleLv1() override = default;

    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    bool tryGoDemoBattleEnd();
    void control() override;
    bool isStateTurnToTarget() const override;
    bool isStateUpdatePose() const override;
    bool isStateUpdateMoveCount() const override;
    bool isStateTireMove() const override;
    bool isStateWarp() const override;
    void exeDemoBattleStart();
    bool updateRunSpeed();
    void exeRun();
    bool tryStartJumpOrWarp();
    bool tryStartFire();
    bool tryStartThrow(bool isProvokeOnFail);
    void exeThrow();
    void exeFire();
    void exeJump();
    void exeWarp();
    void exeProvocation();
    void exeProvocationFirst();
    void exeDamage();
    void exeDemoBattleEnd();

    KoopaChaseStateThrow* getStateThrow() const override { return mStateThrow; }

    KoopaChaseStateDamage* getStateDamage() const override { return mStateDamage; }

private:
    KoopaChase* mHost;
    KoopaChaseDemoInfo* mDemoInfoStart = nullptr;
    KoopaChaseDemoInfo* mDemoInfoEnd = nullptr;
    KoopaChaseStateDemo* mStateDemoStart = nullptr;
    KoopaChaseStateDemo* mStateDemoEnd = nullptr;
    KoopaChaseStateDamage* mStateDamage = nullptr;
    KoopaChaseStateFire* mStateFire = nullptr;
    KoopaChaseStateJump* mStateJump = nullptr;
    KoopaChaseStateProvocation* mStateProvocation = nullptr;
    KoopaChaseStateThrow* mStateThrow = nullptr;
    KoopaChaseStateWarp* mStateWarp = nullptr;
    al::CameraInfo* mCameraInfo = nullptr;
    sead::Vector3f mEndDemoTrans = sead::Vector3f::zero;
    sead::Quatf mEndDemoQuat = sead::Quatf::unit;
    al::LiveActor* mEndDemoParts = nullptr;
};
static_assert(sizeof(KoopaChaseBattleLv1) == 0xa0);
