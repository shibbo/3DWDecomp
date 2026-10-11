#pragma once

#include <basis/seadTypes.h>
#include <container/seadPtrArray.h>
#include <math/seadVector.h>

namespace al {
class ActorInitInfo;
class AreaObj;
class AreaObjGroup;
}  // namespace al

class DashPanel;
class DisasterModeController;
class DisasterSpike;
class DisasterSpikeArea;
class DisasterSpikeLaunch;
class DisasterSpikeTorpedo;
class SuperBowser;

/// Spawns and drives the disaster spikes of Bowser's Fury (ocean, launched, torpedo and gold).
class DisasterSpikeDirector {
public:
    /// Tuning values loaded by loadParam() from the "Disaster Spike" section of InitDisasterSpike.
    struct Param {
        s32 mSpikeShadowFadeTime = 150;
        s32 mSpikeShadowFadeWaitTime = 60;
        f32 mSpikeShadowMaxAlpha = 0.6f;
        s32 mSpikeMoveDelayTime = 100;
        s32 mSpikeMoveDelayBufferTime = 150;
        f32 mSpikeMoveTimeGlobalScale = 1.0f;
        s32 mSpikeCooldownTime = 110;
        s32 mSpikeShakeDelayTime = 60;
        s32 mSpikeShakeTime = 120;
        f32 mSpikeShakeStrength = 20.0f;
        f32 mSpikeShakeWaitStrength = 5.0f;
        f32 mSingleSpikeStartDistance = 5000.0f;
        f32 mSpikeNoFallRadiusMin = 500.0f;
        f32 mSpikeNoFallRadiusMax = 2500.0f;
        f32 mSpikeNoFallRadiusInterpRate = 0.05f;
        f32 mSpikeNoFallRadiusMaxPlayerSpeed = 50.0f;
        f32 mSpikeNoFallRadiusPhase0 = 0.0f;
        f32 mSpikeNoFallRadiusPlessieScaleMin = 1.5f;
        f32 mSpikeNoFallRadiusPlessieScaleMax = 1.75f;
        f32 mSpikeNoFallRadiusPlessieNoRideScale = 1.0f;
        f32 mOceanSpikeInnerRadius = 1500.0f;
        f32 mOceanSpikeOuterRadius = 4000.0f;
        f32 mOceanSpikeInnerAngle = 30.0f;
        f32 mOceanSpikeOuterAngle = 90.0f;
        f32 mOceanSpikeVelocityOffset = 300.0f;
        f32 mOceanSpikeSpread = 1000.0f;
        f32 mOceanSpikeScale = 1.5f;
        f32 mOceanSpikeLandHeight = -300.0f;
        f32 mOceanSpikeLandHeight2 = -600.0f;
        f32 mOceanSpikeLandHeight3 = -900.0f;
        s32 mOceanSpikeStartDelay = 100;
        s32 mOceanSpikeWaveDelayMin = 100;
        s32 mOceanSpikeWaveDelayMax = 400;
        s32 mOceanSpikeInterval = 15;
        s32 mOceanSpikeMoveDelay = 0;
        s32 mOceanSpikeSinkDelayMin = 50;
        s32 mOceanSpikeSinkDelayMax = 100;
        s32 mOceanSpikeSinkTime = 60;
        s32 mOceanSpikeSinkAmount = 350;
        s32 mOceanSpikeWaveCount = 3;
        s32 mOceanSpikeMaxCount = 20;
        s32 mBouncyJumpSpeed = 50;      // 0xa4
        s32 mBouncyJumpHighSpeed = 60;  // 0xa8
        s32 mTorpedoSpikeSinkDamageFrames = 45;
        f32 mTorpedoSpikeScale = 1.5f;
        f32 mTorpedoSpikeRestrictionOffset = -800.0f;
        s32 mTorpedoSpikeNum = 96;
        s32 mDashPanelNum = 8;
        sead::Vector3f mTorpedoSpikeLockPos = sead::Vector3f::zero;
        sead::Vector3f mTorpedoSpikeLockDir = sead::Vector3f::ez;
        bool mIsFlipTorpedoSpikePattern = true;
        s32 mTorpedoSpikeRecklessStartDelay = 0;
        s32 mTorpedoSpikeRecklessBetweenDelay3 = 15;
        s32 mTorpedoSpikeRecklessBetweenDelay4 = 10;
        sead::Vector3f mTorpedoSpikeRecklessGlobalOffsetSmallRamp = {0.0f, 600.0f, 5000.0f};
        sead::Vector3f mTorpedoSpikeRecklessGlobalOffsetBigRamp = {0.0f, 600.0f, 10000.0f};
        s32 mTorpedoSpikeRecklessSpawnPointCount = 6;
        sead::Vector3f mTorpedoSpikeRecklessSpawnPointOffsets[16];
        s32 mTorpedoSpikeRecklessSpawnPointPatternSize = 6;
        s32 mTorpedoSpikeRecklessSpawnPointPattern[32];
        s32 mLaunchSpikeInterval = 15;
        s32 mLaunchSpikeAmbientInterval = 90;
        f32 mLaunchSpikeScale = 2.0f;
        s32 mLaunchSpikeShootTime = 60;
        f32 mLaunchSpikeShootDistance = 10000.0f;
        s32 mLaunchSpikeFadeTime = 30;
        f32 mLaunchSpikeUpRate = 0.5f;
        f32 mLaunchSpikeAreaScaleOffset = 2.0f;
        f32 mLaunchSpikesPerSpike = 0.75f;
        s32 mLaunchSpikeOceanSpikeDelay = 30;
        sead::Vector3f mLaunchSpikeOffsets[10];
        s32 mGoldSpikeNumPerIsland = 3;
        s32 mGoldSpikeCoinCount = 10;
        f32 mGoldSpikeCoinBaseSpeed = 5.0f;
        f32 mGoldSpikeCoinSpread = 8.0f;
        s32 mGoldSpikeNumPerIslandOverride[32];
    };

