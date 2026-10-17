#include "MapObj/Fury/DisasterSpikeDirector.hpp"

#include <cmath>
#include <cstdio>
#include <math/seadQuat.h>

#include "AreaObj/DisasterModeArea.hpp"
#include "AreaObj/DisasterSpikeArea.hpp"
#include "Enemy/SuperBowser.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/Util/ActorResourceUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "Library/Yaml/ByamlIter.hpp"
#include "MapObj/DashPanel.hpp"
#include "MapObj/DisasterModeController.hpp"
#include "MapObj/DisasterSpike.hpp"
#include "MapObj/Fury/DisasterSpikeLaunch.hpp"
#include "MapObj/Fury/DisasterSpikeTorpedo.hpp"
#include "MapObj/IslandKeeper.hpp"
#include "Player/Normal/PlayerActor.hpp"
#include "Project/AreaObj/AreaObjGroup.hpp"
#include "Project/AreaObj/AreaShape.hpp"
#include "Project/Collision/CollisionUtil.hpp"
#include "Raidon/RaidonSurf.hpp"
#include "Scene/SceneObjID.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/DemoUtil.hpp"
#include "Util/InkUtil.hpp"

namespace {
/**
 * @brief Check whether an area is valid, enabled and active.
 * @param pArea The area.
 * @return True if the area takes part in the disaster.
 */
inline bool isAreaActive(const al::AreaObj* pArea) {
    return pArea->mIsValid && !pArea->mIsDisabled && pArea->_66;
}

/**
 * @brief Get the disaster spike area of a group.
 * @param pGroup The DisasterSpikeArea group.
 * @param index Index of the area in the group.
 * @return The area.
 */
inline DisasterSpikeArea* getSpikeArea(const al::AreaObjGroup* pGroup, s32 index) {
    return static_cast<DisasterSpikeArea*>(pGroup->getAreaObj(index));
}

/**
 * @brief Get the disaster mode area of a group.
 * @param pGroup The DisasterModeArea group.
 * @param index Index of the area in the group.
 * @return The area.
 */
inline DisasterModeArea* getModeArea(const al::AreaObjGroup* pGroup, s32 index) {
    return static_cast<DisasterModeArea*>(pGroup->getAreaObj(index));
}
}  // namespace

/**
 * @brief Construct the director, its parameters and every spike it can spawn.
 * @param pController The disaster mode controller owning the director.
 * @param rInfo Actor init info used to initialize the spikes.
 */
DisasterSpikeDirector::DisasterSpikeDirector(DisasterModeController* pController,
                                             const al::ActorInitInfo& rInfo)
    : mController(pController) {
    loadParam();
    mCheckAreas.allocBuffer(16, nullptr);
    mDisasterModeAreaGroup =
        rc::tryFindAreaObjGroup(mController, rc::AreaObjType::DisasterModeArea);
    mDisasterSpikeAreaGroup =
        rc::tryFindAreaObjGroup(mController, rc::AreaObjType::DisasterSpikeArea);

    if (mDisasterSpikeAreaGroup != nullptr) {
        for (s32 i = 0; i < mDisasterSpikeAreaGroup->getSize(); i++) {
            getSpikeArea(mDisasterSpikeAreaGroup, i)->initSpikes(rInfo, this);
        }
    }

    mOceanSpikes.allocBuffer(mParam.mOceanSpikeMaxCount, nullptr);
    for (s32 i = 0; i < mParam.mOceanSpikeMaxCount; i++) {
        mOceanSpikes.pushBack(new DisasterSpike("DisasterSpike"));
        mOceanSpikes[i]->setDisasterSpikeDirector(this);
        mOceanSpikes[i]->setOceanSpike();
        mOceanSpikes.unsafeAt(i)->init(rInfo);
        al::setScaleAll(mOceanSpikes[i], mParam.mOceanSpikeScale);
        al::setQuat(mOceanSpikes[i], sead::Quatf::unit);
        al::invalidateClipping(mOceanSpikes[i]);
    }

    if (DisasterModeController::isLastBowserBattle(mController)) {
        mTorpedoSpikes.allocBuffer(mParam.mTorpedoSpikeNum, nullptr);
        for (s32 i = 0; i < mParam.mTorpedoSpikeNum; i++) {
            mTorpedoSpikes.pushBack(new DisasterSpikeTorpedo("DisasterSpikeTorpedo"));
            mTorpedoSpikes[i]->setDisasterSpikeDirector(this);
            mTorpedoSpikes.unsafeAt(i)->init(rInfo);
            al::setScaleAll(mTorpedoSpikes[i], mParam.mTorpedoSpikeScale);
            al::setQuat(mTorpedoSpikes[i], sead::Quatf::unit);
        }

        mDashPanels.allocBuffer(mParam.mDashPanelNum, nullptr);
        for (s32 i = 0; i < mParam.mDashPanelNum; i++) {
            mDashPanels.pushBack(new DashPanel("DashPanel"));
            mDashPanels[i]->initNoPlacement(rInfo);
        }
    }

    mLaunchSpikes.allocBuffer(cLaunchSpikeNum, nullptr);
    for (s32 i = 0; i < cLaunchSpikeNum; i++) {
        mLaunchSpikes.pushBack(new DisasterSpikeLaunch("DisasterSpikeLaunch", this));
        mLaunchSpikes.unsafeAt(i)->init(rInfo);
    }

    if (shouldUseGoldSpikes()) {
        mGoldSpikes.allocBuffer(cGoldSpikeNum, nullptr);
        for (s32 i = 0; i < cGoldSpikeNum; i++) {
            mGoldSpikes.pushBack(new DisasterSpike("DisasterSpikeGold"));
            mGoldSpikes[i]->setDisasterSpikeDirector(this);
            mGoldSpikes.unsafeAt(i)->init(rInfo);
            mGoldSpikes.unsafeAt(i)->kill();
        }
    }
}

/**
 * @brief Load the spike parameters from the controller's InitDisasterSpike file.
 */
