#include "System/Data/SingleModeData.hpp"
#include "Enemy/SuperBowser.hpp"
#include "Library/Scene/IUseSceneObjHolder.hpp"
#include "MapObj/DisasterModeController.hpp"
#include "System/ControlUserDataHolder.hpp"
#include "System/Data/OceanScenarioList.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "System/Data/SingleModeStockItemArray.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolder.hpp"
#include "System/IslandData.hpp"
#include "System/IslandDataList.hpp"
#include "System/IslandSaveDataHolder.hpp"
#include "System/ScenarioInfo.hpp"
#include <cstring>
#include <nn/oe.h>
#include <stream/seadStream.h>

// Minimal declarations of external types that have not been reconstructed yet.
namespace neko {
class Target {
  public:
    const sead::Vector3f* tryGetHostTrans() const;

    sead::Vector3f mTrans;
    s32 mIndex;
    bool mIsActive;
    s32 mParentId;
    s32 mId;
};
} // namespace neko

extern sead::SafeString sPhaseName;

namespace {

/**
 * @brief Serialized layout of the single-mode progress block.
 */
struct SingleModeSaveData {
    u32 mFlags;
    u32 mSeenCutsceneFlags;
    SingleModeData::MainScenarioInfo mMainScenario;
    s32 mPhase3DarkBowserHitPointPreBattle;
    s32 mLastIsland;
    s32 mCurrentIsland;
    SingleModeData::CheckpointInfo mCheckpoint;
    SingleModeData::CheckpointInfo mGoalItemCheckpoint;
    s32 mIslandCheckpoint;
    s32 mGoalItemNum;
    s32 mUnlockedPhase;
    s32 mUnlockedIslandNum;
    bool mIsDisasterMode;
    bool mIsNewToPhase1;
    bool mIsNewToPhase2;
    bool mIsNewToPhase3;
    bool mIsNewToPhase4;
    bool mIsNewToPhase2Boss;
    bool mIsNewToPhase3Boss;
    bool mIsNewToPhase1BowserIntro;
    bool mIsNewToPhase2BowserIntro;
    bool mIsNewToPhase3BowserIntro;
    bool mIsNewToPhase1BowserExit;
    bool mIsNewToPhase2BowserExit;
    bool mIsNewToPhase3BowserExit;
    bool mShouldFadeToWhite;
    bool mIsHardToSuperDisasterTransition;
    bool mIsAutoForeshadow;
    s32 mDisasterModeFrames;
    s32 mDisasterModePostBossPeaceFrames;
    s32 mDisasterModeFlowIndex;
    s32 mGigaBellLockCount;
    bool mIsGigaBellUnlocked;
    s32 mPhase1DarkBowserHitPoint;
    s32 mPhase2DarkBowserHitPoint;
    s32 mPhase3DarkBowserHitPoint;
    s32 mPhase4DarkBowserHitPoint;
    bool mIsGigaBellRespawnValid;
    sead::Vector3f mGigaBellRespawnTrans;
    bool mIsGenericRespawnValid;
    sead::Vector3f mGenericRespawnTrans;
    sead::Vector3f mGenericRespawnFront;
    u32 mStockItemCounts[6];
    s32 mDisasterBlocks[0x200];
    SingleModeData::NekoSaveData mNekoSaveData[SingleModeData::cNekoSaveDataNum];
    u32 mGuideMessageSeenFlags;
    u8 mUnknownAF8[0xc];
    u32 mSavedGenericItemFlags;
    s8 mLuckyIslandPos[SingleModeData::cLuckyIslandPosNum];
    SingleModeData::NekoParentData mNekoParentData[SingleModeData::cNekoParentDataNum];
    sead::Vector3f mGigaBellRespawnFront;
    s32 mPhase4DarkBowserHitPointPreBattle;
    s32 mBlockHards[6];
    s32 mDeathCount;
    u8 mUnknownB5C[0x10];
    u32 mIslandPlayTime;
    s64 mPhaseTotalPlayTime;
    u8 mReserved[0x250];
};
static_assert(sizeof(SingleModeSaveData) == 0xdc8);

/**
 * @brief Measure the active play time elapsed since a recorded time stamp.
 * @param rStart Program active time at which the measurement started.
 * @return The number of whole seconds elapsed.
 */
s64 calcElapsedSeconds(const nn::TimeSpan& rStart) {
    const u64 nanoseconds = nn::oe::GetProgramTotalActiveTime().nanoseconds - rStart.nanoseconds;

    // Signed division by 1,000,000,000 through the high half of a reciprocal product.
    const u64 low = static_cast<u32>(nanoseconds);
    const s64 high = static_cast<s64>(nanoseconds) >> 32;
    const s64 middle = high * 0x26d694b3 + ((low * 0x26d694b3) >> 32);
    const s64 highProduct = high * 0x112e0be8;
    const u64 carry = low * 0x112e0be8 + static_cast<u32>(middle);
    const s64 productHigh = highProduct + (middle >> 32) + (carry >> 32);
    return (productHigh >> 26) + (nanoseconds >> 63);
}

/**
 * @brief Copy a block list when both lists have the same size, or clear it otherwise.
 * @param pDst List to overwrite.
 * @param rSrc List to copy from.
 */
void copyBlockList(sead::Buffer<s32>* pDst, const sead::Buffer<s32>& rSrc) {
    if (rSrc.size() == pDst->size()) {
        const s32* pSrcBuffer = rSrc.getBufferPtr();
        s32* pDstBuffer = pDst->getBufferPtr();
        for (s32 i = 0; i != rSrc.size(); i++) {
            pDstBuffer[i] = pSrcBuffer[i];
        }
    } else {
        pDst->fill(0);
    }
}

/**
 * @brief Record a destroyed block in the first free list slot.
 * @param pList List of destroyed block identifiers.
 * @param blockId Identifier of the destroyed block.
 */
void addBlockToList(sead::Buffer<s32>* pList, s32 blockId) {
    for (s32& rEntry : *pList) {
        if (rEntry == 0) {
            rEntry = blockId;
            return;
        }
    }
}

/**
 * @brief Copy a full set of saved cat-target states.
 * @param pSrc States to copy.
 * @param pDst States to overwrite.
 */
void copyNekoSaveData(const SingleModeData::NekoSaveData* pSrc,
                      SingleModeData::NekoSaveData* pDst) {
    for (s32 i = 0; i < SingleModeData::cNekoSaveDataNum; i++) {
        pDst[i] = pSrc[i];
    }
}

/**
 * @brief Copy a full set of saved cat-parent demo states.
 * @param pSrc States to copy.
 * @param pDst States to overwrite.
 */
void copyNekoParentData(const SingleModeData::NekoParentData* pSrc,
                        SingleModeData::NekoParentData* pDst) {
    for (s32 i = 0; i < SingleModeData::cNekoParentDataNum; i++) {
        pDst[i] = pSrc[i];
    }
}

/**
 * @brief Clear a full set of saved cat-target states.
 * @param pData States to clear.
 */
void resetNekoSaveData(SingleModeData::NekoSaveData* pData) {
    for (s32 i = 0; i < SingleModeData::cNekoSaveDataNum; i++) {
        pData[i] = SingleModeData::NekoSaveData();
    }
}

/**
 * @brief Clear a full set of saved cat-parent demo states.
 * @param pData States to clear.
 */
void resetNekoParentData(SingleModeData::NekoParentData* pData) {
    for (s32 i = 0; i < SingleModeData::cNekoParentDataNum; i++) {
        pData[i] = SingleModeData::NekoParentData();
    }
}

} // namespace

SingleModeData::Options SingleModeData::sOptions;

/**
 * @brief Pack the shared options into their save representation.
 * @return The packed option bits.
 */
u16 SingleModeData::getOptionsData() {
    return (sOptions.mIsCameraReverseHorizontal ? 1 : 0) |
           (sOptions.mIsCameraReverseVertical ? 2 : 0) |
           (((sOptions.mCameraSensitivity + 2) & 7) << 2) | ((sOptions.mAssistModeType & 3) << 5);
}

/**
 * @brief Unpack the shared options from their save representation.
 * @param data Packed option bits.
 */
void SingleModeData::setOptionsData(u16 data) {
    const u8 bits = static_cast<u8>(data);
    sOptions.mIsCameraReverseHorizontal = bits & 1;
    sOptions.mIsCameraReverseVertical = (bits >> 1) & 1;
    sOptions.mCameraSensitivity = ((bits >> 2) & 7) - 2;
    sOptions.mAssistModeType = (bits >> 5) & 3;
}

/**
 * @brief Allocate the progress records of a single-mode file and reset it.
 * @param pHolder Game-data holder owning the file.
 * @param fileId Index of the save file.
 */
SingleModeData::SingleModeData(GameDataHolder* pHolder, int fileId)
    : GameDataFileBase(pHolder, fileId, true) {
    mpIslands = new IslandSaveDataHolder(128);
    mpOceanQuadrants = new IslandSaveDataHolder(4);
    mpStockItems = new SingleModeStockItemArray(pHolder);
    mDisasterBlocks.tryAllocBuffer(0x200, nullptr);
    mpNekoSaveData = new NekoSaveData[cNekoSaveDataNum];
    mpNekoParentData = new NekoParentData[cNekoParentDataNum];
    mBlockHards.tryAllocBuffer(6, nullptr);
    initializeData();
}

/**
 * @brief Reset all single-mode progress to a new game.
 */
