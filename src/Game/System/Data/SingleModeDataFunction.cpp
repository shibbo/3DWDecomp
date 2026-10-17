#include "System/Data/SingleModeDataFunction.hpp"
#include <attributes.h>
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Play/Placement/PlacementHolder.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "MapObj/DisasterModeController.hpp"
#include "MapObj/GoalItemHolder.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Scene/PhaseBossScene.hpp"
#include "Scene/PhaseScene.hpp"
#include "Scene/SceneObjID.hpp"
#include "System/ControlUserDataHolder.hpp"
#include "System/Data/OceanScenarioList.hpp"
#include "System/Data/SingleModeData.hpp"
#include "System/Data/SingleModeStockItemArray.hpp"
#include "System/GameDataFile.hpp"
#include "System/IslandData.hpp"
#include "System/IslandDataList.hpp"
#include "System/IslandSaveDataHolder.hpp"
#include "System/ScenarioInfo.hpp"
#include "Util/ControlUserUtil.hpp"

namespace {

/// Scenario types read from the island and ocean scenario lists.
constexpr u32 cScenarioTypeCloud = 3;
constexpr u32 cScenarioTypeNeko = 6;
constexpr u32 cScenarioTypeLucky = 9;

/// Ocean quadrant holding the lucky shines.
constexpr s32 cLuckyShineQuadrant = 3;

/// Upper bound of the scenarios tracked per island.
constexpr s32 cScenarioNumMax = 64;

/// Play-report event types.
constexpr preport::KeyEventType cEventTypeShine = static_cast<preport::KeyEventType>(7);
constexpr preport::KeyEventType cEventTypeIsland = static_cast<preport::KeyEventType>(13);
constexpr preport::KeyEventType cEventTypePhaseClear = static_cast<preport::KeyEventType>(20);
constexpr preport::KeyEventType cEventTypeCommonMax = static_cast<preport::KeyEventType>(22);

/// Play-report keys.
constexpr preport::Key cKeyFileId = static_cast<preport::Key>(9);
constexpr preport::Key cKeyFileName = static_cast<preport::Key>(10);
constexpr preport::Key cKeyPlayTime = static_cast<preport::Key>(11);
constexpr preport::Key cKeySinglePlayTime = static_cast<preport::Key>(12);
constexpr preport::Key cKeyDeathCount = static_cast<preport::Key>(17);
constexpr preport::Key cKeyIs2PAssist = static_cast<preport::Key>(19);
constexpr preport::Key cKeyIsHandheld = static_cast<preport::Key>(26);
constexpr preport::Key cKeyPadTypes = static_cast<preport::Key>(27);
constexpr preport::Key cKeyPhaseName = static_cast<preport::Key>(33);
constexpr preport::Key cKeyGoalItemNum = static_cast<preport::Key>(34);
constexpr preport::Key cKeyIslandId = static_cast<preport::Key>(35);
constexpr preport::Key cKeyIslandPlayTime = static_cast<preport::Key>(48);
constexpr preport::Key cKeyIslandEventValue = static_cast<preport::Key>(49);
constexpr preport::Key cKeyPhasePlayTime = static_cast<preport::Key>(52);

/// Controller kinds recorded in the island play report.
enum PadTypeReport {
    PadTypeReport_Unknown,
    PadTypeReport_Handheld,
    PadTypeReport_JoyDual,
    PadTypeReport_JoyLeft,
    PadTypeReport_JoyRight,
    PadTypeReport_FullKey,
};

/**
 * @brief Check whether a scenario of an island or ocean quadrant is complete.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @param islandId One-based island identifier, or a non-positive ocean island identifier.
 * @param scenarioIndex Scenario index.
 * @return True when the scenario is complete.
 */
inline bool isIslandScenarioComplete(GameDataHolderAccessor accessor, s32 islandId,
                                     s32 scenarioIndex) {
    if (islandId <= 0) {
        s32 quadrant = IslandDataFunction::getQuadrantIndexFromIslandID(islandId);
        return accessor.getHolder()
            ->getSingleFile()
            ->getOceanQuadrantSaveData(quadrant)
            ->isScenarioComplete(scenarioIndex);
    }

    return accessor.getHolder()
        ->getSingleFile()
        ->getIslandSaveData(islandId - 1)
        ->isScenarioComplete(scenarioIndex);
}

} // namespace

namespace rc {

/**
 * @brief Pairs a map-unit flag with the cutscene it unlocks.
 */
struct FlagConversion {
    s32 mMapUnitFlag;
    s32 mCutscene;
};

FlagConversion sFlagConversion[9] = {
    {1, 9}, {7, 10}, {8, 11}, {9, 12}, {2, 13}, {4, 14}, {5, 16}, {3, 18}, {6, 20},
};

} // namespace rc

s32 SingleModeData::sLastVisitedLighthouseID;

/**
 * @brief Check whether an actor is disabled while Plessie chases the player.
 * @param rInfo Initialization info of the actor.
 * @return True when the actor is flagged with isDisablePlessieChase during a Plessie chase.
 */
bool SingleModeDataFunction::isActorInPlessieChaseDisabled(const al::ActorInitInfo& rInfo) {
    if (!al::isSingleMode(rInfo)) {
        return false;
    }

    bool isDisable = false;
    if (al::tryGetArg(&isDisable, rInfo, "isDisablePlessieChase") && isDisable) {
        GameDataHolderAccessor accessor(rInfo.getActorSceneInfo().sceneObjHolder);
        if (rc::isPlessieChase(accessor.getHolder()->getSingleFile()->getUnlockedPhase())) {
            return true;
        }
    }

    return false;
}

/**
 * @brief Read the unlocked single-mode phase.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @return The unlocked phase.
 */
int SingleModeDataFunction::getUnlockedPhase(GameDataHolderAccessor accessor) {
    return accessor.getHolder()->getSingleFile()->getUnlockedPhase();
}

/**
 * @brief Check whether a placement layer is active during a phase.
 * @param layerId Placement layer of the object, or a negative value for no layer.
 * @param phase Current single-mode phase.
 * @param isPlessieChase True to test the Plessie-chase layout.
 * @return True when the object should be placed.
 */
bool SingleModeDataFunction::isValidPlacement(int layerId, int phase, bool isPlessieChase) {
    if (layerId < 0) {
        return true;
    }

    bool isInvalid;
    switch (phase) {
    case rc::SingleModePhases::PHASE3:
    case rc::SingleModePhases::PHASE3_BOSS:
    case rc::SingleModePhases::PHASE3_PLESSIE_CHASE:
        isInvalid = layerId == 17 || layerId == 14 || layerId > 22;
        break;
    case rc::SingleModePhases::PHASE4:
    case rc::SingleModePhases::PHASE4_BOSS:
    case rc::SingleModePhases::PHASE4_PLESSIE_CHASE:
        isInvalid = layerId == 14 || layerId == 17 || layerId == 20;
        break;
    case rc::SingleModePhases::PHASE2:
    case rc::SingleModePhases::PHASE2_BOSS:
        if (layerId > 18 || layerId == 14) {
            return false;
        }

        return true;
    default:
        if (layerId > 15) {
            return false;
        }

        return true;
    }

    if (isPlessieChase) {
        switch (layerId) {
        case 15:
        case 18:
        case 22:
            return false;
        default:
            break;
        }
    } else if (layerId == 21) {
        return false;
    }

    return !isInvalid;
}

/**
 * @brief Check whether an actor's placement layer is active in the current phase.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @param rInfo Initialization info of the actor.
 * @param isPlessieChase True to test the Plessie-chase layout.
 * @return True when the actor should be placed.
 */
bool SingleModeDataFunction::isValidPlacement(GameDataHolderAccessor accessor,
                                              const al::ActorInitInfo& rInfo,
                                              bool isPlessieChase) {
    s32 phase = accessor.getHolder()->getSingleFile()->getUnlockedPhase();
    return isValidPlacement(al::tryGetLayerID(rInfo), phase, isPlessieChase);
}

/**
 * @brief Check whether an actor's placement layer is active for the Plessie chase.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @param rInfo Initialization info of the actor.
 * @return True when the actor should be placed.
 */
bool SingleModeDataFunction::isValidPlessieChasePlacement(GameDataHolderAccessor accessor,
                                                          const al::ActorInitInfo& rInfo) {
    s32 phase = accessor.getHolder()->getSingleFile()->getUnlockedPhase();
    return isValidPlessieChasePlacement(al::tryGetLayerID(rInfo), phase);
}

/**
 * @brief Check whether a placement layer is active for the Plessie chase.
 * @param layerId Placement layer of the object, or a negative value for no layer.
 * @param phase Current single-mode phase.
 * @return True when the object should be placed.
 */
bool SingleModeDataFunction::isValidPlessieChasePlacement(int layerId, int phase) {
    if (layerId < 0) {
        return true;
    }

    if (phase == rc::SingleModePhases::PHASE4_PLESSIE_CHASE ||
        phase == rc::SingleModePhases::PHASE3_PLESSIE_CHASE) {
        if (layerId == 22) {
            return false;
        }
    } else if (layerId == 21) {
        return false;
    }

    return true;
}

/**
 * @brief Record whether the last demo was cancelled.
 * @param writer Writer to the game-data holder.
 * @param isCancelled True when the demo was cancelled.
 */
void SingleModeDataFunction::setDemoWasCancelled(GameDataHolderWriter writer, bool isCancelled) {
    writer.getHolder()->setDemoWasCancelled(isCancelled);
}

/**
 * @brief Check whether the last demo was cancelled.
 * @param accessor Accessor to the game-data holder.
 * @return True when the demo was cancelled.
 */
bool SingleModeDataFunction::isDemoWasCancelled(GameDataHolderAccessor accessor) {
    return accessor.getHolder()->isDemoWasCancelled();
}

/**
 * @brief Request or cancel a save.
 * @param writer Writer to the game-data holder.
 * @param isRequested True to request a save.
 */
void SingleModeDataFunction::setSaveRequested(GameDataHolderWriter writer, bool isRequested) {
    writer.getHolder()->setSaveRequested(isRequested);
}

/**
 * @brief Check whether a save has been requested.
 * @param accessor Accessor to the game-data holder.
 * @return True when a save was requested.
 */
bool SingleModeDataFunction::isSaveRequested(GameDataHolderAccessor accessor) {
    return accessor.getHolder()->isSaveRequested();
}

/**
 * @brief Check whether the prologue phase is active.
 * @param accessor Accessor to the game-data holder.
 * @return True during phase 0.
 */
bool SingleModeDataFunction::isPhase0(GameDataHolderAccessor accessor) {
    return accessor.getHolder()->isPhase0();
}

/**
 * @brief Check whether the scenario named by an actor's ScenarioID argument is complete.
 * @param pActor Actor whose zone selects the island.
 * @param rInfo Initialization info of the actor.
 * @return True when the scenario of the actor's island is complete.
 */
bool SingleModeDataFunction::isIslandScenarioIDComplete(const al::LiveActor* pActor,
                                                        const al::ActorInitInfo& rInfo) {
    if (al::isSingleMode(rInfo)) {
        s32 scenarioId = -1;
        al::tryGetArg(&scenarioId, rInfo, "ScenarioID");
        if (scenarioId >= 0) {
            s32 islandId = pActor->mPlacementHolder->getZoneNo();
            if (isIslandScenarioComplete(GameDataHolderAccessor(pActor), islandId,
                                         scenarioId - 1)) {
                return true;
            }
        }
    }

    return false;
}

/**
 * @brief Check whether a scenario of an island or ocean quadrant is complete.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @param islandId Island index, or a negative ocean island identifier.
 * @param scenarioId Scenario index.
 * @return True when the scenario is complete.
 */
bool SingleModeDataFunction::isScenarioComplete(GameDataHolderAccessor accessor, int islandId,
                                                int scenarioId) {
    return isIslandScenarioComplete(accessor, islandId + 1, scenarioId);
}

/**
 * @brief Check whether Fury Bowser's super hard mode is on.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @param hitPoint Current hit points of Fury Bowser.
 * @return True when the super hard mode is on.
 */
bool SingleModeDataFunction::isSuperHardModeOn(GameDataHolderAccessor accessor, int hitPoint) {
    return accessor.getHolder()->getSingleFile()->isSuperHardModeOn(hitPoint);
}

/**
 * @brief Notify the single-mode file that a stage started.
 * @param writer Writer to an initialized single-mode game-data holder.
 */
void SingleModeDataFunction::onStageStart(GameDataHolderWriter writer) {
    writer.getHolder()->getSingleFile()->onStageStart();
}

/**
 * @brief Notify the single-mode file that a stage ended.
 * @param writer Writer to an initialized single-mode game-data holder.
 */
void SingleModeDataFunction::onStageEnd(GameDataHolderWriter writer) {
    writer.getHolder()->getSingleFile()->onStageEnd();
}

/**
 * @brief Restart the current stage and mark the scene as restarting.
 * @param writer Writer to an initialized single-mode game-data holder.
 */