void DisasterSpikeDirector::loadParam() {
    al::ByamlIter rootIter;
    al::ByamlIter iter;
    if (!al::tryGetActorInitFileIter(&rootIter, mController, "InitDisasterSpike", nullptr)) {
        return;
    }

    rootIter.tryGetIterByKey(&iter, "Disaster Spike");
    iter.tryGetIntByKey(&mParam.mSpikeShadowFadeTime, "SpikeShadowFadeTime");
    iter.tryGetIntByKey(&mParam.mSpikeShadowFadeWaitTime, "SpikeShadowFadeWaitTime");
    iter.tryGetFloatByKey(&mParam.mSpikeShadowMaxAlpha, "SpikeShadowMaxAlpha");
    iter.tryGetIntByKey(&mParam.mSpikeMoveDelayTime, "SpikeMoveDelayTime");
    iter.tryGetIntByKey(&mParam.mSpikeMoveDelayBufferTime, "SpikeMoveDelayBufferTime");
    iter.tryGetFloatByKey(&mParam.mSpikeMoveTimeGlobalScale, "SpikeMoveTimeGlobalScale");
    iter.tryGetIntByKey(&mParam.mSpikeCooldownTime, "SpikeCooldownTime");
    iter.tryGetIntByKey(&mParam.mSpikeShakeDelayTime, "SpikeShakeDelayTime");
    iter.tryGetIntByKey(&mParam.mSpikeShakeTime, "SpikeShakeTime");
    iter.tryGetFloatByKey(&mParam.mSpikeShakeStrength, "SpikeShakeStrength");
    iter.tryGetFloatByKey(&mParam.mSpikeShakeWaitStrength, "SpikeShakeWaitStrength");
    iter.tryGetFloatByKey(&mParam.mSingleSpikeStartDistance, "SingleSpikeStartDistance");
    iter.tryGetFloatByKey(&mParam.mSpikeNoFallRadiusMin, "SpikeNoFallRadiusMin");
    iter.tryGetFloatByKey(&mParam.mSpikeNoFallRadiusMax, "SpikeNoFallRadiusMax");
    iter.tryGetFloatByKey(&mParam.mSpikeNoFallRadiusInterpRate, "SpikeNoFallRadiusInterpRate");
    iter.tryGetFloatByKey(&mParam.mSpikeNoFallRadiusMaxPlayerSpeed,
                          "SpikeNoFallRadiusMaxPlayerSpeed");
    iter.tryGetFloatByKey(&mParam.mSpikeNoFallRadiusPhase0, "SpikeNoFallRadiusPhase0");
    iter.tryGetFloatByKey(&mParam.mSpikeNoFallRadiusPlessieScaleMin,
                          "SpikeNoFallRadiusPlessieScaleMin");
    iter.tryGetFloatByKey(&mParam.mSpikeNoFallRadiusPlessieScaleMax,
                          "SpikeNoFallRadiusPlessieScaleMax");
    iter.tryGetFloatByKey(&mParam.mSpikeNoFallRadiusPlessieNoRideScale,
                          "SpikeNoFallRadiusPlessieNoRideScale");
    iter.tryGetFloatByKey(&mParam.mOceanSpikeInnerRadius, "OceanSpikeInnerRadius");
    iter.tryGetFloatByKey(&mParam.mOceanSpikeOuterRadius, "OceanSpikeOuterRadius");
    iter.tryGetFloatByKey(&mParam.mOceanSpikeInnerAngle, "OceanSpikeInnerAngle");
    iter.tryGetFloatByKey(&mParam.mOceanSpikeOuterAngle, "OceanSpikeOuterAngle");
    iter.tryGetFloatByKey(&mParam.mOceanSpikeVelocityOffset, "OceanSpikeVelocityOffset");
    iter.tryGetFloatByKey(&mParam.mOceanSpikeSpread, "OceanSpikeSpread");
    iter.tryGetFloatByKey(&mParam.mOceanSpikeScale, "OceanSpikeScale");
    iter.tryGetFloatByKey(&mParam.mOceanSpikeLandHeight, "OceanSpikeLandHeight");
    iter.tryGetFloatByKey(&mParam.mOceanSpikeLandHeight2, "OceanSpikeLandHeight2");
    iter.tryGetFloatByKey(&mParam.mOceanSpikeLandHeight3, "OceanSpikeLandHeight3");
    iter.tryGetIntByKey(&mParam.mOceanSpikeStartDelay, "OceanSpikeStartDelay");
    iter.tryGetIntByKey(&mParam.mOceanSpikeWaveDelayMin, "OceanSpikeWaveDelayMin");
    iter.tryGetIntByKey(&mParam.mOceanSpikeWaveDelayMax, "OceanSpikeWaveDelayMax");
    iter.tryGetIntByKey(&mParam.mOceanSpikeInterval, "OceanSpikeInterval");
    iter.tryGetIntByKey(&mParam.mOceanSpikeMoveDelay, "OceanSpikeMoveDelay");
    iter.tryGetIntByKey(&mParam.mOceanSpikeSinkDelayMin, "OceanSpikeSinkDelayMin");
    iter.tryGetIntByKey(&mParam.mOceanSpikeSinkDelayMax, "OceanSpikeSinkDelayMax");
    iter.tryGetIntByKey(&mParam.mOceanSpikeSinkTime, "OceanSpikeSinkTime");
    iter.tryGetIntByKey(&mParam.mOceanSpikeSinkAmount, "OceanSpikeSinkAmount");
    iter.tryGetIntByKey(&mParam.mOceanSpikeWaveCount, "OceanSpikeWaveCount");
    iter.tryGetIntByKey(&mParam.mOceanSpikeMaxCount, "OceanSpikeMaxCount");
    iter.tryGetIntByKey(&mParam.mBouncyJumpSpeed, "BouncySpikeJumpPower");
    iter.tryGetIntByKey(&mParam.mBouncyJumpHighSpeed, "BouncySpikeHighJumpPower");
    iter.tryGetIntByKey(&mParam.mTorpedoSpikeSinkDamageFrames, "TorpedoSpikeSinkDamageFrames");
    iter.tryGetFloatByKey(&mParam.mTorpedoSpikeScale, "TorpedoSpikeScale");
    iter.tryGetFloatByKey(&mParam.mTorpedoSpikeRestrictionOffset,
                          "TorpedoSpikeRestrictionOffset");
    iter.tryGetIntByKey(&mParam.mTorpedoSpikeRecklessStartDelay,
                        "TorpedoSpikeRecklessStartDelay");
    iter.tryGetIntByKey(&mParam.mTorpedoSpikeRecklessBetweenDelay3,
                        "TorpedoSpikeRecklessBetweenDelay3");
    iter.tryGetIntByKey(&mParam.mTorpedoSpikeRecklessBetweenDelay4,
                        "TorpedoSpikeRecklessBetweenDelay4");
    iter.tryGetFloatByKey(&mParam.mTorpedoSpikeRecklessGlobalOffsetSmallRamp.x,
                          "TorpedoSpikeRecklessGlobalOffsetSmallRampX");
    iter.tryGetFloatByKey(&mParam.mTorpedoSpikeRecklessGlobalOffsetSmallRamp.y,
                          "TorpedoSpikeRecklessGlobalOffsetSmallRampY");
    iter.tryGetFloatByKey(&mParam.mTorpedoSpikeRecklessGlobalOffsetSmallRamp.z,
                          "TorpedoSpikeRecklessGlobalOffsetSmallRampZ");
    iter.tryGetFloatByKey(&mParam.mTorpedoSpikeRecklessGlobalOffsetBigRamp.x,
                          "TorpedoSpikeRecklessGlobalOffsetBigRampX");
    iter.tryGetFloatByKey(&mParam.mTorpedoSpikeRecklessGlobalOffsetBigRamp.y,
                          "TorpedoSpikeRecklessGlobalOffsetBigRampY");
    iter.tryGetFloatByKey(&mParam.mTorpedoSpikeRecklessGlobalOffsetBigRamp.z,
                          "TorpedoSpikeRecklessGlobalOffsetBigRampZ");
    iter.tryGetIntByKey(&mParam.mTorpedoSpikeRecklessSpawnPointCount,
                        "TorpedoSpikeRecklessSpawnPointCount");

    for (s32 i = 0; i < 16; i++) {
        char key[64];
        sead::Vector3f& offset = mParam.mTorpedoSpikeRecklessSpawnPointOffsets[i];
        snprintf(key, sizeof(key), "TorpedoSpikeRecklessSpawnPointOffset%dX", i + 1);
        iter.tryGetFloatByKey(&offset.x, key);
        snprintf(key, sizeof(key), "TorpedoSpikeRecklessSpawnPointOffset%dY", i + 1);
        iter.tryGetFloatByKey(&offset.y, key);
        snprintf(key, sizeof(key), "TorpedoSpikeRecklessSpawnPointOffset%dZ", i + 1);
        iter.tryGetFloatByKey(&offset.z, key);
    }

    iter.tryGetIntByKey(&mParam.mTorpedoSpikeRecklessSpawnPointPatternSize,
                        "TorpedoSpikeRecklessSpawnPointPatternSize");
    for (s32 i = 0; i < 32; i++) {
        char key[64];
        snprintf(key, sizeof(key), "TorpedoSpikeRecklessSpawnPointPattern%d", i + 1);
        iter.tryGetIntByKey(&mParam.mTorpedoSpikeRecklessSpawnPointPattern[i], key);
    }

    iter.tryGetIntByKey(&mParam.mLaunchSpikeInterval, "LaunchSpikeInterval");
    iter.tryGetIntByKey(&mParam.mLaunchSpikeAmbientInterval, "LaunchSpikeAmbientInterval");
    iter.tryGetFloatByKey(&mParam.mLaunchSpikeScale, "LaunchSpikeScale");
    iter.tryGetIntByKey(&mParam.mLaunchSpikeShootTime, "LaunchSpikeShootTime");
    iter.tryGetFloatByKey(&mParam.mLaunchSpikeShootDistance, "LaunchSpikeShootDistance");
    iter.tryGetIntByKey(&mParam.mLaunchSpikeFadeTime, "LaunchSpikeFadeTime");
    iter.tryGetFloatByKey(&mParam.mLaunchSpikeUpRate, "LaunchSpikeUpRate");
    iter.tryGetFloatByKey(&mParam.mLaunchSpikeAreaScaleOffset, "LaunchSpikeAreaScaleOffset");
    iter.tryGetFloatByKey(&mParam.mLaunchSpikesPerSpike, "LaunchSpikesPerSpike");
    iter.tryGetIntByKey(&mParam.mLaunchSpikeOceanSpikeDelay, "LaunchSpikeOceanSpikeDelay");

    for (s32 i = 0; i < 10; i++) {
        char key[32];
        sead::Vector3f& offset = mParam.mLaunchSpikeOffsets[i];
        offset = sead::Vector3f::zero;
        snprintf(key, sizeof(key), "LaunchSpikeOffsetX%d", i + 1);
        iter.tryGetFloatByKey(&offset.x, key);
        snprintf(key, sizeof(key), "LaunchSpikeOffsetY%d", i + 1);
        iter.tryGetFloatByKey(&offset.y, key);
        snprintf(key, sizeof(key), "LaunchSpikeOffsetZ%d", i + 1);
        iter.tryGetFloatByKey(&offset.z, key);
    }

    iter.tryGetIntByKey(&mParam.mGoldSpikeNumPerIsland, "GoldSpikeNumPerIsland");
    iter.tryGetIntByKey(&mParam.mGoldSpikeCoinCount, "GoldSpikeCoinCount");
    iter.tryGetFloatByKey(&mParam.mGoldSpikeCoinBaseSpeed, "GoldSpikeCoinBaseSpeed");
    iter.tryGetFloatByKey(&mParam.mGoldSpikeCoinSpread, "GoldSpikeCoinSpread");
    for (s32 i = 0; i < 32; i++) {
        char key[32];
        mParam.mGoldSpikeNumPerIslandOverride[i] = -1;
        snprintf(key, sizeof(key), "GoldSpikeNumPerIslandOverride%d", i);
        iter.tryGetIntByKey(&mParam.mGoldSpikeNumPerIslandOverride[i], key);
    }

    loadTorpedoParam(&mTorpedoParam, "Torpedo Spikes V1", false);
    loadTorpedoParam(&mTorpedoParamV2, "Torpedo Spikes V2", true);
}