void SingleModeData::initializeData() {
    GameDataFileBase::initializeData(true);
    mSeenCutsceneFlags = 0;
    mSavedGenericItemFlags = 0;
    mCheckpoint.reset();
    mGoalItemCheckpoint.reset();
    mFlags = 0;
    mMainScenario.mIslandId = 0;
    mMainScenario.mScenarioIndex = 0;
    mLastIsland = -1;
    mCurrentIsland = -1;
    mIslandCheckpoint = -1;
    mGoalItemNum = 2;
    mBossPlayTime = 0;
    mPhaseTotalPlayTime = 0;
    mIslandPlayTime = 0;
    sPhaseName = sead::SafeString();
    mIsPhaseEnd = false;
    mUnlockedPhase = 1;
    mUnlockedIslandNum = 0;
    mDeathCount = 0;
    mIsDisasterMode = false;
    mDisasterModePostBossPeaceFrames = 0;
    mDisasterModeFlowIndex = 0;
    mShouldFadeToWhite = false;
    mIsHardToSuperDisasterTransition = false;
    mIsAutoForeshadow = false;
    mDisasterModeFrames = 0;
    mIsNewToPhase1 = true;
    mIsNewToPhase2 = true;
    mIsNewToPhase3 = true;
    mIsNewToPhase4 = true;
    mIsNewToPhase2Boss = true;
    mIsNewToPhase3Boss = true;
    mIsNewToPhase1BowserIntro = true;
    mIsNewToPhase2BowserIntro = true;
    mIsNewToPhase3BowserIntro = true;
    mIsNewToPhase1BowserExit = true;
    mIsNewToPhase2BowserExit = true;
    mIsNewToPhase3BowserExit = true;
    mGigaBellLockCount = -1;
    mIsGigaBellUnlocked = false;
    mPhase1DarkBowserHitPoint = 100;
    mPhase2DarkBowserHitPoint = 200;
    mPhase3DarkBowserHitPoint = 200;
    mPhase4DarkBowserHitPoint = 200;
    mPhase3DarkBowserHitPointPreBattle = 200;
    mPhase4DarkBowserHitPointPreBattle = 200;
    mPhase4DarkBowserHitPointFinal = 200;
    clearGetGigaBellPlayerRespawnPoint();
    mIsGenericRespawnValid = false;
    mGenericRespawnFront = sead::Vector3f::ez;
    mGenericRespawnTrans.set(0.0f, 0.0f, 0.0f);
    mpIslands->initialize();
    mpOceanQuadrants->initialize();
    mpStockItems->initialize();
    mDisasterBlocks.fill(0);

    resetNekoSaveData(mpNekoSaveData);
    resetNekoParentData(mpNekoParentData);
    mBlockHards.fill(0);
    mGuideMessageSeenFlags[0] = 0;
    mGuideMessageSeenFlags[1] = 0;
    mMapZoomRatio = 0.0f;

    for (s32 i = 0; i < cLuckyIslandPosNum; i++) {
        mLuckyIslandPos[i] = -1;
    }
}

/**
 * @brief Name the current progression phase for play reports.
 * @param pUser Scene-object user used to detect the final Fury Bowser chase, or nullptr.
 * @return The shared phase-name string.
 */
sead::SafeString& SingleModeData::getPhaseName(const al::IUseSceneObjHolder* pUser) {
    switch (mUnlockedPhase) {
    case 0:
        sPhaseName = "phase_0";
        break;
    default:
        sPhaseName = "unknown";
        break;
    case 1:
        sPhaseName = "phase_1";
        break;
    case 2:
        sPhaseName = "boss_1";
        break;
    case 3:
        sPhaseName = "phase_2";
        break;
    case 4:
        sPhaseName = "boss_2";
        break;
    case 5:
        if (pUser != nullptr) {
            DisasterModeController* pController =
                DisasterModeController::tryGetController(pUser);
            if (pController != nullptr && pController->getSuperBowser() != nullptr &&
                pController->getSuperBowser()->isLastPhase3Bowser()) {
                sPhaseName = "chase";
                break;
            }
        }

        sPhaseName = "phase_3";
        break;
    case 6:
        sPhaseName = "boss_3";
        break;
    case 7:
        sPhaseName = "chase_3";
        break;
    case 8:
        sPhaseName = "phase_4";
        break;
    case 9:
        sPhaseName = "boss_4";
        break;
    case 10:
        sPhaseName = "chase_4";
        break;
    }

    return sPhaseName;
}

/**
 * @brief Name the phase cleared at a progression phase for play reports.
 * @param phase Progression phase identifier.
 * @return The shared phase-name string.
 */
sead::SafeString& SingleModeData::getPhaseNameForPhaseClearPR(int phase) {
    switch (phase) {
    case 0:
        sPhaseName = "phase_0";
        break;
    case 1:
    case 2:
        sPhaseName = "phase_1";
        break;
    case 3:
    case 4:
        sPhaseName = "phase_2";
        break;
    case 5:
    case 6:
    case 7:
        sPhaseName = "phase_3";
        break;
    case 8:
    case 10:
        sPhaseName = "phase_4";
        break;
    default:
        sPhaseName = "unknown";
        break;
    }

    return sPhaseName;
}

/**
 * @brief Restart the boss-fight play timer from zero.
 */
void SingleModeData::resetBossPlayTime() {
    mBossPlayTime = 0;
    setBossPlayTime();
}

/**
 * @brief Start measuring boss-fight play time from now.
 */
void SingleModeData::setBossPlayTime() {
    mBossPlayStartTime = nn::oe::GetProgramTotalActiveTime();
}

/**
 * @brief Accumulate the boss-fight play time.
 * @return The accumulated boss-fight play time in seconds.
 */
s32 SingleModeData::getBossPlayTime() {
    mBossPlayTime += calcElapsedSeconds(mBossPlayStartTime);
    return mBossPlayTime;
}

/**
 * @brief Restart the island play timer from zero.
 */
void SingleModeData::resetIslandPlayTime() {
    mIslandPlayTime = 0;
    setIslandPlayTime();
}

/**
 * @brief Start measuring island play time from now.
 */
void SingleModeData::setIslandPlayTime() {
    mIslandPlayStartTime = nn::oe::GetProgramTotalActiveTime();
}

/**
 * @brief Accumulate the island play time.
 * @return The accumulated island play time in seconds.
 */
s32 SingleModeData::getIslandPlayTime() {
    mIslandPlayTime += calcElapsedSeconds(mIslandPlayStartTime);
    return mIslandPlayTime;
}

/**
 * @brief Restart the total phase play timer from zero.
 */
void SingleModeData::resetPhaseTotalPlayTime() {
    mPhaseTotalPlayTime = 0;
    setPhasePlayTime();
}

/**
 * @brief Start measuring phase play time from now.
 */
void SingleModeData::setPhasePlayTime() {
    mPhasePlayStartTime = nn::oe::GetProgramTotalActiveTime();
}

/**
 * @brief Add the time elapsed since the last update to the total phase play time.
 */
void SingleModeData::updatePhasePlayTime() {
    mPhaseTotalPlayTime += calcElapsedSeconds(mPhasePlayStartTime);
    setPhasePlayTime();
}

/**
 * @brief Update and read the total phase play time.
 * @return The total phase play time in seconds.
 */
s32 SingleModeData::getPhasePlayTime() {
    updatePhasePlayTime();
    return mPhaseTotalPlayTime;
}

/**
 * @brief Update the all-shines flag from the collected goal-item count.
 */
void SingleModeData::calculateAllShineCollected() {
    if (mGoalItemNum > 99) {
        mFlags |= 0x400;
    } else {
        mFlags &= ~0x400;
    }
}

/**
 * @brief Remove the graffiti vandalism from every island.
 */
void SingleModeData::clearIslandGraffitiVandalized() { mpIslands->clearIslandVandalizedOnly(); }

/**
 * @brief Check a single-mode progression flag.
 * @return True when the progression flag is set.
 */
bool SingleModeData::isAllShineCollected() const { return hasFlag(0x400); }

/**
 * @brief Check whether every island and ocean scenario has been completed.
 * @return True when no scenario is left incomplete.
 */
bool SingleModeData::isAllShineCollectedIslandData() {
    const s32 islandNum = mpHolder->getIslandDataList()->getNum();
    for (s32 i = 0; i < islandNum; i++) {
        IslandData* pIsland = mpHolder->getIslandDataList()->getIslandByIndex(i);
        if (!pIsland->isValid()) {
            continue;
        }

        const s32 scenarioNum = pIsland->getNumScenarios();
        const IslandSaveData* pSaveData = mpIslands->getIslandSaveData(i);
        for (s32 j = 0; j < scenarioNum; j++) {
            if (!pSaveData->isScenarioComplete(j)) {
                return false;
            }
        }
    }

    for (s32 i = 0; i < 4; i++) {
        const s32 scenarioNum =
            mpHolder->getOceanScenarioList()->getScenarioListByQuadrant(i)->mCount;
        const IslandSaveData* pSaveData = mpOceanQuadrants->getIslandSaveData(i);
        for (s32 j = 0; j < scenarioNum; j++) {
            if (!pSaveData->isScenarioComplete(j)) {
                return false;
            }
        }
    }

    return true;
}

/**
 * @brief Check a single-mode progression flag.
 * @return True when the progression flag is set.
 */
bool SingleModeData::isCompleteEndingPictureSeen() const { return hasFlag(0x100); }

/**
 * @brief Check whether all completion requirements are met.
 * @return True when the ending picture, final boss, and all-shines flags are set.
 */