void SingleModeDataFunction::restartStage(GameDataHolderWriter writer) {
    writer.getHolder()->getSingleFile()->restartStage();
    writer.getHolder()->setSceneRestart(true);
}

/**
 * @brief Check whether the scene is being restarted.
 * @param writer Writer to the game-data holder.
 * @return True while the scene restarts.
 */
bool SingleModeDataFunction::isSceneRestart(GameDataHolderWriter writer) {
    return writer.getHolder()->isSceneRestart();
}

/**
 * @brief Clear the scene-restart mark.
 * @param writer Writer to the game-data holder.
 */
void SingleModeDataFunction::resetSceneRestart(GameDataHolderWriter writer) {
    writer.getHolder()->setSceneRestart(false);
}

/**
 * @brief Check whether an island is visited for the first time.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @param islandId Island index.
 * @return True when the island was never visited.
 */
bool SingleModeDataFunction::isIslandFirstVisit(GameDataHolderAccessor accessor, int islandId) {
    return accessor.getHolder()->getSingleFile()->getIslands()->isIslandFirstVisit(islandId);
}

/**
 * @brief Check whether an island is unlocked.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @param islandId Island index.
 * @return True when the island is unlocked.
 */
bool SingleModeDataFunction::isIslandUnlocked(GameDataHolderAccessor accessor, int islandId) {
    return accessor.getHolder()->getSingleFile()->getIslands()->isIslandUnlocked(islandId);
}

/**
 * @brief Unlock an island.
 * @param writer Writer to an initialized single-mode game-data holder.
 * @param islandId Island index.
 */
void SingleModeDataFunction::setIslandUnlocked(GameDataHolderWriter writer, int islandId) {
    writer.getHolder()->getSingleFile()->setIslandUnlocked(islandId);
}

/**
 * @brief Count the unlocked islands.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @return The number of unlocked islands.
 */
int SingleModeDataFunction::getUnlockedIslandNum(GameDataHolderAccessor accessor) {
    return accessor.getHolder()->getSingleFile()->getUnlockedIslandNum();
}

/**
 * @brief Read the previously visited valid island.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @return The previously visited island.
 */
int SingleModeDataFunction::getLastValidIslandVisited(GameDataHolderAccessor accessor) {
    return accessor.getHolder()->getSingleFile()->getLastValidIslandVisited();
}

/**
 * @brief Store the previously visited valid island.
 * @param writer Writer to an initialized single-mode game-data holder.
 * @param islandId Island index.
 */
void SingleModeDataFunction::setLastValidIslandVisited(GameDataHolderWriter writer, int islandId) {
    writer.getHolder()->getSingleFile()->setLastValidIslandVisited(islandId);
}

/**
 * @brief Read the currently visited valid island.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @return The currently visited island.
 */
int SingleModeDataFunction::getCurValidIslandVisited(GameDataHolderAccessor accessor) {
    return accessor.getHolder()->getSingleFile()->getCurValidIslandVisited();
}

/**
 * @brief Store the currently visited valid island.
 * @param writer Writer to an initialized single-mode game-data holder.
 * @param islandId Island index.
 */
void SingleModeDataFunction::setCurValidIslandVisited(GameDataHolderWriter writer, int islandId) {
    writer.getHolder()->getSingleFile()->setCurValidIslandVisited(islandId);
}

/**
 * @brief Read the active scenario of an island.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @param islandId Island index.
 * @return The active scenario index.
 */
int SingleModeDataFunction::getCurActiveScenarioIndex(GameDataHolderAccessor accessor,
                                                      int islandId) {
    return accessor.getHolder()->getSingleFile()->getCurActiveScenarioIndex(islandId);
}

/**
 * @brief Store the active scenario of an island.
 * @param writer Writer to an initialized single-mode game-data holder.
 * @param islandId Island index.
 * @param scenarioIndex Active scenario index.
 */
void SingleModeDataFunction::setCurActiveScenarioIndex(GameDataHolderWriter writer, int islandId,
                                                       int scenarioIndex) {
    writer.getHolder()->getSingleFile()->setCurActiveScenarioIndex(islandId, scenarioIndex);
}

/**
 * @brief Mark the name of an island's active scenario as seen.
 * @param writer Writer to an initialized single-mode game-data holder.
 * @param islandId Island index.
 */
void SingleModeDataFunction::setCurActiveScenarioNameSeen(GameDataHolderWriter writer,
                                                          int islandId) {
    writer.getHolder()->getSingleFile()->setCurActiveScenarioNameSeen(islandId);
}

/**
 * @brief Check whether the name of an island's active scenario was seen.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @param islandId Island index.
 * @return True when the scenario name was seen.
 */
bool SingleModeDataFunction::wasActiveScenarioNameSeen(GameDataHolderAccessor accessor,
                                                       int islandId) {
    return accessor.getHolder()->getSingleFile()->wasActiveScenarioNameSeen(islandId);
}

/**
 * @brief Record a passed checkpoint and the current disaster-mode state.
 * @param writer Writer to an initialized single-mode game-data holder.
 * @param islandId Island owning the checkpoint.
 * @param checkpointId Checkpoint identifier within the island.
 * @param pUser Scene-object user used to find the disaster-mode controller.
 */
void SingleModeDataFunction::setCheckpointPass(GameDataHolderWriter writer, int islandId,
                                               int checkpointId,
                                               const al::IUseSceneObjHolder* pUser) {
    SingleModeData* pFile = writer.getHolder()->getSingleFile();
    pFile->setCheckpointVisited(islandId, checkpointId);
    pFile->setIslandCheckpointVisited(-1);
    clearGigaBellPlayerRespawnPoint(writer);
    recordDisasterMode(writer, DisasterForceSetting_Controller, pUser);
}

/**
 * @brief Forget the Giga Bell respawn point.
 * @param writer Writer to an initialized single-mode game-data holder.
 */
void SingleModeDataFunction::clearGigaBellPlayerRespawnPoint(GameDataHolderWriter writer) {
    writer.getHolder()->getSingleFile()->clearGetGigaBellPlayerRespawnPoint();
}

/**
 * @brief Record the disaster-mode state in the single-mode file.
 * @param writer Writer to an initialized single-mode game-data holder.
 * @param setting How to determine the recorded state.
 * @param pUser Scene-object user used to find the disaster-mode controller.
 */
void SingleModeDataFunction::recordDisasterMode(GameDataHolderWriter writer,
                                                DisasterForceSetting setting,
                                                const al::IUseSceneObjHolder* pUser) {
    switch (setting) {
    case DisasterForceSetting_Off: {
        SingleModeData* pFile = writer.getHolder()->getSingleFile();
        pFile->setDisasterModeFrames(0);
        pFile->setDisasterMode(false);
        break;
    }
    case DisasterForceSetting_On: {
        SingleModeData* pFile = writer.getHolder()->getSingleFile();
        pFile->setDisasterModeFrames(0);
        pFile->setDisasterMode(true);
        break;
    }
    case DisasterForceSetting_Controller: {
        DisasterModeController* pController = DisasterModeController::tryGetController(pUser);
        if (pController != nullptr) {
            s32 frames = pController->calcDisasterFrames();
            bool isDisaster = pController->isDisasterMode();
            SingleModeData* pFile = writer.getHolder()->getSingleFile();
            pFile->setDisasterModeFrames(frames);
            pFile->setDisasterMode(isDisaster);
        }

        break;
    }
    case DisasterForceSetting_Calm: {
        SingleModeData* pFile = writer.getHolder()->getSingleFile();
        pFile->setDisasterModeFrames(100);
        pFile->setDisasterMode(false);
        break;
    }
    }
}

/**
 * @brief Read the last passed checkpoint.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @param pIslandId Receives the island owning the checkpoint.
 * @param pCheckpointId Receives the checkpoint identifier.
 */
void SingleModeDataFunction::getLastCheckpointPass(GameDataHolderAccessor accessor,
                                                   int* pIslandId, int* pCheckpointId) {
    const SingleModeData::CheckpointInfo& rCheckpoint =
        accessor.getHolder()->getSingleFile()->getCheckpoint();
    *pIslandId = rCheckpoint.mIslandId;
    *pCheckpointId = rCheckpoint.mCheckpointId;
}

/**
 * @brief Record a passed goal-item checkpoint.
 * @param writer Writer to an initialized single-mode game-data holder.
 * @param islandId Island owning the checkpoint.
 * @param checkpointId Checkpoint identifier within the island.
 */
void SingleModeDataFunction::setGoalItemCheckpointPass(GameDataHolderWriter writer, int islandId,
                                                       int checkpointId) {
    SingleModeData* pFile = writer.getHolder()->getSingleFile();
    pFile->setGoalItemCheckpointVisited(islandId, checkpointId);
    pFile->setIslandCheckpointVisited(-1);
    clearGigaBellPlayerRespawnPoint(writer);
}

/**
 * @brief Read the last passed goal-item checkpoint.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @param pIslandId Receives the island owning the checkpoint.
 * @param pCheckpointId Receives the checkpoint identifier.
 */
void SingleModeDataFunction::getLastGoalItemCheckpointPass(GameDataHolderAccessor accessor,
                                                           int* pIslandId, int* pCheckpointId) {
    const SingleModeData::CheckpointInfo& rCheckpoint =
        accessor.getHolder()->getSingleFile()->getGoalItemCheckpoint();
    *pIslandId = rCheckpoint.mIslandId;
    *pCheckpointId = rCheckpoint.mCheckpointId;
}

/**
 * @brief Record a passed island checkpoint.
 * @param writer Writer to an initialized single-mode game-data holder.
 * @param islandId Island whose checkpoint was passed.
 */
void SingleModeDataFunction::setIslandCheckpointPass(GameDataHolderWriter writer, int islandId) {
    SingleModeData* pFile = writer.getHolder()->getSingleFile();
    pFile->clearGetGigaBellPlayerRespawnPoint();
    pFile->setIslandCheckpointVisited(islandId);
}

/**
 * @brief Read the last passed island checkpoint.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @param pIslandId Receives the island whose checkpoint was passed.
 * @return True when an island checkpoint was passed.
 */
bool SingleModeDataFunction::getLastIslandCheckpointPass(GameDataHolderAccessor accessor,
                                                         int* pIslandId) {
    s32 islandId = accessor.getHolder()->getSingleFile()->getIslandCheckpoint();
    *pIslandId = islandId;
    return islandId >= 0;
}

/**
 * @brief Forget the last passed island checkpoint.
 * @param writer Writer to an initialized single-mode game-data holder.
 */
void SingleModeDataFunction::clearLastIslandCheckpointPass(GameDataHolderWriter writer) {
    writer.getHolder()->getSingleFile()->clearLastIslandCheckPointPass();
}

/**
 * @brief Check whether any respawn position is saved.
 * @param writer Writer to an initialized single-mode game-data holder.
 * @return True when a position is saved.
 */
bool SingleModeDataFunction::isAnySavedPosition(GameDataHolderWriter writer) {
    return writer.getHolder()->getSingleFile()->isAnySavedPosition();
}

/**
 * @brief Mark an island as vandalized by Fury Bowser and request a save.
 * @param writer Writer to an initialized single-mode game-data holder.
 * @param islandId Island index.
 * @param phase Phase in which the island was vandalized.
 */
void SingleModeDataFunction::setVandalizeIsland(GameDataHolderWriter writer, int islandId,
                                                int phase) {
    SingleModeData* pFile = writer.getHolder()->getSingleFile();
    writer.getHolder()->setSaveRequested(true);
    pFile->setVandalize(islandId, phase);
}

/**
 * @brief Clear the vandalized mark of an island and request a save.
 * @param writer Writer to an initialized single-mode game-data holder.
 * @param islandId Island index.
 * @param phase Phase in which the island was vandalized.
 */
void SingleModeDataFunction::clearVandalizeIsland(GameDataHolderWriter writer, int islandId,
                                                  int phase) {
    SingleModeData* pFile = writer.getHolder()->getSingleFile();
    writer.getHolder()->setSaveRequested(true);
    pFile->clearVandalize(islandId, phase);
}

/**
 * @brief Check whether an island is vandalized.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @param islandId Island index.
 * @param phase Phase to test.
 * @param pActive Receives whether the vandalism is active.
 * @return True when the island is vandalized.
 */
bool SingleModeDataFunction::isVandalizedIsland(GameDataHolderAccessor accessor, int islandId,
                                                int phase, bool* pActive) {
    return accessor.getHolder()->getSingleFile()->isVandalized(islandId, phase, pActive);
}

/**
 * @brief Forget every checkpoint except the Giga Bell one.
 * @param writer Writer to an initialized single-mode game-data holder.
 */
void SingleModeDataFunction::clearAllButGigaBellCheckpoint(GameDataHolderWriter writer) {
    writer.getHolder()->getSingleFile()->clearAllButGigaBellCheckpoint();
}

/**
 * @brief Forget every checkpoint.
 * @param writer Writer to an initialized single-mode game-data holder.
 */
void SingleModeDataFunction::clearAllCheckpoints(GameDataHolderWriter writer) {
    writer.getHolder()->getSingleFile()->clearAllCheckpointPass();
}