/**
 * @brief Check whether the gold spikes are unlocked.
 * @return True once the third phase is unlocked.
 */
bool DisasterSpikeDirector::shouldUseGoldSpikes() const {
    return SingleModeDataFunction::getUnlockedPhase(mController) > 2;
}

/**
 * @brief Load torpedo spike parameters from a section of the InitDisasterSpike file.
 * @param pParam Parameters to fill; a TorpedoSpikeParamV2 if isV2 is set.
 * @param pKey Key of the section to read.
 * @param isV2 Whether the V2-only parameters are loaded too.
 */
void DisasterSpikeDirector::loadTorpedoParam(TorpedoSpikeParam* pParam, const char* pKey,
                                             bool isV2) {
    al::ByamlIter rootIter;
    al::ByamlIter iter;
    if (!al::tryGetActorInitFileIter(&rootIter, mController, "InitDisasterSpike", nullptr)) {
        return;
    }

    rootIter.tryGetIterByKey(&iter, pKey);
    iter.tryGetIntByKey(&pParam->mCount, "TorpedoSpikeCount");
    iter.tryGetIntByKey(&pParam->mCount2, "TorpedoSpikeCount2");
    iter.tryGetIntByKey(&pParam->mCount3, "TorpedoSpikeCount3");
    iter.tryGetIntByKey(&pParam->mCount3BigRamp, "TorpedoSpikeCount3BigRamp");
    iter.tryGetIntByKey(&pParam->mCount4, "TorpedoSpikeCount4");
    iter.tryGetIntByKey(&pParam->mStartDelayFrames, "TorpedoSpikeStartDelayFrames");
    iter.tryGetIntByKey(&pParam->mStartDelayFrames2, "TorpedoSpikeStartDelayFrames2");
    iter.tryGetIntByKey(&pParam->mStartDelayFrames3, "TorpedoSpikeStartDelayFrames3");
    iter.tryGetIntByKey(&pParam->mStartDelayFrames4, "TorpedoSpikeStartDelayFrames4");
    iter.tryGetIntByKey(&pParam->mAppearFrames, "TorpedoSpikeAppearFrames");
    iter.tryGetIntByKey(&pParam->mAppearFrames3, "TorpedoSpikeAppearFrames3");
    iter.tryGetIntByKey(&pParam->mAppearFrames4, "TorpedoSpikeAppearFrames4");
    iter.tryGetFloatByKey(&pParam->mAppearOffset.x, "TorpedoSpikeAppearOffsetX");
    iter.tryGetFloatByKey(&pParam->mAppearOffset.y, "TorpedoSpikeAppearOffsetY");
    iter.tryGetFloatByKey(&pParam->mAppearOffset.z, "TorpedoSpikeAppearOffsetZ");
    iter.tryGetFloatByKey(&pParam->mAppearArcHeight, "TorpedoSpikeAppearArcHeight");
    iter.tryGetFloatByKey(&pParam->mAppearArcSpread, "TorpedoSpikeAppearArcSpread");
    iter.tryGetIntByKey(&pParam->mShootFrames, "TorpedoSpikeShootFrames");
    iter.tryGetIntByKey(&pParam->mShootFrames3, "TorpedoSpikeShootFrames3");
    iter.tryGetFloatByKey(&pParam->mShootSpeed, "TorpedoSpikeShootSpeed");
    iter.tryGetFloatByKey(&pParam->mShootSpeed2, "TorpedoSpikeShootSpeed2");
    iter.tryGetFloatByKey(&pParam->mShootSpeed3, "TorpedoSpikeShootSpeed3");
    iter.tryGetFloatByKey(&pParam->mShootSpeed4, "TorpedoSpikeShootSpeed4");
    iter.tryGetFloatByKey(&pParam->mBaseOffset.x, "TorpedoSpikeBaseOffsetX");
    iter.tryGetFloatByKey(&pParam->mBaseOffset.y, "TorpedoSpikeBaseOffsetY");
    iter.tryGetFloatByKey(&pParam->mBaseOffset.z, "TorpedoSpikeBaseOffsetZ");
    iter.tryGetFloatByKey(&pParam->mBaseOffsetFirst.x, "TorpedoSpikeBaseOffsetFirstX");
    iter.tryGetFloatByKey(&pParam->mBaseOffsetFirst.y, "TorpedoSpikeBaseOffsetFirstY");
    iter.tryGetFloatByKey(&pParam->mBaseOffsetFirst.z, "TorpedoSpikeBaseOffsetFirstZ");
    iter.tryGetFloatByKey(&pParam->mPlayerLead, "TorpedoSpikePlayerLead");
    iter.tryGetFloatByKey(&pParam->mPlayerLeadInterpRate, "TorpedoSpikePlayerLeadInterpRate");
    iter.tryGetBoolByKey(&pParam->mIsPlayerLeadUseVelocity, "TorpedoSpikePlayerLeadUseVelocity");
    iter.tryGetFloatByKey(&pParam->mSpacingX, "TorpedoSpikeSpacingX");
    iter.tryGetFloatByKey(&pParam->mSpacingZ, "TorpedoSpikeSpacingZ");
    iter.tryGetFloatByKey(&pParam->mFollowRate, "TorpedoSpikeFollowRate");
    iter.tryGetFloatByKey(&pParam->mFollowStartAngleScale, "TorpedoSpikeFollowStartAngleScale");
    iter.tryGetFloatByKey(&pParam->mFollowAngleLimit, "TorpedoSpikeFollowAngleLimit");
    iter.tryGetIntByKey(&pParam->mLockFrame, "TorpedoSpikeLockFrame");
    iter.tryGetIntByKey(&pParam->mAttackDelay, "TorpedoSpikeAttackDelay");
    iter.tryGetFloatByKey(&pParam->mMinDistanceFromBowser, "TorpedoSpikeMinDistanceFromBowser");
    iter.tryGetFloatByKey(&pParam->mMinDistanceFromBowser3, "TorpedoSpikeMinDistanceFromBowser3");
    iter.tryGetFloatByKey(&pParam->mMinDistanceFromBowserBigRamp,
                          "TorpedoSpikeMinDistanceFromBowserBigRamp");
    iter.tryGetFloatByKey(&pParam->mMinDistanceFromBowserBigRamp3,
                          "TorpedoSpikeMinDistanceFromBowserBigRamp3");

    char key[64];
    for (s32 i = 0; i < 24; i++) {
        snprintf(key, sizeof(key), "TorpedoSpikeXOffsetScale%d", i + 1);
        if (!iter.tryGetFloatByKey(&pParam->mOffsetScale[i][0], key)) {
            pParam->mOffsetScale[i][0] = 0.0f;
        }
    }

    for (s32 i = 0; i < 24; i++) {
        snprintf(key, sizeof(key), "TorpedoSpikeZOffsetScale%d", i + 1);
        if (!iter.tryGetFloatByKey(&pParam->mOffsetScale[i][1], key)) {
            pParam->mOffsetScale[i][1] = 0.0f;
        }
    }

    auto* paramV2 = static_cast<TorpedoSpikeParamV2*>(pParam);
    if (isV2) {
        for (s32 i = 0; i < 24; i++) {
            snprintf(key, sizeof(key), "TorpedoSpikeOffsetScale2X_%d", i + 1);
            if (!iter.tryGetFloatByKey(&paramV2->mOffsetScale2[i][0], key)) {
                paramV2->mOffsetScale2[i][0] = 0.0f;
            }

            snprintf(key, sizeof(key), "TorpedoSpikeOffsetScale2Z_%d", i + 1);
            if (!iter.tryGetFloatByKey(&paramV2->mOffsetScale2[i][1], key)) {
                paramV2->mOffsetScale2[i][1] = 0.0f;
            }
        }

        for (s32 i = 0; i < 24; i++) {
            snprintf(key, sizeof(key), "TorpedoSpikeOffsetScale3X_%d", i + 1);
            if (!iter.tryGetFloatByKey(&paramV2->mOffsetScale3[i][0], key)) {
                paramV2->mOffsetScale3[i][0] = 0.0f;
            }

            snprintf(key, sizeof(key), "TorpedoSpikeOffsetScale3Z_%d", i + 1);
            if (!iter.tryGetFloatByKey(&paramV2->mOffsetScale3[i][1], key)) {
                paramV2->mOffsetScale3[i][1] = 0.0f;
            }
        }
    }

    for (s32 i = 0; i < 24; i++) {
        snprintf(key, sizeof(key), "TorpedoSpikeXOffsetScale4_%d", i + 1);
        if (!iter.tryGetFloatByKey(&pParam->mOffsetScale4[i][0], key)) {
            pParam->mOffsetScale4[i][0] = 0.0f;
        }
    }

    for (s32 i = 0; i < 24; i++) {
        snprintf(key, sizeof(key), "TorpedoSpikeZOffsetScale4_%d", i + 1);
        if (!iter.tryGetFloatByKey(&pParam->mOffsetScale4[i][1], key)) {
            pParam->mOffsetScale4[i][1] = 0.0f;
        }
    }

    if (isV2) {
        iter.tryGetFloatByKey(&paramV2->mAngle, "TorpedoSpikeAngle");
        iter.tryGetIntByKey(&paramV2->mShootFrames2, "TorpedoSpikeShootFrames2");
        iter.tryGetFloatByKey(&paramV2->mMinDistanceFromBowser2,
                              "TorpedoSpikeMinDistanceFromBowser2");
        iter.tryGetFloatByKey(&paramV2->mMinDistanceFromBowserBigRamp2,
                              "TorpedoSpikeMinDistanceFromBowserBigRamp2");
        iter.tryGetFloatByKey(&paramV2->mMaxDistanceFromCenter,
                              "TorpedoSpikeMaxDistanceFromCenter");
        iter.tryGetFloatByKey(&paramV2->mMaxDistanceFromCenter2,
                              "TorpedoSpikeMaxDistanceFromCenter2");
        iter.tryGetFloatByKey(&paramV2->mMaxDistanceFromCenter3,
                              "TorpedoSpikeMaxDistanceFromCenter3");
        iter.tryGetFloatByKey(&paramV2->mMaxDistanceFromCenter4,
                              "TorpedoSpikeMaxDistanceFromCenter4");
        iter.tryGetFloatByKey(&paramV2->mPlayerLeadP8, "TorpedoSpikePlayerLeadP8");
    }
}