bool SingleModeData::isFileComplete() const { return (mFlags & 0x700) == 0x700; }

/**
 * @brief Check a single-mode progression flag.
 * @return True when the progression flag is set.
 */
bool SingleModeData::isDarkBowserV2Defeated() const { return hasFlag(0x200); }

/**
 * @brief Check whether the super-hard Fury Bowser should be used.
 * @param hitPoint Hit points compared against the stored threshold.
 * @return True when the hard fight is unlocked and not yet won, or when the late-game
 * threshold applies.
 */
bool SingleModeData::isSuperHardModeOn(int hitPoint) const {
    if (isDarkBowserV2Available() && !isDarkBowserV2Defeated()) {
        return true;
    }

    if (mGoalItemNum >= 47 && mUnlockedPhase == 5 && mGigaBellLockCount > hitPoint) {
        return true;
    }

    return false;
}

/**
 * @brief Check a single-mode progression flag.
 * @return True when the progression flag is set.
 */
bool SingleModeData::isDarkBowserV2Available() const { return hasFlag(0x400); }

/**
 * @brief Read single-mode progression.
 * @return The stored progression identifier or count.
 */
int SingleModeData::getUnlockedPhase() const { return mUnlockedPhase; }

/**
 * @brief Handle the single-mode save notification, which requires no additional work.
 */
void SingleModeData::onSave() {}

/**
 * @brief Start saving the file.
 */
void SingleModeData::startSave() { GameDataFileBase::startSave(); }

/**
 * @brief Check whether all completion requirements are met.
 * @return True when the ending picture, final boss, and all-shines flags are set.
 */
bool SingleModeData::isMeowserJrAvailable() const { return (mFlags & 0x700) == 0x700; }

/**
 * @brief Record single-mode progression.
 */
void SingleModeData::setCompleteEndingPictureSeen() { mFlags |= 0x100; }

/**
 * @brief Record single-mode progression.
 */
void SingleModeData::setDarkBowserV2Defeated() { mFlags |= 0x200; }

/**
 * @brief Reset player figures for a single-mode restart.
 */
void SingleModeData::restartStage() { mpUsers->resetFigureType(); }

/**
 * @brief Copy all saved progress from another single-mode file.
 * @param rOther File to copy from.
 */
void SingleModeData::copySingleModeFile(const SingleModeData& rOther) {
    initializeData();
    copyFileBase(rOther);
    mFlags = rOther.mFlags;
    mpIslands->copy(*rOther.mpIslands);
    mpOceanQuadrants->copy(*rOther.mpOceanQuadrants);
    mpStockItems->copy(rOther.mpStockItems);
    mMainScenario.mIslandId = rOther.mMainScenario.mIslandId;
    mMainScenario.mScenarioIndex = rOther.mMainScenario.mScenarioIndex;
    mLastIsland = rOther.mLastIsland;
    mCurrentIsland = rOther.mCurrentIsland;
    mCheckpoint = rOther.mCheckpoint;
    mGoalItemCheckpoint = rOther.mGoalItemCheckpoint;
    mIslandCheckpoint = rOther.mIslandCheckpoint;
    mGoalItemNum = rOther.mGoalItemNum;
    mUnlockedPhase = rOther.mUnlockedPhase;
    mUnlockedIslandNum = rOther.mUnlockedIslandNum;
    mIsDisasterMode = rOther.mIsDisasterMode;
    mIsNewToPhase1 = rOther.mIsNewToPhase1;
    mIsNewToPhase2 = rOther.mIsNewToPhase2;
    mIsNewToPhase3 = rOther.mIsNewToPhase3;
    mIsNewToPhase4 = rOther.mIsNewToPhase4;
    mIsNewToPhase2Boss = rOther.mIsNewToPhase2Boss;
    mIsNewToPhase3Boss = rOther.mIsNewToPhase3Boss;
    mIsNewToPhase1BowserIntro = rOther.mIsNewToPhase1BowserIntro;
    mIsNewToPhase2BowserIntro = rOther.mIsNewToPhase2BowserIntro;
    mIsNewToPhase3BowserIntro = rOther.mIsNewToPhase3BowserIntro;
    mIsNewToPhase1BowserExit = rOther.mIsNewToPhase1BowserExit;
    mIsNewToPhase2BowserExit = rOther.mIsNewToPhase2BowserExit;
    mIsNewToPhase3BowserExit = rOther.mIsNewToPhase3BowserExit;
    mShouldFadeToWhite = rOther.mShouldFadeToWhite;
    mIsHardToSuperDisasterTransition = rOther.mIsHardToSuperDisasterTransition;
    mIsAutoForeshadow = rOther.mIsAutoForeshadow;
    mDisasterModeFrames = rOther.mDisasterModeFrames;
    mDisasterModePostBossPeaceFrames = rOther.mDisasterModePostBossPeaceFrames;
    mDisasterModeFlowIndex = rOther.mDisasterModeFlowIndex;
    mGigaBellLockCount = rOther.mGigaBellLockCount;
    mIsGigaBellUnlocked = rOther.mIsGigaBellUnlocked;
    mPhase1DarkBowserHitPoint = rOther.mPhase1DarkBowserHitPoint;
    mPhase2DarkBowserHitPoint = rOther.mPhase2DarkBowserHitPoint;
    mPhase3DarkBowserHitPoint = rOther.mPhase3DarkBowserHitPoint;
    mPhase4DarkBowserHitPoint = rOther.mPhase4DarkBowserHitPoint;
    mPhase3DarkBowserHitPointPreBattle = rOther.mPhase3DarkBowserHitPointPreBattle;
    mPhase4DarkBowserHitPointPreBattle = rOther.mPhase4DarkBowserHitPointPreBattle;
    mIsGigaBellRespawnValid = rOther.mIsGigaBellRespawnValid;
    mGigaBellRespawnTrans = rOther.mGigaBellRespawnTrans;
    mGigaBellRespawnFront = rOther.mGigaBellRespawnFront;
    mIsGenericRespawnValid = rOther.mIsGenericRespawnValid;
    mGenericRespawnTrans = rOther.mGenericRespawnTrans;
    mGenericRespawnFront = rOther.mGenericRespawnFront;
    mSeenCutsceneFlags = rOther.mSeenCutsceneFlags;
    mSavedGenericItemFlags = rOther.mSavedGenericItemFlags;

    for (s32 i = 0; i < cLuckyIslandPosNum; i++) {
        mLuckyIslandPos[i] = rOther.mLuckyIslandPos[i];
    }

    copyBrokenDisasterBlockList(rOther.mDisasterBlocks);

    copyNekoSaveData(rOther.mpNekoSaveData, mpNekoSaveData);
    copyNekoParentData(rOther.mpNekoParentData, mpNekoParentData);

    copyBrokenBlockHardList(rOther.mBlockHards);
    mGuideMessageSeenFlags[0] = rOther.mGuideMessageSeenFlags[0];
    mMapZoomRatio = rOther.mMapZoomRatio;
    mDeathCount = rOther.mDeathCount;
    mBossPlayTime = rOther.mBossPlayTime;
    mIslandPlayTime = rOther.mIslandPlayTime;
    mPhaseTotalPlayTime = rOther.mPhaseTotalPlayTime;
}

/**
 * @brief Copy the destroyed disaster-block list of another file.
 * @param rList List to copy; a list of a different size clears this file's list instead.
 */
void SingleModeData::copyBrokenDisasterBlockList(const sead::Buffer<s32>& rList) {
    copyBlockList(&mDisasterBlocks, rList);
}

/**
 * @brief Copy the destroyed hard-block list of another file.
 * @param rList List to copy; a list of a different size clears this file's list instead.
 */
void SingleModeData::copyBrokenBlockHardList(const sead::Buffer<s32>& rList) {
    copyBlockList(&mBlockHards, rList);
}

/**
 * @brief Start a single-mode stage.
 * @param worldId World index of the stage.
 * @param stageId Stage index within the world.
 */
void SingleModeData::startStage(int worldId, int stageId) {
    GameDataFunction::calcCourseId(GameDataHolderAccessor(mpHolder), worldId, stageId);
}

/**
 * @brief Unlock an island and count it.
 * @param islandId Island to unlock.
 */
void SingleModeData::setIslandUnlocked(int islandId) {
    mpIslands->setIslandUnlocked(islandId);
    mUnlockedIslandNum++;
}

/**
 * @brief Check whether an island is unlocked.
 * @param islandId Island to check.
 * @return True when the island is unlocked.
 */
bool SingleModeData::isIslandUnlocked(int islandId) const {
    return mpIslands->isIslandUnlocked(islandId);
}

/**
 * @brief Vandalize an island during a phase.
 * @param islandId Island to vandalize.
 * @param phase Phase in which the vandalism happens.
 */
void SingleModeData::setVandalize(int islandId, int phase) {
    mpIslands->setIslandVandalized(islandId, phase);
}

/**
 * @brief Remove the vandalism of an island during a phase.
 * @param islandId Island to clean up.
 * @param phase Phase in which the vandalism happened.
 */
void SingleModeData::clearVandalize(int islandId, int phase) {
    mpIslands->clearIslandVandalized(islandId, phase);
}

/**
 * @brief Check whether an island is vandalized.
 * @param islandId Island to check.
 * @param phase Phase to check.
 * @param pActive Receives whether the vandalism is active.
 * @return True when the island is vandalized.
 */
bool SingleModeData::isVandalized(int islandId, int phase, bool* pActive) const {
    return mpIslands->isIslandVandalized(islandId, phase, pActive);
}

