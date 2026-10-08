#pragma once

#include <basis/seadTypes.h>

/**
 * @brief Ids of the game's scene objects (al::getSceneObj / al::tryGetSceneObj).
 * @note Only ids whose object is known from reconstructed code are listed.
 */
enum SceneObjID : s32 {
    SceneObjID_CoinRotater = 1,
    SceneObjID_CourseSelectDirector = 2,
    SceneObjID_DrcAssistDirectorList = 4,
    SceneObjID_FootPrintServer = 7,
    SceneObjID_GameDataHolder = 8,
    SceneObjID_GhostPlayerDirector = 9,
    SceneObjID_GreenStarKeeper = 10,
    SceneObjID_IllustItemKeeper = 11,
    SceneObjID_PlayerCooperation = 14,
    SceneObjID_PlayerFireBallAppearWatcher = 15,
    SceneObjID_PlayerGroup = 16,
    SceneObjID_PlayerProcess = 17,
    SceneObjID_PlayerStocker = 18,
    SceneObjID_ScoreHolder = 20,
    SceneObjID_PlayerAliveWatcher = 24,
    SceneObjID_PlayerRetargettingSelector = 25,
    SceneObjID_CameraChangeLayout = 27,
    SceneObjID_GuideGameWindow = 28,
    SceneObjID_ChikaChikaBlockSynchronizer = 29,
    SceneObjID_BlockAssistWatcher = 30,
    SceneObjID_SceneEventMessageSender = 31,
    SceneObjID_FurEnv = 33,
    SceneObjID_ControllerEventWatcher = 34,
    SceneObjID_GoalItemHolder = 36,
    SceneObjID_CatGullHolder = 37,
    SceneObjID_LuckyIslandHolder = 38,
    SceneObjID_IslandAreaWatcher = 39,
    SceneObjID_OceanScenarioList = 42,
    SceneObjID_IslandDataList = 43,
    SceneObjID_IslandKeeper = 44,
    SceneObjID_IslandMap = 45,
    SceneObjID_OceanWater = 46,
    SceneObjID_ShardsWatcherHolder = 47,
    SceneObjID_StampDirector = 48,
    SceneObjID_GigaBellManager = 50,
    SceneObjID_RaidonSurf = 51,
    SceneObjID_CloudBonusWatcher = 52,
    SceneObjID_SingleModeSceneLayout = 53,
    SceneObjID_SuperbViewAreaHolder = 54,
    SceneObjID_PlayerKoopaJr = 56,
    SceneObjID_DisasterModeController = 58,
    SceneObjID_NekoParentHolder = 59,
};