/**
 * @brief Read the type of a scenario.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @param islandId Island identifier, or a negative ocean island identifier.
 * @param scenarioId One-based scenario identifier.
 * @return The scenario type, or -1 for an invalid scenario.
 */
int SingleModeDataFunction::getScenarioType(GameDataHolderAccessor accessor, int islandId,
                                            int scenarioId) {
    if (scenarioId < 0) {
        return -1;
    }

    if (islandId >= 0) {
        IslandData* pIsland = accessor.getHolder()->getIslandDataList()->getIslandByIndex(islandId - 1);
        return pIsland->getScenarioDataByIndex(scenarioId - 1)->mScenarioType;
    }

    s32 quadrant = IslandDataFunction::getQuadrantIndexFromIslandID(islandId);
    ScenarioList* pList =
        accessor.getHolder()->getOceanScenarioList()->getScenarioListByQuadrant(quadrant);
    return pList->getScenarioDataByIndex(scenarioId - 1)->mScenarioType;
}

/**
 * @brief Check whether every cloud scenario of an ocean quadrant is complete.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @param quadrant Ocean quadrant index.
 * @return True when all cloud scenarios are complete.
 */
bool SingleModeDataFunction::allCloudScenariosComplete(GameDataHolderAccessor accessor,
                                                       int quadrant) {
    ScenarioList* pList =
        accessor.getHolder()->getOceanScenarioList()->getScenarioListByQuadrant(quadrant);
    for (s32 i = 0; i < pList->mCount; i++) {
        ScenarioData* pScenario = pList->getScenarioDataByIndex(i);
        if (pScenario->mScenarioType == cScenarioTypeCloud) {
            s32 islandId = IslandDataFunction::getIslandIDFromQuadrantIndex(quadrant);
            if (!isScenarioComplete(accessor, islandId - 1, pScenario->mScenarioId - 1)) {
                return false;
            }
        }
    }

    return true;
}

/**
 * @brief Find the first cloud scenario of an ocean quadrant.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @param quadrant Ocean quadrant index.
 * @param pInfo Receives the island and scenario of the cloud scenario, when found.
 */
void SingleModeDataFunction::getFirstCloudScenarioByQuadrant(GameDataHolderAccessor accessor,
                                                             int quadrant, ScenarioInfo* pInfo) {
    ScenarioList* pList =
        accessor.getHolder()->getOceanScenarioList()->getScenarioListByQuadrant(quadrant);
    for (s32 i = 0; i < pList->mCount; i++) {
        ScenarioData* pScenario = pList->getScenarioDataByIndex(i);
        if (pScenario->mScenarioType == cScenarioTypeCloud) {
            pInfo->mIslandId = IslandDataFunction::getIslandIDFromQuadrantIndex(quadrant) - 1;
            pInfo->mScenarioIndex = pScenario->mScenarioId - 1;
            return;
        }
    }
}

/**
 * @brief Check whether an ocean scenario is a lucky shine.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @param islandId Ocean island identifier.
 * @param scenarioId One-based scenario identifier.
 * @return True when the scenario is a lucky shine.
 */
bool SingleModeDataFunction::isLuckyShine(GameDataHolderAccessor accessor, int islandId,
                                          int scenarioId) {
    s32 quadrant = IslandDataFunction::getQuadrantIndexFromIslandID(islandId);
    ScenarioList* pList =
        accessor.getHolder()->getOceanScenarioList()->getScenarioListByQuadrant(quadrant);
    return pList->getScenarioDataByIndex(scenarioId - 1)->mScenarioType == cScenarioTypeLucky;
}

/**
 * @brief Check whether an ocean scenario is a cat (neko) shine.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @param islandId Ocean island identifier.
 * @param scenarioId One-based scenario identifier.
 * @return True when the scenario is a cat shine.
 */
bool SingleModeDataFunction::isNekoShine(GameDataHolderAccessor accessor, int islandId,
                                         int scenarioId) {
    if (islandId > 0) {
        return false;
    }

    s32 quadrant = IslandDataFunction::getQuadrantIndexFromIslandID(islandId);
    ScenarioList* pList =
        accessor.getHolder()->getOceanScenarioList()->getScenarioListByQuadrant(quadrant);
    return pList->getScenarioDataByIndex(scenarioId - 1)->mScenarioType == cScenarioTypeNeko;
}

/**
 * @brief Check whether only cat (neko) shines remain to be collected.
 * @param pActor Actor used to find the goal-item holder.
 * @return True when the last remaining shine is a cat shine.
 */
bool SingleModeDataFunction::allNekoShinesRemaining(const al::LiveActor* pActor) {
    auto* pHolder = al::tryGetSceneObj<GoalItemHolder>(pActor, SceneObjID_GoalItemHolder);
    if (pHolder != nullptr) {
        return pHolder->isLastShineNeko();
    }

    return false;
}

/**
 * @brief Check whether only disaster shines remain to be collected.
 * @param pActor Actor used to find the goal-item holder.
 * @return True when the last remaining shine is a disaster shine.
 */
bool SingleModeDataFunction::allDisasterShinesRemaining(const al::LiveActor* pActor) {
    auto* pHolder = al::tryGetSceneObj<GoalItemHolder>(pActor, SceneObjID_GoalItemHolder);
    if (pHolder != nullptr) {
        return pHolder->isLastShineDisaster();
    }

    return false;
}

/**
 * @brief Count the lucky shines that are not collected yet.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @return The number of remaining lucky shines.
 */
int SingleModeDataFunction::getNumRemainingLuckyShines(GameDataHolderAccessor accessor) {
    ScenarioList* pList =
        accessor.getHolder()->getOceanScenarioList()->getScenarioListByQuadrant(
            cLuckyShineQuadrant);
    s32 num = 0;
    for (s32 i = 0; i < pList->mCount; i++) {
        ScenarioData* pScenario = pList->getScenarioDataByIndex(i);
        if (pScenario->mScenarioType == cScenarioTypeLucky) {
            s32 islandId = IslandDataFunction::getIslandIDFromQuadrantIndex(cLuckyShineQuadrant);
            num += !isScenarioComplete(accessor, islandId - 1, pScenario->mScenarioId - 1);
        }
    }

    return num;
}

/**
 * @brief Mark a lucky-island position as completed and request a save.
 * @param writer Writer to an initialized single-mode game-data holder.
 * @param posIndex Lucky-island position index.
 */
void SingleModeDataFunction::setLuckyIslandPosCompleted(GameDataHolderWriter writer,
                                                        int posIndex) {
    writer.getHolder()->setSaveRequested(true);
    writer.getHolder()->getSingleFile()->setLuckyIslandPosCompleted(posIndex);
}

/**
 * @brief Check whether a lucky-island position is completed.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @param posIndex Lucky-island position index.
 * @return True when the position is completed.
 */
bool SingleModeDataFunction::wasLuckyIslandPosCompleted(GameDataHolderAccessor accessor,
                                                        int posIndex) {
    return accessor.getHolder()->getSingleFile()->wasLuckyIslandPosCompleted(posIndex);
}

/**
 * @brief Read the lucky shine placed at a lucky-island position.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @param posIndex Lucky-island position index.
 * @return The lucky-shine index.
 */
int SingleModeDataFunction::getLuckyShineIdxByPosIdx(GameDataHolderAccessor accessor,
                                                      int posIndex) {
    return accessor.getHolder()->getSingleFile()->getLuckyShineIdxByPosIdx(posIndex);
}

/**
 * @brief Find the ocean island owning an ocean scenario.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @param scenarioIndex Ocean scenario index.
 * @return The ocean island identifier, or 0 when the scenario is not found.
 */
int SingleModeDataFunction::getQuadrantIslandFromScenario(GameDataHolderAccessor accessor,
                                                          int scenarioIndex) {
    s32 quadrant = accessor.getHolder()->getOceanScenarioList()->getQuadrantIndexFromScenarioId(
        scenarioIndex + 1);
    if (quadrant < 0) {
        return 0;
    }

    return IslandDataFunction::getIslandIDFromQuadrantIndex(quadrant);
}

/**
 * @brief Check whether an ocean scenario is complete.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @param scenarioIndex Ocean scenario index.
 * @return True when the scenario is complete.
 */
bool SingleModeDataFunction::isOceanScenarioComplete(GameDataHolderAccessor accessor,
                                                     int scenarioIndex) {
    s32 quadrant = accessor.getHolder()->getOceanScenarioList()->getQuadrantIndexFromScenarioId(
        scenarioIndex + 1);
    if (quadrant < 0) {
        return false;
    }

    return accessor.getHolder()
        ->getSingleFile()
        ->getOceanQuadrantSaveData(quadrant)
        ->isScenarioComplete(scenarioIndex);
}

/**
 * @brief Find the first scenario of an island that is not complete.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @param islandId Island index.
 * @return The first incomplete scenario, or -1 when all are complete.
 */
int SingleModeDataFunction::getFirstAvailableScenario(GameDataHolderAccessor accessor,
                                                      int islandId) {
    const IslandSaveData* pSaveData =
        accessor.getHolder()->getSingleFile()->getIslandSaveData(islandId);
    for (s32 i = 0; i < cScenarioNumMax; i++) {
        if (!pSaveData->isScenarioComplete(i)) {
            return i;
        }
    }

    return -1;
}

/**
 * @brief Check whether every scenario of an island or ocean quadrant is complete.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @param islandId Island index, or a negative ocean island identifier.
 * @return True when all scenarios are complete.
 */
bool SingleModeDataFunction::allScenariosComplete(GameDataHolderAccessor accessor, int islandId) {
    s32 scenarioNum;
    const IslandSaveData* pSaveData;
    if (islandId >= 0) {
        scenarioNum =
            accessor.getHolder()->getIslandDataList()->getIslandByIndex(islandId)->getNumScenarios();
        pSaveData = accessor.getHolder()->getSingleFile()->getIslandSaveData(islandId);
    } else {
        s32 quadrant = IslandDataFunction::getQuadrantIndexFromIslandID(islandId + 1);
        scenarioNum = accessor.getHolder()
                          ->getOceanScenarioList()
                          ->getScenarioListByQuadrant(quadrant)
                          ->mCount;
        pSaveData = accessor.getHolder()->getSingleFile()->getOceanQuadrantSaveData(quadrant);
    }

    for (s32 i = 0; i < scenarioNum; i++) {
        if (!pSaveData->isScenarioComplete(i)) {
            return false;
        }
    }

    return true;
}

/**
 * @brief Complete a scenario and request a save.
 * @param writer Writer to an initialized single-mode game-data holder.
 * @param rInfo Scenario to complete.
 */
void SingleModeDataFunction::completeScenario(GameDataHolderWriter writer,
                                              const ScenarioInfo& rInfo) {
    writer.getHolder()->setSaveRequested(true);
    writer.getHolder()->getSingleFile()->completeScenario(rInfo);
}

/**
 * @brief Check whether a shard of an island is collected.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @param islandId Island index.
 * @param shardIndex Shard index.
 * @return True when the shard is collected.
 */
bool SingleModeDataFunction::isShardCollected(GameDataHolderAccessor accessor, int islandId,
                                              int shardIndex) {
    return (getShardFlag(accessor, islandId) & (1 << shardIndex)) != 0;
}

/**
 * @brief Read the collected-shard flags of an island.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @param islandId Island index.
 * @return The collected-shard flags.
 */
u8 SingleModeDataFunction::getShardFlag(GameDataHolderAccessor accessor, int islandId) {
    return accessor.getHolder()->getSingleFile()->getIslandSaveData(islandId)->mStateBytes[0];
}

/**
 * @brief Access the collected-shard flags of an island.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @param islandId Island index.
 * @return The collected-shard flags.
 */
u8* SingleModeDataFunction::getShardFlagPtr(GameDataHolderAccessor accessor, int islandId) {
    return &accessor.getHolder()->getSingleFile()->getIslandSaveDataPtr(islandId)->mStateBytes[0];
}

/**
 * @brief Remember the last visited lighthouse for the guide window.
 * @param lighthouseId Lighthouse identifier.
 */
void SingleModeDataFunction::setLastVisitedLighthouseIDForGuideWindow(int lighthouseId) {
    SingleModeData::sLastVisitedLighthouseID = lighthouseId;
}

/**
 * @brief Read the last visited lighthouse for the guide window.
 * @return The lighthouse identifier.
 */
int SingleModeDataFunction::getLastVisitedLighthouseIDForGuideWindow() {
    return SingleModeData::sLastVisitedLighthouseID;
}

/**
 * @brief Read the completed-scenario flags of an island or ocean quadrant.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @param islandId Island index, or a negative ocean island identifier.
 * @return The completed-scenario flags.
 */
u64 SingleModeDataFunction::getScenarioFlag(GameDataHolderAccessor accessor, int islandId) {
    if (islandId >= 0) {
        return accessor.getHolder()
            ->getSingleFile()
            ->getIslandSaveData(islandId)
            ->mCompletedScenarios;
    }

    s32 quadrant = IslandDataFunction::getQuadrantIndexFromIslandID(islandId + 1);
    return accessor.getHolder()
        ->getSingleFile()
        ->getOceanQuadrantSaveData(quadrant)
        ->mCompletedScenarios;
}