/**
 * @brief Read single-mode progression.
 * @return The stored progression identifier or count.
 */
int SingleModeData::getUnlockedIslandNum() const { return mUnlockedIslandNum; }

/**
 * @brief Read single-mode progression.
 * @return The stored progression identifier or count.
 */
int SingleModeData::getLastValidIslandVisited() const { return mLastIsland; }

/**
 * @brief Record the previous valid island.
 * @param islandId Valid island identifier to remember.
 */
void SingleModeData::setLastValidIslandVisited(int islandId) { mLastIsland = islandId; }

/**
 * @brief Read single-mode progression.
 * @return The stored progression identifier or count.
 */
int SingleModeData::getCurValidIslandVisited() const { return mCurrentIsland; }

/**
 * @brief Record the current valid island and mark it as visited.
 * @param islandId Valid island identifier.
 */
void SingleModeData::setCurValidIslandVisited(int islandId) {
    mCurrentIsland = islandId;
    if (mpIslands->isIslandFirstVisit(islandId)) {
        mpIslands->setIslandVisited(islandId);
    }
}

/**
 * @brief Read the active scenario of an island.
 * @param islandId Island to query; negative identifiers have no scenario.
 * @return The active scenario index, or -1.
 */
int SingleModeData::getCurActiveScenarioIndex(int islandId) const {
    if (islandId < 0) {
        return -1;
    }

    return mpIslands->getIslandSaveData(islandId)->mCurActiveScenario;
}

/**
 * @brief Access the saved progress of an island.
 * @param islandId Island to access.
 * @return The island's saved progress.
 */
const IslandSaveData* SingleModeData::getIslandSaveData(int islandId) const {
    return mpIslands->getIslandSaveData(islandId);
}

/**
 * @brief Set the active scenario of an island.
 * @param islandId Island to update; negative identifiers are ignored.
 * @param scenarioIndex Scenario to activate.
 */
void SingleModeData::setCurActiveScenarioIndex(int islandId, int scenarioIndex) {
    if (islandId < 0) {
        return;
    }

    mpIslands->getIslandSaveDataPtr(islandId)->setCurActiveScenario(scenarioIndex);
}

/**
 * @brief Access the mutable saved progress of an island.
 * @param islandId Island to access.
 * @return The island's saved progress.
 */
IslandSaveData* SingleModeData::getIslandSaveDataPtr(int islandId) {
    return mpIslands->getIslandSaveDataPtr(islandId);
}

/**
 * @brief Mark the name of an island's active scenario as seen.
 * @param islandId Island to update; negative identifiers are ignored.
 */
void SingleModeData::setCurActiveScenarioNameSeen(int islandId) {
    if (islandId < 0) {
        return;
    }

    mpIslands->getIslandSaveDataPtr(islandId)->setCurActiveScenarioNameSeen();
}

/**
 * @brief Check whether the name of an island's active scenario was seen.
 * @param islandId Island to check; negative identifiers report false.
 * @return True when the scenario name was seen.
 */
bool SingleModeData::wasActiveScenarioNameSeen(int islandId) {
    if (islandId < 0) {
        return false;
    }

    return mpIslands->getIslandSaveDataPtr(islandId)->wasActiveScenarioNameSeen();
}

/**
 * @brief Record a passed checkpoint and forget the other saved positions.
 * @param islandId Island owning the checkpoint.
 * @param checkpointId Checkpoint identifier.
 */
void SingleModeData::setCheckpointVisited(int islandId, int checkpointId) {
    mCheckpoint = CheckpointInfo(islandId, checkpointId);
    mGoalItemCheckpoint = CheckpointInfo();
    mIslandCheckpoint = -1;
    mIsGenericRespawnValid = false;
}

/**
 * @brief Record a passed goal-item checkpoint and forget the other saved positions.
 * @param islandId Island owning the checkpoint.
 * @param checkpointId Checkpoint identifier.
 */
void SingleModeData::setGoalItemCheckpointVisited(int islandId, int checkpointId) {
    mCheckpoint = CheckpointInfo();
    mGoalItemCheckpoint = CheckpointInfo(islandId, checkpointId);
    mIslandCheckpoint = -1;
    mIsGenericRespawnValid = false;
}

/**
 * @brief Record a passed island checkpoint unless the island already has a saved position.
 * @param islandId Island whose checkpoint was passed.
 * @return True when the island checkpoint was recorded.
 */
bool SingleModeData::setIslandCheckpointVisited(int islandId) {
    if (islandId < 0) {
        return false;
    }

    if (mCheckpoint.mIslandId == islandId) {
        return false;
    }

    if (mGoalItemCheckpoint.mIslandId == islandId) {
        return false;
    }

    if (mIslandCheckpoint == islandId) {
        return false;
    }

    mIslandCheckpoint = islandId;
    mCheckpoint.reset();
    mGoalItemCheckpoint.reset();
    mIsGenericRespawnValid = false;
    return true;
}

/**
 * @brief Forget the passed island checkpoint.
 */
void SingleModeData::clearLastIslandCheckPointPass() { mIslandCheckpoint = -1; }

/**
 * @brief Forget every saved position except the Giga Bell respawn point.
 */
void SingleModeData::clearAllButGigaBellCheckpoint() {
    mCheckpoint.reset();
    mGoalItemCheckpoint.reset();
    mIslandCheckpoint = -1;
    mIsGenericRespawnValid = false;
}

/**
 * @brief Forget every saved position.
 */
void SingleModeData::clearAllCheckpointPass() {
    clearAllButGigaBellCheckpoint();
    clearGetGigaBellPlayerRespawnPoint();
}

/**
 * @brief Forget the Giga Bell respawn point.
 */
void SingleModeData::clearGetGigaBellPlayerRespawnPoint() {
    mIsGigaBellRespawnValid = false;
    mGigaBellRespawnTrans.set(0.0f, 0.0f, 0.0f);
    mGigaBellRespawnFront = sead::Vector3f::ez;
}

/**
 * @brief Check whether the file stores any position to restart from.
 * @return True when a checkpoint, respawn point, or final-phase position is saved.
 */
bool SingleModeData::isAnySavedPosition() const {
    if (mCheckpoint.mCheckpointId >= 0 && mCheckpoint.mIslandId >= 0) {
        return true;
    }

    if (mIsGigaBellRespawnValid) {
        return true;
    }

    if (mIsGenericRespawnValid) {
        return true;
    }

    if (mIslandCheckpoint >= 0) {
        return true;
    }

    if (isPhase4BossDefeated() && mUnlockedPhase == 8) {
        return true;
    }

    return false;
}

/**
 * @brief Check a single-mode progression flag.
 * @return True when the progression flag is set.
 */
bool SingleModeData::isPhase4BossDefeated() const { return hasFlag(0x800); }

/**
 * @brief Check a single-mode progression flag.
 * @return True when the progression flag is set.
 */
bool SingleModeData::hasSeenEnding() const { return hasFlag(0x1); }

/**
 * @brief Record single-mode progression.
 */
void SingleModeData::setHasSeenEnding() { mFlags |= 0x1; }

/**
 * @brief Check whether phase 0 is being played for the first time.
 * @return True until phase 0 has been started once.
 */
bool SingleModeData::isFirstPhase0() const { return !hasFlag(0x20); }

/**
 * @brief Record single-mode progression.
 */
void SingleModeData::setIsFirstPhase0() { mFlags |= 0x20; }

/**
 * @brief Check a single-mode progression flag.
 * @return True when the progression flag is set.
 */
bool SingleModeData::isFirstPhase2BossDefeated() const { return hasFlag(0x40); }

/**
 * @brief Record single-mode progression.
 */
void SingleModeData::setFirstPhase2BossDefeated() { mFlags |= 0x40; }

/**
 * @brief Check a single-mode progression flag.
 * @return True when the progression flag is set.
 */
bool SingleModeData::isFirstPhase3BossDefeated() const { return hasFlag(0x80); }

/**
 * @brief Record single-mode progression.
 */
void SingleModeData::setFirstPhase3BossDefeated() { mFlags |= 0x80; }

/**
 * @brief Record single-mode progression.
 */
void SingleModeData::setPhase4BossDefeated() { mFlags |= 0x800; }

/**
 * @brief Check a single-mode progression flag.
 * @return True when the progression flag is set.
 */
bool SingleModeData::isAlreadyPlayRidon() const { return hasFlag(0x1000); }

/**
 * @brief Record single-mode progression.
 */
void SingleModeData::setPlayRidon() { mFlags |= 0x1000; }

/**
 * @brief Forget that the Plessie ride was played.
 */
void SingleModeData::resetPlayRidon() { mFlags &= ~0x1000; }

/**
 * @brief Set the unlocked progression phase.
 * @param phase Progression phase identifier.
 */
void SingleModeData::setUnlockedPhase(int phase) { mUnlockedPhase = phase; }

/**
 * @brief Return from a Plessie chase phase to the preceding battle phase.
 */
void SingleModeData::setPhaseFromPlessieChase() {
    switch (mUnlockedPhase) {
    case 7:
        mUnlockedPhase = 5;
        setPhase3DarkBowserHitPoint(getPhase3DarkBowserHitPointPreBattle());
        break;
    case 10:
        mUnlockedPhase = 8;
        setPhase4DarkBowserHitPoint(getPhase4DarkBowserHitPointPreBattle());
        break;
    }
}

/**
 * @brief Store Fury Bowser's phase-3 hit points.
 * @param hitPoint Remaining hit points.
 */