/**
 * @brief Start a disaster: reset the spike state and kill every placed spike.
 */
void DisasterSpikeDirector::begin() {
    mIslandID = -1;
    mIsFirstTrigger = true;
    mOceanSpikeTimer = 0;
    mPrevPlayerPos = getPlayerPosition();
    mSpikeNoFallRadius = calcSpikeNoFallRadiusTarget();
    mSpikeNoFallRadiusPlessie = calcSpikeNoFallRadiusTargetPlessie();
    mTorpedoSpikePlayerLead = calcTorpedoSpikePlayerLead();
    mCheckAreas.clear();
    activateAreasWithIslandID(-1);

    for (s32 i = 0; i < mDisasterSpikeAreaGroup->getSize(); i++) {
        DisasterSpikeArea* area = getSpikeArea(mDisasterSpikeAreaGroup, i);
        for (s32 j = 0; j < area->getDisasterSpikes()->size(); j++) {
            area->getDisasterSpikes()->unsafeAt(j)->kill();
        }
    }

    if (shouldUseGoldSpikes()) {
        setUpGoldSpikes();
    }

    mAllFallTimer = 0;
    if (isAllFall()) {
        for (s32 i = 0; i < mDisasterSpikeAreaGroup->getSize(); i++) {
            getSpikeArea(mDisasterSpikeAreaGroup, i)->trigger(true);
        }
    }
}

/**
 * @brief Get the position of the nearest player.
 * @return The player position.
 */
sead::Vector3f DisasterSpikeDirector::getPlayerPosition() const {
    return al::getTrans(al::tryFindNearestPlayerActor(mController));
}

/**
 * @brief Compute the radius around the player where no spike falls, based on the player speed.
 * @return The target radius.
 */
f32 DisasterSpikeDirector::calcSpikeNoFallRadiusTarget() const {
    if (SingleModeDataFunction::isPhase0(mController)) {
        return mParam.mSpikeNoFallRadiusPhase0;
    }

    auto* player = static_cast<PlayerActor*>(al::tryFindNearestPlayerActor(mController));
    sead::Vector3f velocity =
        al::getTrans(al::tryFindNearestPlayerActor(mController)) - mPrevPlayerPos;
    if (player->isRaidonExist()) {
        auto* raidon = al::tryGetSceneObj<RaidonSurf>(mController, SceneObjID_RaidonSurf);
        if (raidon != nullptr) {
            velocity = al::getVelocity(raidon);
        }
    }

    f32 rate = sead::Vector2f(velocity.x, velocity.z).length() /
               mParam.mSpikeNoFallRadiusMaxPlayerSpeed;
    f32 min = mParam.mSpikeNoFallRadiusMin;
    f32 range = mParam.mSpikeNoFallRadiusMax - min;
    if (rate < 0.0f) {
        rate = 0.0f;
    } else if (rate > 1.0f) {
        rate = 1.0f;
    }

    return min + range * rate;
}

/**
 * @brief Compute the radius around Plessie where no spike falls, based on her speed.
 * @return The target radius, or 0 if Plessie does not exist.
 */
f32 DisasterSpikeDirector::calcSpikeNoFallRadiusTargetPlessie() const {
    auto* raidon = al::tryGetSceneObj<RaidonSurf>(mController, SceneObjID_RaidonSurf);
    if (raidon == nullptr) {
        return 0.0f;
    }

    f32 min = mParam.mSpikeNoFallRadiusMin;
    f32 max = mParam.mSpikeNoFallRadiusMax;
    if (!raidon->isAllGetOffPlayer()) {
        max *= mParam.mSpikeNoFallRadiusPlessieScaleMax;
        min *= mParam.mSpikeNoFallRadiusPlessieScaleMin;
    }

    const sead::Vector3f& velocity = al::getVelocity(raidon);
    f32 rate = sead::Mathf::sqrt(velocity.x * velocity.x + velocity.z * velocity.z) /
               mParam.mSpikeNoFallRadiusMaxPlayerSpeed;
    f32 range = max - min;
    if (rate < 0.0f) {
        rate = 0.0f;
    } else if (rate > 1.0f) {
        rate = 1.0f;
    }

    f32 radius = min + range * rate;
    if (raidon->isAllGetOffPlayer()) {
        radius *= mParam.mSpikeNoFallRadiusPlessieNoRideScale;
    }

    return radius;
}

/**
 * @brief Compute how far ahead of the player the torpedo spikes aim.
 * @return The offset from the player the torpedo spikes aim at.
 */
sead::Vector3f DisasterSpikeDirector::calcTorpedoSpikePlayerLead() const {
    auto* player = static_cast<PlayerActor*>(al::tryFindNearestPlayerActor(mController));
    if (player == nullptr) {
        return sead::Vector3f::zero;
    }

    f32 lead = SingleModeDataFunction::isDarkBowserV2Available(mController) ?
                   mTorpedoParamV2.mPlayerLead :
                   mTorpedoParam.mPlayerLead;
    if (SingleModeDataFunction::isDarkBowserV2Available(mController)) {
        SuperBowser* bowser = mController->getSuperBowser();
        if (bowser != nullptr && bowser->getLastPhase3CurrentIndex() == 8) {
            lead = mTorpedoParamV2.mPlayerLeadP8;
        }
    }

    bool isUseVelocity = SingleModeDataFunction::isDarkBowserV2Available(mController) ?
                             mTorpedoParamV2.mIsPlayerLeadUseVelocity :
                             mTorpedoParam.mIsPlayerLeadUseVelocity;
    sead::Vector3f velocity = sead::Vector3f::zero;
    if (isUseVelocity) {
        if (player->isRaidonExist()) {
            auto* raidon = al::tryGetSceneObj<RaidonSurf>(mController, SceneObjID_RaidonSurf);
            if (raidon != nullptr) {
                velocity = al::getVelocity(raidon);
            }
        } else {
            velocity = al::getVelocity(player);
        }

        velocity.y = 0.0f;
        return velocity * lead;
    }

    sead::Vector3f front = sead::Vector3f::ez;
    if (player->isRaidonExist()) {
        auto* raidon = al::tryGetSceneObj<RaidonSurf>(mController, SceneObjID_RaidonSurf);
        if (raidon != nullptr) {
            velocity = al::getVelocity(raidon);
            al::calcFrontDir(&front, raidon);
        }
    } else {
        velocity = al::getVelocity(player);
        al::calcFrontDir(&front, player);
    }

    return front * (lead * sead::Vector2f(velocity.x, velocity.z).length());
}

