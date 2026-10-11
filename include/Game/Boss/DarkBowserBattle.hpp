#pragma once

#include <basis/seadTypes.h>
#include <math/seadMatrix.h>
#include <math/seadVector.h>

#include "Library/Nerve/NerveStateBase.hpp"

namespace al {
class ActorInitInfo;
class CameraTicket;
class HitSensor;
class SensorMsg;
}  // namespace al

class CameraPoserDarkBowser;
class DarkBowser;
class DarkBowserDamage;
class DarkBowserFirebomb;
class DarkBowserLaser;
class DarkBowserShellDive;
class DarkBowserShellSpike;
class DarkBowserWheel;
class GuideFrameOutSingleMode;

/** @brief Fury Bowser's battle state, which drives all of his attacks. */
class DarkBowserBattle : public al::NerveStateBase {
public:
    DarkBowserBattle(DarkBowser* pDarkBowser, const al::ActorInitInfo& rInfo);

    s32 getAttackLevel(s32 offset) const;
    void registerGuideFrameOut();
    void appear() override;
    void kill() override;
    virtual void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther);
    virtual bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSelf,
                            al::HitSensor* pOther);
    void control() override;
    void updateTrackedPosition();
    void calcAndSetEyePosition();
    void exeBegin();
    void decideNextState();
    void exeWait();
    void decideFinal();
    void exeFirebomb();
    void exeShellDive();
    void endShellDive();
    void exeWheel();
    void exeSpike();
    void exeLaser();
    void exeDamage();
    void endCamera();
    void decidePhase1();
    void decidePhase2();
    void decidePhase3();
    ~DarkBowserBattle() override;

private:
    DarkBowser* mHost;                                  // 0x18
    DarkBowserFirebomb* mFirebomb = nullptr;            // 0x20
    DarkBowserShellDive* mShellDive = nullptr;          // 0x28
    DarkBowserDamage* mDamage = nullptr;                // 0x30
    DarkBowserWheel* mWheel = nullptr;                  // 0x38
    DarkBowserShellSpike* mSpike = nullptr;             // 0x40
    DarkBowserLaser* mLaser = nullptr;                  // 0x48
    al::CameraTicket* mCameraTicket = nullptr;          // 0x50
    CameraPoserDarkBowser* mCameraPoser = nullptr;      // 0x58
    GuideFrameOutSingleMode* mGuideFrameOut = nullptr;  // 0x60
    const sead::Matrix34f* mFaceMtx = nullptr;          // 0x68
    sead::Vector3f mTrackedPos = sead::Vector3f::zero;  // 0x70
    sead::Vector3f mEyePos = sead::Vector3f::zero;      // 0x7C
    s32 mHealthStage = 0;                               // 0x88
    s32 mShellAttackCount = 0;                          // 0x8C
    s32 mAttackCount = 0;                               // 0x90
    s32 mRandomAttack = 0;                              // 0x94
    bool mIsKnockedBack = false;                        // 0x98
};
static_assert(sizeof(DarkBowserBattle) == 0xa0);