void SingleModeData::setPhase3DarkBowserHitPoint(int hitPoint) {
    mPhase3DarkBowserHitPoint = hitPoint;
}

/**
 * @brief Read Fury Bowser's phase-3 hit points before the battle.
 * @return The stored hit points.
 */
int SingleModeData::getPhase3DarkBowserHitPointPreBattle() const {
    return mPhase3DarkBowserHitPointPreBattle;
}

/**
 * @brief Store Fury Bowser's phase-4 hit points.
 * @param hitPoint Remaining hit points; negative values restore the full 200.
 */
void SingleModeData::setPhase4DarkBowserHitPoint(int hitPoint) {
    mPhase4DarkBowserHitPoint = hitPoint < 0 ? 200 : hitPoint;
}

/**
 * @brief Read Fury Bowser's phase-4 hit points before the battle.
 * @return The stored hit points.
 */
int SingleModeData::getPhase4DarkBowserHitPointPreBattle() const {
    return mPhase4DarkBowserHitPointPreBattle;
}

/**
 * @brief Check whether a cutscene was seen.
 * @param cutsceneId Cutscene bit index.
 * @return True when the cutscene was seen.
 */
bool SingleModeData::hasSeenCutscene(int cutsceneId) const {
    return ((1 << cutsceneId) & mSeenCutsceneFlags) != 0;
}

/**
 * @brief Mark a cutscene as seen.
 * @param cutsceneId Cutscene bit index.
 */
void SingleModeData::setHasSeenCutscene(int cutsceneId) { mSeenCutsceneFlags |= 1 << cutsceneId; }

/**
 * @brief Mark a cutscene as not seen.
 * @param cutsceneId Cutscene bit index.
 */
void SingleModeData::clearHasSeenCutscene(int cutsceneId) {
    mSeenCutsceneFlags &= ~(1 << cutsceneId);
}

/**
 * @brief Check whether a generic item was saved.
 * @param itemId Item bit index.
 * @return True when the item was saved.
 */
bool SingleModeData::isGenericItemSaved(int itemId) const {
    return ((1 << itemId) & mSavedGenericItemFlags) != 0;
}

/**
 * @brief Mark a generic item as saved.
 * @param itemId Item bit index.
 */
void SingleModeData::setGenericItemSaved(int itemId) { mSavedGenericItemFlags |= 1 << itemId; }

/**
 * @brief Read Fury Bowser's phase-1 hit points.
 * @return The stored hit points.
 */
int SingleModeData::getPhase1DarkBowserHitPoint() const { return mPhase1DarkBowserHitPoint; }

/**
 * @brief Store Fury Bowser's phase-1 hit points.
 * @param hitPoint Remaining hit points.
 */
void SingleModeData::setPhase1DarkBowserHitPoint(int hitPoint) {
    mPhase1DarkBowserHitPoint = hitPoint;
}

/**
 * @brief Read Fury Bowser's phase-2 hit points.
 * @return The stored hit points.
 */
int SingleModeData::getPhase2DarkBowserHitPoint() const { return mPhase2DarkBowserHitPoint; }

/**
 * @brief Store Fury Bowser's phase-2 hit points.
 * @param hitPoint Remaining hit points.
 */
void SingleModeData::setPhase2DarkBowserHitPoint(int hitPoint) {
    mPhase2DarkBowserHitPoint = hitPoint;
}

/**
 * @brief Read Fury Bowser's phase-3 hit points.
 * @return The stored hit points.
 */
int SingleModeData::getPhase3DarkBowserHitPoint() const { return mPhase3DarkBowserHitPoint; }

/**
 * @brief Read Fury Bowser's phase-4 hit points.
 * @return The stored hit points.
 */
int SingleModeData::getPhase4DarkBowserHitPoint() const { return mPhase4DarkBowserHitPoint; }

/**
 * @brief Read Fury Bowser's hit points for the final phase-4 battle.
 * @return The stored hit points.
 */
int SingleModeData::getPhase4DarkBowserHitPointFinal() const {
    return mPhase4DarkBowserHitPointFinal;
}

/**
 * @brief Store Fury Bowser's hit points for the final phase-4 battle.
 * @param hitPoint Remaining hit points.
 */
void SingleModeData::setPhase4DarkBowserHitPointFinal(int hitPoint) {
    mPhase4DarkBowserHitPointFinal = hitPoint;
}

/**
 * @brief Store Fury Bowser's phase-3 hit points before the battle.
 * @param hitPoint Remaining hit points.
 */
void SingleModeData::setPhase3DarkBowserHitPointPreBattle(int hitPoint) {
    mPhase3DarkBowserHitPointPreBattle = hitPoint;
}

/**
 * @brief Store Fury Bowser's phase-4 hit points before the battle.
 * @param hitPoint Remaining hit points.
 */
void SingleModeData::setPhase4DarkBowserHitPointPreBattle(int hitPoint) {
    mPhase4DarkBowserHitPointPreBattle = hitPoint;
}

/**
 * @brief Check whether a phase or demo is being reached for the first time.
 * @return True until the corresponding flag is cleared.
 */
bool SingleModeData::isNewToPhase1() { return mIsNewToPhase1; }

/**
 * @brief Set whether a phase or demo is being reached for the first time.
 * @param isNew True to show first-time content.
 */
void SingleModeData::setIsNewToPhase1(bool isNew) { mIsNewToPhase1 = isNew; }

/**
 * @brief Check whether a phase or demo is being reached for the first time.
 * @return True until the corresponding flag is cleared.
 */
bool SingleModeData::isNewToPhase2() { return mIsNewToPhase2; }

/**
 * @brief Set whether a phase or demo is being reached for the first time.
 * @param isNew True to show first-time content.
 */
void SingleModeData::setIsNewToPhase2(bool isNew) { mIsNewToPhase2 = isNew; }

/**
 * @brief Check whether a phase or demo is being reached for the first time.
 * @return True until the corresponding flag is cleared.
 */
bool SingleModeData::isNewToPhase3() { return mIsNewToPhase3; }

/**
 * @brief Set whether a phase or demo is being reached for the first time.
 * @param isNew True to show first-time content.
 */
void SingleModeData::setIsNewToPhase3(bool isNew) { mIsNewToPhase3 = isNew; }

/**
 * @brief Check whether a phase or demo is being reached for the first time.
 * @return True until the corresponding flag is cleared.
 */
bool SingleModeData::isNewToPhase4() { return mIsNewToPhase4; }

/**
 * @brief Set whether a phase or demo is being reached for the first time.
 * @param isNew True to show first-time content.
 */
void SingleModeData::setIsNewToPhase4(bool isNew) { mIsNewToPhase4 = isNew; }

/**
 * @brief Check whether a phase or demo is being reached for the first time.
 * @return True until the corresponding flag is cleared.
 */
bool SingleModeData::isNewToPhase2Boss() { return mIsNewToPhase2Boss; }

/**
 * @brief Set whether a phase or demo is being reached for the first time.
 * @param isNew True to show first-time content.
 */
void SingleModeData::setIsNewToPhase2Boss(bool isNew) { mIsNewToPhase2Boss = isNew; }

/**
 * @brief Check whether a phase or demo is being reached for the first time.
 * @return True until the corresponding flag is cleared.
 */
bool SingleModeData::isNewToPhase3Boss() { return mIsNewToPhase3Boss; }

/**
 * @brief Set whether a phase or demo is being reached for the first time.
 * @param isNew True to show first-time content.
 */
void SingleModeData::setIsNewToPhase3Boss(bool isNew) { mIsNewToPhase3Boss = isNew; }

/**
 * @brief Check whether a phase or demo is being reached for the first time.
 * @return True until the corresponding flag is cleared.
 */
bool SingleModeData::isNewToPhase1BowserIntro() { return mIsNewToPhase1BowserIntro; }

/**
 * @brief Set whether a phase or demo is being reached for the first time.
 * @param isNew True to show first-time content.
 */
void SingleModeData::setIsNewToPhase1BowserIntro(bool isNew) { mIsNewToPhase1BowserIntro = isNew; }

/**
 * @brief Check whether a phase or demo is being reached for the first time.
 * @return True until the corresponding flag is cleared.
 */
bool SingleModeData::isNewToPhase2BowserIntro() { return mIsNewToPhase2BowserIntro; }

/**
 * @brief Set whether a phase or demo is being reached for the first time.
 * @param isNew True to show first-time content.
 */
void SingleModeData::setIsNewToPhase2BowserIntro(bool isNew) { mIsNewToPhase2BowserIntro = isNew; }

/**
 * @brief Check whether a phase or demo is being reached for the first time.
 * @return True until the corresponding flag is cleared.
 */
bool SingleModeData::isNewToPhase3BowserIntro() { return mIsNewToPhase3BowserIntro; }

/**
 * @brief Set whether a phase or demo is being reached for the first time.
 * @param isNew True to show first-time content.
 */
void SingleModeData::setIsNewToPhase3BowserIntro(bool isNew) { mIsNewToPhase3BowserIntro = isNew; }

/**
 * @brief Check whether a phase or demo is being reached for the first time.
 * @return True until the corresponding flag is cleared.
 */
bool SingleModeData::isNewToPhase1BowserExit() { return mIsNewToPhase1BowserExit; }

/**
 * @brief Check whether a phase or demo is being reached for the first time.
 * @return True until the corresponding flag is cleared.
 */
bool SingleModeData::isNewToPhase2BowserExit() { return mIsNewToPhase2BowserExit; }