/**
 * @brief Add the spike areas of an island to the areas checked for the player.
 * @param islandID Id of the island, or -1 for the areas outside of islands.
 */
void DisasterSpikeDirector::activateAreasWithIslandID(s32 islandID) {
    if (mDisasterSpikeAreaGroup == nullptr) {
        return;
    }

    for (s32 i = 0; i < mDisasterSpikeAreaGroup->getSize(); i++) {
        DisasterSpikeArea* area = getSpikeArea(mDisasterSpikeAreaGroup, i);
        if (area->getIslandID() == islandID) {
            area->clearTriggerFlags();
            mCheckAreas.pushBack(area);
        }
    }
}

/**
 * @brief Pick the spikes replaced by gold spikes on every island.
 */
void DisasterSpikeDirector::setUpGoldSpikes() {
    for (s32 i = 0; i < mDisasterSpikeAreaGroup->getSize(); i++) {
        DisasterSpikeArea* area = getSpikeArea(mDisasterSpikeAreaGroup, i);
        for (s32 j = 0; j < area->getDisasterSpikes()->size(); j++) {
            area->getDisasterSpikes()->at(j)->setReplacedByGold(false);
        }
    }

    for (s32 i = 0; i < cGoldSpikeNum; i++) {
        mGoldSpikes[i]->clearGoldSpikeOriginalSpike();
        mGoldSpikes.unsafeAt(i)->kill();
    }

    IslandSpikeData islandData[cIslandNum];
    for (s32 i = 0; i < mDisasterSpikeAreaGroup->getSize(); i++) {
        DisasterSpikeArea* area = getSpikeArea(mDisasterSpikeAreaGroup, i);
        if (!isAreaActive(area) || area->getIslandID() == -1 || !area->canSpawnGoldSpikes()) {
            continue;
        }

        s32 j = 0;
        for (; j < cIslandNum; j++) {
            IslandSpikeData& data = islandData[j];
            if (data.mIslandID == -1) {
                data.mIslandID = area->getIslandID();
                data.mAreas[0] = area;
                break;
            }

            if (data.mIslandID == area->getIslandID()) {
                for (s32 k = 0; k < 4; k++) {
                    if (data.mAreas[k] == nullptr) {
                        data.mAreas[k] = area;
                        break;
                    }
                }

                break;
            }
        }

        if (j == cIslandNum) {
            break;
        }
    }

    for (s32 i = 0; i < cIslandNum; i++) {
        s32 islandID = islandData[i].mIslandID;
        if (islandID == -1) {
            return;
        }

        s32 num = mParam.mGoldSpikeNumPerIslandOverride[islandID];
        if (num == -1) {
            num = mParam.mGoldSpikeNumPerIsland;
        }

        if (!islandData[i].tryMarkGoldSpikes(this, num)) {
            al::tryGetSceneObj<IslandKeeper>(mController, SceneObjID_IslandKeeper)
                ->findIsland(islandID);
        }
    }
}

/**
 * @brief Check whether every spike falls at once.
 * @return True if all spikes fall and the game is past phase 0.
 */
bool DisasterSpikeDirector::isAllFall() {
    return mIsAllFall && !SingleModeDataFunction::isPhase0(mController);
}

/**
 * @brief Update the spikes: falling areas, launched, ocean and torpedo spikes.
 */
void DisasterSpikeDirector::update() {
    if (mController->getSuperBowser()->isDoingEndingPreparations()) {
        return;
    }

    mSpikeNoFallRadius = al::lerpValue(mParam.mSpikeNoFallRadiusInterpRate, mSpikeNoFallRadius,
                                       calcSpikeNoFallRadiusTarget());
    mSpikeNoFallRadiusPlessie =
        al::lerpValue(mParam.mSpikeNoFallRadiusInterpRate, mSpikeNoFallRadiusPlessie,
                      calcSpikeNoFallRadiusTargetPlessie());
    if (mDisasterSpikeAreaGroup == nullptr || rc::isAnyActiveDemo(mController)) {
        return;
    }

    if (updateIslandID()) {
        updateCheckAreas();
        if (mIslandID != -1 && mDisasterSpikeAreaGroup != nullptr) {
            for (s32 i = 0; i < mDisasterSpikeAreaGroup->getSize(); i++) {
                DisasterSpikeArea* area = getSpikeArea(mDisasterSpikeAreaGroup, i);
                if (isAreaActive(area) && area->getIslandID() == mIslandID) {
                    area->setSpikesKillOutOfView(false);
                } else if (area->getIslandID() != -1) {
                    area->setSpikesKillOutOfView(true);
                }
            }
        }
    }

    updateLaunchSpikes();

    bool isInArea = false;
    for (s32 i = 0; i < mCheckAreas.size(); i++) {
        DisasterSpikeArea* area = mCheckAreas[i];
        if (area->isInVolume(getPlayerPosition())) {
            isInArea = true;
            if (area->trigger(mIsFirstTrigger)) {
                mCheckAreas.erase(i);
                break;
            }
        }
    }

    SuperBowser* bowser = mController->getSuperBowser();
    bool isLastPhase3 = bowser != nullptr && bowser->isLastPhase3Bowser();
    if (isInArea || mIsInDisasterModeArea || mIslandID != -1 || isLastPhase3) {
        mOceanSpikeTimer = -1;
        mOceanSpikeCount = 0;
    } else {
        updateOceanSpikes();
    }

    if (isLastPhase3) {
        al::lerpVec(&mTorpedoSpikePlayerLead, mTorpedoSpikePlayerLead, calcTorpedoSpikePlayerLead(),
                    SingleModeDataFunction::isDarkBowserV2Available(mController) ?
                        mTorpedoParamV2.mPlayerLeadInterpRate :
                        mTorpedoParam.mPlayerLeadInterpRate);
    }

    mIsFirstTrigger = false;
    mOceanSpikeSpawnOrigin = calcOceanSpikeSpawnOrigin();
    mPrevPlayerPos = getPlayerPosition();
    mAllFallTimer++;
}

/**
 * @brief Update the id of the island the player is on.
 * @return True if the island changed.
 */
bool DisasterSpikeDirector::updateIslandID() {
    if (mDisasterModeAreaGroup == nullptr) {
        return false;
    }

    s32 prevIslandID = mIslandID;
    al::AreaObj* area = mDisasterModeAreaGroup->getInVolumeAreaObj(getPlayerPosition());
    mIsInDisasterModeArea = area != nullptr;
    mIslandID = area != nullptr ? area->mZoneID : -1;
    return mIslandID != prevIslandID;
}

/**
 * @brief Rebuild the spike areas checked for the player from the current island.
 */
void DisasterSpikeDirector::updateCheckAreas() {
    mCheckAreas.clear();
    activateAreasWithIslandID(-1);
    if (mIslandID != -1) {
        activateAreasWithIslandID(mIslandID);
    }
}

/**
 * @brief Queue launched spikes on Fury Bowser when the player enters an anticipation area.
 */
void DisasterSpikeDirector::updateLaunchSpikes() {
    if (mController->getSuperBowser() == nullptr) {
        return;
    }

    for (s32 i = 0; i < mDisasterModeAreaGroup->getSize(); i++) {
        DisasterModeArea* area = getModeArea(mDisasterModeAreaGroup, i);
        bool isIn = isPlayerInAnticipationArea(area, i);
        bool wasIn = area->isPlayerInAnticipation();
        if (isPlayerInAnticipationAreaWithIslandID(area->getIslandID())) {
            area->setPlayerInAnticipation(isIn);
            continue;
        }

        area->setPlayerInAnticipation(isIn);
        if (isIn && !wasIn) {
            s32 spikeCount = 0;
            for (s32 j = 0; j < mDisasterSpikeAreaGroup->getSize(); j++) {
                DisasterSpikeArea* spikeArea = getSpikeArea(mDisasterSpikeAreaGroup, j);
                if (isAreaActive(spikeArea) && spikeArea->getIslandID() == area->getIslandID()) {
                    spikeCount += spikeArea->getInactiveSpikeCount();
                }
            }

            mController->getSuperBowser()->setLaunchSpikeQueueCount(
                static_cast<s32>(mParam.mLaunchSpikesPerSpike * spikeCount));
        }
    }
}

/**
 * @brief Spawn the waves of ocean spikes around the player while they are off the islands.
 */
