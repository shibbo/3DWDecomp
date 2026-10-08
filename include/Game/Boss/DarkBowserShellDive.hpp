#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>

#include "Library/Nerve/NerveStateBase.hpp"

namespace al {
class ActorInitInfo;
class HitSensor;
class LiveActor;
}  // namespace al

class DarkBowser;
class DarkBowserRingBeam;
class SePlayObj;

/**
 * @brief Fury Bowser's shell dive attack: he jumps up, tracks the player from above, dives down
 * and gets stuck in the ground (or hops back up for another dive).
 */
class DarkBowserShellDive : public al::NerveStateBase {
public:
    /** @brief Maximum number of placed dive targets (final battle). */
    static constexpr s32 cTargetNumMax = 24;

    DarkBowserShellDive(DarkBowser* pHost, const al::ActorInitInfo& rInfo);

    void appear() override;
    void kill() override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther);
    void exeJump();
    void exeTrack();
    sead::Vector3f calcLinearPos(const al::LiveActor* pTarget, f32 frame);
    void updateShadowEffects();
    void exeDive();
    void exeHop();
    void startRingBeam();
    sead::Vector3f calcHopAwayVelocity() const;
    void exeStuck();
    void tryAppearGuideMessage();
    f32 calcAnimRate() const;
    void exeEnd();
    void setLevel(s32 level);
    bool isEnding() const;
    bool isAerial() const;
    void forceRecover();
    void hopToPlayer() const;
    ~DarkBowserShellDive() override;

    /**
     * @brief Makes the next dive start from a knock back (after a side kick or a bomb hit).
     * @param recoverFrame Frames to stay knocked back before diving.
     */
    void requestKnockBack(s32 recoverFrame) {
        mIsKnockBack = true;
        mKnockBackFrame = recoverFrame;
    }

    /** @brief Enables the extra (low health) dive pattern. */
    void enableExtraPattern() { mPatternFlags |= 1; }

    /**
     * @brief Whether the dive wants the aerial camera.
     * @return True on the frame the landing warning appears.
     */
    bool isRequestAerialCamera() const { return mIsRequestAerialCamera; }

private:
    DarkBowser* mHost;                                         // 0x18
    s32 mLevel = 0;                                            // 0x20
    s32 mDiveCount = 0;                                        // 0x24
    s32 mDiveNum;                                              // 0x28
    s32 mStuckActionStep = -1;                                 // 0x2C
    s32 mKnockBackFrame = 0;                                   // 0x30
    bool mIsKnockBack = false;                                 // 0x34
    s32 _38 = -1;                                              // 0x38
    sead::Vector3f mStartPos = sead::Vector3f::zero;           // 0x3C
    sead::Vector3f mTargetPos = sead::Vector3f::zero;          // 0x48
    bool mIsUseFixedTarget = false;                            // 0x54
    s32 mTargetNum = 0;                                        // 0x58
    sead::Vector3f mTargets[cTargetNumMax];                    // 0x5C
    s32 mTargetOrder[cTargetNumMax];                           // 0x17C
    s32 mPatternFlags = 0;                                     // 0x1DC
    s32 mDiveTotal = 1;                                        // 0x1E0
    bool mIsWaitWater = false;                                 // 0x1E4
    DarkBowserRingBeam* mRingBeam = nullptr;                   // 0x1E8
    bool mIsShowGuide = true;                                  // 0x1F0
    bool mIsRequestAerialCamera = false;                       // 0x1F1
    SePlayObj* mSeObj;                                         // 0x1F8
};
static_assert(sizeof(DarkBowserShellDive) == 0x200);