/**
 * @brief Check whether a phase or demo is being reached for the first time.
 * @return True until the corresponding flag is cleared.
 */
bool SingleModeData::isNewToPhase3BowserExit() { return mIsNewToPhase3BowserExit; }

/**
 * @brief Set whether a phase or demo is being reached for the first time.
 * @param isNew True to show first-time content.
 */
void SingleModeData::setIsNewToPhase1BowserExit(bool isNew) { mIsNewToPhase1BowserExit = isNew; }

/**
 * @brief Set whether a phase or demo is being reached for the first time.
 * @param isNew True to show first-time content.
 */
void SingleModeData::setIsNewToPhase2BowserExit(bool isNew) { mIsNewToPhase2BowserExit = isNew; }

/**
 * @brief Set whether a phase or demo is being reached for the first time.
 * @param isNew True to show first-time content.
 */
void SingleModeData::setIsNewToPhase3BowserExit(bool isNew) { mIsNewToPhase3BowserExit = isNew; }

/**
 * @brief Check whether the next transition fades to white.
 * @return True when a white fade is requested.
 */
bool SingleModeData::shouldFadeToWhite() { return mShouldFadeToWhite; }

/**
 * @brief Request or cancel a white fade for the next transition.
 * @param isFade True to fade to white.
 */
void SingleModeData::setShouldFadeToWhite(bool isFade) { mShouldFadeToWhite = isFade; }

/**
 * @brief Read the Giga Bell respawn point.
 * @param pTrans Receives the respawn position.
 * @param pFront Receives the respawn facing direction.
 * @return True when a respawn point is stored.
 */
bool SingleModeData::tryGetGigaBellPlayerRespawnPoint(sead::Vector3f* pTrans,
                                                      sead::Vector3f* pFront) const {
    if (!mIsGigaBellRespawnValid) {
        return false;
    }

    *pTrans = mGigaBellRespawnTrans;
    *pFront = mGigaBellRespawnFront;
    return true;
}

/**
 * @brief Store the Giga Bell respawn point.
 * @param rTrans Respawn position.
 * @param rFront Respawn facing direction.
 */
void SingleModeData::setGigaBellPlayerRespawnPoint(const sead::Vector3f& rTrans,
                                                   const sead::Vector3f& rFront) {
    mIsGigaBellRespawnValid = true;
    mGigaBellRespawnTrans = rTrans;
    mGigaBellRespawnFront = rFront;
}

/**
 * @brief Read the generic respawn point.
 * @param pTrans Receives the respawn position.
 * @param pFront Receives the respawn facing direction.
 * @return True when a respawn point is stored.
 */
bool SingleModeData::tryGetGenericPlayerRespawn(sead::Vector3f* pTrans,
                                                sead::Vector3f* pFront) const {
    if (!mIsGenericRespawnValid) {
        return false;
    }

    *pTrans = mGenericRespawnTrans;
    *pFront = mGenericRespawnFront;
    return true;
}

/**
 * @brief Forget the generic respawn point.
 */
void SingleModeData::clearGenericPlayerRespawn() {
    mIsGenericRespawnValid = false;
    mGenericRespawnTrans.set(0.0f, 0.0f, 0.0f);
    mGenericRespawnFront = sead::Vector3f::ez;
}

/**
 * @brief Store the generic respawn point and forget the other saved positions.
 * @param rTrans Respawn position.
 * @param rFront Respawn facing direction.
 */
void SingleModeData::setGenericPlayerRespawn(const sead::Vector3f& rTrans,
                                             const sead::Vector3f& rFront) {
    mGenericRespawnFront = rFront;
    mGenericRespawnTrans = rTrans;
    mIsGenericRespawnValid = true;
    mIslandCheckpoint = -1;
    mCheckpoint.reset();
    mGoalItemCheckpoint.reset();
}

/**
 * @brief Record a destroyed disaster block.
 * @param blockId Identifier of the destroyed block.
 */
void SingleModeData::destroyDisasterBlock(int blockId) { addBlockToList(&mDisasterBlocks, blockId); }

/**
 * @brief Check whether a disaster block was destroyed.
 * @param blockId Identifier of the block.
 * @return True when the block was destroyed.
 */
bool SingleModeData::isDisasterBlockDestroyed(int blockId) {
    for (sead::Buffer<s32>::iterator it = mDisasterBlocks.begin(); it != mDisasterBlocks.end();
         ++it) {
        if (*it == blockId) {
            return true;
        }

        if (*it == 0) {
            return false;
        }
    }

    return false;
}

/**
 * @brief Forget every destroyed disaster block.
 */
void SingleModeData::resetDisasterBlocks() { mDisasterBlocks.fill(0); }

/**
 * @brief Record a destroyed hard block.
 * @param blockId Identifier of the destroyed block.
 */
void SingleModeData::destroyBlockHard(int blockId) { addBlockToList(&mBlockHards, blockId); }

/**
 * @brief Check whether a hard block was destroyed.
 * @param blockId Identifier of the block.
 * @return True when the block was destroyed.
 */
bool SingleModeData::isBlockHardDestroyed(int blockId) {
    for (sead::Buffer<s32>::iterator it = mBlockHards.begin(); it != mBlockHards.end(); ++it) {
        if (*it == blockId) {
            return true;
        }

        if (*it == 0) {
            return false;
        }
    }

    return false;
}

/**
 * @brief Forget every destroyed hard block.
 */
void SingleModeData::resetBlockHards() { mBlockHards.fill(0); }

/**
 * @brief Mark a guide message as seen.
 * @param messageId Message bit index.
 * @param isFirstSet True for the first message set, false for the second.
 */
void SingleModeData::setGuideMessageSeen(u32 messageId, bool isFirstSet) {
    const u32 mask = 1 << messageId;
    u32& rFlags = isFirstSet ? mGuideMessageSeenFlags[0] : mGuideMessageSeenFlags[1];
    rFlags |= mask;
}

/**
 * @brief Mark a guide message as not seen.
 * @param messageId Message bit index.
 * @param isFirstSet True for the first message set, false for the second.
 */
void SingleModeData::resetGuideMessageSeen(u32 messageId, bool isFirstSet) {
    const u32 mask = 1 << messageId;
    u32& rFlags = isFirstSet ? mGuideMessageSeenFlags[0] : mGuideMessageSeenFlags[1];
    rFlags &= ~mask;
}

/**
 * @brief Check whether a guide message was seen.
 * @param messageId Message bit index.
 * @param isFirstSet True for the first message set, false for the second.
 * @return True when the message was seen.
 */
bool SingleModeData::isGuideMessageAlreadySeen(u32 messageId, bool isFirstSet) const {
    const u32 mask = 1 << messageId;
    const u32& rFlags = isFirstSet ? mGuideMessageSeenFlags[0] : mGuideMessageSeenFlags[1];
    return (rFlags & mask) != 0;
}

/**
 * @brief Set whether horizontal camera control is inverted.
 * @param pHolder Game-data holder (unused).
 * @param isReverse True to invert the camera.
 */
void SingleModeData::setCameraReverseHorizontal(GameDataHolder* pHolder, bool isReverse) {
    sOptions.mIsCameraReverseHorizontal = isReverse;
}

/**
 * @brief Set whether vertical camera control is inverted.
 * @param pHolder Game-data holder (unused).
 * @param isReverse True to invert the camera.
 */
void SingleModeData::setCameraReverseVertical(GameDataHolder* pHolder, bool isReverse) {
    sOptions.mIsCameraReverseVertical = isReverse;
}

/**
 * @brief Set the camera sensitivity.
 * @param pHolder Game-data holder (unused).
 * @param sensitivity Camera sensitivity level.
 */
void SingleModeData::setCameraSensitivity(GameDataHolder* pHolder, s8 sensitivity) {
    sOptions.mCameraSensitivity = sensitivity;
}

/**
 * @brief Set the assist-mode type.
 * @param pHolder Game-data holder (unused).
 * @param type Assist-mode type.
 */
void SingleModeData::setAssistModeType(GameDataHolder* pHolder, u8 type) {
    sOptions.mAssistModeType = type;
}

/**
 * @brief Read the saved state of a cat target.
 * @param id Identifier of the cat target.
 * @param pTarget Receives the saved state, or nullptr to only check for it.
 * @return True when a saved state exists.
 */
bool SingleModeData::tryGetNekoSaveData(int id, neko::Target* pTarget) const {
    for (s32 i = 0; i < cNekoSaveDataNum; i++) {
        const NekoSaveData data = mpNekoSaveData[i];
        if (data.mId == id) {
            if (pTarget != nullptr) {
                pTarget->mTrans = data.mTrans;
                pTarget->mIndex = data.mIndex;
                pTarget->mParentId = data.mParentId;
            }

            return true;
        }

        if (data.mId == 0) {
            return false;
        }
    }

    return false;
}

/**
 * @brief Restore the saved cat targets of a parent.
 * @param parentId Identifier of the parent.
 * @param pTargets Targets of the parent, indexed by their saved index.
 * @return The number of restored targets.
 */
int SingleModeData::tryGetNekoSaveDataByParentID(int parentId,
                                                 sead::PtrArray<neko::Target>* pTargets) const {
    s32 count = 0;
    for (s32 i = 0; i < cNekoSaveDataNum; i++) {
        const NekoSaveData data = mpNekoSaveData[i];
        if (pTargets == nullptr || data.mParentId != parentId) {
            continue;
        }

        if (data.mIndex >= pTargets->capacity()) {
            continue;
        }

        neko::Target* pTarget = pTargets->at(data.mIndex);
        if (pTarget == nullptr) {
            continue;
        }

        pTarget->mId = data.mId;
        pTarget->mIsActive = false;
        count++;
    }

    return count;
}