/**
 * @brief Collect a shard of an island and request a save.
 * @param writer Writer to an initialized single-mode game-data holder.
 * @param islandId Island index.
 * @param shardIndex Shard index.
 */
void SingleModeDataFunction::collectShard(GameDataHolderWriter writer, int islandId,
                                          int shardIndex) {
    writer.getHolder()->setSaveRequested(true);
    IslandSaveData* pSaveData = writer.getHolder()->getSingleFile()->getIslandSaveDataPtr(islandId);
    pSaveData->mStateBytes[0] |= 1 << shardIndex;
}

/**
 * @brief Reset a scenario.
 * @param writer Writer to an initialized single-mode game-data holder.
 * @param rInfo Scenario to reset.
 */
void SingleModeDataFunction::resetScenario(GameDataHolderWriter writer,
                                           const ScenarioInfo& rInfo) {
    writer.getHolder()->getSingleFile()->resetScenario(rInfo);
}

/**
 * @brief Find the active main scenario of an island.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @param islandId Island index.
 * @return The first incomplete main scenario, or -1 when all are complete.
 */
int SingleModeDataFunction::getIslandActiveScenario(GameDataHolderAccessor accessor,
                                                    int islandId) {
    const IslandSaveData* pSaveData =
        accessor.getHolder()->getSingleFile()->getIslandSaveData(islandId);
    IslandData* pIsland = accessor.getHolder()->getIslandDataList()->getIslandByIndex(islandId);
    for (s32 i = 0; i < sead::Mathi::min(pIsland->getNumScenarios(), 3); i++) {
        if (!pSaveData->isScenarioComplete(i)) {
            return i;
        }
    }

    return -1;
}

/**
 * @brief Count the completed scenarios of an island or ocean quadrant.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @param islandId Island index, or a negative ocean island identifier.
 * @return The number of completed scenarios.
 */
int SingleModeDataFunction::calcNumCompletedScenarios(GameDataHolderAccessor accessor,
                                                      int islandId) {
    if (islandId >= 0) {
        return accessor.getHolder()
            ->getSingleFile()
            ->getIslandSaveData(islandId)
            ->getNumScenariosComplete();
    }

    s32 quadrant = IslandDataFunction::getQuadrantIndexFromIslandID(islandId + 1);
    return accessor.getHolder()
        ->getSingleFile()
        ->getOceanQuadrantSaveData(quadrant)
        ->getNumScenariosComplete();
}

/**
 * @brief Count the collected goal items (Cat Shines).
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @return The number of collected goal items.
 */
int SingleModeDataFunction::getGoalItemsCollected(GameDataHolderAccessor accessor) {
    return accessor.getHolder()->getSingleFile()->getGoalItemNum();
}

/**
 * @brief Read the number of collectable goal items (Cat Shines).
 * @return The number of collectable goal items.
 */
int SingleModeDataFunction::getMaxCollectableGoalItems() {
    return 100;
}

/**
 * @brief Count one more player death.
 * @param writer Writer to an initialized single-mode game-data holder.
 */
void SingleModeDataFunction::incDeathCount(GameDataHolderWriter writer) {
    writer.getHolder()->getSingleFile()->incDeathCount();
}

/**
 * @brief Record the values shared by every play-report event.
 * @param writer Writer to an initialized game-data holder.
 * @param pUser Scene-object user used to name the phase.
 * @param isIncludeCurrentPlay True to include the current play in the play time.
 * @param isGoalItemGet True when a goal item was just collected.
 */
void SingleModeDataFunction::reportCommonPlayReportItems(GameDataHolderWriter writer,
                                                         const al::IUseSceneObjHolder* pUser,
                                                         bool isIncludeCurrentPlay,
                                                         bool isGoalItemGet) {
    GameDataHolder* pHolder = writer.getHolder();
    pHolder->addSessionId();
    SingleModeData* pSingleFile = pHolder->getSingleFile();
    GameDataFile* pFile = pHolder->getGameDataFile(pHolder->getLastPlayingFileId());
    if (pHolder->isSingleMode()) {
        pHolder->setPlayReportData(cKeyFileId, pHolder->getLastSingleModePlayingFileID());
        if (pSingleFile != nullptr) {
            pHolder->setPlayReportData(cKeyFileName, pSingleFile->getName());
            pHolder->setPlayReportData(cKeyPhaseName, pSingleFile->getPhaseName(pUser));
            pHolder->setPlayReportData(cKeyGoalItemNum,
                                       pSingleFile->getGoalItemNum() + isGoalItemGet);
            pHolder->setPlayReportData(cKeyIslandId, pSingleFile->getCurValidIslandVisited());
            pHolder->setPlayReportData(cKeySinglePlayTime,
                                       pSingleFile->getTotalPlayTimePR(isIncludeCurrentPlay));
        }

        if (pFile != nullptr) {
            pHolder->setPlayReportData(cKeyPlayTime, pFile->getTotalPlayTimePR(false));
        }
    } else {
        pHolder->setPlayReportData(cKeyFileId, pHolder->getLastPlayingFileId());
        if (pFile != nullptr) {
            pHolder->setPlayReportData(cKeyFileName, pFile->getName());
            pHolder->setPlayReportData(cKeyPlayTime,
                                       pFile->getTotalPlayTimePR(isIncludeCurrentPlay));
            pHolder->setPlayReportData(cKeySinglePlayTime,
                                       pSingleFile->getTotalPlayTimePR(false));
        }
    }
}

/**
 * @brief Record an integer play-report value.
 * @param writer Writer to an initialized game-data holder.
 * @param key Play-report key.
 * @param value Value to record.
 */
void SingleModeDataFunction::setPlayReportData(GameDataHolderWriter writer, preport::Key key,
                                               int value) {
    writer.getHolder()->setPlayReportData(key, value);
}

/**
 * @brief Record a string play-report value.
 * @param writer Writer to an initialized game-data holder.
 * @param key Play-report key.
 * @param rValue Value to record.
 */
void SingleModeDataFunction::setPlayReportData(GameDataHolderWriter writer, preport::Key key,
                                               sead::SafeString& rValue) {
    writer.getHolder()->setPlayReportData(key, rValue);
}

/**
 * @brief Record a 64-bit play-report value.
 * @param writer Writer to an initialized game-data holder.
 * @param key Play-report key.
 * @param value Value to record.
 */
void SingleModeDataFunction::setPlayReportData(GameDataHolderWriter writer, preport::Key key,
                                               s64 value) {
    writer.getHolder()->setPlayReportData(key, value);
}

/**
 * @brief Begin the play report of a collected shine.
 * @param writer Writer to an initialized single-mode game-data holder.
 * @param pUser Scene-object user used to name the phase.
 * @param shineIndex Index of the collected shine.
 * @return True when the report could not be started.
 */
bool SingleModeDataFunction::reportShineEvent(GameDataHolderWriter writer,
                                              const al::IUseSceneObjHolder* pUser,
                                              int shineIndex) {
    GameDataHolder* pHolder = writer.getHolder();
    SingleModeData* pSingleFile = pHolder->getSingleFile();
    GameDataFile* pFile = pHolder->getGameDataFile(pHolder->getLastPlayingFileId());
    s32 eventId = shineIndex + 7;
    pHolder->getPlayReportManager()->requestSaveData();
    if (pHolder->beginPlayReport(cEventTypeShine, eventId, 1)) {
        return true;
    }

    pHolder->addSessionId();
    pHolder->setPlayReportData(cKeyFileId, pHolder->getLastSingleModePlayingFileID());
    pHolder->setPlayReportData(cKeyFileName, pSingleFile->getName());
    pHolder->setPlayReportData(cKeySinglePlayTime, pSingleFile->getTotalPlayTimePR(true));
    pHolder->setPlayReportData(cKeyPlayTime, pFile->getTotalPlayTimePR(false));
    pHolder->setPlayReportData(cKeyPhaseName, pSingleFile->getPhaseName(pUser));
    pHolder->setPlayReportData(cKeyGoalItemNum, pSingleFile->getGoalItemNum() + 1);
    pHolder->setPlayReportData(cKeyIs2PAssist, pHolder->is2PAssistMode());
    return false;
}

/**
 * @brief Report an island event.
 * @param writer Writer to an initialized single-mode game-data holder.
 * @param pUser Scene-object user used to name the phase.
 * @param eventValue Event-specific value.
 * @param islandId Island of the event.
 */
void SingleModeDataFunction::reportIslandEvent(GameDataHolderWriter writer,
                                               const al::IUseSceneObjHolder* pUser,
                                               int eventValue, int islandId) {
    GameDataHolder* pHolder = writer.getHolder();
    s32 padTypes[4];
    s32 padNum = 0;
    for (s32 i = 0; i < rc::getControlUserNumMax(); i++) {
        if (!rc::isActiveControlUser(writer, i)) {
            continue;
        }

        const ControlUserData* pUserData =
            pHolder->getControlUserDataHolder()->getControlUserData(i);
        if (al::isPadTypeFullKey(pUserData->mPadPort)) {
            padTypes[padNum] = PadTypeReport_FullKey;
        } else if (al::isPadTypeHandheld(pUserData->mPadPort)) {
            padTypes[padNum] = PadTypeReport_Handheld;
        } else if (al::isPadTypeJoyDual(pUserData->mPadPort)) {
            padTypes[padNum] = PadTypeReport_JoyDual;
        } else if (al::isPadTypeJoySingle(pUserData->mPadPort)) {
            if (al::isPadTypeJoyLeft(pUserData->mPadPort)) {
                padTypes[padNum] = PadTypeReport_JoyLeft;
            } else if (al::isPadTypeJoyRight(pUserData->mPadPort)) {
                padTypes[padNum] = PadTypeReport_JoyRight;
            } else {
                padTypes[padNum] = PadTypeReport_Unknown;
            }
        } else {
            padTypes[padNum] = PadTypeReport_Unknown;
        }

        padNum++;
    }

    SingleModeData* pSingleFile = pHolder->getSingleFile();
    GameDataFile* pFile = pHolder->getGameDataFile(pHolder->getLastPlayingFileId());
    pHolder->getPlayReportManager()->requestSaveData();
    if (pHolder->beginPlayReport(cEventTypeIsland, 13, 1)) {
        return;
    }

    pHolder->addSessionId();
    pHolder->setPlayReportData(cKeyFileId, pHolder->getLastSingleModePlayingFileID());
    pHolder->setPlayReportData(cKeyFileName, pSingleFile->getName());
    pHolder->setPlayReportData(cKeyPhaseName, pSingleFile->getPhaseName(pUser));
    pHolder->setPlayReportData(cKeyIs2PAssist, pHolder->is2PAssistMode());
    pHolder->setPlayReportData(cKeyIsHandheld,
                               al::isPadTypeHandheld(al::getMainControllerPort()));
    pHolder->setPlayReportData(cKeyPadTypes, padTypes, padNum);
    pHolder->setPlayReportData(cKeyGoalItemNum, pSingleFile->getGoalItemNum());
    pHolder->setPlayReportData(cKeyDeathCount, pSingleFile->getDeathCount());
    pHolder->setPlayReportData(cKeySinglePlayTime, pSingleFile->getTotalPlayTimePR(true));
    pHolder->setPlayReportData(cKeyPlayTime, pFile->getTotalPlayTimePR(false));
    pHolder->setPlayReportData(cKeyIslandId, islandId);
    pHolder->setPlayReportData(cKeyIslandPlayTime, getIslandPlayTime(writer));
    pHolder->setPlayReportData(cKeyIslandEventValue, eventValue);
    pHolder->endPlayReport();
}

/**
 * @brief Read the play time on the current island.
 * @param writer Writer to an initialized single-mode game-data holder.
 * @return The island play time.
 */
int SingleModeDataFunction::getIslandPlayTime(GameDataHolderWriter writer) {
    return writer.getHolder()->getSingleFile()->getIslandPlayTime();
}

/**
 * @brief Report a cleared phase and reset the phase play time.
 * @param writer Writer to an initialized single-mode game-data holder.
 * @param pUser Scene-object user used to name the phase.
 * @param phase Cleared phase.
 */
void SingleModeDataFunction::reportPhaseClearEvent(GameDataHolderWriter writer,
                                                   al::IUseSceneObjHolder* pUser, int phase) {
    GameDataHolder* pHolder = writer.getHolder();
    SingleModeData* pSingleFile = pHolder->getSingleFile();
    if (!pHolder->beginPlayReport(cEventTypePhaseClear, pHolder->isSingleMode() ? 10 : 7, 0)) {
        reportCommonPlayReportItems(writer, pUser, true, false);
        pHolder->setPlayReportData(cKeyPhaseName,
                                   pHolder->getSingleFile()->getPhaseNameForPhaseClearPR(phase));
        pHolder->setPlayReportData(cKeyIs2PAssist, pHolder->is2PAssistMode());
        pHolder->setPlayReportData(cKeyPhasePlayTime, pSingleFile->getPhasePlayTime());
        pHolder->endPlayReport();
    }

    pSingleFile->resetPhaseTotalPlayTime();
}

