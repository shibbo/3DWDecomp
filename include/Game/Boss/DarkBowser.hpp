#pragma once

#include <basis/seadTypes.h>
#include <container/seadSafeArray.h>
#include <math/seadMatrix.h>
#include <math/seadQuat.h>
#include <math/seadVector.h>
#include <prim/seadSafeString.h>

#include "Boss/DarkBowserUtil.hpp"
#include "Library/LiveActor/LiveActor.hpp"

namespace al {
class AudioDirector;
class CollisionPartsFilterBase;
class EffectObjFollowCamera;
class EffectSystem;
class LayoutActor;
template <class T>
class DeriveActorGroup;
}  // namespace al

class DarkBowserBattle;
class DarkBowserRingBeam;
class DarkBowserStateDemo;
class DemoCutscene;
class GameSkyProjection;
class GigaBellItem;
class InkBomb;
class InkPuddle;
class SuperBowserFireFlame;
class SuperBowserHealthBar;

/** @brief Fury Bowser, the giant boss fought in the phase battles of Bowser's Fury. */
class DarkBowser : public al::LiveActor {
public:
    /** @brief A place the player can be put at when the battle starts. */
    struct SpawnInfo {
        sead::Vector3f mTrans = sead::Vector3f::zero;          // 0x00
        sead::Vector3f mFront = sead::Vector3f::ez;            // 0x0C
        sead::Vector3f mGigaBellTrans = sead::Vector3f::zero;  // 0x18
        sead::Quatf mGigaBellQuat = {1.0f, 0.0f, 0.0f, 0.0f};  // 0x24
        f32 mHorizontalCameraAngle = 23.0f;                    // 0x34
        f32 mVerticalCameraAngle = 0.0f;                       // 0x38
    };
    static_assert(sizeof(SpawnInfo) == 0x3c);

    static constexpr s32 cSpawnInfoNum = 8;
    static constexpr s32 cRingBeamNum = 5;
    static constexpr s32 cInkBombNum = 16;
    static constexpr s32 cInkPuddleNum = 64;

    explicit DarkBowser(const char* pName);

    void init(const al::ActorInitInfo& rInfo) override;
    s32 calcPhase(const al::ActorInitInfo& rInfo);
    void initBattleHealth(const al::ActorInitInfo& rInfo, s32 phase);
    void jointRelease();
    void resetHitSensors();
    void createCutscenes(const al::ActorInitInfo& rInfo);
    void calcPlayerSpawn(const al::ActorInitInfo& rInfo);
    void createChildActors(const al::ActorInitInfo& rInfo);
    void initGraphicsAndEffects(const al::ActorInitInfo& rInfo);
    void initDemo(s32 phase);
    void initAfterPlacement() override;
    void appear() override;
    void kill() override;
    void control() override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSelf,
                    al::HitSensor* pOther) override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    void exeDemoAppear();
    void setPlayerSpawnPos();
    void setGigaBellPos();
    void exeBattle();
    void exeDemoEnd();
    void saveHitPoints() const;
    void forceKillObjects();
    DemoCutscene* initCutscene(const al::ActorInitInfo& rInfo, const char* pLinkName,
                               const char* pName, bool isUseBaseMtx) const;
    bool requestHitReaction(const char* pName);
    void requestSmallBlur(const sead::Vector3f* pPos);
    void requestMediumBlur(const sead::Vector3f* pPos);
    void requestLargeBlur(const sead::Vector3f* pPos);
    void jointAim(const sead::Vector3f& rTarget);
    void jointSetPower(f32 rate);
    void requestAddVelocity(const sead::Vector3f& rVelocity);
    void updateWaterSurface();
    void requestDamage(s32 damage);
    void requestFinalHit();
    f32 getCurHitCountPercent() const;
    void showHealthBar();
    void hideHealthBar();
    void setHealthBarState(bool isOffset);
    InkPuddle* getDeadInkPuddle();
    SuperBowserFireFlame* getDeadFireball();
    InkBomb* getDeadInkBomb();
    DarkBowserRingBeam* getDeadRingBeam() const;
    void updateCollider() override;
    void cancelFinalBowserTransform();
    void setSkyToNight();
    void startDemoFade();
    void endDemoFade();
    void preFinalBowserTransform();
    void startFinalBowserTransform();
    ~DarkBowser() override = default;

    /**
     * @brief Whether the current phase ends with Fury Bowser retreating instead of being defeated.
     * @return True if the battle only drains the first half of the health bar.
     */
    bool isRetreatPhase() const { return mPhase != 1 && mHealthStage == 0 && mPhase != 4; }

    /**
     * @brief Whether the current section of the health bar has been fully drained.
     * @return True if no hit points are left in the current section.
     */
    bool isHealthSectionEmpty() const {
        if (mPhase == 4 || mIsV2) {
            if (mHitPoint <= 0) {
                return true;
            }
        } else if (mHitPoint - (sead::Mathi::clamp(mPhase, 0, 2) - mHealthStage - 1) * 100 <= 0) {
            return true;
        }

        return false;
    }

    /** @brief Whether this is the final battle (where damage is not capped per section). */
    bool isFinalBattle() const { return mIsFinalBattle; }

    /** @brief The player actor Fury Bowser is fighting. */
    al::LiveActor* getPlayer() const { return mPlayer; }

    /** @brief Turns gravity back on (e.g. after being knocked away). */
    void validateGravity() { mIsApplyGravity = true; }

    /** @brief Turns gravity off (e.g. while hovering above the player in the shell dive). */
    void invalidateGravity() { mIsApplyGravity = false; }

    /** @brief The battle phase (1 to 4). */
    s32 getPhase() const { return mPhase; }

    /** @brief Index of the current section of the health bar. */
    s32 getHealthStage() const { return mHealthStage; }

    /** @brief Remaining hit points. */
    s32 getHitPoint() const { return mHitPoint; }

    /** @brief Where the shell dive landing warning is shown (tracked by the battle camera). */
    const sead::Vector3f* getShellLandWarningPosPtr() const { return &mShellLandWarningPos; }

