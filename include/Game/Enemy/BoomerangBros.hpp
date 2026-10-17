#pragma once

#include <math/seadVector.h>

#include "Enemy/BrosWeaponArray.hpp"
#include "Library/LiveActor/LiveActor.hpp"

namespace al {
class KeyPoseKeeper;
}

class ActorStateSupportFreeze;
class BoomerangBrosBoomerang;
class BrosMoveStepKeeper;
class BrosStateAttack;
class BrosStateJump;
class EnemyStateBlowDown;
struct EnemyStateBlowDownParam;

/**
 * @brief Boomerang Bro: jumps between key poses and throws boomerangs that come back to it.
 */
class BoomerangBros : public al::LiveActor {
public:
    typedef BrosWeaponArray<BoomerangBrosBoomerang> WeaponArray;

    explicit BoomerangBros(const char* pName);

    void init(const al::ActorInitInfo& rInfo) override;
    void killBySwitch();
    void initAfterPlacement() override;
    void control() override;
    void reappear() override;
    void killComplete(bool isNoAppearItem) override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;

    void exeWait();
    void exeJump();
    void exeAttack();
    void exeCatchWait();
    void exeCatch();
    void exePressDownPress();
    void exePressDownBlow();
    void exeBlowDown();
    void exeSupportFreeze();

private:
    bool isDown() const;
    bool isOnKeyPose() const;
    void startNextMove();
    void holdNextWeapon();
    void killHoldWeapon();
    void killWithItem();

    WeaponArray* mWeapons = nullptr;
    BoomerangBrosBoomerang* mHoldWeapon = nullptr;
    BoomerangBrosBoomerang* mCatchWeapon = nullptr;
    al::KeyPoseKeeper* mKeyPoseKeeper = nullptr;
    BrosMoveStepKeeper* mMoveStepKeeper = nullptr;
    BrosStateAttack* mStateAttack = nullptr;
    BrosStateJump* mStateJump = nullptr;
    EnemyStateBlowDown* mStateBlowDown = nullptr;
    EnemyStateBlowDown* mStatePressDownBlow = nullptr;
    EnemyStateBlowDownParam* mPressDownBlowParam = nullptr;
    ActorStateSupportFreeze* mStateSupportFreeze = nullptr;
    sead::Vector3f mClippingTrans = {0.0f, 0.0f, 0.0f};
    const char* mItemType = nullptr;
    sead::Vector3f mInitTrans = sead::Vector3f::zero;
    sead::Vector3f mInitFront = sead::Vector3f::ez;
};

static_assert(sizeof(BoomerangBros) == 0x1d0);