/**
 * @brief Begin a play-report event and record the common values.
 * @param writer Writer to an initialized game-data holder.
 * @param pUser Scene-object user used to name the phase.
 * @param type Play-report event type.
 * @param eventId Event identifier.
 * @param option Event option.
 * @return True when the report could not be started.
 */
bool SingleModeDataFunction::beginPlayReport(GameDataHolderWriter writer,
                                             const al::IUseSceneObjHolder* pUser,
                                             preport::KeyEventType type, int eventId,
                                             int option) {
    GameDataHolder* pHolder = writer.getHolder();
    if (type <= cEventTypeCommonMax) {
        s32 base = pHolder->isSingleMode() ? 7 : 4;
        if (pHolder->beginPlayReport(type, base + eventId, option)) {
            return true;
        }

        reportCommonPlayReportItems(writer, pUser, type != 0 && type != 2, type == 7);
    } else if (pHolder->beginPlayReport(type, eventId, option)) {
        return true;
    }

    return false;
}

/**
 * @brief Check whether the two-player assist mode is active.
 * @param accessor Accessor to the game-data holder.
 * @return True when a second player assists.
 */
bool SingleModeDataFunction::getIs2PAssistMode(GameDataHolderAccessor accessor) {
    return accessor.getHolder()->is2PAssistMode();
}

/**
 * @brief End the current play-report event.
 * @param writer Writer to an initialized game-data holder.
 */
void SingleModeDataFunction::endPlayReport(GameDataHolderWriter writer) {
    writer.getHolder()->endPlayReport();
}

/**
 * @brief Record a floating-point play-report value.
 * @param writer Writer to an initialized game-data holder.
 * @param key Play-report key.
 * @param value Value to record.
 */
void SingleModeDataFunction::setPlayReportData(GameDataHolderWriter writer, preport::Key key,
                                               float value) {
    writer.getHolder()->setPlayReportData(key, value);
}

/**
 * @brief Record an integer-array play-report value.
 * @param writer Writer to an initialized game-data holder.
 * @param key Play-report key.
 * @param pValues Values to record.
 * @param num Number of values.
 */
void SingleModeDataFunction::setPlayReportData(GameDataHolderWriter writer, preport::Key key,
                                               int* pValues, int num) {
    writer.getHolder()->setPlayReportData(key, pValues, num);
}

/**
 * @brief Record a floating-point-array play-report value.
 * @param writer Writer to an initialized game-data holder.
 * @param key Play-report key.
 * @param pValues Values to record.
 * @param num Number of values.
 */
void SingleModeDataFunction::setPlayReportData(GameDataHolderWriter writer, preport::Key key,
                                               float* pValues, int num) {
    writer.getHolder()->setPlayReportData(key, pValues, num);
}

/**
 * @brief Reset the boss-battle play time.
 * @param writer Writer to an initialized single-mode game-data holder.
 */
void SingleModeDataFunction::resetBossPlayTime(GameDataHolderWriter writer) {
    writer.getHolder()->getSingleFile()->resetBossPlayTime();
}

/**
 * @brief Update the boss-battle play time.
 * @param writer Writer to an initialized single-mode game-data holder.
 */
void SingleModeDataFunction::setBossPlayTime(GameDataHolderWriter writer) {
    writer.getHolder()->getSingleFile()->setBossPlayTime();
}

/**
 * @brief Read the boss-battle play time.
 * @param writer Writer to an initialized single-mode game-data holder.
 * @return The boss-battle play time.
 */
s32 SingleModeDataFunction::getBossPlayTime(GameDataHolderWriter writer) {
    return writer.getHolder()->getSingleFile()->getBossPlayTime();
}

/**
 * @brief Reset the island play time.
 * @param writer Writer to an initialized single-mode game-data holder.
 */
void SingleModeDataFunction::resetIslandPlayTime(GameDataHolderWriter writer) {
    writer.getHolder()->getSingleFile()->resetIslandPlayTime();
}

/**
 * @brief Update the island play time.
 * @param writer Writer to an initialized single-mode game-data holder.
 */
void SingleModeDataFunction::setIslandPlayTime(GameDataHolderWriter writer) {
    writer.getHolder()->getSingleFile()->setIslandPlayTime();
}

/**
 * @brief Update the phase play time.
 * @param writer Writer to an initialized single-mode game-data holder.
 */
void SingleModeDataFunction::setPhasePlayTime(GameDataHolderWriter writer) {
    writer.getHolder()->getSingleFile()->setPhasePlayTime();
}

/**
 * @brief Send the network status play report.
 * @param writer Writer to an initialized game-data holder.
 */
void SingleModeDataFunction::sendNetworkStatus(GameDataHolderWriter writer) {
    writer.getHolder()->sendNetworkStatus();
}

/**
 * @brief Recalculate whether every shine is collected.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 */
void SingleModeDataFunction::calculateAllShineCollected(GameDataHolderAccessor accessor) {
    accessor.getHolder()->getSingleFile()->calculateAllShineCollected();
}

/**
 * @brief Check whether every shine is collected.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @return True when every shine is collected.
 */
bool SingleModeDataFunction::isAllShineCollected(GameDataHolderAccessor accessor) {
    return accessor.getHolder()->getSingleFile()->isAllShineCollected();
}

/**
 * @brief Check whether the completion ending picture was seen.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @return True when the picture was seen.
 */
bool SingleModeDataFunction::isCompleteEndingPictureSeen(GameDataHolderAccessor accessor) {
    return accessor.getHolder()->getSingleFile()->isCompleteEndingPictureSeen();
}

/**
 * @brief Check whether the single-mode file is complete.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @return True when the file is complete.
 */
bool SingleModeDataFunction::isFileComplete(GameDataHolderAccessor accessor) {
    return accessor.getHolder()->getSingleFile()->isFileComplete();
}

/**
 * @brief Mark the completion ending picture as seen.
 * @param writer Writer to an initialized single-mode game-data holder.
 */
void SingleModeDataFunction::setCompleteEndingPictureSeen(GameDataHolderWriter writer) {
    writer.getHolder()->getSingleFile()->setCompleteEndingPictureSeen();
}

/**
 * @brief Check whether the rematch with the stronger Fury Bowser is available.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @return True when the rematch is available.
 */
bool SingleModeDataFunction::isDarkBowserV2Available(GameDataHolderAccessor accessor) {
    return accessor.getHolder()->getSingleFile()->isDarkBowserV2Available();
}

/**
 * @brief Check whether the stronger Fury Bowser was defeated.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @return True when the stronger Fury Bowser was defeated.
 */
bool SingleModeDataFunction::isDarkBowserV2Defeated(GameDataHolderAccessor accessor) {
    return accessor.getHolder()->getSingleFile()->isDarkBowserV2Defeated();
}

/**
 * @brief Mark the stronger Fury Bowser as defeated.
 * @param writer Writer to an initialized single-mode game-data holder.
 */
void SingleModeDataFunction::setDarkBowserV2Defeated(GameDataHolderWriter writer) {
    writer.getHolder()->getSingleFile()->setDarkBowserV2Defeated();
}

/**
 * @brief Check whether Bowser Jr. can be played.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @return True when Bowser Jr. is available.
 */
bool SingleModeDataFunction::isMeowserJrAvailable(GameDataHolderAccessor accessor) {
    return accessor.getHolder()->getSingleFile()->isMeowserJrAvailable();
}

/**
 * @brief Mark whether the current phase has ended.
 * @param writer Writer to an initialized single-mode game-data holder.
 * @param isEnd True when the phase has ended.
 */
void SingleModeDataFunction::setPhaseEnd(GameDataHolderWriter writer, bool isEnd) {
    writer.getHolder()->getSingleFile()->setPhaseEnd(isEnd);
}

/**
 * @brief Check whether the current phase has ended.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @return True when the phase has ended.
 */
bool SingleModeDataFunction::isPhaseEnd(GameDataHolderAccessor accessor) {
    return accessor.getHolder()->getSingleFile()->isPhaseEnd();
}

/**
 * @brief Check whether the ending was seen.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @return True when the ending was seen.
 */
bool SingleModeDataFunction::hasSeenEnding(GameDataHolderAccessor accessor) {
    return accessor.getHolder()->getSingleFile()->hasSeenEnding();
}

/**
 * @brief Mark the ending as seen, optionally resetting the final battle.
 * @param writer Writer to an initialized single-mode game-data holder.
 * @param isReset True to clear the checkpoints and restore Fury Bowser's hit points.
 */
void SingleModeDataFunction::setHasSeenEnding(GameDataHolderWriter writer, bool isReset) {
    writer.getHolder()->getSingleFile()->setHasSeenEnding();
    if (isReset) {
        clearAllCheckpoints(writer);
        setPhase4DarkBowserHitPoint(writer, 200);
    }
}

/**
 * @brief Store Fury Bowser's hit points of phase 4.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @param hitPoint Hit points.
 */
void SingleModeDataFunction::setPhase4DarkBowserHitPoint(GameDataHolderAccessor accessor,
                                                         int hitPoint) {
    accessor.getHolder()->getSingleFile()->setPhase4DarkBowserHitPoint(hitPoint);
}

/**
 * @brief Check whether phase 0 is played for the first time.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @return True on the first play of phase 0.
 */
bool SingleModeDataFunction::isFirstPhase0(GameDataHolderAccessor accessor) {
    return accessor.getHolder()->getSingleFile()->isFirstPhase0();
}

/**
 * @brief Mark phase 0 as played.
 * @param writer Writer to an initialized single-mode game-data holder.
 */
void SingleModeDataFunction::setIsFirstPhase0(GameDataHolderWriter writer) {
    writer.getHolder()->getSingleFile()->setIsFirstPhase0();
}

/**
 * @brief Check whether the phase-2 boss was defeated once.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @return True when the boss was defeated.
 */
bool SingleModeDataFunction::isFirstPhase2BossDefeated(GameDataHolderAccessor accessor) {
    return accessor.getHolder()->getSingleFile()->isFirstPhase2BossDefeated();
}

/**
 * @brief Mark the phase-2 boss as defeated.
 * @param writer Writer to an initialized single-mode game-data holder.
 */
void SingleModeDataFunction::setFirstPhase2BossDefeated(GameDataHolderWriter writer) {
    writer.getHolder()->getSingleFile()->setFirstPhase2BossDefeated();
}

/**
 * @brief Check whether the phase-3 boss was defeated once.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @return True when the boss was defeated.
 */
bool SingleModeDataFunction::isFirstPhase3BossDefeated(GameDataHolderAccessor accessor) {
    return accessor.getHolder()->getSingleFile()->isFirstPhase3BossDefeated();
}

/**
 * @brief Mark the phase-3 boss as defeated.
 * @param writer Writer to an initialized single-mode game-data holder.
 */
void SingleModeDataFunction::setFirstPhase3BossDefeated(GameDataHolderWriter writer) {
    writer.getHolder()->getSingleFile()->setFirstPhase3BossDefeated();
}

/**
 * @brief Check whether the phase-4 boss was defeated.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @return True when the boss was defeated.
 */
bool SingleModeDataFunction::isPhase4BossDefeated(GameDataHolderAccessor accessor) {
    return accessor.getHolder()->getSingleFile()->isPhase4BossDefeated();
}

/**
 * @brief Mark the phase-4 boss as defeated.
 * @param writer Writer to an initialized single-mode game-data holder.
 */
void SingleModeDataFunction::setPhase4BossDefeated(GameDataHolderWriter writer) {
    writer.getHolder()->getSingleFile()->setPhase4BossDefeated();
}

/**
 * @brief Check whether Plessie was already ridden.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @return True when Plessie was ridden.
 */
bool SingleModeDataFunction::isAlreadyPlayRidon(GameDataHolderAccessor accessor) {
    return accessor.getHolder()->getSingleFile()->isAlreadyPlayRidon();
}

/**
 * @brief Mark Plessie as ridden.
 * @param writer Writer to an initialized single-mode game-data holder.
 */
void SingleModeDataFunction::setPlayRidon(GameDataHolderWriter writer) {
    writer.getHolder()->getSingleFile()->setPlayRidon();
}

/**
 * @brief Forget that Plessie was ridden.
 * @param writer Writer to an initialized single-mode game-data holder.
 */
void SingleModeDataFunction::resetPlayRidon(GameDataHolderWriter writer) {
    writer.getHolder()->getSingleFile()->resetPlayRidon();
}

/**
 * @brief Check whether a cutscene was seen.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @param cutsceneId Cutscene identifier.
 * @return True when the cutscene was seen.
 */
bool SingleModeDataFunction::hasSeenCutscene(GameDataHolderAccessor accessor, int cutsceneId) {
    return accessor.getHolder()->getSingleFile()->hasSeenCutscene(cutsceneId);
}

/**
 * @brief Mark a cutscene as seen.
 * @param writer Writer to an initialized single-mode game-data holder.
 * @param cutsceneId Cutscene identifier.
 */