void DisasterSpikeDirector::updateOceanSpikes() {
    mOceanSpikeTimer++;
    s32 frame = mOceanSpikeTimer - mParam.mOceanSpikeStartDelay;
    if (mController->getSuperBowser() != nullptr) {
        mController->getSuperBowser()->clearLaunchSpikeAmbientFrameCount();
        s32 launchFrame = mParam.mLaunchSpikeOceanSpikeDelay + frame;
        if (launchFrame >= 0 && launchFrame % mParam.mOceanSpikeInterval == 0 &&
            mOceanSpikeCount < mParam.mOceanSpikeWaveCount) {
            mController->getSuperBowser()->tryLaunchSpike();
        }
    }

    if (mOceanSpikeTimer < mParam.mOceanSpikeStartDelay) {
        return;
    }

    s32 waveFrames = mParam.mOceanSpikeInterval * mParam.mOceanSpikeWaveCount;
    if (frame == waveFrames) {
        mOceanSpikeWaveDelay =
            al::getRandom(mParam.mOceanSpikeWaveDelayMin, mParam.mOceanSpikeWaveDelayMax);
    }

    if (frame > waveFrames) {
        if (frame > mOceanSpikeWaveDelay + waveFrames) {
            mOceanSpikeCount = 0;
            mOceanSpikeTimer = mParam.mOceanSpikeStartDelay;
        }

        return;
    }

    if (frame % mParam.mOceanSpikeInterval != 0 ||
        mOceanSpikeCount >= mParam.mOceanSpikeWaveCount) {
        return;
    }

    if (trySpawnSpike()) {
        mOceanSpikeCount++;
    } else {
        mOceanSpikeTimer = mParam.mOceanSpikeStartDelay + waveFrames;
    }
}

/**
 * @brief Get the torpedo spike parameters for Fury Bowser's current form.
 * @return The torpedo spike parameters.
 */
DisasterSpikeDirector::TorpedoSpikeParam DisasterSpikeDirector::getTorpedoParam() const {
    if (SingleModeDataFunction::isDarkBowserV2Available(mController)) {
        return mTorpedoParamV2;
    }

    return mTorpedoParam;
}

/**
 * @brief Compute the point the ocean spikes spawn around, ahead of the moving player.
 * @return The spawn origin.
 */
sead::Vector3f DisasterSpikeDirector::calcOceanSpikeSpawnOrigin() const {
    sead::Vector3f playerPos = getPlayerPosition();
    sead::Vector3f trans = getPlayerPosition();
    sead::Vector3f velocity;
    velocity.x = trans.x - mPrevPlayerPos.x;
    velocity.z = trans.z - mPrevPlayerPos.z;

    al::LiveActor* player = al::tryFindNearestPlayerActor(mController);
    if (player != nullptr) {
        velocity.y = 0.0f;
        sead::Vector3f dir = velocity;
        sead::Vector3f front = sead::Vector3f::ez;
        dir.normalize();
        al::calcFrontDir(&front, player);
        if (dir.dot(front) < 0.0f) {
            velocity = sead::Vector3f::zero;
        }
    }

    playerPos.y = 0.0f;
    return playerPos + velocity * mParam.mOceanSpikeVelocityOffset;
}

/**
 * @brief Get the spike parameters.
 * @return A copy of the spike parameters.
 */
DisasterSpikeDirector::Param DisasterSpikeDirector::getParam() const {
    return mParam;
}

/**
 * @brief Get the torpedo spike parameters of Fury Bowser's second form.
 * @return A copy of the V2 torpedo spike parameters.
 */
DisasterSpikeDirector::TorpedoSpikeParamV2 DisasterSpikeDirector::getTorpedoParamV2() const {
    return mTorpedoParamV2;
}

/**
 * @brief Get the radius around the player where no spike falls.
 * @return The radius.
 */
f32 DisasterSpikeDirector::getSpikeNoFallRadius() const {
    return mSpikeNoFallRadius;
}

/**
 * @brief Get the radius around Plessie where no spike falls.
 * @return The radius.
 */
f32 DisasterSpikeDirector::getSpikeNoFallRadiusPlessie() const {
    return mSpikeNoFallRadiusPlessie;
}

/**
 * @brief Set whether every spike falls at once.
 * @param isAllFall True to make every spike fall.
 */
void DisasterSpikeDirector::setAllFall(bool isAllFall) {
    mIsAllFall = isAllFall;
}

/**
 * @brief Get the frames elapsed since the disaster began.
 * @return The frame count.
 */
s32 DisasterSpikeDirector::getAllFallTimer() {
    return mAllFallTimer;
}

/**
 * @brief Check whether a spike landed on top of another one.
 * @param pBottom The lower spike.
 * @param pTop The spike that may lie on pBottom.
 * @return True if pTop lies on the tip of pBottom.
 */
bool DisasterSpikeDirector::isDoubleSpike(DisasterSpike* pBottom, DisasterSpike* pTop) {
    if (pBottom == pTop || al::isDead(pBottom) || al::isDead(pTop)) {
        return false;
    }

    sead::Vector3f up = sead::Vector3f::ey;
    al::calcUpDir(&up, pBottom);
    sead::Vector3f tip = pBottom->getEndPosition();
    tip += up * al::getScale(pBottom).y * 600.0f;
    return (al::getTrans(pTop) - tip).squaredLength() <= 10000.0f;
}

/**
 * @brief Find the spike lying on top of a spike.
 * @param pSpike The lower spike.
 * @return The spike on top, or nullptr if there is none.
 */
DisasterSpike* DisasterSpikeDirector::tryGetSpikeTop(DisasterSpike* pSpike) {
    if (mDisasterSpikeAreaGroup == nullptr) {
        return nullptr;
    }

    for (s32 i = 0; i < mDisasterSpikeAreaGroup->getSize(); i++) {
        sead::PtrArray<DisasterSpike>* spikes =
            getSpikeArea(mDisasterSpikeAreaGroup, i)->getDisasterSpikes();
        for (s32 j = 0; j < spikes->size(); j++) {
            DisasterSpike* spike = spikes->at(j);
            if (isDoubleSpike(pSpike, spike)) {
                return spike;
            }
        }
    }

    return nullptr;
}

/**
 * @brief Find the spike a spike lies on.
 * @param pSpike The upper spike.
 * @return The spike below, or nullptr if there is none.
 */
DisasterSpike* DisasterSpikeDirector::tryGetSpikeBottom(DisasterSpike* pSpike) {
    if (mDisasterSpikeAreaGroup == nullptr) {
        return nullptr;
    }

    for (s32 i = 0; i < mDisasterSpikeAreaGroup->getSize(); i++) {
        sead::PtrArray<DisasterSpike>* spikes =
            getSpikeArea(mDisasterSpikeAreaGroup, i)->getDisasterSpikes();
        for (s32 j = 0; j < spikes->size(); j++) {
            DisasterSpike* spike = spikes->at(j);
            if (isDoubleSpike(spike, pSpike)) {
                return spike;
            }
        }
    }

    return nullptr;
}

/**
 * @brief Shoot a torpedo spike.
 * @param pBowser Fury Bowser shooting the spike.
 * @param index Index of the spike in its volley.
 * @param isFirst Whether this is the first volley.
 * @return True if a free torpedo spike was found.
 */
bool DisasterSpikeDirector::spawnTorpedoSpike(SuperBowser* pBowser, s32 index, bool isFirst) {
    DisasterSpikeTorpedo* torpedo = nullptr;
    for (s32 i = 0; i < mParam.mTorpedoSpikeNum; i++) {
        DisasterSpikeTorpedo* candidate = mTorpedoSpikes[i];
        if (al::isDead(candidate)) {
            torpedo = candidate;
            break;
        }
    }

    if (torpedo == nullptr) {
        return false;
    }

    al::setScaleAll(torpedo, mParam.mTorpedoSpikeScale);
    torpedo->appear(pBowser, index, isFirst);
    return true;
}

/**
 * @brief Shoot a torpedo spike from the next reckless spawn point.
 * @param pBowser Fury Bowser shooting the spike.
 * @return True if a free torpedo spike was found.
 */
bool DisasterSpikeDirector::spawnRecklessTorpedoSpike(SuperBowser* pBowser) {
    DisasterSpikeTorpedo* torpedo = nullptr;
    for (s32 i = 0; i < mParam.mTorpedoSpikeNum; i++) {
        DisasterSpikeTorpedo* candidate = mTorpedoSpikes[i];
        if (al::isDead(candidate)) {
            torpedo = candidate;
            break;
        }
    }

    if (torpedo == nullptr) {
        return false;
    }

    al::setScaleAll(torpedo, mParam.mTorpedoSpikeScale);
    torpedo->appearReckless(pBowser, getNextRecklessTorpedoSpawnPoint());
    return true;
}

/**
 * @brief Get the next reckless torpedo spawn point following the spawn pattern.
 * @return The spawn point.
 */