    static_assert(sizeof(Param) == 0x378);

    /// Tuning values of the torpedo spikes Fury Bowser shoots at Plessie.
    struct TorpedoSpikeParam {
        s32 mCount = 3;
        s32 mCount2 = 7;
        s32 mCount3 = 12;
        s32 mCount3BigRamp = 16;
        s32 mCount4 = 16;
        s32 mStartDelayFrames = 30;
        s32 mStartDelayFrames2 = 15;
        s32 mStartDelayFrames3 = 30;
        s32 mStartDelayFrames4 = 30;
        s32 mAppearFrames = 90;
        s32 mAppearFrames3 = 60;
        s32 mAppearFrames4 = 60;
        sead::Vector3f mAppearOffset = {0.0f, 5000.0f, -5000.0f};
        f32 mAppearArcHeight = 6000.0f;
        f32 mAppearArcSpread = 0.15f;
        s32 mShootFrames = 180;
        s32 mShootFrames3 = 140;
        f32 mShootSpeed = 50.0f;
        f32 mShootSpeed2 = 50.0f;
        f32 mShootSpeed3 = 70.0f;
        f32 mShootSpeed4 = 70.0f;
        sead::Vector3f mBaseOffset = {0.0f, 600.0f, 16000.0f};
        sead::Vector3f mBaseOffsetFirst = {0.0f, 600.0f, 20000.0f};
        f32 mPlayerLead = 100.0f;
        f32 mPlayerLeadInterpRate = 0.1f;
        bool mIsPlayerLeadUseVelocity = true;
        f32 mSpacingX = 2000.0f;
        f32 mSpacingZ = 2000.0f;
        f32 mFollowRate = 0.02f;
        f32 mFollowStartAngleScale = 2.0f;
        f32 mFollowAngleLimit = 60.0f;
        s32 mLockFrame = 180;
        s32 mAttackDelay = 90;
        f32 mMinDistanceFromBowser = 13000.0f;
        f32 mMinDistanceFromBowser3 = 15000.0f;
        f32 mMinDistanceFromBowserBigRamp = 18000.0f;
        f32 mMinDistanceFromBowserBigRamp3 = 19000.0f;
        f32 mOffsetScale[24][2];
        f32 mOffsetScale4[24][2];
    };

    static_assert(sizeof(TorpedoSpikeParam) == 0x22c);

    /// Torpedo spike tuning used once Fury Bowser's second form is available.
    struct TorpedoSpikeParamV2 : TorpedoSpikeParam {
        f32 mOffsetScale2[24][2];
        f32 mOffsetScale3[24][2];
        f32 mAngle = 0.0f;
        s32 mShootFrames2 = 120;
        f32 mMinDistanceFromBowser2 = 18000.0f;
        f32 mMinDistanceFromBowserBigRamp2 = 22000.0f;
        f32 mMaxDistanceFromCenter = 5000.0f;
        f32 mMaxDistanceFromCenter2 = 5000.0f;
        f32 mMaxDistanceFromCenter3 = 5000.0f;
        f32 mMaxDistanceFromCenter4 = 5000.0f;
        f32 mPlayerLeadP8 = 25.0f;
    };

    static_assert(sizeof(TorpedoSpikeParamV2) == 0x3d0);

    /// Spike areas of one island, used to pick the spikes replaced by gold spikes.
    struct IslandSpikeData {
        bool tryMarkGoldSpikes(DisasterSpikeDirector* pDirector, s32 num);

        s32 mIslandID = -1;
        DisasterSpikeArea* mAreas[4] = {};
    };

    static_assert(sizeof(IslandSpikeData) == 0x28);

    DisasterSpikeDirector(DisasterModeController* pController, const al::ActorInitInfo& rInfo);

