#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"

class IUsePlayerPuppet;

/// Roller skates the player can ride; they carry the rider until they hit a wall or water.
class SkateShoes : public al::LiveActor {
public:
    explicit SkateShoes(const char* pName);

    void init(const al::ActorInitInfo& rInfo) override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSelf,
                    al::HitSensor* pOther) override;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;
    void calcAnim() override;
    void control() override;
    bool hideActor() override;

    bool isEnableAttack() const;
    bool onGround(u32 checkFrame) const;
    void setPuppetQT();
    bool isEnableWind(const al::SensorMsg* pMsg) const;
    f32 calcDashAccel() const;
    f32 calcDashTurnSpeed() const;
    void appearFromObject(const sead::Vector3f& rTrans, const sead::Vector3f& rVelocity);
    void reset();
    void updateVelocity(bool isOnGround);
    void updateTurn(f32 turnSpeed);
    bool tryWallHitEnd();

    void exeAppear();
    void exeWait();
    void exeGetOn();
    void exeAccel();
    void exeDash();
    void exeRide();
    void exeFall();
    void exeJump();
    void exeTakeOff();
    void exeInstantTakeOff();
    void exeRespawn();

private:
    inline void takeOffPuppet();

    IUsePlayerPuppet* mPuppet = nullptr;                // 0x148
    sead::Vector3f mRespawnTrans = sead::Vector3f::zero;  // 0x150
    f32 mRotateZ = 0.0f;                                // 0x15c
    f32 mSpeed = 0.0f;                                  // 0x160
    s32 mDashPanelTime = 0;                             // 0x164
    bool mIsNoTakeOff = false;                          // 0x168
    bool mIsInKeepSkateShoeArea = false;                // 0x169
    bool mIsEnableReset = false;                        // 0x16a
    bool mIsIgnoreInitialGravity = false;               // 0x16b
    bool mIsCancelForGoal = false;                      // 0x16c
    bool mIsSingleMode = false;                         // 0x16d
    bool mIsRespawned = false;                          // 0x16e
};

static_assert(sizeof(SkateShoes) == 0x170);