/**
 * @brief Save the state of a cat target in the first free slot.
 * @param id Identifier of the cat target.
 * @param pTarget Target to save.
 */
void SingleModeData::setNekoSaveData(int id, const neko::Target* pTarget) {
    for (s32 i = 0; i < cNekoSaveDataNum; i++) {
        NekoSaveData& rData = mpNekoSaveData[i];
        if (rData.mId == 0) {
            rData.set(id, pTarget->mTrans, pTarget->mParentId, *pTarget->tryGetHostTrans(),
                      pTarget->mIndex);
            return;
        }
    }
}

/**
 * @brief Check whether the demo of a cat parent was seen.
 * @param parentId Identifier of the parent.
 * @return True when the demo was seen.
 */
bool SingleModeData::hasNekoParentSeenDemo(int parentId) const {
    for (s32 i = 0; i < cNekoParentDataNum; i++) {
        const NekoParentData data = mpNekoParentData[i];
        if (data.mParentId == parentId && data.mIsSeenDemo) {
            return true;
        }
    }

    return false;
}

/**
 * @brief Mark the demo of a cat parent as seen in the first free slot.
 * @param parentId Identifier of the parent.
 */
void SingleModeData::setNekoParentSeenDemo(int parentId) {
    for (s32 i = 0; i < cNekoParentDataNum; i++) {
        NekoParentData& rData = mpNekoParentData[i];
        if (rData.mParentId == 0) {
            rData.mParentId = parentId;
            rData.mIsSeenDemo = true;
            return;
        }
    }
}

/**
 * @brief Spend coins.
 * @param count Number of coins to remove.
 */
void SingleModeData::subtractCoin(int count) { mCoinCount -= count; }

/**
 * @brief Check whether a playable character is unlocked.
 * @param charaType Character type; type 0 is always unlocked.
 * @return True when the character is unlocked.
 */
bool SingleModeData::hasUnlockedChar(int charaType) const {
    if (static_cast<u32>(charaType - 1) <= 3) {
        return hasFlag(1 << (charaType + 1));
    }

    return charaType == 0;
}

/**
 * @brief Check whether any extra character is unlocked.
 * @return True when an extra character is unlocked.
 */
bool SingleModeData::hasUnlockedAnyChar() const { return hasFlag(0x1e); }

/**
 * @brief Unlock a playable character.
 * @param charaType Character type from 1 through 4; other types are ignored.
 */
void SingleModeData::setUnlockedChar(int charaType) {
    if (static_cast<u32>(charaType - 1) <= 3) {
        mFlags |= 1 << (charaType + 1);
    }
}

/**
 * @brief Advance the main scenario to the next incomplete main scenario.
 */
void SingleModeData::updateMainScenario() {
    s32 scenario = mMainScenario.mScenarioIndex;
    for (s32 island = mMainScenario.mIslandId; island < mpIslands->getNum(); island++) {
        IslandSaveData* pSaveData = mpIslands->getIslandSaveDataPtr(island);
        IslandDataList* pIslandList = mpHolder->getIslandDataList();
        if (island >= pIslandList->getNum()) {
            return;
        }

        IslandData* pIsland = pIslandList->getIslandByIndex(island);
        const s32 scenarioNum =
            SingleModeDataFunction::getScenarioNum(GameDataHolderAccessor(mpHolder), island);
        for (; scenario < scenarioNum; scenario++) {
            if (pIsland->getScenarioDataByIndex(scenario)->isMainScenario() &&
                !pSaveData->isScenarioComplete(scenario)) {
                mMainScenario.mIslandId = island;
                mMainScenario.mScenarioIndex = scenario;
                pSaveData->setCurActiveScenario(scenario);
                return;
            }
        }

        scenario = 0;
    }
}

/**
 * @brief Access the saved progress of an ocean quadrant.
 * @param quadrant Quadrant index.
 * @return The quadrant's saved progress.
 */
const IslandSaveData* SingleModeData::getOceanQuadrantSaveData(int quadrant) {
    return mpOceanQuadrants->getIslandSaveData(quadrant);
}

/**
 * @brief Access the mutable saved progress of an ocean quadrant.
 * @param quadrant Quadrant index.
 * @return The quadrant's saved progress.
 */
IslandSaveData* SingleModeData::getOceanQuadrantSaveDataPtr(int quadrant) {
    return mpOceanQuadrants->getIslandSaveDataPtr(quadrant);
}

/**
 * @brief Complete a scenario and collect its goal item.
 * @param rInfo Island and scenario to complete.
 */
void SingleModeData::completeScenario(const ScenarioInfo& rInfo) {
    mGoalItemNum++;
    mNewFile = false;
    if (mGoalItemNum > 99) {
        mFlags |= 0x400;
        mPhase4DarkBowserHitPointFinal = mPhase4DarkBowserHitPoint;
        mPhase4DarkBowserHitPoint = 200;
        mPhase4DarkBowserHitPointPreBattle = 200;
    } else {
        mFlags &= ~0x400;
    }

    if (rInfo.mScenarioIndex < 0) {
        return;
    }

    if (rInfo.mIslandId < 0) {
        const s32 quadrant = IslandDataFunction::getQuadrantIndexFromIslandID(rInfo.mIslandId + 1);
        mpOceanQuadrants->getIslandSaveDataPtr(quadrant)->completeScenario(rInfo.mScenarioIndex);
        return;
    }

    mpIslands->getIslandSaveDataPtr(rInfo.mIslandId)->completeScenario(rInfo.mScenarioIndex);
    if (mMainScenario.mIslandId == rInfo.mIslandId && mMainScenario.mScenarioIndex == rInfo.mScenarioIndex) {
        updateMainScenario();
    }
}

/**
 * @brief Reset a completed scenario and give back its goal item.
 * @param rInfo Island and scenario to reset.
 */
void SingleModeData::resetScenario(const ScenarioInfo& rInfo) {
    mNewFile = false;
    if (rInfo.mIslandId < 0 || rInfo.mScenarioIndex < 0) {
        return;
    }

    if (mpIslands->getIslandSaveDataPtr(rInfo.mIslandId)->isScenarioComplete(rInfo.mScenarioIndex)) {
        mGoalItemNum--;
        mFlags &= ~0x400;
        getIslandSaveDataPtr(rInfo.mIslandId)->resetScenario(rInfo.mScenarioIndex);
    }

    if (mMainScenario.mIslandId == rInfo.mIslandId && mMainScenario.mScenarioIndex == rInfo.mScenarioIndex) {
        updateMainScenario();
    }
}

/**
 * @brief Rate the progress of the file for the file-select screen.
 * @return The number of stars to show, from 0 through 5.
 */
int SingleModeData::calcClearStarLevel() {
    const s32 goalItemNum = mGoalItemNum;
    const s32 phase = mUnlockedPhase;
    s32 level = 0;
    if (phase >= 3 && phase <= 4) {
        level = 1;
    } else if (phase >= 5 && phase <= 7) {
        level = 2;
    } else if (phase >= 8 && phase <= 10) {
        level = 3;
    }

    if (goalItemNum >= SingleModeDataFunction::getMaxCollectableGoalItems()) {
        level = 4;
    }

    if (isFileComplete()) {
        level = 5;
    }

    return level;
}

/**
 * @brief Load the file from a save stream.
 * @param pStream Stream to read from.
 * @return True when the file was read completely.
 */