void SingleModeDataFunction::setHasSeenCutscene(GameDataHolderWriter writer, int cutsceneId) {
    writer.getHolder()->getSingleFile()->setHasSeenCutscene(cutsceneId);
}

/**
 * @brief Forget that a cutscene was seen.
 * @param writer Writer to an initialized single-mode game-data holder.
 * @param cutsceneId Cutscene identifier.
 */
void SingleModeDataFunction::clearHasSeenCutscene(GameDataHolderWriter writer, int cutsceneId) {
    writer.getHolder()->getSingleFile()->clearHasSeenCutscene(cutsceneId);
}

/**
 * @brief Check whether a character is unlocked.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @param charaType Character type.
 * @return True when the character is unlocked.
 */
bool SingleModeDataFunction::hasUnlockedChar(GameDataHolderAccessor accessor, int charaType) {
    return accessor.getHolder()->getSingleFile()->hasUnlockedChar(charaType);
}

/**
 * @brief Check whether any character is unlocked.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @return True when a character is unlocked.
 */
bool SingleModeDataFunction::hasUnlockedAnyChar(GameDataHolderAccessor accessor) {
    return accessor.getHolder()->getSingleFile()->hasUnlockedAnyChar();
}

/**
 * @brief Unlock a character.
 * @param writer Writer to an initialized single-mode game-data holder.
 * @param charaType Character type.
 */
void SingleModeDataFunction::setUnlockedChar(GameDataHolderWriter writer, int charaType) {
    writer.getHolder()->getSingleFile()->setUnlockedChar(charaType);
}

/**
 * @brief Check whether a generic item is saved.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @param itemId Item identifier.
 * @return True when the item is saved.
 */
bool SingleModeDataFunction::isGenericItemSaved(GameDataHolderAccessor accessor, int itemId) {
    return accessor.getHolder()->getSingleFile()->isGenericItemSaved(itemId);
}

/**
 * @brief Mark a generic item as saved.
 * @param writer Writer to an initialized single-mode game-data holder.
 * @param itemId Item identifier.
 */
void SingleModeDataFunction::setGenericItemSaved(GameDataHolderWriter writer, int itemId) {
    writer.getHolder()->getSingleFile()->setGenericItemSaved(itemId);
}

/**
 * @brief Check whether disaster mode was active at the saved checkpoint.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @return True when disaster mode was active.
 */
bool SingleModeDataFunction::getIsDisasterMode(GameDataHolderAccessor accessor) {
    return accessor.getHolder()->getSingleFile()->isDisasterMode();
}

/**
 * @brief Check whether disaster mode transitions from hard to super hard.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @return True while the transition is pending.
 */
bool SingleModeDataFunction::isHardToSuperDisasterTransition(GameDataHolderAccessor accessor) {
    return accessor.getHolder()->getSingleFile()->isHardToSuperDisasterTransition();
}

/**
 * @brief Mark whether disaster mode transitions from hard to super hard.
 * @param writer Writer to an initialized single-mode game-data holder.
 * @param isTransition True while the transition is pending.
 */
void SingleModeDataFunction::setHardToSuperDisasterTransition(GameDataHolderWriter writer,
                                                              bool isTransition) {
    writer.getHolder()->getSingleFile()->setHardToSuperDisasterTransition(isTransition);
}

/**
 * @brief Check whether the disaster foreshadowing starts automatically.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @return True when foreshadowing starts automatically.
 */
bool SingleModeDataFunction::isAutoForeshadow(GameDataHolderAccessor accessor) {
    return accessor.getHolder()->getSingleFile()->isAutoForeshadow();
}

/**
 * @brief Set whether the disaster foreshadowing starts automatically.
 * @param writer Writer to an initialized single-mode game-data holder.
 * @param isAuto True to start foreshadowing automatically.
 */
void SingleModeDataFunction::setAutoForeshadow(GameDataHolderWriter writer, bool isAuto) {
    writer.getHolder()->getSingleFile()->setAutoForeshadow(isAuto);
}

/**
 * @brief Read the saved disaster-mode frames.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @return The disaster-mode frames.
 */
int SingleModeDataFunction::getDisasterModeFrames(GameDataHolderAccessor accessor) {
    return accessor.getHolder()->getSingleFile()->getDisasterModeFrames();
}

/**
 * @brief Read the peaceful frames left after a boss battle.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @return The peaceful frames.
 */
int SingleModeDataFunction::getDisasterModePostBossPeaceFrames(GameDataHolderAccessor accessor) {
    return accessor.getHolder()->getSingleFile()->getDisasterModePostBossPeaceFrames();
}

/**
 * @brief Store the peaceful frames left after a boss battle.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @param frames Peaceful frames.
 */
void SingleModeDataFunction::setDisasterModePostBossPeaceFrames(GameDataHolderAccessor accessor,
                                                                int frames) {
    accessor.getHolder()->getSingleFile()->setDisasterModePostBossPeaceFrames(frames);
}

/**
 * @brief Read the disaster-mode flow index.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @return The flow index.
 */
int SingleModeDataFunction::getDisasterModeFlowIndex(GameDataHolderAccessor accessor) {
    return accessor.getHolder()->getSingleFile()->getDisasterModeFlowIndex();
}

/**
 * @brief Store the disaster-mode flow index.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @param index Flow index.
 */
void SingleModeDataFunction::setDisasterModeFlowIndex(GameDataHolderAccessor accessor,
                                                      int index) {
    accessor.getHolder()->getSingleFile()->setDisasterModeFlowIndex(index);
}

/**
 * @brief Read the Giga Bell lock count.
 * @param writer Writer to an initialized single-mode game-data holder.
 * @return The lock count.
 */
int SingleModeDataFunction::getGigaBellLockCount(GameDataHolderWriter writer) {
    return writer.getHolder()->getSingleFile()->getGigaBellLockCount();
}

/**
 * @brief Store the Giga Bell lock count.
 * @param writer Writer to an initialized single-mode game-data holder.
 * @param count Lock count.
 */
void SingleModeDataFunction::setGigaBellLockCount(GameDataHolderWriter writer, int count) {
    writer.getHolder()->getSingleFile()->setGigaBellLockCount(count);
}

/**
 * @brief Check whether the Giga Bell is unlocked.
 * @param writer Writer to an initialized single-mode game-data holder.
 * @return True when the Giga Bell is unlocked.
 */
bool SingleModeDataFunction::isGigaBellUnlocked(GameDataHolderWriter writer) {
    return writer.getHolder()->getSingleFile()->isGigaBellUnlocked();
}

/**
 * @brief Set whether the Giga Bell is unlocked.
 * @param writer Writer to an initialized single-mode game-data holder.
 * @param isUnlocked True when the Giga Bell is unlocked.
 */
void SingleModeDataFunction::setGigaBellUnlocked(GameDataHolderWriter writer, bool isUnlocked) {
    writer.getHolder()->getSingleFile()->setGigaBellUnlocked(isUnlocked);
}

/**
 * @brief Count the scenarios of an island or ocean quadrant.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @param islandId Island index, or a negative ocean island identifier.
 * @return The number of scenarios.
 */
int SingleModeDataFunction::getScenarioNum(GameDataHolderAccessor accessor, int islandId) {
    if (islandId >= 0) {
        return accessor.getHolder()
            ->getIslandDataList()
            ->getIslandByIndex(islandId)
            ->getNumScenarios();
    }

    s32 quadrant = IslandDataFunction::getQuadrantIndexFromIslandID(islandId + 1);
    return accessor.getHolder()->getOceanScenarioList()->getScenarioListByQuadrant(quadrant)->mCount;
}

/**
 * @brief Store the unlocked phase.
 * @param writer Writer to an initialized single-mode game-data holder.
 * @param phase Unlocked phase.
 */
void SingleModeDataFunction::setUnlockedPhase(GameDataHolderWriter writer, int phase) {
    writer.getHolder()->getSingleFile()->setUnlockedPhase(phase);
}

/**
 * @brief Read Fury Bowser's hit points of phase 1.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @return The hit points.
 */
int SingleModeDataFunction::getPhase1DarkBowserHitPoint(GameDataHolderAccessor accessor) {
    return accessor.getHolder()->getSingleFile()->getPhase1DarkBowserHitPoint();
}

/**
 * @brief Store Fury Bowser's hit points of phase 1.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @param hitPoint Hit points.
 */
void SingleModeDataFunction::setPhase1DarkBowserHitPoint(GameDataHolderAccessor accessor,
                                                         int hitPoint) {
    accessor.getHolder()->getSingleFile()->setPhase1DarkBowserHitPoint(hitPoint);
}

/**
 * @brief Read Fury Bowser's hit points of phase 2.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @return The hit points.
 */
int SingleModeDataFunction::getPhase2DarkBowserHitPoint(GameDataHolderAccessor accessor) {
    return accessor.getHolder()->getSingleFile()->getPhase2DarkBowserHitPoint();
}

/**
 * @brief Store Fury Bowser's hit points of phase 2.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @param hitPoint Hit points.
 */
void SingleModeDataFunction::setPhase2DarkBowserHitPoint(GameDataHolderAccessor accessor,
                                                         int hitPoint) {
    accessor.getHolder()->getSingleFile()->setPhase2DarkBowserHitPoint(hitPoint);
}

/**
 * @brief Read Fury Bowser's hit points of phase 3.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @return The hit points.
 */
int SingleModeDataFunction::getPhase3DarkBowserHitPoint(GameDataHolderAccessor accessor) {
    return accessor.getHolder()->getSingleFile()->getPhase3DarkBowserHitPoint();
}

/**
 * @brief Store Fury Bowser's hit points of phase 3.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @param hitPoint Hit points.
 */
void SingleModeDataFunction::setPhase3DarkBowserHitPoint(GameDataHolderAccessor accessor,
                                                         int hitPoint) {
    accessor.getHolder()->getSingleFile()->setPhase3DarkBowserHitPoint(hitPoint);
}

/**
 * @brief Read Fury Bowser's hit points of phase 4.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @return The hit points.
 */
int SingleModeDataFunction::getPhase4DarkBowserHitPoint(GameDataHolderAccessor accessor) {
    return accessor.getHolder()->getSingleFile()->getPhase4DarkBowserHitPoint();
}

/**
 * @brief Read Fury Bowser's hit points of the final phase-4 battle.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @return The hit points.
 */
int SingleModeDataFunction::getPhase4DarkBowserHitPointFinal(GameDataHolderAccessor accessor) {
    return accessor.getHolder()->getSingleFile()->getPhase4DarkBowserHitPointFinal();
}

/**
 * @brief Store Fury Bowser's hit points of the final phase-4 battle.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @param hitPoint Hit points.
 */
void SingleModeDataFunction::setPhase4DarkBowserHitPointFinal(GameDataHolderAccessor accessor,
                                                              int hitPoint) {
    accessor.getHolder()->getSingleFile()->setPhase4DarkBowserHitPointFinal(hitPoint);
}

/**
 * @brief Remember Fury Bowser's phase-3 hit points before the boss battle.
 * @param writer Writer to an initialized single-mode game-data holder.
 */
void SingleModeDataFunction::setPhase3DarkBowserHitPointPreBoss(GameDataHolderWriter writer) {
    SingleModeData* pFile = writer.getHolder()->getSingleFile();
    pFile->setPhase3DarkBowserHitPointPreBattle(pFile->getPhase3DarkBowserHitPoint());
}

/**
 * @brief Remember Fury Bowser's phase-4 hit points before the boss battle.
 * @param writer Writer to an initialized single-mode game-data holder.
 */
void SingleModeDataFunction::setPhase4DarkBowserHitPointPreBoss(GameDataHolderWriter writer) {
    SingleModeData* pFile = writer.getHolder()->getSingleFile();
    pFile->setPhase4DarkBowserHitPointPreBattle(pFile->getPhase4DarkBowserHitPoint());
}

/**
 * @brief Check whether phase 1 is new to the player.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @return True when the phase is new.
 */
bool SingleModeDataFunction::isNewToPhase1(GameDataHolderAccessor accessor) {
    return accessor.getHolder()->getSingleFile()->isNewToPhase1();
}

/**
 * @brief Set whether phase 1 is new to the player.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @param isNew True when the phase is new.
 */
void SingleModeDataFunction::setIsNewToPhase1(GameDataHolderAccessor accessor, bool isNew) {
    accessor.getHolder()->getSingleFile()->setIsNewToPhase1(isNew);
}

/**
 * @brief Check whether phase 2 is new to the player.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @return True when the phase is new.
 */
bool SingleModeDataFunction::isNewToPhase2(GameDataHolderAccessor accessor) {
    return accessor.getHolder()->getSingleFile()->isNewToPhase2();
}

/**
 * @brief Set whether phase 2 is new to the player.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @param isNew True when the phase is new.
 */
void SingleModeDataFunction::setIsNewToPhase2(GameDataHolderAccessor accessor, bool isNew) {
    accessor.getHolder()->getSingleFile()->setIsNewToPhase2(isNew);
}

/**
 * @brief Check whether phase 3 is new to the player.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @return True when the phase is new.
 */
