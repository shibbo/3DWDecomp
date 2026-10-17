#pragma once

#include "System/GameDataHolderWriter.hpp"
#include <container/seadPtrArray.h>
#include <math/seadVector.h>
#include <preport/PlayReportManager.h>
#include <prim/seadEnum.h>
#include <prim/seadSafeString.h>

namespace al {
class ActorInitInfo;
class IUseSceneObjHolder;
class LiveActor;
} // namespace al

namespace neko {
class Target;
}

class ScenarioInfo;
class SingleModeScene;

class SingleModeDataFunction {
  public:
    /**
     * @brief How the disaster-mode state is recorded at a checkpoint.
     */
    enum DisasterForceSetting {
        DisasterForceSetting_Off,
        DisasterForceSetting_On,
        DisasterForceSetting_Controller,
        DisasterForceSetting_Calm,
    };

    static bool isActorInPlessieChaseDisabled(const al::ActorInitInfo& rInfo);
    static int getUnlockedPhase(GameDataHolderAccessor accessor);
    static bool isValidPlacement(int layerId, int phase, bool isPlessieChase);
    static bool isValidPlacement(GameDataHolderAccessor accessor, const al::ActorInitInfo& rInfo,
                                 bool isPlessieChase);
    static bool isValidPlessieChasePlacement(GameDataHolderAccessor accessor,
                                             const al::ActorInitInfo& rInfo);
    static bool isValidPlessieChasePlacement(int layerId, int phase);
    static void setDemoWasCancelled(GameDataHolderWriter writer, bool isCancelled);
    static bool isDemoWasCancelled(GameDataHolderAccessor accessor);
    static void setSaveRequested(GameDataHolderWriter writer, bool isRequested);
    static bool isSaveRequested(GameDataHolderAccessor accessor);
    static bool isPhase0(GameDataHolderAccessor accessor);
    static bool isIslandScenarioIDComplete(const al::LiveActor* pActor,
                                           const al::ActorInitInfo& rInfo);
    static bool isScenarioComplete(GameDataHolderAccessor accessor, int islandId, int scenarioId);
    static bool isSuperHardModeOn(GameDataHolderAccessor accessor, int hitPoint);
    static void onStageStart(GameDataHolderWriter writer);
    static void onStageEnd(GameDataHolderWriter writer);
    static void restartStage(GameDataHolderWriter writer);
    static bool isSceneRestart(GameDataHolderWriter writer);
    static void resetSceneRestart(GameDataHolderWriter writer);
    static bool isIslandFirstVisit(GameDataHolderAccessor accessor, int islandId);
    static bool isIslandUnlocked(GameDataHolderAccessor accessor, int islandId);
    static void setIslandUnlocked(GameDataHolderWriter writer, int islandId);
    static int getUnlockedIslandNum(GameDataHolderAccessor accessor);
    static int getLastValidIslandVisited(GameDataHolderAccessor accessor);
    static void setLastValidIslandVisited(GameDataHolderWriter writer, int islandId);
    static int getCurValidIslandVisited(GameDataHolderAccessor accessor);
    static void setCurValidIslandVisited(GameDataHolderWriter writer, int islandId);
    static int getCurActiveScenarioIndex(GameDataHolderAccessor accessor, int islandId);
    static void setCurActiveScenarioIndex(GameDataHolderWriter writer, int islandId,
                                          int scenarioIndex);
    static void setCurActiveScenarioNameSeen(GameDataHolderWriter writer, int islandId);
    static bool wasActiveScenarioNameSeen(GameDataHolderAccessor accessor, int islandId);
    static void setCheckpointPass(GameDataHolderWriter writer, int islandId, int checkpointId,
                                  const al::IUseSceneObjHolder* pUser);
    static void clearGigaBellPlayerRespawnPoint(GameDataHolderWriter writer);
    static void recordDisasterMode(GameDataHolderWriter writer, DisasterForceSetting setting,
                                   const al::IUseSceneObjHolder* pUser);
    static void getLastCheckpointPass(GameDataHolderAccessor accessor, int* pIslandId,
                                      int* pCheckpointId);
    static void setGoalItemCheckpointPass(GameDataHolderWriter writer, int islandId,
                                          int checkpointId);
    static void getLastGoalItemCheckpointPass(GameDataHolderAccessor accessor, int* pIslandId,
                                              int* pCheckpointId);
    static void setIslandCheckpointPass(GameDataHolderWriter writer, int islandId);
    static bool getLastIslandCheckpointPass(GameDataHolderAccessor accessor, int* pIslandId);
    static void clearLastIslandCheckpointPass(GameDataHolderWriter writer);
    static bool isAnySavedPosition(GameDataHolderWriter writer);
    static void setVandalizeIsland(GameDataHolderWriter writer, int islandId, int phase);
    static void clearVandalizeIsland(GameDataHolderWriter writer, int islandId, int phase);
    static bool isVandalizedIsland(GameDataHolderAccessor accessor, int islandId, int phase,
                                   bool* pActive);
    static void clearAllButGigaBellCheckpoint(GameDataHolderWriter writer);
    static void clearAllCheckpoints(GameDataHolderWriter writer);
    static int getScenarioType(GameDataHolderAccessor accessor, int islandId, int scenarioId);
    static bool allCloudScenariosComplete(GameDataHolderAccessor accessor, int quadrant);
    static void getFirstCloudScenarioByQuadrant(GameDataHolderAccessor accessor, int quadrant,
                                                ScenarioInfo* pInfo);
    static bool isLuckyShine(GameDataHolderAccessor accessor, int islandId, int scenarioId);
    static bool isNekoShine(GameDataHolderAccessor accessor, int islandId, int scenarioId);
    static bool allNekoShinesRemaining(const al::LiveActor* pActor);
    static bool allDisasterShinesRemaining(const al::LiveActor* pActor);
    static int getNumRemainingLuckyShines(GameDataHolderAccessor accessor);
    static void setLuckyIslandPosCompleted(GameDataHolderWriter writer, int posIndex);
    static bool wasLuckyIslandPosCompleted(GameDataHolderAccessor accessor, int posIndex);
    static int getLuckyShineIdxByPosIdx(GameDataHolderAccessor accessor, int posIndex);
    static int getQuadrantIslandFromScenario(GameDataHolderAccessor accessor, int scenarioIndex);
    static bool isOceanScenarioComplete(GameDataHolderAccessor accessor, int scenarioIndex);
    static int getFirstAvailableScenario(GameDataHolderAccessor accessor, int islandId);
    static bool allScenariosComplete(GameDataHolderAccessor accessor, int islandId);
    static void completeScenario(GameDataHolderWriter writer, const ScenarioInfo& rInfo);
    static bool isShardCollected(GameDataHolderAccessor accessor, int islandId, int shardIndex);
    static u8 getShardFlag(GameDataHolderAccessor accessor, int islandId);
    static u8* getShardFlagPtr(GameDataHolderAccessor accessor, int islandId);
    static void setLastVisitedLighthouseIDForGuideWindow(int lighthouseId);
    static int getLastVisitedLighthouseIDForGuideWindow();
    static u64 getScenarioFlag(GameDataHolderAccessor accessor, int islandId);
    static void collectShard(GameDataHolderWriter writer, int islandId, int shardIndex);
    static void resetScenario(GameDataHolderWriter writer, const ScenarioInfo& rInfo);
    static int getIslandActiveScenario(GameDataHolderAccessor accessor, int islandId);
    static int calcNumCompletedScenarios(GameDataHolderAccessor accessor, int islandId);
    static int getGoalItemsCollected(GameDataHolderAccessor accessor);
    static int getMaxCollectableGoalItems();
    static void incDeathCount(GameDataHolderWriter writer);
    static void reportCommonPlayReportItems(GameDataHolderWriter writer,
                                            const al::IUseSceneObjHolder* pUser,
                                            bool isIncludeCurrentPlay, bool isGoalItemGet);
    static void setPlayReportData(GameDataHolderWriter writer, preport::Key key, int value);
    static void setPlayReportData(GameDataHolderWriter writer, preport::Key key,
                                  sead::SafeString& rValue);
    static void setPlayReportData(GameDataHolderWriter writer, preport::Key key, s64 value);
    static bool reportShineEvent(GameDataHolderWriter writer, const al::IUseSceneObjHolder* pUser,
                                 int shineIndex);
    static void reportIslandEvent(GameDataHolderWriter writer,
                                  const al::IUseSceneObjHolder* pUser, int eventValue,
                                  int islandId);
    static int getIslandPlayTime(GameDataHolderWriter writer);
    static void reportPhaseClearEvent(GameDataHolderWriter writer, al::IUseSceneObjHolder* pUser,
                                      int phase);
    static bool beginPlayReport(GameDataHolderWriter writer, const al::IUseSceneObjHolder* pUser,
                                preport::KeyEventType type, int eventId, int option);
    static bool getIs2PAssistMode(GameDataHolderAccessor accessor);
    static void endPlayReport(GameDataHolderWriter writer);
    static void setPlayReportData(GameDataHolderWriter writer, preport::Key key, float value);
    static void setPlayReportData(GameDataHolderWriter writer, preport::Key key, int* pValues,
                                  int num);
    static void setPlayReportData(GameDataHolderWriter writer, preport::Key key, float* pValues,
                                  int num);
    static void resetBossPlayTime(GameDataHolderWriter writer);
    static void setBossPlayTime(GameDataHolderWriter writer);
    static s32 getBossPlayTime(GameDataHolderWriter writer);
    static void resetIslandPlayTime(GameDataHolderWriter writer);
    static void setIslandPlayTime(GameDataHolderWriter writer);
    static void setPhasePlayTime(GameDataHolderWriter writer);
    static void sendNetworkStatus(GameDataHolderWriter writer);
    static void calculateAllShineCollected(GameDataHolderAccessor accessor);
    static bool isAllShineCollected(GameDataHolderAccessor accessor);
    static bool isCompleteEndingPictureSeen(GameDataHolderAccessor accessor);
    static bool isFileComplete(GameDataHolderAccessor accessor);
    static void setCompleteEndingPictureSeen(GameDataHolderWriter writer);
    static bool isDarkBowserV2Available(GameDataHolderAccessor accessor);
    static bool isDarkBowserV2Defeated(GameDataHolderAccessor accessor);
    static void setDarkBowserV2Defeated(GameDataHolderWriter writer);
    static bool isMeowserJrAvailable(GameDataHolderAccessor accessor);
    static void setPhaseEnd(GameDataHolderWriter writer, bool isEnd);
    static bool isPhaseEnd(GameDataHolderAccessor accessor);
    static bool hasSeenEnding(GameDataHolderAccessor accessor);
    static void setHasSeenEnding(GameDataHolderWriter writer, bool isReset);
    static void setPhase4DarkBowserHitPoint(GameDataHolderAccessor accessor, int hitPoint);
    static bool isFirstPhase0(GameDataHolderAccessor accessor);
    static void setIsFirstPhase0(GameDataHolderWriter writer);
    static bool isFirstPhase2BossDefeated(GameDataHolderAccessor accessor);
    static void setFirstPhase2BossDefeated(GameDataHolderWriter writer);
    static bool isFirstPhase3BossDefeated(GameDataHolderAccessor accessor);
    static void setFirstPhase3BossDefeated(GameDataHolderWriter writer);
    static bool isPhase4BossDefeated(GameDataHolderAccessor accessor);
    static void setPhase4BossDefeated(GameDataHolderWriter writer);
    static bool isAlreadyPlayRidon(GameDataHolderAccessor accessor);
    static void setPlayRidon(GameDataHolderWriter writer);
    static void resetPlayRidon(GameDataHolderWriter writer);
    static bool hasSeenCutscene(GameDataHolderAccessor accessor, int cutsceneId);
    static void setHasSeenCutscene(GameDataHolderWriter writer, int cutsceneId);
    static void clearHasSeenCutscene(GameDataHolderWriter writer, int cutsceneId);
    static bool hasUnlockedChar(GameDataHolderAccessor accessor, int charaType);
    static bool hasUnlockedAnyChar(GameDataHolderAccessor accessor);
    static void setUnlockedChar(GameDataHolderWriter writer, int charaType);
    static bool isGenericItemSaved(GameDataHolderAccessor accessor, int itemId);
    static void setGenericItemSaved(GameDataHolderWriter writer, int itemId);
    static bool getIsDisasterMode(GameDataHolderAccessor accessor);
    static bool isHardToSuperDisasterTransition(GameDataHolderAccessor accessor);
    static void setHardToSuperDisasterTransition(GameDataHolderWriter writer, bool isTransition);
    static bool isAutoForeshadow(GameDataHolderAccessor accessor);
    static void setAutoForeshadow(GameDataHolderWriter writer, bool isAuto);
    static int getDisasterModeFrames(GameDataHolderAccessor accessor);
    static int getDisasterModePostBossPeaceFrames(GameDataHolderAccessor accessor);
    static void setDisasterModePostBossPeaceFrames(GameDataHolderAccessor accessor, int frames);
    static int getDisasterModeFlowIndex(GameDataHolderAccessor accessor);
    static void setDisasterModeFlowIndex(GameDataHolderAccessor accessor, int index);
    static int getGigaBellLockCount(GameDataHolderWriter writer);
    static void setGigaBellLockCount(GameDataHolderWriter writer, int count);
    static bool isGigaBellUnlocked(GameDataHolderWriter writer);
    static void setGigaBellUnlocked(GameDataHolderWriter writer, bool isUnlocked);
    static int getScenarioNum(GameDataHolderAccessor accessor, int islandId);
    static void setUnlockedPhase(GameDataHolderWriter writer, int phase);
    static int getPhase1DarkBowserHitPoint(GameDataHolderAccessor accessor);
    static void setPhase1DarkBowserHitPoint(GameDataHolderAccessor accessor, int hitPoint);
    static int getPhase2DarkBowserHitPoint(GameDataHolderAccessor accessor);
    static void setPhase2DarkBowserHitPoint(GameDataHolderAccessor accessor, int hitPoint);
    static int getPhase3DarkBowserHitPoint(GameDataHolderAccessor accessor);
    static void setPhase3DarkBowserHitPoint(GameDataHolderAccessor accessor, int hitPoint);
    static int getPhase4DarkBowserHitPoint(GameDataHolderAccessor accessor);
    static int getPhase4DarkBowserHitPointFinal(GameDataHolderAccessor accessor);
    static void setPhase4DarkBowserHitPointFinal(GameDataHolderAccessor accessor, int hitPoint);
    static void setPhase3DarkBowserHitPointPreBoss(GameDataHolderWriter writer);
    static void setPhase4DarkBowserHitPointPreBoss(GameDataHolderWriter writer);
    static bool isNewToPhase1(GameDataHolderAccessor accessor);
    static void setIsNewToPhase1(GameDataHolderAccessor accessor, bool isNew);
    static bool isNewToPhase2(GameDataHolderAccessor accessor);
    static void setIsNewToPhase2(GameDataHolderAccessor accessor, bool isNew);
    static bool isNewToPhase3(GameDataHolderAccessor accessor);
    static void setIsNewToPhase3(GameDataHolderAccessor accessor, bool isNew);
    static bool isNewToPhase4(GameDataHolderAccessor accessor);
    static void setIsNewToPhase4(GameDataHolderAccessor accessor, bool isNew);
    static bool isNewToPhase2Boss(GameDataHolderAccessor accessor);
    static void setIsNewToPhase2Boss(GameDataHolderAccessor accessor, bool isNew);
    static bool isNewToPhase3Boss(GameDataHolderAccessor accessor);
    static void setIsNewToPhase3Boss(GameDataHolderAccessor accessor, bool isNew);
    static bool isNewToPhase1BowserIntro(GameDataHolderAccessor accessor);
    static void setIsNewToPhase1BowserIntro(GameDataHolderAccessor accessor, bool isNew);
    static bool isNewToPhase2BowserIntro(GameDataHolderAccessor accessor);
    static void setIsNewToPhase2BowserIntro(GameDataHolderAccessor accessor, bool isNew);
    static bool isNewToPhase3BowserIntro(GameDataHolderAccessor accessor);
    static void setIsNewToPhase3BowserIntro(GameDataHolderAccessor accessor, bool isNew);
    static bool isNewToPhase1BowserExit(GameDataHolderAccessor accessor);
    static void setIsNewToPhase1BowserExit(GameDataHolderAccessor accessor, bool isNew);
    static bool isNewToPhase2BowserExit(GameDataHolderAccessor accessor);
    static void setIsNewToPhase2BowserExit(GameDataHolderAccessor accessor, bool isNew);
    static bool isNewToPhase3BowserExit(GameDataHolderAccessor accessor);
    static void setIsNewToPhase3BowserExit(GameDataHolderAccessor accessor, bool isNew);
    static bool shouldFadeToWhite(GameDataHolderAccessor accessor);
    static void setShouldFadeToWhite(GameDataHolderAccessor accessor, bool isFade);
    static bool tryGetGigaBellPlayerRespawnPoint(GameDataHolderAccessor accessor,
                                                 sead::Vector3f* pTrans, sead::Vector3f* pFront);
    static bool isGigaBellPlayerRespawnPointValid(GameDataHolderAccessor accessor);
    static void setGigaBellPlayerRespawnPoint(GameDataHolderWriter writer,
                                              const sead::Vector3f& rTrans,
                                              const sead::Vector3f& rFront);
    static bool tryGetGenericPlayerRespawnPosition(GameDataHolderAccessor accessor,
                                                   sead::Vector3f* pTrans,
                                                   sead::Vector3f* pFront);
    static bool isGenericRespawnPlayerPositionValid(GameDataHolderAccessor accessor);
    static void clearGenericPlayerRespawnPosition(GameDataHolderWriter writer);
    static void setGenericPlayerRespawnPosition(GameDataHolderWriter writer,
                                                const sead::Vector3f& rTrans,
                                                const sead::Vector3f& rFront);
    static u32 getStockItemCount(GameDataHolderAccessor accessor, int itemType);
    static u32 getStockItemCountByIndex(GameDataHolderAccessor accessor, int index);
    static void useStockItem(GameDataHolderWriter writer, int itemType);
    static void setGuideMessageSeen(GameDataHolderWriter writer, u32 messageId, bool isFirstSet);
    static void resetGuideMessageSeen(GameDataHolderWriter writer, u32 messageId,
                                      bool isFirstSet);
    static bool isGuideMessageAlreadySeen(GameDataHolderAccessor accessor, u32 messageId,
                                          bool isFirstSet);
    static void destroyDisasterBlock(GameDataHolderWriter writer, int blockId);
    static bool isDisasterBlockDestroyed(GameDataHolderAccessor accessor, int blockId);
    static void destroyBlockHard(GameDataHolderWriter writer, int blockId);
    static bool isBlockHardDestroyed(GameDataHolderAccessor accessor, int blockId);
    static bool setCameraSensitivity(GameDataHolderWriter writer, int sensitivity);
    static s8 getCameraSensitiviy(GameDataHolderAccessor accessor);
    static bool setCameraReverseVertical(GameDataHolderWriter writer, bool isReverse);
    static bool getCameraReverseVertical(GameDataHolderWriter writer);
    static bool setCameraReverseHorizontal(GameDataHolderWriter writer, bool isReverse);
    static bool getCameraReverseHorizontal(GameDataHolderWriter writer);
    static void setMapEnabled(GameDataHolderWriter writer, bool isEnabled);
    static bool isMapEnabled(GameDataHolderAccessor accessor);
    static f32 getMapZoomRatio(GameDataHolderAccessor accessor);
    static void setMapZoomRatio(GameDataHolderWriter writer, f32 ratio);
    static bool setAssistModeType(GameDataHolderWriter writer, u8 type);
    static u8 getAssistModeType(GameDataHolderWriter writer);
    static void setIs2PAssistMode(GameDataHolderWriter writer, bool isAssist);
    static bool tryGetNekoSaveData(GameDataHolderAccessor accessor, int id, neko::Target* pTarget);
    static int tryGetNekoSaveDataByParentID(GameDataHolderAccessor accessor, int parentId,
                                            sead::PtrArray<neko::Target>* pTargets);
    static void setNekoSaveData(GameDataHolderWriter writer, int id, const neko::Target* pTarget);
    static bool hasNekoParentSeenDemo(GameDataHolderAccessor accessor, int parentId);
    static void setNekoParentSeenDemo(GameDataHolderWriter writer, int parentId);
};

namespace rc {

// clang-format off
SEAD_ENUM(SingleModePhases, PHASE0 , PHASE1 , PHASE1_BOSS , PHASE2 , PHASE2_BOSS , PHASE3 , PHASE3_BOSS , PHASE3_PLESSIE_CHASE , PHASE4 , PHASE4_BOSS , PHASE4_PLESSIE_CHASE)
// clang-format on

const char* getSingleModePhaseName(int phase);
int getNonBossPhase(int phase, bool isAfterPlessieChase);
int phaseNumToInt(SingleModePhases phase);
int phaseNumToInt(int phase);
int phaseNameToInt(const char* pName);
SingleModeScene* createPhaseScene(int phase);
bool isBossPhase(int phase);
int convertMapUnitFlagToCutscene(int flag);
int getPhaseIntroCutsceneFlag(int phase);
bool isPlessieChase(int phase);

} // namespace rc
