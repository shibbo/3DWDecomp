#pragma once

#include <basis/seadTypes.h>
#include <gfx/seadColor.h>
#include <math/seadMatrix.h>
#include <math/seadVector.h>

#include "Boss/DarkBowserUtil.hpp"
#include "Library/Nerve/NerveStateBase.hpp"

namespace al {
class ActorInitInfo;
class DirLightParam;
class HitSensor;
class LiveActor;
}  // namespace al

class DarkBowser;
class DarkBowserJump;

/**
 * @brief Fury Bowser's mouth laser attack state: jumps to a spot, turns to the player,
 * charges, then sweeps the laser across the player one or more times.
 */
class DarkBowserLaser : public al::NerveStateBase {
public:
    DarkBowserLaser(DarkBowser* pHost, const al::ActorInitInfo& rInfo);

    void appear() override;
    void kill() override;
    void endLaserLight();
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther);
    bool checkLaserToSphere(const sead::Vector3f& rPos, f32 radius) const;
    void exeJump();
    void exeBegin();
    void beginLaserLight();
    void exeCharge();
    void calcTargets();
    void exeShoot();
    void exeEnd();
    bool isInLaserFrustum(const sead::Vector3f& rPos, f32 radius) const;
    void updateLaserLight();
    void updateLaserLightFadeIn();
    void updateLaserLightFadeOut();

    /**
     * @brief Sets the attack level (how aggressive the attack is).
     * @param level Attack level.
     */
    void setLevel(s32 level) { mLevel = level; }

private:
    DarkBowser* mHost;                                // 0x18
    al::LiveActor* mTarget;                           // 0x20
    DarkBowserJump* mJump;                            // 0x28
    s32 mLevel;                                       // 0x30
    s32 mShotCount;                                   // 0x34
    s32 mSweepSign;                                   // 0x38
    sead::Vector3f mTargetOffset;                     // 0x3C
    sead::Vector3f mPrevAimPos;                       // 0x48
    sead::Vector3f mAimPos;                           // 0x54
    sead::Vector3f mSensorPos;                        // 0x60
    bool _6c;                                         // 0x6C
    const sead::Matrix34f* mBeamMtx;                  // 0x70
    DarkBowserUtil::LaserParam mLaserParam;           // 0x78
    al::DirLightParam* mLight;                        // 0x100
    bool mIsLaserLight;                               // 0x108
    sead::Vector3f mSavedLightDir;                    // 0x10C
    sead::Color4f mSavedLightColor;                   // 0x118
    sead::Color4f mLightColor;                        // 0x128
    s32 mLightFadeFrame;                              // 0x138
};
static_assert(sizeof(DarkBowserLaser) == 0x140);