bool SingleModeDataFunction::isNewToPhase3(GameDataHolderAccessor accessor) {
    return accessor.getHolder()->getSingleFile()->isNewToPhase3();
}

/**
 * @brief Set whether phase 3 is new to the player.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @param isNew True when the phase is new.
 */
void SingleModeDataFunction::setIsNewToPhase3(GameDataHolderAccessor accessor, bool isNew) {
    accessor.getHolder()->getSingleFile()->setIsNewToPhase3(isNew);
}

/**
 * @brief Check whether phase 4 is new to the player.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @return True when the phase is new.
 */
bool SingleModeDataFunction::isNewToPhase4(GameDataHolderAccessor accessor) {
    return accessor.getHolder()->getSingleFile()->isNewToPhase4();
}

/**
 * @brief Set whether phase 4 is new to the player.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @param isNew True when the phase is new.
 */
void SingleModeDataFunction::setIsNewToPhase4(GameDataHolderAccessor accessor, bool isNew) {
    accessor.getHolder()->getSingleFile()->setIsNewToPhase4(isNew);
}

/**
 * @brief Check whether the phase-2 boss is new to the player.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @return True when the boss is new.
 */
bool SingleModeDataFunction::isNewToPhase2Boss(GameDataHolderAccessor accessor) {
    return accessor.getHolder()->getSingleFile()->isNewToPhase2Boss();
}

/**
 * @brief Set whether the phase-2 boss is new to the player.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @param isNew True when the boss is new.
 */
void SingleModeDataFunction::setIsNewToPhase2Boss(GameDataHolderAccessor accessor, bool isNew) {
    accessor.getHolder()->getSingleFile()->setIsNewToPhase2Boss(isNew);
}

/**
 * @brief Check whether the phase-3 boss is new to the player.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @return True when the boss is new.
 */
bool SingleModeDataFunction::isNewToPhase3Boss(GameDataHolderAccessor accessor) {
    return accessor.getHolder()->getSingleFile()->isNewToPhase3Boss();
}

/**
 * @brief Set whether the phase-3 boss is new to the player.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @param isNew True when the boss is new.
 */
void SingleModeDataFunction::setIsNewToPhase3Boss(GameDataHolderAccessor accessor, bool isNew) {
    accessor.getHolder()->getSingleFile()->setIsNewToPhase3Boss(isNew);
}

/**
 * @brief Check whether Bowser's phase-1 intro is new to the player.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @return True when the intro is new.
 */
bool SingleModeDataFunction::isNewToPhase1BowserIntro(GameDataHolderAccessor accessor) {
    return accessor.getHolder()->getSingleFile()->isNewToPhase1BowserIntro();
}

/**
 * @brief Set whether Bowser's phase-1 intro is new to the player.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @param isNew True when the intro is new.
 */
void SingleModeDataFunction::setIsNewToPhase1BowserIntro(GameDataHolderAccessor accessor,
                                                         bool isNew) {
    accessor.getHolder()->getSingleFile()->setIsNewToPhase1BowserIntro(isNew);
}

/**
 * @brief Check whether Bowser's phase-2 intro is new to the player.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @return True when the intro is new.
 */
bool SingleModeDataFunction::isNewToPhase2BowserIntro(GameDataHolderAccessor accessor) {
    return accessor.getHolder()->getSingleFile()->isNewToPhase2BowserIntro();
}

/**
 * @brief Set whether Bowser's phase-2 intro is new to the player.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @param isNew True when the intro is new.
 */
void SingleModeDataFunction::setIsNewToPhase2BowserIntro(GameDataHolderAccessor accessor,
                                                         bool isNew) {
    accessor.getHolder()->getSingleFile()->setIsNewToPhase2BowserIntro(isNew);
}

/**
 * @brief Check whether Bowser's phase-3 intro is new to the player.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @return True when the intro is new.
 */
bool SingleModeDataFunction::isNewToPhase3BowserIntro(GameDataHolderAccessor accessor) {
    return accessor.getHolder()->getSingleFile()->isNewToPhase3BowserIntro();
}

/**
 * @brief Set whether Bowser's phase-3 intro is new to the player.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @param isNew True when the intro is new.
 */
void SingleModeDataFunction::setIsNewToPhase3BowserIntro(GameDataHolderAccessor accessor,
                                                         bool isNew) {
    accessor.getHolder()->getSingleFile()->setIsNewToPhase3BowserIntro(isNew);
}

/**
 * @brief Check whether Bowser's phase-1 exit is new to the player.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @return True when the exit is new.
 */
bool SingleModeDataFunction::isNewToPhase1BowserExit(GameDataHolderAccessor accessor) {
    return accessor.getHolder()->getSingleFile()->isNewToPhase1BowserExit();
}

/**
 * @brief Set whether Bowser's phase-1 exit is new to the player.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @param isNew True when the exit is new.
 */
void SingleModeDataFunction::setIsNewToPhase1BowserExit(GameDataHolderAccessor accessor,
                                                        bool isNew) {
    accessor.getHolder()->getSingleFile()->setIsNewToPhase1BowserExit(isNew);
}

/**
 * @brief Check whether Bowser's phase-2 exit is new to the player.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @return True when the exit is new.
 */
bool SingleModeDataFunction::isNewToPhase2BowserExit(GameDataHolderAccessor accessor) {
    return accessor.getHolder()->getSingleFile()->isNewToPhase2BowserExit();
}

/**
 * @brief Set whether Bowser's phase-2 exit is new to the player.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @param isNew True when the exit is new.
 */
void SingleModeDataFunction::setIsNewToPhase2BowserExit(GameDataHolderAccessor accessor,
                                                        bool isNew) {
    accessor.getHolder()->getSingleFile()->setIsNewToPhase2BowserExit(isNew);
}

/**
 * @brief Check whether Bowser's phase-3 exit is new to the player.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @return True when the exit is new.
 */
bool SingleModeDataFunction::isNewToPhase3BowserExit(GameDataHolderAccessor accessor) {
    return accessor.getHolder()->getSingleFile()->isNewToPhase3BowserExit();
}

/**
 * @brief Set whether Bowser's phase-3 exit is new to the player.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @param isNew True when the exit is new.
 */
void SingleModeDataFunction::setIsNewToPhase3BowserExit(GameDataHolderAccessor accessor,
                                                        bool isNew) {
    accessor.getHolder()->getSingleFile()->setIsNewToPhase3BowserExit(isNew);
}

/**
 * @brief Check whether the next transition fades to white.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @return True when the transition fades to white.
 */
bool SingleModeDataFunction::shouldFadeToWhite(GameDataHolderAccessor accessor) {
    return accessor.getHolder()->getSingleFile()->shouldFadeToWhite();
}

/**
 * @brief Set whether the next transition fades to white.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @param isFade True to fade to white.
 */
void SingleModeDataFunction::setShouldFadeToWhite(GameDataHolderAccessor accessor, bool isFade) {
    accessor.getHolder()->getSingleFile()->setShouldFadeToWhite(isFade);
}

/**
 * @brief Read the Giga Bell respawn point.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @param pTrans Receives the respawn position.
 * @param pFront Receives the respawn direction.
 * @return True when a respawn point is stored.
 */
bool SingleModeDataFunction::tryGetGigaBellPlayerRespawnPoint(GameDataHolderAccessor accessor,
                                                              sead::Vector3f* pTrans,
                                                              sead::Vector3f* pFront) {
    return accessor.getHolder()->getSingleFile()->tryGetGigaBellPlayerRespawnPoint(pTrans, pFront);
}

/**
 * @brief Check whether a Giga Bell respawn point is stored.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @return True when the respawn point is valid.
 */
bool SingleModeDataFunction::isGigaBellPlayerRespawnPointValid(GameDataHolderAccessor accessor) {
    return accessor.getHolder()->getSingleFile()->isGigaBellRespawnValid();
}

/**
 * @brief Store the Giga Bell respawn point.
 * @param writer Writer to an initialized single-mode game-data holder.
 * @param rTrans Respawn position.
 * @param rFront Respawn direction.
 */
void SingleModeDataFunction::setGigaBellPlayerRespawnPoint(GameDataHolderWriter writer,
                                                           const sead::Vector3f& rTrans,
                                                           const sead::Vector3f& rFront) {
    writer.getHolder()->getSingleFile()->setGigaBellPlayerRespawnPoint(rTrans, rFront);
}

/**
 * @brief Read the generic respawn point.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @param pTrans Receives the respawn position.
 * @param pFront Receives the respawn direction.
 * @return True when a respawn point is stored.
 */
bool SingleModeDataFunction::tryGetGenericPlayerRespawnPosition(GameDataHolderAccessor accessor,
                                                                sead::Vector3f* pTrans,
                                                                sead::Vector3f* pFront) {
    return accessor.getHolder()->getSingleFile()->tryGetGenericPlayerRespawn(pTrans, pFront);
}

/**
 * @brief Check whether a generic respawn point is stored.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @return True when the respawn point is valid.
 */
bool SingleModeDataFunction::isGenericRespawnPlayerPositionValid(GameDataHolderAccessor accessor) {
    return accessor.getHolder()->getSingleFile()->isGenericRespawnValid();
}

/**
 * @brief Forget the generic respawn point.
 * @param writer Writer to an initialized single-mode game-data holder.
 */
void SingleModeDataFunction::clearGenericPlayerRespawnPosition(GameDataHolderWriter writer) {
    writer.getHolder()->getSingleFile()->clearGenericPlayerRespawn();
}

/**
 * @brief Store the generic respawn point.
 * @param writer Writer to an initialized single-mode game-data holder.
 * @param rTrans Respawn position.
 * @param rFront Respawn direction.
 */
void SingleModeDataFunction::setGenericPlayerRespawnPosition(GameDataHolderWriter writer,
                                                             const sead::Vector3f& rTrans,
                                                             const sead::Vector3f& rFront) {
    writer.getHolder()->getSingleFile()->setGenericPlayerRespawn(rTrans, rFront);
}

/**
 * @brief Count the stocked items of a type.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @param itemType Item type.
 * @return The stocked count.
 */
u32 SingleModeDataFunction::getStockItemCount(GameDataHolderAccessor accessor, int itemType) {
    return accessor.getHolder()->getSingleFile()->getStockItems()->getStockItemCount(itemType);
}

/**
 * @brief Count the stocked items at an index.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @param index Stock index.
 * @return The stocked count.
 */
u32 SingleModeDataFunction::getStockItemCountByIndex(GameDataHolderAccessor accessor, int index) {
    return accessor.getHolder()->getSingleFile()->getStockItems()->getStockItemCountByIndex(index);
}

/**
 * @brief Use one stocked item.
 * @param writer Writer to an initialized single-mode game-data holder.
 * @param itemType Item type.
 */
void SingleModeDataFunction::useStockItem(GameDataHolderWriter writer, int itemType) {
    writer.getHolder()->getSingleFile()->getStockItems()->useStockItem(itemType);
}

/**
 * @brief Mark a guide message as seen.
 * @param writer Writer to an initialized single-mode game-data holder.
 * @param messageId Message bit index.
 * @param isFirstSet True to use the first flag set.
 */
void SingleModeDataFunction::setGuideMessageSeen(GameDataHolderWriter writer, u32 messageId,
                                                 bool isFirstSet) {
    writer.getHolder()->getSingleFile()->setGuideMessageSeen(messageId, isFirstSet);
}

/**
 * @brief Forget that a guide message was seen.
 * @param writer Writer to an initialized single-mode game-data holder.
 * @param messageId Message bit index.
 * @param isFirstSet True to use the first flag set.
 */
void SingleModeDataFunction::resetGuideMessageSeen(GameDataHolderWriter writer, u32 messageId,
                                                   bool isFirstSet) {
    writer.getHolder()->getSingleFile()->resetGuideMessageSeen(messageId, isFirstSet);
}

/**
 * @brief Check whether a guide message was seen.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @param messageId Message bit index.
 * @param isFirstSet True to use the first flag set.
 * @return True when the message was seen.
 */
bool SingleModeDataFunction::isGuideMessageAlreadySeen(GameDataHolderAccessor accessor,
                                                       u32 messageId, bool isFirstSet) {
    return accessor.getHolder()->getSingleFile()->isGuideMessageAlreadySeen(messageId, isFirstSet);
}

/**
 * @brief Destroy a disaster block and request a save.
 * @param writer Writer to an initialized single-mode game-data holder.
 * @param blockId Block identifier.
 */
void SingleModeDataFunction::destroyDisasterBlock(GameDataHolderWriter writer, int blockId) {
    writer.getHolder()->setSaveRequested(true);
    writer.getHolder()->getSingleFile()->destroyDisasterBlock(blockId);
}

/**
 * @brief Check whether a disaster block is destroyed.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @param blockId Block identifier.
 * @return True when the block is destroyed.
 */
bool SingleModeDataFunction::isDisasterBlockDestroyed(GameDataHolderAccessor accessor,
                                                      int blockId) {
    return accessor.getHolder()->getSingleFile()->isDisasterBlockDestroyed(blockId);
}

