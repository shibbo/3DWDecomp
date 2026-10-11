#pragma once

#include <basis/seadTypes.h>
#include <container/seadPtrArray.h>
#include <math/seadMatrix.h>
#include <math/seadVector.h>

namespace al {
class ActorInitInfo;
class IUseSceneObjHolder;
class JointAimInfo;
class JointSpringController;
class LiveActor;
class SkyProjection;
}  // namespace al

class DarkBowser;

/** @brief Shared helpers for Fury Bowser (DarkBowser) and its battle states. */
namespace DarkBowserUtil {

/** @brief A chain of joint aim controllers (the body or the beam joints) aimed as a whole. */
struct ControlledJointChain {
    /** @brief One aimed joint of the chain. */
    struct Entry {
        al::JointAimInfo* mAimInfo;    // 0x0
        const sead::Matrix34f* mMtx;   // 0x8
    };

    static constexpr s32 cEntryNum = 5;

    Entry mEntries[cEntryNum];      // 0x00
    s32 mCapacity = cEntryNum;      // 0x50
    s32 mCount = 0;                 // 0x54
};
static_assert(sizeof(ControlledJointChain) == 0x58);

/** @brief State of Fury Bowser's mouth laser. */
struct LaserParam {
    sead::Matrix34f mMtx;         // 0x00
    sead::Vector3f mSide;         // 0x30
    sead::Vector3f mUp;           // 0x3C
    sead::Vector3f mFront;        // 0x48
    sead::Vector3f mHitPos;       // 0x54
    sead::Vector3f mOrigin;       // 0x60
    bool mIsHitLand;              // 0x6C
    s32 mHitFrame;                // 0x70
    al::LiveActor* mBeamActor;    // 0x78
    al::LiveActor* mNearActor;    // 0x80
};

sead::Vector3f calculateArc(sead::Vector3f start, sead::Vector3f end, f32 height, f32 rate,
                            s32 moveEaseType, s32 heightEaseType);
void startDisasterMode(const al::IUseSceneObjHolder* pHolder);
void endDisasterMode(const al::IUseSceneObjHolder* pHolder);
bool isDisasterMode(const al::IUseSceneObjHolder* pHolder);
void setSkyboxBlendPercentage(al::SkyProjection* pSky, f32 percentage);
void graphicsAreaPrioritySet(al::LiveActor* pActor, const char* pPlacementId, s32 priority);
void setSkyToDay(al::LiveActor* pActor, al::SkyProjection* pSkyDay, al::SkyProjection* pSkyNight,
                 bool isInstant);
void setSkyToNight(al::LiveActor* pActor, al::SkyProjection* pSkyDay,
                   al::SkyProjection* pSkyNight, bool isInstant);
void hideDarkBowser(DarkBowser* pDarkBowser);
void showDarkBowser(DarkBowser* pDarkBowser);
void initDarkBowserJointControllers(al::LiveActor* pActor, const al::ActorInitInfo& rInfo,
                                    const char* pSuffix, ControlledJointChain& rChainA,
                                    ControlledJointChain& rChainB,
                                    sead::PtrArray<al::JointSpringController>* pSprings);
s32 jointChainAppend(const al::LiveActor* pActor, ControlledJointChain& rChain,
                     const char* pJointName, f32 interpoleRate, f32 pitchDegree,
                     f32 limitDegree);
void initDarkBowserHairJointController(al::LiveActor* pActor,
                                       sead::PtrArray<al::JointSpringController>* pSprings,
                                       bool isFinal);
void jointChainInit(const al::LiveActor* pActor, ControlledJointChain& rChain);
void jointChainPitch(ControlledJointChain& rChain, s32 index, f32 pitchDegree);
void jointChainConstraint(ControlledJointChain& rChain, s32 index, f32 limitDegree);
void jointChainAim(ControlledJointChain& rChain, const sead::Vector3f& rTarget,
                   f32 interpoleRate);
void jointChaimAimNoInterpolate(ControlledJointChain& rChain, const sead::Vector3f& rTarget);
void jointChainSetPowerRate(ControlledJointChain& rChain, f32 rate);
void jointChainRelease(ControlledJointChain& rChain);
bool isFacingWithinThreshold(const sead::Vector3f& rDirA, const sead::Vector3f& rDirB,
                             f32 threshold);
void startLaserEffects(al::LiveActor* pActor, LaserParam* pParam);
void stopLaserEffects(al::LiveActor* pActor);
void calcLaserHitPos(al::LiveActor* pActor, const sead::Vector3f* pAxes,
                     const sead::Vector3f& rOrigin, sead::Vector3f* pHitPos);
void playLaserEffects(al::LiveActor* pActor, bool* pIsHitLand, s32* pHitFrame,
                      LaserParam* pParam);
void scaleLaserEffects(al::LiveActor* pActor, const sead::Vector3f& rHitPos,
                       const sead::Vector3f& rOrigin, f32 scale);
void updateLaserEffects(al::LiveActor* pActor, LaserParam* pParam);

}  // namespace DarkBowserUtil