private:
    DarkBowserStateDemo* mStateDemo = nullptr;                         // 0x148
    DarkBowserBattle* mBattle = nullptr;                               // 0x150
    al::LiveActor* mPlayer = nullptr;                                  // 0x158
    sead::SafeArray<SpawnInfo, cSpawnInfoNum> mSpawnInfos;             // 0x160
    s32 mSpawnInfoNum = 0;                                             // 0x340
    s32 mSpawnInfoIndex = 0;                                           // 0x344
    al::DeriveActorGroup<InkPuddle>* mInkPuddleGroup = nullptr;        // 0x348
    al::DeriveActorGroup<SuperBowserFireFlame>* mFireballGroup = nullptr;  // 0x350
    al::DeriveActorGroup<InkBomb>* mInkBombGroup = nullptr;            // 0x358
    al::DeriveActorGroup<GigaBellItem>* mGigaBellGroup = nullptr;      // 0x360
    DarkBowserRingBeam* mRingBeams[cRingBeamNum];                      // 0x368
    s32 mInkPuddleIndex = 0;                                           // 0x390
    SuperBowserHealthBar* mHealthBar = nullptr;                        // 0x398
    s32 mHitPoint = 100;                                               // 0x3A0
    s32 _3A4 = 100;                                                    // 0x3A4
    sead::Vector3f mShellLandWarningPos = sead::Vector3f::zero;        // 0x3A8
    sead::Vector3f mBlurPos = sead::Vector3f::zero;                    // 0x3B4
    DemoCutscene* mDemoCutscene = nullptr;                             // 0x3C0
    DemoCutscene* mFinalCutscene = nullptr;                            // 0x3C8
    GameSkyProjection* mSkyLake = nullptr;                             // 0x3D0
    GameSkyProjection* mSkyDisaster = nullptr;                         // 0x3D8
    void* _3E0;                                                        // 0x3E0
    al::LayoutActor* mDisasterModeLayout = nullptr;                    // 0x3E8
    sead::Matrix34f mBaseMtx = sead::Matrix34f::ident;                 // 0x3F0
    s32 mFadeFrame = 1000;                                             // 0x420
    s32 mStopBgmTimer = -1;                                            // 0x424
    al::EffectSystem* mEffectSystem = nullptr;                         // 0x428
    sead::FixedSafeString<32> mHitReactionName;                        // 0x430
    al::EffectObjFollowCamera* mRainEffect = nullptr;                  // 0x470
    al::AudioDirector* mAudioDirector = nullptr;                       // 0x478
    sead::FixedSafeString<64> mPhaseName;                              // 0x480
    DarkBowserUtil::ControlledJointChain mJointChainA;                 // 0x4D0
    DarkBowserUtil::ControlledJointChain mJointChainB;                 // 0x528
    s32 mPhase = 1;                                                    // 0x580
    s32 mHealthStage = 0;                                              // 0x584
    sead::Vector3f mWaterSurfacePos = sead::Vector3f::zero;            // 0x588
    bool mIsApplyGravity = true;                                       // 0x594
    bool mIsInWater;                                                   // 0x595
    bool mIsFinalBattle = false;                                       // 0x596
    bool mIsFinalHit = false;                                          // 0x597
    bool mIsV2 = false;                                                // 0x598
    bool mIsDemoCancelled = false;                                     // 0x599
    bool mIsSuperHardSky = false;                                      // 0x59A
    al::CollisionPartsFilterBase* mColliderFilter = nullptr;           // 0x5A0
};
static_assert(sizeof(DarkBowser) == 0x5a8);
