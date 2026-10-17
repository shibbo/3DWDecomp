#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"

class IUsePlayerPuppet;
class SuperSkateRail;

/// Roller skates from Bowser's Fury; besides rolling like the regular skates they grind on rails.
class SuperSkateShoes : public al::LiveActor {
public:
    explicit SuperSkateShoes(const char* pName);

    void init(const al::ActorInitInfo& rInfo) override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSelf,
                    al::HitSensor* pOther) override;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;
    void calcAnim() override;
    bool hideActor() override;
    void control() override;

    bool isEnableAttack() const;
    void setPuppetQT();
    bool isEnableWind(const al::SensorMsg* pMsg) const;
    f32 calcDashAccel() const;
    f32 calcDashTurnSpeed() const;
    void appearFromObject(const sead::Vector3f& rTrans, const sead::Vector3f& rVelocity);
    void reset();
    void updateVelocity(bool isOnGround);
    void updateTurn(f32 turnSpeed);
    bool tryWallHitEnd();
    void stopGrind();
    void startGrind(SuperSkateRail* pRail);

    void exeAppear();
    void exeWait();
    void exeGetOn();
    void exeAccel();
    void exeDash();
    void exeRide();
    void exeFall();
    void exeJump();
    void exeTakeOff();
    void exeRespawn();
    void exeGrind();

private:
    inline void takeOffPuppet();
    inline bool isRolling() const;
    inline bool isGrinding() const;

    IUsePlayerPuppet* mPuppet = nullptr;                  // 0x148
    f32 mRotateZ = 0.0f;                                  // 0x150
    f32 mSpeed = 0.0f;                                    // 0x154
    s32 mDashPanelTime = 0;                               // 0x158
    bool mIsNoTakeOff = false;                            // 0x15c
    f32 mSpeedScale = 1.0f;                               // 0x160
    sead::Vector3f mRespawnTrans = sead::Vector3f::zero;  // 0x164
    SuperSkateRail* mRail = nullptr;                      // 0x170
    f32 mGrindSpeed = 0.0f;                               // 0x178
    bool mIsGrindReverse = false;                         // 0x17c
};

static_assert(sizeof(SuperSkateShoes) == 0x180);
