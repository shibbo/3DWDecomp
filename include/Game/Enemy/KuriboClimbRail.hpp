#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"

namespace al {
class Nerve;
}  // namespace al

class ActorMicRumbler;
class ActorStateSupportFreeze;
class EnemyStateBlowDown;
class KuriboClimbStateBodyAttack;

/**
 * @brief Goomba that climbs along a rail on walls and slopes, turns around at the rail ends and
 * jumps off to body attack players.
 */
class KuriboClimbRail : public al::LiveActor {
public:
    explicit KuriboClimbRail(const char* pName);

    void init(const al::ActorInitInfo& rInfo) override;
    void initAfterPlacement() override;
    void control() override;
    void kill() override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;

    void exeTurn();
    void exeWalk();
    void exeBodyAttackReady();
    void exeBodyAttack();
    void exeRecoverTurn();
    void exeRecover();
    void exePressDown();
    void exeBlowDown();
    void exeSupportFreeze();
    void exeFall();

    bool isEnableBodyAttack() const;
    void startBodyAttack();

private:
    void appearItemToAttacker();

    KuriboClimbStateBodyAttack* mStateBodyAttack = nullptr;
    EnemyStateBlowDown* mStateBlowDown = nullptr;
    ActorStateSupportFreeze* mStateSupportFreeze = nullptr;
    s32 _160 = 0;
    s32 _164 = 0;
    f32 mWalkSpeed = 0.0f;
    s32 _16c = 0;
    sead::Vector3f mGroundPos = {0.0f, 0.0f, 0.0f};
    al::HitSensor* mHipDropSensor = nullptr;
    const char* mItemType = "Coin";
    sead::Vector3f mClippingPos = {0.0f, 0.0f, 0.0f};
    bool mIsPlacementSlope = false;
    bool mIsControlShadowLength = false;
    ActorMicRumbler* mMicRumbler = nullptr;
    const al::Nerve* mNerveBeforeFreeze = nullptr;
    void* mBodyAttackController = nullptr;
};

static_assert(sizeof(KuriboClimbRail) == 0x1B8);
