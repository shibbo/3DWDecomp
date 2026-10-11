#pragma once

#include <math/seadVector.h>
#include <random/seadRandom.h>

#include "Library/LiveActor/LiveActor.hpp"

namespace al {
class RumbleCalculatorCosMultLinear;
template <class T>
class DeriveActorGroup;
}  // namespace al

class ActorJointLookController;
class Bunbun;
class GateKeeperStateDemo;
class PunpunDivision;
class PunpunShuriken;

/**
 * @brief Punpun (Motley Bossblob): a gate keeper boss that splits into clones, throws its
 * shuriken and has to be stomped three times.
 */
class Punpun : public al::LiveActor {
public:
    using DivisionGroup = al::DeriveActorGroup<PunpunDivision>;

    explicit Punpun(const char* pName);

    void init(const al::ActorInitInfo& rInfo) override;
    static ActorJointLookController* makePunpunLookController(const al::LiveActor* pActor);
    void control() override;
    void kill() override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    bool receiveDamage(al::HitSensor* pOther);
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;
    void setBunbun(Bunbun* pBunbun);
    void exeDemoAppear();
    void updateShurikenPose();
    void exePreDemoAppearDelay();
    void exeDivideMove();
    void exeDivideWait();
    void exeDivide();
    void exeDivideAppear();
    void exeThrowWait();
    void exeThrowSign();
    void exeThrow();
    void exeThrowEnd();
    void exeHide();
    void exePressDown();
    void exeDiePressDown();
    void exeDie();
    f32 getThrowWaitTurnSpeed();

private:
    s32 getDivisionNum() const;
    void killAliveDivisions();
    bool isDividing() const;
    bool isDamaged() const;

    GateKeeperStateDemo* mStateDemo = nullptr;
    s32 mLevel = 0;
    s32 mDamageCount = 0;
    s32 mFireBallHitCount = 0;
    DivisionGroup* mDivisionGroup = nullptr;
    PunpunShuriken* mShuriken;
    ActorJointLookController* mLookController = nullptr;
    sead::Vector3f mInitFront = sead::Vector3f::ez;
    sead::Vector3f mDivideStartTrans = {0.0f, 0.0f, 0.0f};
    sead::Vector3f mDivideTargetTrans = {0.0f, 0.0f, 0.0f};
    f32 mDivideDistance = 700.0f;
    al::RumbleCalculatorCosMultLinear* mRumble;
    bool mIsUseOwnRandom = false;
    sead::Random mRandom;
    s32 mDividePatternIndex = 0;
    f32 mDieTurnDegree = 0.0f;
    f32 mDieTurnSpeed = 0.0f;
    bool mIsSingleMode = false;
    s32 mKoopaJrHitCooldown = 0;
    Bunbun* mBunbun = nullptr;
};

static_assert(sizeof(Punpun) == 0x1d8);