bool SingleModeData::readFromStream(sead::ReadStream* pStream) {
    if (!GameDataFileBase::readFromStream(pStream)) {
        return false;
    }

    s32 size;
    pStream->readS32(size);
    SingleModeSaveData save;
    std::memset(&save, 0, sizeof(save));
    pStream->readMemBlock(&save, sizeof(save));
    mFlags = save.mFlags;
    mSeenCutsceneFlags = save.mSeenCutsceneFlags;
    mMainScenario = save.mMainScenario;
    mLastIsland = save.mLastIsland;
    mCurrentIsland = save.mCurrentIsland;
    mCheckpoint = save.mCheckpoint;
    mGoalItemCheckpoint = save.mGoalItemCheckpoint;
    mIslandCheckpoint = save.mIslandCheckpoint;
    mGoalItemNum = save.mGoalItemNum;
    mUnlockedPhase = save.mUnlockedPhase;
    mUnlockedIslandNum = save.mUnlockedIslandNum;
    mIsDisasterMode = save.mIsDisasterMode;
    mIsNewToPhase1 = save.mIsNewToPhase1;
    mIsNewToPhase2 = save.mIsNewToPhase2;
    mIsNewToPhase3 = save.mIsNewToPhase3;
    mIsNewToPhase4 = save.mIsNewToPhase4;
    mIsNewToPhase2Boss = save.mIsNewToPhase2Boss;
    mIsNewToPhase3Boss = save.mIsNewToPhase3Boss;
    mIsNewToPhase1BowserIntro = save.mIsNewToPhase1BowserIntro;
    mIsNewToPhase2BowserIntro = save.mIsNewToPhase2BowserIntro;
    mIsNewToPhase3BowserIntro = save.mIsNewToPhase3BowserIntro;
    mIsNewToPhase1BowserExit = save.mIsNewToPhase1BowserExit;
    mIsNewToPhase2BowserExit = save.mIsNewToPhase2BowserExit;
    mIsNewToPhase3BowserExit = save.mIsNewToPhase3BowserExit;
    mShouldFadeToWhite = save.mShouldFadeToWhite;
    mIsHardToSuperDisasterTransition = save.mIsHardToSuperDisasterTransition;
    mIsAutoForeshadow = save.mIsAutoForeshadow;
    mDisasterModeFrames = save.mDisasterModeFrames;
    mDisasterModePostBossPeaceFrames = save.mDisasterModePostBossPeaceFrames;
    mDisasterModeFlowIndex = save.mDisasterModeFlowIndex;
    mGigaBellLockCount = save.mGigaBellLockCount;
    mIsGigaBellUnlocked = save.mIsGigaBellUnlocked;
    mPhase1DarkBowserHitPoint = save.mPhase1DarkBowserHitPoint;
    mPhase2DarkBowserHitPoint = save.mPhase2DarkBowserHitPoint;
    mPhase3DarkBowserHitPoint = save.mPhase3DarkBowserHitPoint;
    mPhase4DarkBowserHitPoint = save.mPhase4DarkBowserHitPoint;
    mPhase3DarkBowserHitPointPreBattle = save.mPhase3DarkBowserHitPointPreBattle;
    mPhase4DarkBowserHitPointPreBattle = save.mPhase4DarkBowserHitPointPreBattle;
    mIsGigaBellRespawnValid = save.mIsGigaBellRespawnValid;
    mGigaBellRespawnTrans = save.mGigaBellRespawnTrans;
    mGigaBellRespawnFront = save.mGigaBellRespawnFront;
    mIsGenericRespawnValid = save.mIsGenericRespawnValid;
    mGenericRespawnTrans = save.mGenericRespawnTrans;
    mGenericRespawnFront = save.mGenericRespawnFront;
    mGuideMessageSeenFlags[0] = save.mGuideMessageSeenFlags;
    mDeathCount = save.mDeathCount;
    mSavedGenericItemFlags = save.mSavedGenericItemFlags;
    mPhaseTotalPlayTime = save.mPhaseTotalPlayTime;
    mIslandPlayTime = save.mIslandPlayTime;

    for (s32 i = 0; i < cLuckyIslandPosNum; i++) {
        mLuckyIslandPos[i] = save.mLuckyIslandPos[i];
    }

    for (s32 i = 0; i < 6; i++) {
        mpStockItems->setStockItemCountByIndex(i, save.mStockItemCounts[i]);
    }

    for (s32 i = 0; i < mDisasterBlocks.size(); i++) {
        mDisasterBlocks[i] = save.mDisasterBlocks[i];
    }

    copyNekoSaveData(save.mNekoSaveData, mpNekoSaveData);
    copyNekoParentData(save.mNekoParentData, mpNekoParentData);

    for (s32 i = 0; i < mBlockHards.size(); i++) {
        mBlockHards[i] = save.mBlockHards[i];
    }

    if (!mpIslands->readFromStream(pStream)) {
        return false;
    }

    if (!mpOceanQuadrants->readFromStream(pStream)) {
        return false;
    }

    onSave();
    return true;
}

/**
 * @brief Write the file to a save stream.
 * @param pStream Stream to write to.
 * @param isSkip True to only advance the stream past the progress block.
 */
void SingleModeData::writeToStream(sead::WriteStream* pStream, bool isSkip) const {
    GameDataFileBase::writeToStream(pStream, isSkip);
    pStream->writeS32(sizeof(SingleModeSaveData));
    if (isSkip) {
        pStream->skip(sizeof(SingleModeSaveData));
    } else {
        SingleModeSaveData save;
        std::memset(&save, 0, sizeof(save));
        save.mFlags = mFlags;
        save.mSeenCutsceneFlags = mSeenCutsceneFlags;
        save.mMainScenario = mMainScenario;
        save.mLastIsland = mLastIsland;
        save.mCurrentIsland = mCurrentIsland;
        save.mCheckpoint = mCheckpoint;
        save.mGoalItemCheckpoint = mGoalItemCheckpoint;
        save.mIslandCheckpoint = mIslandCheckpoint;
        save.mGoalItemNum = mGoalItemNum;
        save.mUnlockedPhase = mUnlockedPhase;
        save.mUnlockedIslandNum = mUnlockedIslandNum;
        save.mIsDisasterMode = mIsDisasterMode;
        save.mIsNewToPhase1 = mIsNewToPhase1;
        save.mIsNewToPhase2 = mIsNewToPhase2;
        save.mIsNewToPhase3 = mIsNewToPhase3;
        save.mIsNewToPhase4 = mIsNewToPhase4;
        save.mIsNewToPhase2Boss = mIsNewToPhase2Boss;
        save.mIsNewToPhase3Boss = mIsNewToPhase3Boss;
        save.mIsNewToPhase1BowserIntro = mIsNewToPhase1BowserIntro;
        save.mIsNewToPhase2BowserIntro = mIsNewToPhase2BowserIntro;
        save.mIsNewToPhase3BowserIntro = mIsNewToPhase3BowserIntro;
        save.mIsNewToPhase1BowserExit = mIsNewToPhase1BowserExit;
        save.mIsNewToPhase2BowserExit = mIsNewToPhase2BowserExit;
        save.mIsNewToPhase3BowserExit = mIsNewToPhase3BowserExit;
        save.mShouldFadeToWhite = mShouldFadeToWhite;
        save.mIsHardToSuperDisasterTransition = mIsHardToSuperDisasterTransition;
        save.mIsAutoForeshadow = mIsAutoForeshadow;
        save.mDisasterModePostBossPeaceFrames = mDisasterModePostBossPeaceFrames;
        save.mDisasterModeFlowIndex = mDisasterModeFlowIndex;
        save.mDisasterModeFrames = mDisasterModeFrames;
        save.mGigaBellLockCount = mGigaBellLockCount;
        save.mIsGigaBellUnlocked = mIsGigaBellUnlocked;
        save.mPhase1DarkBowserHitPoint = mPhase1DarkBowserHitPoint;
        save.mPhase2DarkBowserHitPoint = mPhase2DarkBowserHitPoint;
        save.mPhase3DarkBowserHitPoint = mPhase3DarkBowserHitPoint;
        save.mPhase4DarkBowserHitPoint = mPhase4DarkBowserHitPoint;
        save.mPhase3DarkBowserHitPointPreBattle = mPhase3DarkBowserHitPointPreBattle;
        save.mPhase4DarkBowserHitPointPreBattle = mPhase4DarkBowserHitPointPreBattle;
        save.mIsGigaBellRespawnValid = mIsGigaBellRespawnValid;
        save.mGigaBellRespawnTrans = mGigaBellRespawnTrans;
        save.mGigaBellRespawnFront = mGigaBellRespawnFront;
        save.mIsGenericRespawnValid = mIsGenericRespawnValid;
        save.mGenericRespawnTrans = mGenericRespawnTrans;
        save.mGenericRespawnFront = mGenericRespawnFront;
        save.mGuideMessageSeenFlags = mGuideMessageSeenFlags[0];
        save.mDeathCount = mDeathCount;
        save.mSavedGenericItemFlags = mSavedGenericItemFlags;
        save.mPhaseTotalPlayTime = mPhaseTotalPlayTime;
        save.mIslandPlayTime = mIslandPlayTime;

        for (s32 i = 0; i < cLuckyIslandPosNum; i++) {
            save.mLuckyIslandPos[i] = mLuckyIslandPos[i];
        }

        for (s32 i = 0; i < 6; i++) {
            save.mStockItemCounts[i] = mpStockItems->getStockItemCountByIndex(i);
        }

        for (s32 i = 0; i < mDisasterBlocks.size(); i++) {
            save.mDisasterBlocks[i] = mDisasterBlocks[i];
        }

        copyNekoSaveData(mpNekoSaveData, save.mNekoSaveData);
        copyNekoParentData(mpNekoParentData, save.mNekoParentData);

        for (s32 i = 0; i < mBlockHards.size(); i++) {
            save.mBlockHards[i] = mBlockHards[i];
        }

        pStream->writeMemBlock(&save, sizeof(save));
    }

    mpIslands->writeToStream(pStream, isSkip);
    mpOceanQuadrants->writeToStream(pStream, isSkip);
}

/**
 * @brief Record a completed lucky-island position in the first free slot.
 * @param posIndex Index of the completed position.
 */
void SingleModeData::setLuckyIslandPosCompleted(int posIndex) {
    for (s32 i = 0; i < cLuckyIslandPosNum; i++) {
        if (mLuckyIslandPos[i] == -1) {
            mLuckyIslandPos[i] = posIndex;
            return;
        }
    }
}

/**
 * @brief Check whether a lucky-island position was completed.
 * @param posIndex Index of the position.
 * @return True when the position was completed.
 */
bool SingleModeData::wasLuckyIslandPosCompleted(int posIndex) {
    for (s32 i = 0; i < cLuckyIslandPosNum; i++) {
        if (mLuckyIslandPos[i] == posIndex) {
            return true;
        }
    }

    return false;
}

/**
 * @brief Find the lucky goal item collected at a lucky-island position.
 * @param posIndex Index of the position.
 * @return Index of the lucky goal item, or -1 when the position was not completed.
 */
int SingleModeData::getLuckyShineIdxByPosIdx(int posIndex) {
    for (s32 i = 0; i < cLuckyIslandPosNum; i++) {
        if (mLuckyIslandPos[i] == posIndex) {
            return i;
        }
    }

    return -1;
}