sead::Vector3f DisasterSpikeDirector::getNextRecklessTorpedoSpawnPoint() {
    s32 index = mParam.mTorpedoSpikeRecklessSpawnPointPattern[mRecklessSpawnPointIndex] - 1;
    mRecklessSpawnPointIndex =
        mRecklessSpawnPointIndex + 1 >= mParam.mTorpedoSpikeRecklessSpawnPointPatternSize ?
            0 :
            mRecklessSpawnPointIndex + 1;
    return getRecklessTorpedoSpawnPoint(index);
}

/**
 * @brief Get a dash panel.
 * @param index Index of the dash panel.
 * @return The dash panel, or nullptr if the index is out of range.
 */
DashPanel* DisasterSpikeDirector::getDashPanel(s32 index) {
    return mDashPanels[index];
}

/**
 * @brief Find a dash panel that is not in use.
 * @return The dash panel, or nullptr if all are in use.
 */
DashPanel* DisasterSpikeDirector::tryGetDashPanel() {
    for (s32 i = 0; i < mParam.mDashPanelNum; i++) {
        if (al::isDead(mDashPanels[i])) {
            return mDashPanels[i];
        }
    }

    return nullptr;
}

/**
 * @brief Kill the spikes that are still falling.
 */
void DisasterSpikeDirector::forceKillFallingDisasterSpikes() {
    if (mDisasterSpikeAreaGroup == nullptr) {
        return;
    }

    for (s32 i = 0; i < mDisasterSpikeAreaGroup->getSize(); i++) {
        sead::PtrArray<DisasterSpike>* spikes =
            getSpikeArea(mDisasterSpikeAreaGroup, i)->getDisasterSpikes();
        for (s32 j = 0; j < spikes->size(); j++) {
            DisasterSpike* spike = spikes->at(j);
            if (!spike->hasLanded()) {
                spike->kill();
            }
        }
    }
}

/**
 * @brief Lock the position and direction the torpedo spikes aim at.
 * @param pos The locked position.
 * @param dir The locked direction.
 */
void DisasterSpikeDirector::setTorpedoSpikeLockPosDir(sead::Vector3f pos, sead::Vector3f dir) {
    mParam.mTorpedoSpikeLockPos = pos;
    mParam.mTorpedoSpikeLockDir = dir;
}

/**
 * @brief Get how far ahead of the player the torpedo spikes aim.
 * @return The smoothed player lead.
 */
sead::Vector3f DisasterSpikeDirector::getTorpedoSpikePlayerLead() const {
    return mTorpedoSpikePlayerLead;
}

/**
 * @brief Find an alive torpedo spike by id.
 * @param id Id of the torpedo spike.
 * @return The torpedo spike, or nullptr if none is alive with that id.
 */
DisasterSpikeTorpedo* DisasterSpikeDirector::getTorpedoSpikeWithID(s32 id) {
    for (s32 i = 0; i < mParam.mTorpedoSpikeNum; i++) {
        DisasterSpikeTorpedo* torpedo = mTorpedoSpikes[i];
        if (al::isAlive(torpedo) && torpedo->getID() == id) {
            return torpedo;
        }
    }

    return nullptr;
}

/**
 * @brief Set whether the torpedo spike pattern is mirrored.
 * @param isFlip True to mirror the pattern.
 */
void DisasterSpikeDirector::setFlipTorpedoSpikePattern(bool isFlip) {
    mParam.mIsFlipTorpedoSpikePattern = isFlip;
}

/**
 * @brief Compute a reckless torpedo spawn point relative to Fury Bowser.
 * @param index Index of the spawn point.
 * @return The spawn point, or zero if Fury Bowser does not exist.
 */
sead::Vector3f DisasterSpikeDirector::getRecklessTorpedoSpawnPoint(s32 index) const {
    if (mController->getSuperBowser() == nullptr) {
        return sead::Vector3f::zero;
    }

    const sead::Vector3f& globalOffset =
        mController->getSuperBowser()->isPlessieChaseBigRamp() ?
            mParam.mTorpedoSpikeRecklessGlobalOffsetBigRamp :
            mParam.mTorpedoSpikeRecklessGlobalOffsetSmallRamp;
    sead::Vector3f offset = globalOffset + mParam.mTorpedoSpikeRecklessSpawnPointOffsets[index];
    if (mController->getSuperBowser()->getCurrentSpawnInfo() != nullptr) {
        sead::Quatf quat = sead::Quatf::unit;
        sead::Vector3f front = mController->getSuperBowser()->getCurrentSpawnInfo()->mFrontDir;
        al::makeQuatFrontUp(&quat, front, sead::Vector3f::ey);
        al::rotateVectorQuat(&offset, quat);
    }

    return al::getTrans(mController->getSuperBowser()) + offset;
}

/**
 * @brief Kill every torpedo spike.
 */
void DisasterSpikeDirector::forceKillTorpedoSpikes() {
    for (s32 i = 0; i < mParam.mTorpedoSpikeNum; i++) {
        mTorpedoSpikes.unsafeAt(i)->kill();
    }
}

/**
 * @brief Find a launched spike that is not in use.
 * @return The launched spike, or nullptr if all are in use.
 */
DisasterSpikeLaunch* DisasterSpikeDirector::tryGetLaunchSpike() {
    for (s32 i = 0; i < cLaunchSpikeNum; i++) {
        if (al::isDead(mLaunchSpikes[i])) {
            return mLaunchSpikes[i];
        }
    }

    return nullptr;
}

/**
 * @brief Kill every launched spike.
 */
void DisasterSpikeDirector::forceKillLaunchSpikes() {
    for (s32 i = 0; i < cLaunchSpikeNum; i++) {
        mLaunchSpikes[i]->forceKill();
    }
}

/**
 * @brief Kill every launched spike and every placed spike.
 */
void DisasterSpikeDirector::forceKillAllSpikes() {
    forceKillLaunchSpikes();
    if (mDisasterSpikeAreaGroup == nullptr) {
        return;
    }

    for (s32 i = 0; i < mDisasterSpikeAreaGroup->getSize(); i++) {
        sead::PtrArray<DisasterSpike>* spikes =
            getSpikeArea(mDisasterSpikeAreaGroup, i)->getDisasterSpikes();
        for (s32 j = 0; j < spikes->size(); j++) {
            DisasterSpike* spike = spikes->at(j);
            if (!al::isDead(spike)) {
                spike->kill();
            }
        }
    }
}

/**
 * @brief Get the point Fury Bowser launches spikes from.
 * @return The position of Fury Bowser's Armor joint.
 */
sead::Vector3f DisasterSpikeDirector::getLaunchSpikeOrigin() const {
    sead::Vector3f origin = sead::Vector3f::zero;
    al::calcJointPos(&origin, mController->getSuperBowser(), "Armor");
    return origin;
}

/**
 * @brief Compute the starting position of a launched spike on Fury Bowser's shell.
 * @param index Index of the launch offset.
 * @return The launch position.
 */
sead::Vector3f DisasterSpikeDirector::getLaunchSpikePos(s32 index) const {
    sead::Vector3f up = sead::Vector3f::ey;
    sead::Vector3f front = sead::Vector3f::ez;
    sead::Quatf quat = sead::Quatf::unit;
    sead::Vector3f offset = mParam.mLaunchSpikeOffsets[index];
    al::calcJointUpDir(&up, mController->getSuperBowser(), "Armor");
    al::calcJointFrontDir(&front, mController->getSuperBowser(), "Armor");
    al::makeQuatFrontUp(&quat, front, up);
    al::rotateVectorQuat(&offset, quat);
    return getLaunchSpikeOrigin() + offset;
}

/**
 * @brief Find a gold spike that is not in use.
 * @return The gold spike, or nullptr if gold spikes are locked or all in use.
 */
DisasterSpike* DisasterSpikeDirector::tryGetGoldSpike() const {
    if (!shouldUseGoldSpikes()) {
        return nullptr;
    }

    for (s32 i = 0; i < cGoldSpikeNum; i++) {
        if (al::isDead(mGoldSpikes[i]) && !mGoldSpikes[i]->isGoldSpikeInUse()) {
            return mGoldSpikes[i];
        }
    }

    return nullptr;
}

/**
 * @brief Compute the angle of the direction from the player to the ocean spike spawn origin.
 * @return The angle in degrees on the XZ plane.
 */
f32 DisasterSpikeDirector::getPlayerAngle() const {
    const sead::Vector3f& trans = al::getTrans(al::tryFindNearestPlayerActor(mController));
    sead::Vector2f dir = {mOceanSpikeSpawnOrigin.x - trans.x, mOceanSpikeSpawnOrigin.z - trans.z};
    if (dir.squaredLength() < 2500.0f) {
        sead::Vector3f front = sead::Vector3f::zero;
        al::calcFrontDir(&front, al::tryFindNearestPlayerActor(mController));
        sead::Vector2f frontDir = {front.x, front.z};
        return al::calcAngleDegree(sead::Vector2f::ex, frontDir);
    }

    return al::calcAngleDegree(sead::Vector2f::ex, dir);
}