    void loadParam();
    bool shouldUseGoldSpikes() const;
    void loadTorpedoParam(TorpedoSpikeParam* pParam, const char* pKey, bool isV2);
    void begin();
    sead::Vector3f getPlayerPosition() const;
    f32 calcSpikeNoFallRadiusTarget() const;
    f32 calcSpikeNoFallRadiusTargetPlessie() const;
    sead::Vector3f calcTorpedoSpikePlayerLead() const;
    void activateAreasWithIslandID(s32 islandID);
    void setUpGoldSpikes();
    bool isAllFall();
    void update();
    bool updateIslandID();
    void updateCheckAreas();
    void updateLaunchSpikes();
    void updateOceanSpikes();
    TorpedoSpikeParam getTorpedoParam() const;
    sead::Vector3f calcOceanSpikeSpawnOrigin() const;
    Param getParam() const;
    TorpedoSpikeParamV2 getTorpedoParamV2() const;
    f32 getSpikeNoFallRadius() const;
    f32 getSpikeNoFallRadiusPlessie() const;
    void setAllFall(bool isAllFall);
    s32 getAllFallTimer();
    bool isDoubleSpike(DisasterSpike* pBottom, DisasterSpike* pTop);
    DisasterSpike* tryGetSpikeTop(DisasterSpike* pSpike);
    DisasterSpike* tryGetSpikeBottom(DisasterSpike* pSpike);
    bool spawnTorpedoSpike(SuperBowser* pBowser, s32 index, bool isFirst);
    bool spawnRecklessTorpedoSpike(SuperBowser* pBowser);
    sead::Vector3f getNextRecklessTorpedoSpawnPoint();
    DashPanel* getDashPanel(s32 index);
    DashPanel* tryGetDashPanel();
    void forceKillFallingDisasterSpikes();
    void setTorpedoSpikeLockPosDir(sead::Vector3f pos, sead::Vector3f dir);
    sead::Vector3f getTorpedoSpikePlayerLead() const;
    DisasterSpikeTorpedo* getTorpedoSpikeWithID(s32 id);
    void setFlipTorpedoSpikePattern(bool isFlip);
    sead::Vector3f getRecklessTorpedoSpawnPoint(s32 index) const;
    void forceKillTorpedoSpikes();
    DisasterSpikeLaunch* tryGetLaunchSpike();
    void forceKillLaunchSpikes();
    void forceKillAllSpikes();
    sead::Vector3f getLaunchSpikeOrigin() const;
    sead::Vector3f getLaunchSpikePos(s32 index) const;
    DisasterSpike* tryGetGoldSpike() const;
    f32 getPlayerAngle() const;
    bool trySpawnSpike();
    bool getValidOceanSpikePosition(sead::Vector3f& rPos);
    bool isPlayerInAnticipationArea(al::AreaObj* pArea, s32 index);
    bool isPlayerInAnticipationAreaWithIslandID(s32 islandID);

private:
    static constexpr s32 cLaunchSpikeNum = 32;
    static constexpr s32 cGoldSpikeNum = 16;
    static constexpr s32 cIslandNum = 16;

    bool mIsAllFall = false;
    s32 mAllFallTimer = 0;
    Param mParam;
    f32 mSpikeNoFallRadius = 0.0f;         // 0x380
    f32 mSpikeNoFallRadiusPlessie = 0.0f;  // 0x384
    DisasterModeController* mController;   // 0x388
    sead::PtrArray<DisasterSpikeArea> mCheckAreas;
    s32 mIslandID = -1;                    // 0x3a0
    bool mIsFirstTrigger = false;
    al::AreaObjGroup* mDisasterModeAreaGroup = nullptr;   // 0x3a8
    al::AreaObjGroup* mDisasterSpikeAreaGroup = nullptr;  // 0x3b0
    bool mIsInDisasterModeArea = false;                   // 0x3b8
    sead::PtrArray<DisasterSpike> mOceanSpikes;           // 0x3c0
    s32 mOceanSpikeTimer = -1;                            // 0x3d0
    s32 mOceanSpikeCount = 0;
    s32 mOceanSpikeWaveDelay = 200;
    sead::Vector3f mPrevPlayerPos = sead::Vector3f::zero;          // 0x3dc
    sead::Vector3f mOceanSpikeSpawnOrigin = sead::Vector3f::zero;  // 0x3e8
    TorpedoSpikeParam mTorpedoParam;                               // 0x3f4
    TorpedoSpikeParamV2 mTorpedoParamV2;                           // 0x620
    sead::PtrArray<DisasterSpikeTorpedo> mTorpedoSpikes;           // 0x9f0
    sead::PtrArray<DashPanel> mDashPanels;                         // 0xa00
    sead::Vector3f mTorpedoSpikePlayerLead = sead::Vector3f::zero; // 0xa10
    s32 mRecklessSpawnPointIndex = 0;                              // 0xa1c
    sead::PtrArray<DisasterSpikeLaunch> mLaunchSpikes;             // 0xa20
    sead::PtrArray<DisasterSpike> mGoldSpikes;                     // 0xa30
};

static_assert(sizeof(DisasterSpikeDirector) == 0xa40);
