#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"

namespace al {
class KeyPoseKeeper;
class MtxConnector;
}  // namespace al

class ActorStateSupportFreeze;
class EnemyStateBlowDown;

/**
 * @brief Floor-stomping enemy: waits, then slams onto the floor and damages players next to it.
 * With several key poses it slides to the next key pose while attacking.
 */
class Peto : public al::LiveActor {
public:
    explicit Peto(const char* pName);

    void init(const al::ActorInitInfo& rInfo) override;
    void killBySwitch();
    void initAfterPlacement() override;
    void control() override;
    void moveEyesToMoveDir();
    void kill() override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;

    void exeStandby();
    void exeWait();
    void exeAttackOmen();
    void exeAttackStart();
    void changeToAttackSensor();
    void exeAttack();
    void changeToWaitSensor();
    void exeKeyMoveAttack();
    void exeAttackEnd();
    void moveEyesToInitPos();
    void exeSupportFreeze();
    void exePressDown();
    void exeBlowDown();

private:
    bool isInAttackRange(al::HitSensor* pSelf, al::HitSensor* pOther);

    s32 mWaitTime = 60;                                   // 0x144
    s32 mAttackTime = 60;                                 // 0x148
    s32 mKeyMoveTime = 0;                                 // 0x14C
    s32 mDelay = 0;                                       // 0x150
    f32 mMoveSpeed = 8.0f;                                // 0x154
    sead::Vector3f mEyeRotate = {0.0f, 0.0f, 0.0f};       // 0x158
    sead::Vector3f mClippingTrans = {0.0f, 0.0f, 0.0f};   // 0x164
    sead::Vector3f mShadowMaskSizeInit = {160.0f, 0.0f, 160.0f};  // 0x170
    sead::Vector3f mShadowMaskSize = {0.0f, 0.0f, 0.0f};  // 0x17C
    sead::Vector3f mSideDir = sead::Vector3f::ex;         // 0x188
    sead::Vector3f mFrontDir = sead::Vector3f::ez;        // 0x194
    al::KeyPoseKeeper* mKeyPoseKeeper = nullptr;          // 0x1A0
    al::MtxConnector* mMtxConnector = nullptr;            // 0x1A8
    EnemyStateBlowDown* mStateBlowDown = nullptr;         // 0x1B0
    bool _1b8 = false;                                    // 0x1B8
    bool mIsUseShadowMask = false;                        // 0x1B9
    ActorStateSupportFreeze* mStateSupportFreeze = nullptr;  // 0x1C0
};

static_assert(sizeof(Peto) == 0x1C8);