/**
 * @brief Spawn an ocean spike around the player.
 * @return True if a spike appeared.
 */
bool DisasterSpikeDirector::trySpawnSpike() {
    DisasterSpike* spike = nullptr;
    for (s32 i = 0; i < mParam.mOceanSpikeMaxCount; i++) {
        DisasterSpike* candidate = mOceanSpikes[i];
        if (al::isDead(candidate)) {
            spike = candidate;
            break;
        }
    }

    if (spike == nullptr) {
        return false;
    }

    sead::Vector3f pos = sead::Vector3f::zero;
    if (!getValidOceanSpikePosition(pos)) {
        return false;
    }

    spike->setPosition(pos);
    al::setScaleAll(spike, mParam.mOceanSpikeScale);
    return spike->tryAppear(false);
}

/**
 * @brief Pick a random position for an ocean spike around the spawn origin.
 * @param rPos The position found.
 * @return True if a valid position was found within ten attempts.
 */
bool DisasterSpikeDirector::getValidOceanSpikePosition(sead::Vector3f& rPos) {
    s32 attempt = 0;
    while (attempt < 10) {
        rPos = mOceanSpikeSpawnOrigin;
        attempt++;
        f32 angle = al::getRandom(mParam.mOceanSpikeInnerAngle, mParam.mOceanSpikeOuterAngle);
        if (((mOceanSpikeCount + attempt) & 1) == 0) {
            angle = -angle;
        }

        angle = sead::Mathf::deg2rad(getPlayerAngle() + angle);
        f32 cosAngle = std::cos(angle);
        f32 sinAngle = std::sin(angle);
        f32 radius = al::getRandom(mParam.mOceanSpikeInnerRadius, mParam.mOceanSpikeOuterRadius);
        rPos += sead::Vector3f(cosAngle, 0.0f, sinAngle) * radius;

        if (rc::isInAreaObj(mController, rc::AreaObjType::OceanSpikeSafeArea, rPos) ||
            rc::isInAreaObj(mController, rc::AreaObjType::IslandArea, rPos) ||
            rc::isInAreaObj(mController, rc::AreaObjType::DisasterModeArea, rPos) ||
            rc::isInAreaObj(mController, rc::AreaObjType::DisasterSpikeArea, rPos)) {
            continue;
        }

        sead::Vector3f up = sead::Vector3f::ey * mParam.mSingleSpikeStartDistance;
        sead::Vector3f down = -up;
        sead::Vector3f hitPos = sead::Vector3f::zero;
        if (alCollisionUtil::getStrikeArrowCollisionParts(mController, &hitPos, rPos + up, down,
                                                          nullptr, nullptr) != nullptr) {
            al::AreaObj* waterArea =
                rc::tryFindAreaObj(mController, rc::AreaObjType::WaterArea, hitPos);
            if (waterArea == nullptr) {
                continue;
            }

            // _28 is the area's base matrix: rest the spike on the water area's surface.
            rPos.y += waterArea->_28(1, 3) + waterArea->getAreaShape()->mScale.y * 500.0f;
            if (rc::isInAreaObj(mController, rc::AreaObjType::OceanSpikeSafeArea, rPos)) {
                continue;
            }
        }

        sead::Vector3f sideX = sead::Vector3f::ex * 400.0f;
        sead::Vector3f sideZ = sead::Vector3f::ez * 400.0f;
        bool isInInk = InkUtil::isInInkLimitArrow(mController, rPos + up + sideX + sideZ, down);
        if (isInInk != InkUtil::isInInkLimitArrow(mController, rPos + up - sideX + sideZ, down) ||
            isInInk != InkUtil::isInInkLimitArrow(mController, rPos + up + sideX - sideZ, down) ||
            isInInk != InkUtil::isInInkLimitArrow(mController, rPos + up - sideX - sideZ, down)) {
            continue;
        }

        f32 landHeight = 0.0f;
        switch (al::getRandom(3) % 3) {
        case 0:
            landHeight = mParam.mOceanSpikeLandHeight;
            break;
        case 1:
            landHeight = mParam.mOceanSpikeLandHeight2;
            break;
        case 2:
            landHeight = mParam.mOceanSpikeLandHeight3;
            break;
        }

        rPos.y += landHeight;

        bool isTooClose = false;
        for (s32 i = 0; i < mParam.mOceanSpikeMaxCount; i++) {
            DisasterSpike* spike = mOceanSpikes[i];
            if (al::isDead(spike)) {
                continue;
            }

            sead::Vector3f startPos = spike->getStartPosition();
            f32 distX = rPos.x - startPos.x;
            f32 distZ = rPos.z - startPos.z;
            if (distX * distX + distZ * distZ <
                mParam.mOceanSpikeSpread * mParam.mOceanSpikeSpread) {
                isTooClose = true;
                break;
            }
        }

        if (!isTooClose) {
            return true;
        }
    }

    return false;
}

/**
 * @brief Check whether the player is in the launch spike anticipation zone of an area.
 * @param pArea The DisasterModeArea.
 * @param index Index of the area in its group.
 * @return True if the player is in the enlarged area and in no overlapping higher area.
 */
bool DisasterSpikeDirector::isPlayerInAnticipationArea(al::AreaObj* pArea, s32 index) {
    sead::Vector3f scale = pArea->getAreaShape()->mScale;
    pArea->getAreaShape()->setScale(sead::Vector3f::ones * mParam.mLaunchSpikeAreaScaleOffset +
                                    scale);
    bool isIn = pArea->isInVolume(getPlayerPosition());
    pArea->getAreaShape()->setScale(scale);
    if (!isIn) {
        return false;
    }

    for (s32 i = 0; i < mDisasterModeAreaGroup->getSize(); i++) {
        al::AreaObj* area = mDisasterModeAreaGroup->getAreaObj(i);
        if (area->getPriority() < pArea->getPriority()) {
            continue;
        }

        if (i <= index && area->getPriority() == pArea->getPriority()) {
            continue;
        }

        sead::Vector3f areaScale = area->getAreaShape()->mScale;
        if (areaScale.x <= mParam.mLaunchSpikeAreaScaleOffset ||
            areaScale.y <= mParam.mLaunchSpikeAreaScaleOffset ||
            areaScale.z <= mParam.mLaunchSpikeAreaScaleOffset) {
            continue;
        }

        area->getAreaShape()->setScale(areaScale -
                                       sead::Vector3f::ones * mParam.mLaunchSpikeAreaScaleOffset);
        bool isInArea = area->isInVolume(getPlayerPosition());
        area->getAreaShape()->setScale(areaScale);
        if (isInArea) {
            return false;
        }
    }

    return true;
}

/**
 * @brief Check whether the player is in the anticipation zone of an island.
 * @param islandID Id of the island.
 * @return True if the player is in an anticipation area of the island.
 */
bool DisasterSpikeDirector::isPlayerInAnticipationAreaWithIslandID(s32 islandID) {
    for (s32 i = 0; i < mDisasterModeAreaGroup->getSize(); i++) {
        DisasterModeArea* area = getModeArea(mDisasterModeAreaGroup, i);
        if (area->getIslandID() == islandID && area->isPlayerInAnticipation()) {
            return true;
        }
    }

    return false;
}

/**
 * @brief Replace random spikes of the island with gold spikes.
 * @param pDirector The spike director.
 * @param num Number of spikes to replace.
 * @return True if exactly num spikes were replaced.
 */
bool DisasterSpikeDirector::IslandSpikeData::tryMarkGoldSpikes(DisasterSpikeDirector* pDirector,
                                                               s32 num) {
    s32 spikeNum = 0;
    for (s32 i = 0; i < 4; i++) {
        if (mAreas[i] == nullptr) {
            break;
        }

        spikeNum += mAreas[i]->getDisasterSpikes()->size();
    }

    s32 markedNum = 0;
    for (s32 attempt = 0; markedNum < num; attempt++) {
        if (attempt >= num * 100) {
            break;
        }

        s32 index = al::getRandom(spikeNum);
        s32 baseIndex = 0;
        for (s32 i = 0; i < 4; i++) {
            if (index < baseIndex + mAreas[i]->getDisasterSpikes()->size()) {
                DisasterSpike* spike = mAreas[i]->getDisasterSpikes()->at(index - baseIndex);
                if (spike->canBeReplacedByGold()) {
                    markedNum++;
                    spike->setReplacedByGold(true);
                }

                break;
            }

            baseIndex += mAreas[i]->getDisasterSpikes()->size();
        }
    }

    return markedNum == num;
}