/**
 * @brief Destroy a hard block and request a save.
 * @param writer Writer to an initialized single-mode game-data holder.
 * @param blockId Block identifier.
 */
void SingleModeDataFunction::destroyBlockHard(GameDataHolderWriter writer, int blockId) {
    writer.getHolder()->setSaveRequested(true);
    writer.getHolder()->getSingleFile()->destroyBlockHard(blockId);
}

/**
 * @brief Check whether a hard block is destroyed.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @param blockId Block identifier.
 * @return True when the block is destroyed.
 */
bool SingleModeDataFunction::isBlockHardDestroyed(GameDataHolderAccessor accessor, int blockId) {
    return accessor.getHolder()->getSingleFile()->isBlockHardDestroyed(blockId);
}

/**
 * @brief Change the camera sensitivity and request a save.
 * @param writer Writer to the game-data holder.
 * @param sensitivity Camera sensitivity level.
 * @return True when the setting changed.
 */
bool SingleModeDataFunction::setCameraSensitivity(GameDataHolderWriter writer, int sensitivity) {
    if (SingleModeData::sOptions.mCameraSensitivity == sensitivity) {
        return false;
    }

    writer.getHolder()->setSaveRequested(true);
    SingleModeData::setCameraSensitivity(writer.getHolder(), sensitivity);
    return true;
}

/**
 * @brief Read the camera sensitivity.
 * @param accessor Accessor to the game-data holder.
 * @return The camera sensitivity level.
 */
s8 SingleModeDataFunction::getCameraSensitiviy(GameDataHolderAccessor accessor) {
    return SingleModeData::sOptions.mCameraSensitivity;
}

/**
 * @brief Change whether vertical camera control is inverted and request a save.
 * @param writer Writer to the game-data holder.
 * @param isReverse True to invert the camera.
 * @return True when the setting changed.
 */
bool SingleModeDataFunction::setCameraReverseVertical(GameDataHolderWriter writer,
                                                      bool isReverse) {
    if (getCameraReverseVertical(writer) == isReverse) {
        return false;
    }

    writer.getHolder()->setSaveRequested(true);
    SingleModeData::setCameraReverseVertical(writer.getHolder(), isReverse);
    return true;
}

/**
 * @brief Check whether vertical camera control is inverted.
 * @param writer Writer to the game-data holder.
 * @return True when the camera is inverted.
 */
bool SingleModeDataFunction::getCameraReverseVertical(GameDataHolderWriter writer) {
    return SingleModeData::sOptions.mIsCameraReverseVertical;
}

/**
 * @brief Change whether horizontal camera control is inverted and request a save.
 * @param writer Writer to the game-data holder.
 * @param isReverse True to invert the camera.
 * @return True when the setting changed.
 */
bool SingleModeDataFunction::setCameraReverseHorizontal(GameDataHolderWriter writer,
                                                        bool isReverse) {
    if (getCameraReverseHorizontal(writer) == isReverse) {
        return false;
    }

    writer.getHolder()->setSaveRequested(true);
    SingleModeData::setCameraReverseHorizontal(writer.getHolder(), isReverse);
    return true;
}

/**
 * @brief Check whether horizontal camera control is inverted.
 * @param writer Writer to the game-data holder.
 * @return True when the camera is inverted.
 */
bool SingleModeDataFunction::getCameraReverseHorizontal(GameDataHolderWriter writer) {
    return SingleModeData::sOptions.mIsCameraReverseHorizontal;
}

/**
 * @brief Enable or disable the map.
 * @param writer Writer to the game-data holder.
 * @param isEnabled True to enable the map.
 */
void SingleModeDataFunction::setMapEnabled(GameDataHolderWriter writer, bool isEnabled) {
    writer.getHolder()->setMapEnabled(isEnabled);
}

/**
 * @brief Check whether the map is enabled.
 * @param accessor Accessor to the game-data holder.
 * @return True when the map is enabled.
 */
bool SingleModeDataFunction::isMapEnabled(GameDataHolderAccessor accessor) {
    return accessor.getHolder()->isMapEnabled();
}

/**
 * @brief Read the map zoom ratio.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @return The map zoom ratio.
 */
f32 SingleModeDataFunction::getMapZoomRatio(GameDataHolderAccessor accessor) {
    return accessor.getHolder()->getSingleFile()->getMapZoomRatio();
}

/**
 * @brief Store the map zoom ratio.
 * @param writer Writer to an initialized single-mode game-data holder.
 * @param ratio Map zoom ratio.
 */
void SingleModeDataFunction::setMapZoomRatio(GameDataHolderWriter writer, f32 ratio) {
    writer.getHolder()->getSingleFile()->setMapZoomRatio(ratio);
}

/**
 * @brief Change the assist-mode type and request a save.
 * @param writer Writer to the game-data holder.
 * @param type Assist-mode type.
 * @return True when the setting changed.
 */
bool SingleModeDataFunction::setAssistModeType(GameDataHolderWriter writer, u8 type) {
    if (SingleModeData::sOptions.mAssistModeType == type) {
        return false;
    }

    writer.getHolder()->setSaveRequested(true);
    SingleModeData::setAssistModeType(writer.getHolder(), type);
    return true;
}

/**
 * @brief Read the assist-mode type.
 * @param writer Writer to the game-data holder.
 * @return The assist-mode type.
 */
u8 SingleModeDataFunction::getAssistModeType(GameDataHolderWriter writer) {
    return SingleModeData::sOptions.mAssistModeType;
}

/**
 * @brief Enable or disable the two-player assist mode.
 * @param writer Writer to the game-data holder.
 * @param isAssist True to enable the assist mode.
 */
void SingleModeDataFunction::setIs2PAssistMode(GameDataHolderWriter writer, bool isAssist) {
    writer.getHolder()->set2PAssistMode(isAssist);
}

/**
 * @brief Read the saved state of a cat target.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @param id Identifier of the cat target.
 * @param pTarget Receives the saved state.
 * @return True when the state was found.
 */
bool SingleModeDataFunction::tryGetNekoSaveData(GameDataHolderAccessor accessor, int id,
                                                neko::Target* pTarget) {
    return accessor.getHolder()->getSingleFile()->tryGetNekoSaveData(id, pTarget);
}

/**
 * @brief Read the saved states of the cat targets of a parent.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @param parentId Identifier of the parent.
 * @param pTargets Receives the saved states.
 * @return The number of states found.
 */
int SingleModeDataFunction::tryGetNekoSaveDataByParentID(GameDataHolderAccessor accessor,
                                                         int parentId,
                                                         sead::PtrArray<neko::Target>* pTargets) {
    return accessor.getHolder()->getSingleFile()->tryGetNekoSaveDataByParentID(parentId, pTargets);
}

/**
 * @brief Save the state of a cat target and request a save.
 * @param writer Writer to an initialized single-mode game-data holder.
 * @param id Identifier of the cat target.
 * @param pTarget Cat target to save.
 */
void SingleModeDataFunction::setNekoSaveData(GameDataHolderWriter writer, int id,
                                             const neko::Target* pTarget) {
    writer.getHolder()->setSaveRequested(true);
    writer.getHolder()->getSingleFile()->setNekoSaveData(id, pTarget);
}

/**
 * @brief Check whether a cat parent's demo was seen.
 * @param accessor Accessor to an initialized single-mode game-data holder.
 * @param parentId Identifier of the parent.
 * @return True when the demo was seen.
 */
bool SingleModeDataFunction::hasNekoParentSeenDemo(GameDataHolderAccessor accessor,
                                                   int parentId) {
    return accessor.getHolder()->getSingleFile()->hasNekoParentSeenDemo(parentId);
}

/**
 * @brief Mark a cat parent's demo as seen and request a save.
 * @param writer Writer to an initialized single-mode game-data holder.
 * @param parentId Identifier of the parent.
 */
void SingleModeDataFunction::setNekoParentSeenDemo(GameDataHolderWriter writer, int parentId) {
    writer.getHolder()->setSaveRequested(true);
    writer.getHolder()->getSingleFile()->setNekoParentSeenDemo(parentId);
}

namespace rc {

namespace {

/// Stage name of every single-mode phase.
const char* cPhaseStageNames[SingleModePhases::size()] = {
    "SingleModeOceanPhase0Stage", "SingleModeOceanStage", "SingleModeBossStage",
    "SingleModeOceanStage",       "SingleModeBossStage",  "SingleModeOceanStage",
    "SingleModeBossStage",        "SingleModeOceanStage", "SingleModeOceanStage",
    "SingleModeBossStage",        "SingleModeOceanStage",
};

} // namespace

/**
 * @brief Read the stage name of a single-mode phase.
 * @param phase Single-mode phase; out-of-range values are clamped.
 * @return The stage name.
 */
const char* getSingleModePhaseName(int phase) {
    return cPhaseStageNames[sead::Mathi::clamp(phase, 0, SingleModePhases::getLastIndex())];
}

/**
 * @brief Map a boss phase to the exploration phase around it.
 * @param phase Single-mode phase.
 * @param isAfterPlessieChase True when the phase-3 boss leads to phase 4.
 * @return The matching exploration phase, or the phase itself when it is no boss phase.
 */
int getNonBossPhase(int phase, bool isAfterPlessieChase) {
    switch (phase) {
    case SingleModePhases::PHASE1_BOSS:
        return SingleModePhases::PHASE1;
    case SingleModePhases::PHASE2_BOSS:
        return SingleModePhases::PHASE2;
    case SingleModePhases::PHASE3_BOSS:
        return isAfterPlessieChase ? SingleModePhases::PHASE4 : SingleModePhases::PHASE3;
    case SingleModePhases::PHASE4_BOSS:
        return SingleModePhases::PHASE4;
    default:
        return phase;
    }
}

/**
 * @brief Convert a single-mode phase to its phase number.
 * @param phase Single-mode phase.
 * @return The phase number from 0 to 4.
 */
int phaseNumToInt(SingleModePhases phase) {
    if (phase == SingleModePhases::PHASE0) {
        return 0;
    }

    if (phase < SingleModePhases::PHASE2) {
        return 1;
    }

    if (phase < SingleModePhases::PHASE3) {
        return 2;
    }

    if (phase < SingleModePhases::PHASE4) {
        return 3;
    }

    return 4;
}

/**
 * @brief Convert a single-mode phase to its phase number.
 * @param phase Single-mode phase.
 * @return The phase number from 0 to 4.
 */
int phaseNumToInt(int phase) {
    return phaseNumToInt(SingleModePhases(phase));
}

/**
 * @brief Find a single-mode phase by name.
 * @param pName Phase name.
 * @return The phase, or PHASE1 when the name is unknown.
 */
int phaseNameToInt(const char* pName) {
    for (s32 i = 0; i < SingleModePhases::size(); i++) {
        if (al::isEqualString(pName, SingleModePhases::text(i))) {
            return SingleModePhases(i);
        }
    }

    return SingleModePhases::PHASE1;
}

/**
 * @brief Create the scene of a single-mode phase.
 * @param phase Single-mode phase.
 * @return The created scene.
 */
SingleModeScene* createPhaseScene(int phase) {
    if (isBossPhase(phase)) {
        return new PhaseBossScene();
    }

    return new PhaseScene();
}

/**
 * @brief Check whether a single-mode phase is a boss phase.
 * @param phase Single-mode phase.
 * @return True for a boss phase.
 */
bool isBossPhase(int phase) {
    switch (phase) {
    case SingleModePhases::PHASE1_BOSS:
    case SingleModePhases::PHASE2_BOSS:
    case SingleModePhases::PHASE3_BOSS:
    case SingleModePhases::PHASE4_BOSS:
        return true;
    default:
        return false;
    }
}

/**
 * @brief Convert a map-unit flag to the cutscene it unlocks.
 * @param flag Map-unit flag.
 * @return The cutscene, or -1 when the flag unlocks none.
 */
int convertMapUnitFlagToCutscene(int flag) {
    for (s32 i = 0; i < 9; i++) {
        if (sFlagConversion[i].mMapUnitFlag == flag) {
            return sFlagConversion[i].mCutscene;
        }
    }

    return -1;
}

/**
 * @brief Read the intro cutscene of a single-mode phase.
 * @param phase Single-mode phase.
 * @return The intro cutscene, or -1 when the phase has none.
 */
int getPhaseIntroCutsceneFlag(int phase) {
    switch (phase) {
    case SingleModePhases::PHASE1:
        return 13;
    case SingleModePhases::PHASE2:
        return 14;
    case SingleModePhases::PHASE3:
        return 16;
    case SingleModePhases::PHASE4:
        return 29;
    default:
        return -1;
    }
}

/**
 * @brief Check whether a single-mode phase is a Plessie chase.
 * @param phase Single-mode phase.
 * @return True for a Plessie chase.
 */
NOINLINE bool isPlessieChase(int phase) {
    switch (phase) {
    case SingleModePhases::PHASE3_PLESSIE_CHASE:
    case SingleModePhases::PHASE4_PLESSIE_CHASE:
        return true;
    default:
        return false;
    }
}

} // namespace rc
