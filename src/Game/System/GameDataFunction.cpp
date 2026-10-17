#include "System/GameDataFunction.hpp"
#include "Library/Scene/SceneObjHolder.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Scene/SceneObjID.hpp"
#include "System/CourseInfoHolder.hpp"
#include "System/Data/SingleModeData.hpp"
#include "System/Data/SingleModeStockItemArray.hpp"
#include "System/Data/StageDataHolder.hpp"
#include "System/Data/StageDatabaseInfo.hpp"
#include "System/Data/StageListHolder.hpp"
#include "System/Data/StockItemList.hpp"
#include "System/GameDataFile.hpp"
#include "System/GameDataFlagFunction.hpp"
#include "Util/ControlUserUtil.hpp"

namespace {

/// Number of save files of each game mode.
constexpr int cFileNum = 4;

/**
 * @brief Find the furthest world whose start demo was shown in one save file.
 * @param accessor Accessor to a valid game-data holder.
 * @param fileId Save-file slot to check.
 * @return The highest open world, at least 1.
 */
int calcOpenWorldIdMaxInFile(GameDataHolderAccessor accessor, int fileId) {
    for (int worldId = GameDataFunction::getWorldNum(accessor); worldId > 0; worldId--) {
        if (GameDataFlagFunction::isShowWorldStartDemo(accessor, worldId, fileId)) {
            return worldId;
        }
    }

    return 1;
}

} // namespace

/**
 * @brief Fetch the game-data holder registered in a scene.
 * @param pUser Object with access to the scene-object holder.
 * @return The game-data holder.
 */
GameDataHolder* GameDataFunction::getGameDataHolder(const al::IUseSceneObjHolder* pUser) {
    return al::getSceneObj<GameDataHolder>(pUser, SceneObjID_GameDataHolder);
}

/**
 * @brief Fetch the game-data holder registered in a scene-object holder.
 * @param pHolder Scene-object holder to query.
 * @return The game-data holder.
 */
GameDataHolder* GameDataFunction::getGameDataHolder(const al::SceneObjHolder* pHolder) {
    return static_cast<GameDataHolder*>(pHolder->getObj(SceneObjID_GameDataHolder));
}

/**
 * @brief Convert the library's game-data holder into the game's holder.
 * @param pHolder Library-side game-data holder.
 * @return The same object as a game-data holder.
 */
GameDataHolder* GameDataFunction::getGameDataHolder(al::GameDataHolderBase* pHolder) {
    return reinterpret_cast<GameDataHolder*>(pHolder);
}

/**
 * @brief Request or cancel a save.
 * @param writer Writer to a valid game-data holder.
 * @param isRequested True to request a save.
 */
void GameDataFunction::setSaveRequested(GameDataHolderWriter writer, bool isRequested) {
    writer.getHolder()->setSaveRequested(isRequested);
}

/**
 * @brief Check whether a save has been requested.
 * @param accessor Accessor to a valid game-data holder.
 * @return True when a save is pending.
 */
bool GameDataFunction::isSaveRequested(GameDataHolderAccessor accessor) {
    return accessor.getHolder()->isSaveRequested();
}

/**
 * @brief Update the figures of every player.
 * @param writer Writer to a valid game-data holder.
 * @param figureType Figure type to apply.
 */
void GameDataFunction::updatePlayerFigures(GameDataHolderWriter writer, int figureType) {
    writer.getHolder()->updatePlayerFigures(figureType);
}

/**
 * @brief Restart the play-time measurement of a Bowser's Fury file.
 * @param accessor Accessor to a valid game-data holder.
 * @param fileId Save-file slot, or -1 for the last played one.
 */
void GameDataFunction::initTotalPlayTimeSM(GameDataHolderAccessor accessor, int fileId) {
    GameDataHolder* pHolder = accessor.getHolder();
    if (fileId == -1) {
        fileId = pHolder->getLastSingleModePlayingFileID();
    }

    pHolder->getSingleModeDataFile(fileId)->initTotalPlayTimePR();
}

/**
 * @brief Read the total play time of the current Bowser's Fury file.
 * @param accessor Accessor to a valid game-data holder.
 * @param isIncludeCurrentPlay True to add the running session.
 * @return The play time, or zero outside single mode.
 */
s64 GameDataFunction::getTotalPlayTimeSM(GameDataHolderAccessor accessor,
                                          bool isIncludeCurrentPlay) {
    GameDataHolder* pHolder = accessor.getHolder();
    if (!pHolder->isSingleMode()) {
        return 0;
    }

    SingleModeData* pFile =
        pHolder->getSingleModeDataFile(pHolder->getLastSingleModePlayingFileID());
    return pFile->getTotalPlayTimePR(isIncludeCurrentPlay);
}

/**
 * @brief Store the running play time of the current Bowser's Fury file.
 * @param accessor Accessor to a valid game-data holder.
 */
void GameDataFunction::setTotalPlayTimeSM(GameDataHolderAccessor accessor) {
    GameDataHolder* pHolder = accessor.getHolder();
    if (!pHolder->isSingleMode()) {
        return;
    }

    pHolder->getSingleModeDataFile(pHolder->getLastSingleModePlayingFileID())
        ->updateTotalPlayTimePR();
    pHolder->getSingleModeDataFile(pHolder->getLastSingleModePlayingFileID())
        ->updatePhasePlayTime();
}

/**
 * @brief Restart the play-time measurement of a 3D World file.
 * @param accessor Accessor to a valid game-data holder.
 * @param fileId Save-file slot, or -1 for the playing one.
 */
void GameDataFunction::initTotalPlayTimeOG(GameDataHolderAccessor accessor, int fileId) {
    GameDataHolder* pHolder = accessor.getHolder();
    if (pHolder->isSingleMode()) {
        return;
    }

    if (fileId == -1) {
        pHolder->getPlayingFile()->initTotalPlayTimePR();
    } else {
        pHolder->getGameDataFile(fileId)->initTotalPlayTimePR();
    }
}

/**
 * @brief Read the total play time of the playing 3D World file.
 * @param accessor Accessor to a valid game-data holder.
 * @param isIncludeCurrentPlay True to add the running session.
 * @return The play time, or zero in single mode.
 */
s64 GameDataFunction::getTotalPlayTimeOG(GameDataHolderAccessor accessor,
                                          bool isIncludeCurrentPlay) {
    if (accessor.getHolder()->isSingleMode()) {
        return 0;
    }

    return accessor.getHolder()->getPlayingFile()->getTotalPlayTimePR(isIncludeCurrentPlay);
}

/**
 * @brief Store the running play time of the playing 3D World file.
 * @param accessor Accessor to a valid game-data holder.
 */
void GameDataFunction::setTotalPlayTimeOG(GameDataHolderAccessor accessor) {
    if (accessor.getHolder()->isSingleMode()) {
        return;
    }

    accessor.getHolder()->getPlayingFile()->updateTotalPlayTimePR();
}

/**
 * @brief Check whether a course identifier is the invalid sentinel.
 * @param courseId Course identifier to check.
 * @return True for the invalid identifier.
 */
bool GameDataFunction::isInvalidCourseId(int courseId) {
    return courseId == 0;
}

/**
 * @brief Return the sentinel course identifier.
 * @return Zero, which does not identify a course.
 */
int GameDataFunction::getInvalidCourseId() {
    return 0;
}

/**
 * @brief Read the number of lives.
 * @param accessor Accessor to a valid game-data holder.
 * @return The life count; always 1 in single mode.
 */
int GameDataFunction::getPlayerLife(GameDataHolderAccessor accessor) {
    if (accessor.getHolder()->isSingleMode()) {
        return 1;
    }

    return accessor.getHolder()->getPlayingFile()->getPlayerLife();
}

/**
 * @brief Add lives.
 * @param writer Writer to a valid game-data holder.
 * @param life Number of lives to add.
 * @return The resulting life count; always 1 in single mode.
 */
int GameDataFunction::addPlayerLife(GameDataHolderWriter writer, int life) {
    if (writer.getHolder()->isSingleMode()) {
        return 1;
    }

    return writer.getHolder()->getPlayingFile()->addPlayerLife(life);
}

/**
 * @brief Check whether the player ran out of lives.
 * @param accessor Accessor to a valid game-data holder.
 * @return True on game over.
 */
bool GameDataFunction::isGameOver(GameDataHolderAccessor accessor) {
    if (accessor.getHolder()->isSingleMode()) {
        return false;
    }

    return accessor.getHolder()->getPlayingFile()->isGameOver();
}

/**
 * @brief Name the character selected by a character bit field.
 * @param accessor Accessor to a valid game-data holder.
 * @param flag One bit per character type.
 * @return The character name, or "Multi" when several characters are set.
 */
const char* GameDataFunction::getPlayerCharacterNameFromBitFlag(GameDataHolderAccessor accessor,
                                                                u32 flag) {
    int characterType = -1;
    int characterNum = 0;
    for (int i = 0; i < 4; i++) {
        if ((flag & (1 << i)) != 0) {
            characterNum++;
            if (characterNum >= 2) {
                return "Multi";
            }

            characterType = i;
        }
    }

    return rc::getControlUserCharacterName(accessor, characterType);
}

/**
 * @brief Notify that the title scene started.
 * @param writer Writer to a valid game-data holder.
 */
void GameDataFunction::onStartTitleScene(GameDataHolderWriter writer) {
    writer.getHolder()->setSkipStartSave(true);
}

/**
 * @brief Notify that the title scene ended.
 * @param writer Writer to a valid game-data holder.
 */
void GameDataFunction::onEndTitleScene(GameDataHolderWriter writer) {
    writer.getHolder()->setSkipStartSave(false);
}

/**
 * @brief Recover from a game over.
 * @param writer Writer to a valid game-data holder.
 */
void GameDataFunction::recoverGameOver(GameDataHolderWriter writer) {
    if (writer.getHolder()->isSingleMode()) {
        return;
    }

    writer.getHolder()->getPlayingFile()->recoverGameOver();
}

/**
 * @brief Recover from a game over in the golden express.
 * @param writer Writer to a valid game-data holder.
 */
void GameDataFunction::recoverGameOverFromGoldenExpress(GameDataHolderWriter writer) {
    if (writer.getHolder()->isSingleMode()) {
        return;
    }

    writer.getHolder()->getPlayingFile()->recoverGameOverFromGoldenExpress();
}

/**
 * @brief Start a 3D World stage.
 * @param writer Writer to a valid game-data holder.
 * @param worldId World of the stage.
 * @param stageId Stage inside the world.
 */
void GameDataFunction::startStage(GameDataHolderWriter writer, int worldId, int stageId) {
    if (writer.getHolder()->isSingleMode()) {
        return;
    }

    writer.getHolder()->getPlayingFile()->startStage(worldId, stageId);
}

/**
 * @brief Start a Bowser's Fury stage.
 * @param writer Writer to a valid game-data holder.
 * @param worldId World of the stage.
 * @param stageId Stage inside the world.
 */
void GameDataFunction::startSingleModeStage(GameDataHolderWriter writer, int worldId,
                                            int stageId) {
    if (!writer.getHolder()->isSingleMode()) {
        return;
    }

    writer.getHolder()->getSingleFile()->startStage(worldId, stageId);
}

/**
 * @brief Notify the active save file that a stage started.
 * @param writer Writer to a valid game-data holder.
 */
void GameDataFunction::onStageStart(GameDataHolderWriter writer) {
    if (writer.getHolder()->isSingleMode()) {
        writer.getHolder()->getSingleFile()->onStageStart();
    } else {
        writer.getHolder()->getPlayingFile()->onStageStart();
    }
}

/**
 * @brief Notify the active save file that a stage ended.
 * @param writer Writer to a valid game-data holder.
 */
void GameDataFunction::onStageEnd(GameDataHolderWriter writer) {
    if (writer.getHolder()->isSingleMode()) {
        writer.getHolder()->getSingleFile()->onStageEnd();
    } else {
        writer.getHolder()->getPlayingFile()->onStageEnd();
    }
}

/**
 * @brief Notify that a save started in the course select.
 * @param writer Writer to a valid game-data holder.
 */
void GameDataFunction::onSaveStartInCourseSelect(GameDataHolderWriter writer) {
    if (writer.getHolder()->isSingleMode()) {
        return;
    }

    writer.getHolder()->getPlayingFile()->onSaveStartInCourseSelect();
}

/**
 * @brief Notify a warp to another world.
 * @param writer Writer to a valid game-data holder.
 * @param worldId Destination world.
 */
void GameDataFunction::onWorldWarp(GameDataHolderWriter writer, int worldId) {
    if (writer.getHolder()->isSingleMode()) {
        return;
    }

    writer.getHolder()->getPlayingFile()->onWorldWarp(worldId);
}

/**
 * @brief Notify the warp-pipe demo between worlds.
 * @param writer Writer to a valid game-data holder.
 */
void GameDataFunction::onWorldWarpDokanDemo(GameDataHolderWriter writer) {
    if (writer.getHolder()->isSingleMode()) {
        return;
    }

    writer.getHolder()->getPlayingFile()->onWorldWarpDokanDemo();
}

/**
 * @brief Notify a return to the title.
 * @param writer Writer to a valid game-data holder.
 * @param worldId World being left.
 */
void GameDataFunction::onGotoTitle(GameDataHolderWriter writer, int worldId) {
    if (writer.getHolder()->isSingleMode()) {
        return;
    }

    writer.getHolder()->getPlayingFile()->onGotoTitle(worldId);
}

/**
 * @brief Record the clear of the current stage.
 * @param writer Writer to a valid game-data holder.
 */
void GameDataFunction::clearStage(GameDataHolderWriter writer) {
    if (writer.getHolder()->isSingleMode()) {
        return;
    }

    writer.getHolder()->getPlayingFile()->clearStage();
}

/**
 * @brief Record the clear of the current stage through a world warp.
 * @param writer Writer to a valid game-data holder.
 */
void GameDataFunction::clearStageWorldWarp(GameDataHolderWriter writer) {
    if (writer.getHolder()->isSingleMode()) {
        return;
    }

    writer.getHolder()->getPlayingFile()->clearStageWorldWarp();
}

/**
 * @brief Record leaving the current stage.
 * @param writer Writer to a valid game-data holder.
 */
void GameDataFunction::retireStage(GameDataHolderWriter writer) {
    if (writer.getHolder()->isSingleMode()) {
        return;
    }

    writer.getHolder()->getPlayingFile()->retireStage();
}

/**
 * @brief Record leaving the current stage through its exit door.
 * @param writer Writer to a valid game-data holder.
 */
void GameDataFunction::retireStageExitDoor(GameDataHolderWriter writer) {
    if (writer.getHolder()->isSingleMode()) {
        return;
    }

    writer.getHolder()->getPlayingFile()->retireStageExitDoor();
}

/**
 * @brief Restart the current stage.
 * @param writer Writer to a valid game-data holder.
 */
void GameDataFunction::restartStage(GameDataHolderWriter writer) {
    if (writer.getHolder()->isSingleMode()) {
        return;
    }

    writer.getHolder()->getPlayingFile()->restartStage();
}

/**
 * @brief Reset the score of the current stage.
 * @param writer Writer to a valid game-data holder.
 */
void GameDataFunction::resetStageScore(GameDataHolderWriter writer) {
    if (writer.getHolder()->isSingleMode()) {
        return;
    }

    writer.getHolder()->getPlayingFile()->resetStageScore();
}

/**
 * @brief Restart the current Captain Toad stage.
 * @param writer Writer to a valid game-data holder.
 */
void GameDataFunction::restartKinopioBrigade(GameDataHolderWriter writer) {
    if (writer.getHolder()->isSingleMode()) {
        return;
    }

    writer.getHolder()->getPlayingFile()->restartKinopioBrigade();
}

/**
 * @brief Restart the current mystery box.
 * @param writer Writer to a valid game-data holder.
 */
void GameDataFunction::restartMysteryBox(GameDataHolderWriter writer) {
    if (writer.getHolder()->isSingleMode()) {
        return;
    }

    writer.getHolder()->getPlayingFile()->restartMysteryBox();
}

/**
 * @brief Restart the current mystery box after its timer ran out.
 * @param writer Writer to a valid game-data holder.
 */
void GameDataFunction::restartTimeupMysteryBox(GameDataHolderWriter writer) {
    if (writer.getHolder()->isSingleMode()) {
        return;
    }

    writer.getHolder()->getPlayingFile()->restartTimeupMysteryBox();
}

/**
 * @brief Enter the current stage again.
 * @param writer Writer to a valid game-data holder.
 */
void GameDataFunction::reenterStage(GameDataHolderWriter writer) {
    if (writer.getHolder()->isSingleMode()) {
        return;
    }

    writer.getHolder()->getPlayingFile()->reenterStage();
}

/**
 * @brief Record the world start demo.
 * @param writer Writer to a valid game-data holder.
 * @param worldId World whose demo is played.
 */
void GameDataFunction::playWorldStartDemo(GameDataHolderWriter writer, int worldId) {
    if (writer.getHolder()->isSingleMode()) {
        return;
    }

    writer.getHolder()->getPlayingFile()->playWorldStartDemo(worldId);
}

/**
 * @brief Update the player holding the best score.
 * @param writer Writer to a valid game-data holder.
 */
void GameDataFunction::updateBestScoreUser(GameDataHolderWriter writer) {
    if (writer.getHolder()->isSingleMode()) {
        return;
    }

    writer.getHolder()->getPlayingFile()->updateBestScoreUser();
}

/**
 * @brief Set the player holding the best score.
 * @param writer Writer to a valid game-data holder.
 * @param userId Player holding the best score.
 */
void GameDataFunction::setBestScoreUserId(GameDataHolderWriter writer, int userId) {
    if (writer.getHolder()->isSingleMode()) {
        return;
    }

    writer.getHolder()->getPlayingFile()->setBestScoreUserId(userId);
}

/**
 * @brief Send a stage play report.
 * @param writer Writer to a valid game-data holder.
 * @param eventId Event to report.
 */
void GameDataFunction::playReportStageEvent(GameDataHolderWriter writer, int eventId) {
    if (writer.getHolder()->isSingleMode()) {
        return;
    }

    writer.getHolder()->getPlayingFile()->playReportStageEvent(eventId);
}

/**
 * @brief Send the options play report.
 * @param writer Writer to a valid game-data holder.
 */
void GameDataFunction::playReportOptionsEvent(GameDataHolderWriter writer) {
    GameDataHolder* pHolder = writer.getHolder();
    if (pHolder->isSingleMode()) {
        pHolder->getGameDataFile(pHolder->getLastPlayingFileId())->playReportOptionsEvent();
    } else {
        pHolder->getPlayingFile()->playReportOptionsEvent();
    }
}

/**
 * @brief Count a stage restart chosen from the pause menu.
 * @param writer Writer to a valid game-data holder.
 */
void GameDataFunction::incMenuRestartCount(GameDataHolderWriter writer) {
    if (writer.getHolder()->isSingleMode()) {
        return;
    }

    writer.getHolder()->getPlayingFile()->incMenuRestartCount();
}

/**
 * @brief Store an item in the reserve.
 * @param writer Writer to a valid game-data holder.
 * @param itemId Item to store.
 * @return True if the item was stored.
 */
bool GameDataFunction::stockItem(GameDataHolderWriter writer, int itemId) {
    if (writer.getHolder()->isSingleMode()) {
        return writer.getHolder()->getSingleFile()->getStockItems()->stockItem(itemId);
    }

    return writer.getHolder()->getStageDataHolderPtr()->stockItem(itemId);
}

/**
 * @brief Read a reserve item of the current stage.
 * @param accessor Accessor to a valid game-data holder.
 * @param index Reserve slot.
 * @return The stored item.
 */
int GameDataFunction::getStockItem(GameDataHolderAccessor accessor, int index) {
    return accessor.getHolder()->getStageDataHolder()->getStockItem(index);
}

/**
 * @brief Read a reserve item of the save file.
 * @param accessor Accessor to a valid game-data holder.
 * @param index Reserve slot.
 * @return The stored item.
 */
int GameDataFunction::getStockItemInCourseSelect(GameDataHolderAccessor accessor, int index) {
    return accessor.getHolder()->getPlayingFile()->getStockItemList()->getStockItem(index);
}

/**
 * @brief Use the first reserve item.
 * @param writer Writer to a valid game-data holder.
 */
void GameDataFunction::useStockItem(GameDataHolderWriter writer) {
    if (writer.getHolder()->isSingleMode()) {
        return;
    }

    writer.getHolder()->getStageDataHolderPtr()->useStockItem();
}

/**
 * @brief Read the first reserve item.
 * @param accessor Accessor to a valid game-data holder.
 * @return The stored item, or zero in single mode.
 */
int GameDataFunction::getTopItem(GameDataHolderAccessor accessor) {
    if (accessor.getHolder()->isSingleMode()) {
        return 0;
    }

    return accessor.getHolder()->getStageDataHolder()->getStockItem(0);
}

/**
 * @brief Add coins to the active save file.
 * @param writer Writer to a valid game-data holder.
 * @param count Number of coins.
 * @return True if the coin counter wrapped around and earned a reward.
 */
bool GameDataFunction::addCoin(GameDataHolderWriter writer, int count) {
    if (writer.getHolder()->isSingleMode()) {
        return writer.getHolder()->getSingleFile()->addCoin(count);
    }

    return writer.getHolder()->getPlayingFile()->addCoin(count);
}

/**
 * @brief Read the coin count of the active save file.
 * @param accessor Accessor to a valid game-data holder.
 * @return The coin count.
 */
int GameDataFunction::getCoinNum(GameDataHolderAccessor accessor) {
    if (accessor.getHolder()->isSingleMode()) {
        return accessor.getHolder()->getSingleFile()->getCoinNum();
    }

    return accessor.getHolder()->getPlayingFile()->getCoinNum();
}

/**
 * @brief Read the total score of the current stage.
 * @param accessor Accessor to a valid game-data holder.
 * @return The score, or zero in single mode.
 */
int GameDataFunction::getTotalScore(GameDataHolderAccessor accessor) {
    if (accessor.getHolder()->isSingleMode()) {
        return 0;
    }

    return accessor.getHolder()->getStageDataHolder()->getTotalScore();
}

/**
 * @brief Read the score of one player in the current stage.
 * @param accessor Accessor to a valid game-data holder.
 * @param userId Player to read.
 * @return The score, or zero in single mode.
 */
int GameDataFunction::getUserScore(GameDataHolderAccessor accessor, int userId) {
    if (accessor.getHolder()->isSingleMode()) {
        return 0;
    }

    return accessor.getHolder()->getStageDataHolder()->getScore(userId);
}

/**
 * @brief Read the number of worlds.
 * @param accessor Accessor to a valid game-data holder.
 * @return The number of worlds.
 */
int GameDataFunction::getWorldNum(GameDataHolderAccessor accessor) {
    return accessor.getHolder()->getStageList()->getWorldNum();
}

/**
 * @brief Read the stage database size.
 * @param accessor Accessor to a valid game-data holder.
 * @return The number of database records.
 */
int GameDataFunction::getCourseTotalNum(GameDataHolderAccessor accessor) {
    return accessor.getHolder()->getStageList()->getCourseTotalNum();
}

/**
 * @brief Compute the course identifier of a stage.
 * @param accessor Accessor to a valid game-data holder.
 * @param worldId World of the stage.
 * @param stageId Stage inside the world.
 * @return The course identifier.
 */
int GameDataFunction::calcCourseId(GameDataHolderAccessor accessor, int worldId, int stageId) {
    return accessor.getHolder()->getStageList()->calcCourseId(worldId, stageId);
}

/**
 * @brief Compute the course identifier of a stage that may not exist.
 * @param accessor Accessor to a valid game-data holder.
 * @param worldId World of the stage.
 * @param stageId Stage inside the world.
 * @return The course identifier, or the invalid identifier.
 */
int GameDataFunction::tryCalcCourseId(GameDataHolderAccessor accessor, int worldId,
                                      int stageId) {
    return accessor.getHolder()->getStageList()->tryCalcCourseId(worldId, stageId);
}

/**
 * @brief Find the course after which the fairy princesses leave the castle.
 * @param accessor Accessor to a valid game-data holder.
 * @return The course identifier, or zero in single mode.
 */
int GameDataFunction::calcStartFairyPrincessLeaveCastleCourseId(GameDataHolderAccessor accessor) {
    if (accessor.getHolder()->isSingleMode()) {
        return 0;
    }

    return findKoopaCastleCourseId(accessor, 7);
}

/**
 * @brief Find the Bowser castle of a world.
 * @param accessor Accessor to a valid game-data holder.
 * @param worldId World to search.
 * @return The course identifier.
 */
int GameDataFunction::findKoopaCastleCourseId(GameDataHolderAccessor accessor, int worldId) {
    return accessor.getHolder()->getStageList()->findKoopaCastleCourseId(worldId);
}

/**
 * @brief Find the course where Rosalina appears.
 * @param accessor Accessor to a valid game-data holder.
 * @return The course identifier, or -2 in single mode.
 */
int GameDataFunction::calcRosettaAppearanceCourseId(GameDataHolderAccessor accessor) {
    if (accessor.getHolder()->isSingleMode()) {
        return -2;
    }

    return calcCourseId(accessor, 9, 2);
}

/**
 * @brief Find the course unlocking the network settings.
 * @param accessor Accessor to a valid game-data holder.
 * @return The course identifier, or 0 in single mode.
 */
int GameDataFunction::calcOpenNetworkSettingCourseId(GameDataHolderAccessor accessor) {
    if (accessor.getHolder()->isSingleMode()) {
        return 0;
    }

    return calcCourseId(accessor, 2, 1);
}

/**
 * @brief Find the course unlocking the remixed worlds.
 * @param accessor Accessor to a valid game-data holder.
 * @return The course identifier, or 0 in single mode.
 */
int GameDataFunction::calcOpenWorldArrangeCourseId(GameDataHolderAccessor accessor) {
    if (accessor.getHolder()->isSingleMode()) {
        return 0;
    }

    return calcCourseId(accessor, 9, 9);
}

/**
 * @brief Find the course showing the world-jump hint.
 * @param accessor Accessor to a valid game-data holder.
 * @return The course identifier, or 0 in single mode.
 */
int GameDataFunction::calcShowWorldJumpMenuInfoCourseId(GameDataHolderAccessor accessor) {
    if (accessor.getHolder()->isSingleMode()) {
        return 0;
    }

    return calcCourseId(accessor, 3, 1);
}

/**
 * @brief Find the course showing the second world-jump hint.
 * @param accessor Accessor to a valid game-data holder.
 * @return The course identifier, or 0 in single mode.
 */
int GameDataFunction::calcShowWorldJumpMenuInfo2ndCourseId(GameDataHolderAccessor accessor) {
    if (accessor.getHolder()->isSingleMode()) {
        return 0;
    }

    return calcCourseId(accessor, 6, 1);
}

/**
 * @brief Find the course explaining character changes.
 * @param accessor Accessor to a valid game-data holder.
 * @return The course identifier, or 0 in single mode.
 */
int GameDataFunction::calcShowCharacterChangeExplainCourseId(GameDataHolderAccessor accessor) {
    if (accessor.getHolder()->isSingleMode()) {
        return 0;
    }

    return calcCourseId(accessor, 2, 4);
}

/**
 * @brief Find the course showing the network guide.
 * @param accessor Accessor to a valid game-data holder.
 * @return The course identifier, or 0 in single mode.
 */
int GameDataFunction::calcShowNetworkGuideCourseId(GameDataHolderAccessor accessor) {
    if (accessor.getHolder()->isSingleMode()) {
        return 0;
    }

    return calcCourseId(accessor, 1, 3);
}

/**
 * @brief Find the course showing the amiibo guide.
 * @param accessor Accessor to a valid game-data holder.
 * @return The course identifier, or 0 in single mode.
 */
int GameDataFunction::calcShowAmiiboGuideCourseId(GameDataHolderAccessor accessor) {
    if (accessor.getHolder()->isSingleMode()) {
        return 0;
    }

    return calcCourseId(accessor, 2, 1);
}

/**
 * @brief Find the course showing the snapshot guide.
 * @param accessor Accessor to a valid game-data holder.
 * @return The course identifier, or 0 in single mode.
 */
int GameDataFunction::calcShowSnapshotGuideCourseId(GameDataHolderAccessor accessor) {
    if (accessor.getHolder()->isSingleMode()) {
        return 0;
    }

    return calcCourseId(accessor, 2, 3);
}

/**
 * @brief Find the course showing the touch guide.
 * @param accessor Accessor to a valid game-data holder.
 * @return The course identifier, or 0 in single mode.
 */
int GameDataFunction::calcShowTouchGuideCourseId(GameDataHolderAccessor accessor) {
    if (accessor.getHolder()->isSingleMode()) {
        return 0;
    }

    return calcCourseId(accessor, 2, 2);
}

/**
 * @brief Split a course identifier into world and stage.
 * @param accessor Accessor to a valid game-data holder.
 * @param pWorldId Receives the world.
 * @param pStageId Receives the stage.
 * @param courseId Course identifier to split.
 */
void GameDataFunction::calcWorldAndStageId(GameDataHolderAccessor accessor, int* pWorldId,
                                           int* pStageId, int courseId) {
    accessor.getHolder()->getStageList()->calcWorldAndStageId(pWorldId, pStageId, courseId);
}

/**
 * @brief Find a stage by its unique course identifier.
 * @param accessor Accessor to a valid game-data holder.
 * @param courseId Course identifier to find.
 * @return The stage record, or nullptr when no course matches.
 */
StageDatabaseInfo* GameDataFunction::findStageDatabaseInfo(GameDataHolderAccessor accessor,
                                                          int courseId) {
    return accessor.getHolder()->getStageList()->findStageDatabaseInfo(courseId);
}

/**
 * @brief Find a world record.
 * @param accessor Accessor to a valid game-data holder.
 * @param worldId World to find.
 * @return The world record.
 */
WorldInfo* GameDataFunction::findWorldInfo(GameDataHolderAccessor accessor, int worldId) {
    return accessor.getHolder()->getStageList()->findWorldInfo(worldId);
}

/**
 * @brief Read the number of Green Stars needed to unlock a course.
 * @param accessor Accessor to a valid game-data holder.
 * @param courseId Course to read.
 * @return The Green Star count, or zero in single mode.
 */
int GameDataFunction::findCourseLockGreenStarNum(GameDataHolderAccessor accessor, int courseId) {
    if (accessor.getHolder()->isSingleMode()) {
        return 0;
    }

    return findStageDatabaseInfo(accessor, courseId)->getLockGreenStarNum();
}

/**
 * @brief Read the number of Green Stars in a course.
 * @param accessor Accessor to a valid game-data holder.
 * @param courseId Course to read.
 * @return The Green Star count, or zero in single mode.
 */
int GameDataFunction::findCourseGreenStarNum(GameDataHolderAccessor accessor, int courseId) {
    if (accessor.getHolder()->isSingleMode()) {
        return 0;
    }

    return findStageDatabaseInfo(accessor, courseId)->getGreenStarNum();
}

/**
 * @brief Count the courses of a world.
 * @param accessor Accessor to a valid game-data holder.
 * @param worldId World to count.
 * @return The number of courses, or zero for an unknown world or in single mode.
 */
int GameDataFunction::calcWorldCourseNum(GameDataHolderAccessor accessor, int worldId) {
    if (accessor.getHolder()->isSingleMode()) {
        return 0;
    }

    WorldInfo* pWorld = findWorldInfo(accessor, worldId);
    return pWorld != nullptr ? pWorld->mStageCount : 0;
}

/**
 * @brief Read the resource name of a stage inside a world.
 * @param accessor Accessor to a valid game-data holder.
 * @param worldId World of the stage.
 * @param index Index of the stage in the world.
 * @return The stage name, or an empty string in single mode.
 */
const char* GameDataFunction::getStageNameFromWorld(GameDataHolderAccessor accessor, int worldId,
                                                    int index) {
    if (accessor.getHolder()->isSingleMode()) {
        return "";
    }

    return findWorldInfo(accessor, worldId)->getStageInfoByIndex(index)->getStageName();
}

/**
 * @brief Find the world containing a stage.
 * @param accessor Accessor to a valid game-data holder.
 * @param pStageName Stage resource name.
 * @return The world, -1 when no stage matches, or zero in single mode.
 */
int GameDataFunction::tryFindWorldIdFromStageName(GameDataHolderAccessor accessor,
                                                  const char* pStageName) {
    if (accessor.getHolder()->isSingleMode()) {
        return 0;
    }

    const int worldNum = getWorldNum(accessor);
    for (int i = 0; i < worldNum; i++) {
        const int worldId = i + 1;
        WorldInfo* pWorld = findWorldInfo(accessor, worldId);
        const int stageNum = pWorld->mStageCount;
        for (int j = 0; j < stageNum; j++) {
            if (al::isEqualString(pStageName, pWorld->getStageInfoByIndex(j)->getStageName())) {
                return worldId;
            }
        }
    }

    return -1;
}

/**
 * @brief Check whether the best time of a course may be shown.
 * @param accessor Accessor to a valid game-data holder.
 * @param courseId Course to check.
 * @return True when the best time can be shown.
 */
bool GameDataFunction::isEnableShowBestTimeStage(GameDataHolderAccessor accessor, int courseId) {
    if (accessor.getHolder()->isSingleMode()) {
        return false;
    }

    if (!GameDataFlagFunction::isAlreadyOpenShowBestTime(accessor)) {
        return false;
    }

    if (isStageContinuousMysteryBox(accessor, courseId)) {
        return false;
    }

    return true;
}

/**
 * @brief Check whether a course is a continuous mystery box.
 * @param accessor Accessor to a valid game-data holder.
 * @param courseId Course to check.
 * @return True when the course matches; false in single mode.
 */
bool GameDataFunction::isStageContinuousMysteryBox(GameDataHolderAccessor accessor, int courseId) {
    if (accessor.getHolder()->isSingleMode()) {
        return false;
    }

    return findStageDatabaseInfo(accessor, courseId)->isContinuousMysteryBox();
}

/**
 * @brief Check whether the playing course is a Captain Toad stage.
 * @param accessor Accessor to a valid game-data holder.
 * @return True for a Captain Toad stage.
 */
bool GameDataFunction::isCurrentStageKinopioBrigade(GameDataHolderAccessor accessor) {
    return isStageKinopioBrigade(accessor, getPlayingCourseId(accessor));
}

/**
 * @brief Read the course being played.
 * @param accessor Accessor to a valid game-data holder.
 * @return The course identifier, or -1 in single mode.
 */
int GameDataFunction::getPlayingCourseId(GameDataHolderAccessor accessor) {
    if (accessor.getHolder()->isSingleMode()) {
        return -1;
    }

    return accessor.getHolder()->getPlayingFile()->getPlayingCourseId();
}

/**
 * @brief Check whether a course is a Captain Toad stage.
 * @param accessor Accessor to a valid game-data holder.
 * @param courseId Course to check.
 * @return True when the course matches; false in single mode.
 */
bool GameDataFunction::isStageKinopioBrigade(GameDataHolderAccessor accessor, int courseId) {
    if (accessor.getHolder()->isSingleMode()) {
        return false;
    }

    return findStageDatabaseInfo(accessor, courseId)->isKinopioBrigade();
}

/**
 * @brief Check whether the playing course is a gatekeeper stage.
 * @param accessor Accessor to a valid game-data holder.
 * @return True for a gatekeeper stage.
 */
bool GameDataFunction::isCurrentStageGateKeeper(GameDataHolderAccessor accessor) {
    return isStageGateKeeper(accessor, getPlayingCourseId(accessor));
}

/**
 * @brief Check whether Bowser's Fury mode is active.
 * @param accessor Accessor to a valid game-data holder.
 * @return True for single mode.
 */
bool GameDataFunction::isSingleMode(GameDataHolderAccessor accessor) {
    return accessor.getHolder()->isSingleMode();
}

/**
 * @brief Check whether a course is a gatekeeper stage.
 * @param accessor Accessor to a valid game-data holder.
 * @param courseId Course to check.
 * @return True when the course matches; false in single mode.
 */
bool GameDataFunction::isStageGateKeeper(GameDataHolderAccessor accessor, int courseId) {
    if (accessor.getHolder()->isSingleMode()) {
        return false;
    }

    return findStageDatabaseInfo(accessor, courseId)->isGateKeeper();
}

/**
 * @brief Check whether a course is an event stage.
 * @param accessor Accessor to a valid game-data holder.
 * @param courseId Course to check.
 * @return True when the course matches; false in single mode.
 */
bool GameDataFunction::isStageEvent(GameDataHolderAccessor accessor, int courseId) {
    if (accessor.getHolder()->isSingleMode()) {
        return false;
    }

    return findStageDatabaseInfo(accessor, courseId)->isEvent();
}

/**
 * @brief Check whether a course is a normal stage.
 * @param accessor Accessor to a valid game-data holder.
 * @param courseId Course to check.
 * @return True when the course matches; true in single mode.
 */
bool GameDataFunction::isStageNormal(GameDataHolderAccessor accessor, int courseId) {
    if (accessor.getHolder()->isSingleMode()) {
        return true;
    }

    return findStageDatabaseInfo(accessor, courseId)->isNormal();
}

/**
 * @brief Check whether a course is a gatekeeper stage with a goal pole.
 * @param accessor Accessor to a valid game-data holder.
 * @param courseId Course to check.
 * @return True when the course matches; false in single mode.
 */
bool GameDataFunction::isStageGateKeeperGoalPole(GameDataHolderAccessor accessor, int courseId) {
    if (accessor.getHolder()->isSingleMode()) {
        return false;
    }

    return findStageDatabaseInfo(accessor, courseId)->isGateKeeperGoalPole();
}

/**
 * @brief Check whether a course is a gatekeeper stage without a goal pole.
 * @param accessor Accessor to a valid game-data holder.
 * @param courseId Course to check.
 * @return True when the course matches; false in single mode.
 */
bool GameDataFunction::isStageGateKeeperNoGoalPole(GameDataHolderAccessor accessor, int courseId) {
    if (accessor.getHolder()->isSingleMode()) {
        return false;
    }

    return findStageDatabaseInfo(accessor, courseId)->isGateKeeperNoGoalPole();
}

/**
 * @brief Check whether a course is a casino room.
 * @param accessor Accessor to a valid game-data holder.
 * @param courseId Course to check.
 * @return True when the course matches; false in single mode.
 */
bool GameDataFunction::isStageCasinoRoom(GameDataHolderAccessor accessor, int courseId) {
    if (accessor.getHolder()->isSingleMode()) {
        return false;
    }

    return findStageDatabaseInfo(accessor, courseId)->isCasinoRoom();
}

/**
 * @brief Check whether a course is a Toad house.
 * @param accessor Accessor to a valid game-data holder.
 * @param courseId Course to check.
 * @return True when the course matches; false in single mode.
 */
bool GameDataFunction::isStageKinopioHouse(GameDataHolderAccessor accessor, int courseId) {
    if (accessor.getHolder()->isSingleMode()) {
        return false;
    }

    return findStageDatabaseInfo(accessor, courseId)->isKinopioHouse();
}

/**
 * @brief Check whether a course is a hidden Toad house.
 * @param accessor Accessor to a valid game-data holder.
 * @param courseId Course to check.
 * @return True when the course matches; false in single mode.
 */
bool GameDataFunction::isStageKinopioHouseHide(GameDataHolderAccessor accessor, int courseId) {
    if (accessor.getHolder()->isSingleMode()) {
        return false;
    }

    return findStageDatabaseInfo(accessor, courseId)->isKinopioHouseHide();
}

/**
 * @brief Check whether a course is a sprixie house.
 * @param accessor Accessor to a valid game-data holder.
 * @param courseId Course to check.
 * @return True when the course matches; false in single mode.
 */
bool GameDataFunction::isStageFairyHouse(GameDataHolderAccessor accessor, int courseId) {
    if (accessor.getHolder()->isSingleMode()) {
        return false;
    }

    return findStageDatabaseInfo(accessor, courseId)->isFairyHouse();
}

/**
 * @brief Check whether a course is a hidden pipe room.
 * @param accessor Accessor to a valid game-data holder.
 * @param courseId Course to check.
 * @return True when the course matches; false in single mode.
 */
bool GameDataFunction::isStageDokanHide(GameDataHolderAccessor accessor, int courseId) {
    if (accessor.getHolder()->isSingleMode()) {
        return false;
    }

    return findStageDatabaseInfo(accessor, courseId)->isDokanHide();
}

/**
 * @brief Check whether a course needs the touch screen.
 * @param accessor Accessor to a valid game-data holder.
 * @param courseId Course to check.
 * @return True when the course matches; false in single mode.
 */
bool GameDataFunction::isStageUseDrc(GameDataHolderAccessor accessor, int courseId) {
    if (accessor.getHolder()->isSingleMode()) {
        return false;
    }

    return findStageDatabaseInfo(accessor, courseId)->isNeedDrc();
}

/**
 * @brief Check whether a course is the golden express.
 * @param accessor Accessor to a valid game-data holder.
 * @param courseId Course to check.
 * @return True when the course matches; false in single mode.
 */
bool GameDataFunction::isStageGoldenExpress(GameDataHolderAccessor accessor, int courseId) {
    if (accessor.getHolder()->isSingleMode()) {
        return false;
    }

    return findStageDatabaseInfo(accessor, courseId)->isGoldenExpress();
}

/**
 * @brief Check whether a course is a Bowser castle.
 * @param accessor Accessor to a valid game-data holder.
 * @param courseId Course to check.
 * @return True when the course matches; false in single mode.
 */
bool GameDataFunction::isStageKoopaCastle(GameDataHolderAccessor accessor, int courseId) {
    if (accessor.getHolder()->isSingleMode()) {
        return false;
    }

    return findStageDatabaseInfo(accessor, courseId)->isKoopaCastle();
}

/**
 * @brief Check whether a course is a normal Bowser castle.
 * @param accessor Accessor to a valid game-data holder.
 * @param courseId Course to check.
 * @return True when the course matches; false in single mode.
 */
bool GameDataFunction::isStageKoopaCastleNormal(GameDataHolderAccessor accessor, int courseId) {
    if (accessor.getHolder()->isSingleMode()) {
        return false;
    }

    return findStageDatabaseInfo(accessor, courseId)->isKoopaCastleNormal();
}

/**
 * @brief Check whether a course is a Bowser tank castle.
 * @param accessor Accessor to a valid game-data holder.
 * @param courseId Course to check.
 * @return True when the course matches; false in single mode.
 */
bool GameDataFunction::isStageKoopaCastleTank(GameDataHolderAccessor accessor, int courseId) {
    if (accessor.getHolder()->isSingleMode()) {
        return false;
    }

    return findStageDatabaseInfo(accessor, courseId)->isKoopaCastleTank();
}

/**
 * @brief Check whether a course is a Bowser express castle.
 * @param accessor Accessor to a valid game-data holder.
 * @param courseId Course to check.
 * @return True when the course matches; false in single mode.
 */
bool GameDataFunction::isStageKoopaCastleExpress(GameDataHolderAccessor accessor, int courseId) {
    if (accessor.getHolder()->isSingleMode()) {
        return false;
    }

    return findStageDatabaseInfo(accessor, courseId)->isKoopaCastleExpress();
}

/**
 * @brief Check whether a course is a normal Bowser express castle.
 * @param accessor Accessor to a valid game-data holder.
 * @param courseId Course to check.
 * @return True when the course matches; false in single mode.
 */
bool GameDataFunction::isStageKoopaCastleExpressNormal(GameDataHolderAccessor accessor,
                                                       int courseId) {
    if (accessor.getHolder()->isSingleMode()) {
        return false;
    }

    return findStageDatabaseInfo(accessor, courseId)->isKoopaCastleExpressNormal();
}

/**
 * @brief Check whether a course is a Bowser fortress.
 * @param accessor Accessor to a valid game-data holder.
 * @param courseId Course to check.
 * @return True when the course matches; false in single mode.
 */
bool GameDataFunction::isStageKoopaCastleFortress(GameDataHolderAccessor accessor, int courseId) {
    if (accessor.getHolder()->isSingleMode()) {
        return false;
    }

    return findStageDatabaseInfo(accessor, courseId)->isKoopaCastleFortress();
}

/**
 * @brief Check whether a course is the Champion's Road.
 * @param accessor Accessor to a valid game-data holder.
 * @param courseId Course to check.
 * @return True when the course matches; false in single mode.
 */
bool GameDataFunction::isStageChampionShip(GameDataHolderAccessor accessor, int courseId) {
    if (accessor.getHolder()->isSingleMode()) {
        return false;
    }

    return findStageDatabaseInfo(accessor, courseId)->isChampionShip();
}

/**
 * @brief Check whether a course was the last one played.
 * @param accessor Accessor to a valid game-data holder.
 * @param courseId Course to check.
 * @return True for the last played course.
 */
bool GameDataFunction::isStageLastPlay(GameDataHolderAccessor accessor, int courseId) {
    if (accessor.getHolder()->isSingleMode()) {
        return false;
    }

    return getLastPlayCourseId(accessor) == courseId;
}

/**
 * @brief Read the last played course.
 * @param accessor Accessor to a valid game-data holder.
 * @return The course identifier, or zero in single mode.
 */
int GameDataFunction::getLastPlayCourseId(GameDataHolderAccessor accessor) {
    if (accessor.getHolder()->isSingleMode()) {
        return 0;
    }

    return accessor.getHolder()->getPlayingFile()->getLastPlayCourseId();
}

/**
 * @brief Check whether a course was the last one played and was cleared for the first time.
 * @param accessor Accessor to a valid game-data holder.
 * @param courseId Course to check.
 * @return True for a first clear of that course.
 */
bool GameDataFunction::isStageLastPlayAndFirstClear(GameDataHolderAccessor accessor,
                                                    int courseId) {
    return isStageLastPlay(accessor, courseId) && isLastPlayCourseFirstClear(accessor);
}

/**
 * @brief Check whether the last played course was cleared for the first time.
 * @param accessor Accessor to a valid game-data holder.
 * @return The file's answer, or false in single mode.
 */
bool GameDataFunction::isLastPlayCourseFirstClear(GameDataHolderAccessor accessor) {
    if (accessor.getHolder()->isSingleMode()) {
        return false;
    }

    return accessor.getHolder()->getPlayingFile()->isLastPlayCourseFirstClear();
}

/**
 * @brief Read the resource name of a course.
 * @param accessor Accessor to a valid game-data holder.
 * @param courseId Course to read.
 * @return The stage name.
 */
const char* GameDataFunction::findStageName(GameDataHolderAccessor accessor, int courseId) {
    return accessor.getHolder()->getStageList()->findStageName(courseId);
}

/**
 * @brief Check whether a course is shown without a course name.
 * @param accessor Accessor to a valid game-data holder.
 * @param courseId Course to check.
 * @return True for rooms and special stages.
 */
bool GameDataFunction::isStageNoCourseName(GameDataHolderAccessor accessor, int courseId) {
    return isStageCasinoRoom(accessor, courseId) || isStageKinopioHouse(accessor, courseId) ||
           isStageKinopioHouseHide(accessor, courseId) || isStageFairyHouse(accessor, courseId) ||
           isStageGoldenExpress(accessor, courseId);
}

/**
 * @brief Read the course of the final Bowser castle.
 * @param accessor Accessor to a valid game-data holder.
 * @return The course identifier.
 */
int GameDataFunction::getLastKoopaCourseId(GameDataHolderAccessor accessor) {
    return accessor.getHolder()->getStageList()->getLastKoopaCourseId();
}

/**
 * @brief Read the first special course following the final Bowser castle.
 * @param accessor Accessor to a valid game-data holder.
 * @return The course identifier.
 */
int GameDataFunction::getLastKoopaGK1CourseId(GameDataHolderAccessor accessor) {
    return accessor.getHolder()->getStageList()->getLastKoopaGK1CourseId();
}

/**
 * @brief Read the second special course following the final Bowser castle.
 * @param accessor Accessor to a valid game-data holder.
 * @return The course identifier.
 */
int GameDataFunction::getLastKoopaGK2CourseId(GameDataHolderAccessor accessor) {
    return accessor.getHolder()->getStageList()->getLastKoopaGK2CourseId();
}

/**
 * @brief Read the last played Bowser's Fury file.
 * @param accessor Accessor to a valid game-data holder.
 * @return The save-file slot.
 */
int GameDataFunction::getLastSingleModePlayingFileID(GameDataHolderAccessor accessor) {
    return accessor.getHolder()->getLastSingleModePlayingFileID();
}

/**
 * @brief Compute the world being played.
 * @param accessor Accessor to a valid game-data holder.
 * @return The world, or zero in single mode.
 */
int GameDataFunction::calcPlayingWorldId(GameDataHolderAccessor accessor) {
    if (accessor.getHolder()->isSingleMode()) {
        return 0;
    }

    const int courseId = getPlayingCourseId(accessor);
    int worldId = -1;
    int stageId = -1;
    calcWorldAndStageId(accessor, &worldId, &stageId, courseId);
    return worldId;
}

/**
 * @brief Check whether the last played course was cleared.
 * @param accessor Accessor to a valid game-data holder.
 * @return The file's answer, or false in single mode.
 */
bool GameDataFunction::isLastPlayCourseClear(GameDataHolderAccessor accessor) {
    if (accessor.getHolder()->isSingleMode()) {
        return false;
    }

    return accessor.getHolder()->getPlayingFile()->isLastPlayCourseClear();
}

/**
 * @brief Check whether the last played course was cleared through a world warp.
 * @param accessor Accessor to a valid game-data holder.
 * @return The file's answer, or false in single mode.
 */
bool GameDataFunction::isLastPlayCourseClearWorldWarp(GameDataHolderAccessor accessor) {
    if (accessor.getHolder()->isSingleMode()) {
        return false;
    }

    return accessor.getHolder()->getPlayingFile()->isLastPlayCourseClearWorldWarp();
}

/**
 * @brief Check whether the last played course awarded its stamp for the first time.
 * @param accessor Accessor to a valid game-data holder.
 * @return The file's answer, or false in single mode.
 */
bool GameDataFunction::isLastPlayCourseFirstAcquireIllustItem(GameDataHolderAccessor accessor) {
    if (accessor.getHolder()->isSingleMode()) {
        return false;
    }

    return accessor.getHolder()->getPlayingFile()->isLastPlayCourseFirstAcquireIllustItem();
}

/**
 * @brief Check whether the last played course set a new best score.
 * @param accessor Accessor to a valid game-data holder.
 * @return The file's answer, or false in single mode.
 */
bool GameDataFunction::isLastPlayCourseUpdateBestScore(GameDataHolderAccessor accessor) {
    if (accessor.getHolder()->isSingleMode()) {
        return false;
    }

    return accessor.getHolder()->getPlayingFile()->isLastPlayCourseUpdateBestScore();
}

/**
 * @brief Check whether the last played course set a new best time.
 * @param accessor Accessor to a valid game-data holder.
 * @return The file's answer, or false in single mode.
 */
bool GameDataFunction::isLastPlayCourseUpdateBestTime(GameDataHolderAccessor accessor) {
    if (accessor.getHolder()->isSingleMode()) {
        return false;
    }

    return accessor.getHolder()->getPlayingFile()->isLastPlayCourseUpdateBestTime();
}

/**
 * @brief Check whether the progress must be saved.
 * @param accessor Accessor to a valid game-data holder.
 * @return The file's answer, or false in single mode.
 */
bool GameDataFunction::isNeedSave(GameDataHolderAccessor accessor) {
    if (accessor.getHolder()->isSingleMode()) {
        return false;
    }

    return accessor.getHolder()->getPlayingFile()->isNeedSave();
}

/**
 * @brief Check whether a casino room should open.
 * @param accessor Accessor to a valid game-data holder.
 * @return The file's answer, or false in single mode.
 */
bool GameDataFunction::isNeedOpenCasinoRoom(GameDataHolderAccessor accessor) {
    if (accessor.getHolder()->isSingleMode()) {
        return false;
    }

    return accessor.getHolder()->getPlayingFile()->isNeedOpenCasinoRoom();
}

/**
 * @brief Check whether the golden express should open.
 * @param accessor Accessor to a valid game-data holder.
 * @return The file's answer, or false in single mode.
 */
bool GameDataFunction::isNeedOpenGoldenExpress(GameDataHolderAccessor accessor) {
    if (accessor.getHolder()->isSingleMode()) {
        return false;
    }

    return accessor.getHolder()->getPlayingFile()->isNeedOpenGoldenExpress();
}

/**
 * @brief Reset the casino-room counter.
 * @param writer Writer to a valid game-data holder.
 */
void GameDataFunction::resetCasinoRoomCounter(GameDataHolderWriter writer) {
    if (writer.getHolder()->isSingleMode()) {
        return;
    }

    writer.getHolder()->getPlayingFile()->resetCasinoRoomCounter();
}

/**
 * @brief Read the player who set the best score in the last stage.
 * @param accessor Accessor to a valid game-data holder.
 * @return The file's answer, or 0 in single mode.
 */
int GameDataFunction::tryGetLastStageBestScoreUserID(GameDataHolderAccessor accessor) {
    if (accessor.getHolder()->isSingleMode()) {
        return 0;
    }

    return accessor.getHolder()->getPlayingFile()->tryGetLastStageBestScoreUserID();
}

/**
 * @brief Recompute the number of stars shown on the file.
 * @param writer Writer to a valid game-data holder.
 */
void GameDataFunction::updateClearStarLevel(GameDataHolderWriter writer) {
    if (writer.getHolder()->isSingleMode()) {
        return;
    }

    writer.getHolder()->getPlayingFile()->calcClearStarLevel();
}

/**
 * @brief Check whether the per-character clear marks are shown.
 * @param accessor Accessor to a valid game-data holder.
 * @return The file's answer, or false in single mode.
 */
bool GameDataFunction::isEnableClearCharacterInfo(GameDataHolderAccessor accessor) {
    if (accessor.getHolder()->isSingleMode()) {
        return false;
    }

    return accessor.getHolder()->getPlayingFile()->isOpenWorldChampionship();
}

/**
 * @brief Check whether the best-time information is shown.
 * @param accessor Accessor to a valid game-data holder.
 * @return True once the file has a star.
 */
bool GameDataFunction::isEnableBestTimeInfo(GameDataHolderAccessor accessor) {
    if (accessor.getHolder()->isSingleMode()) {
        return false;
    }

    return accessor.getHolder()->getPlayingFile()->getClearStarLevel() > 0;
}

/**
 * @brief Check whether the special worlds are available.
 * @param accessor Accessor to a valid game-data holder.
 * @return True once the file has a star.
 */
bool GameDataFunction::isEnableSpecialWorld(GameDataHolderAccessor accessor) {
    if (accessor.getHolder()->isSingleMode()) {
        return false;
    }

    return accessor.getHolder()->getPlayingFile()->getClearStarLevel() > 0;
}

/**
 * @brief Check whether the gold-star flag is enabled.
 * @param accessor Accessor to a valid game-data holder.
 * @return The file's answer, or false in single mode.
 */
bool GameDataFunction::isEnableFlagGoldStar(GameDataHolderAccessor accessor) {
    if (accessor.getHolder()->isSingleMode()) {
        return false;
    }

    return accessor.getHolder()->getPlayingFile()->isOpenWorldChampionship();
}

/**
 * @brief Find the furthest world whose start demo was shown.
 * @param accessor Accessor to a valid game-data holder.
 * @return The highest open world, at least 1, or zero in single mode.
 */
int GameDataFunction::calcOpenWorldIdMax(GameDataHolderAccessor accessor) {
    if (accessor.getHolder()->isSingleMode()) {
        return 0;
    }

    for (int worldId = getWorldNum(accessor); worldId > 0; worldId--) {
        if (GameDataFlagFunction::isShowWorldStartDemo(accessor, worldId)) {
            return worldId;
        }
    }

    return 1;
}

/**
 * @brief Find the furthest world whose start demo was shown in any save file.
 * @param accessor Accessor to a valid game-data holder.
 * @return The highest open world, at least 1, or zero in single mode.
 */
int GameDataFunction::calcOpenWorldIdMaxAllFile(GameDataHolderAccessor accessor) {
    if (accessor.getHolder()->isSingleMode()) {
        return 0;
    }

    int worldIdMax = calcOpenWorldIdMaxInFile(accessor, 0);
    for (int fileId = 1; fileId < cFileNum; fileId++) {
        const int worldId = calcOpenWorldIdMaxInFile(accessor, fileId);
        worldIdMax = worldIdMax > worldId ? worldIdMax : worldId;
    }

    return worldIdMax;
}

/**
 * @brief Read the number of misses in the playing course.
 * @param accessor Accessor to a valid game-data holder.
 * @return The miss count, or zero in single mode.
 */
int GameDataFunction::getMissCount(GameDataHolderAccessor accessor) {
    if (accessor.getHolder()->isSingleMode()) {
        return 0;
    }

    return CourseInfoFunction::getMissCount(accessor, getPlayingCourseId(accessor));
}

/**
 * @brief Count a miss in the playing course.
 * @param writer Writer to a valid game-data holder.
 */
void GameDataFunction::addMissCount(GameDataHolderWriter writer) {
    if (writer.getHolder()->isSingleMode()) {
        return;
    }

    CourseInfoFunction::addMissCount(writer, getPlayingCourseId(writer));
}

/**
 * @brief Check whether the playing course was cleared with the assist block.
 * @param accessor Accessor to a valid game-data holder.
 * @return True when the assist block was used for the clear.
 */
bool GameDataFunction::isClearWithAssistBlock(GameDataHolderAccessor accessor) {
    if (accessor.getHolder()->isSingleMode()) {
        return false;
    }

    return CourseInfoFunction::isClearWithAssistBlock(accessor, getPlayingCourseId(accessor));
}

/**
 * @brief Record that the assist block was used.
 * @param writer Writer to a valid game-data holder.
 */
void GameDataFunction::setUseAssistBlock(GameDataHolderWriter writer) {
    if (writer.getHolder()->isSingleMode()) {
        return;
    }

    writer.getHolder()->getStageDataHolderPtr()->setUseAssistBlock();
}

/**
 * @brief Check whether the player is inside a superb-view area.
 * @param accessor Accessor to a valid game-data holder.
 * @return The file's answer, or false in single mode.
 */
bool GameDataFunction::isIsInsideSuperbView(GameDataHolderAccessor accessor) {
    if (accessor.getHolder()->isSingleMode()) {
        return false;
    }

    return accessor.getHolder()->getPlayingFile()->isIsInsideSuperbView();
}

/**
 * @brief Mark the player as inside a superb-view area.
 * @param writer Writer to a valid game-data holder.
 */
void GameDataFunction::setIsInsideSuperbView(GameDataHolderWriter writer) {
    if (writer.getHolder()->isSingleMode()) {
        return;
    }

    writer.getHolder()->getPlayingFile()->setIsInsideSuperbView();
}

/**
 * @brief Mark the player as outside any superb-view area.
 * @param writer Writer to a valid game-data holder.
 */
void GameDataFunction::resetIsInsideSuperbView(GameDataHolderWriter writer) {
    if (writer.getHolder()->isSingleMode()) {
        return;
    }

    writer.getHolder()->getPlayingFile()->resetIsInsideSuperbView();
}

/**
 * @brief Collect a Green Star of the current stage.
 * @param writer Writer to a valid game-data holder.
 * @param starIndex Green Star to collect.
 */
void GameDataFunction::acquireGreenStar(GameDataHolderWriter writer, int starIndex) {
    if (writer.getHolder()->isSingleMode()) {
        return;
    }

    writer.getHolder()->getStageDataHolderPtr()->acquireGreenStar(starIndex);
}

/**
 * @brief Check whether a Green Star of the current stage was collected.
 * @param accessor Accessor to a valid game-data holder.
 * @param starIndex Green Star to check.
 * @return The stage state, or false in single mode.
 */
bool GameDataFunction::isAcquireGreenStar(GameDataHolderAccessor accessor, int starIndex) {
    if (accessor.getHolder()->isSingleMode()) {
        return false;
    }

    return accessor.getHolder()->getStageDataHolder()->isAcquireGreenStar(starIndex);
}

/**
 * @brief Check whether every Green Star of the playing course was collected.
 * @param accessor Accessor to a valid game-data holder.
 * @return True when all Green Stars were collected; false in single mode.
 */
bool GameDataFunction::isAcquireGreenStarAll(GameDataHolderAccessor accessor) {
    if (accessor.getHolder()->isSingleMode()) {
        return false;
    }

    const int starNum = findCourseGreenStarNum(accessor, getPlayingCourseId(accessor));
    for (int i = 0; i < starNum; i++) {
        if (!isAcquireGreenStar(accessor, i)) {
            return false;
        }
    }

    return true;
}

/**
 * @brief Count the collected Green Stars.
 * @param accessor Accessor to a valid game-data holder.
 * @return The file's answer, or 0 in single mode.
 */
int GameDataFunction::calcTotalAcquireGreenStarNum(GameDataHolderAccessor accessor) {
    if (accessor.getHolder()->isSingleMode()) {
        return 0;
    }

    return accessor.getHolder()->getPlayingFile()->calcTotalAcquireGreenStarNum();
}

/**
 * @brief Read the Green Star total computed when the last stage started.
 * @param accessor Accessor to a valid game-data holder.
 * @return The file's answer, or 0 in single mode.
 */
int GameDataFunction::getLastTotalAcquireGreenStarNum(GameDataHolderAccessor accessor) {
    if (accessor.getHolder()->isSingleMode()) {
        return 0;
    }

    return accessor.getHolder()->getPlayingFile()->getLastTotalAcquireGreenStarNum();
}

/**
 * @brief Read the number of stamps in a course.
 * @param accessor Accessor to a valid game-data holder.
 * @param courseId Course to read.
 * @return The stamp count.
 */
int GameDataFunction::findIllustItemNum(GameDataHolderAccessor accessor, int courseId) {
    return findStageDatabaseInfo(accessor, courseId)->getIllustItemNum();
}

/**
 * @brief Collect the stamp of the current stage.
 * @param writer Writer to a valid game-data holder.
 */
void GameDataFunction::acquireIllustItem(GameDataHolderWriter writer) {
    if (writer.getHolder()->isSingleMode()) {
        return;
    }

    writer.getHolder()->getStageDataHolderPtr()->acquireIllustItem();
}

/**
 * @brief Record the character that picked up the stamp.
 * @param writer Writer to a valid game-data holder.
 * @param characterType Character that picked up the stamp.
 */
void GameDataFunction::setStampPickupCharType(GameDataHolderWriter writer, int characterType) {
    if (writer.getHolder()->isSingleMode()) {
        return;
    }

    writer.getHolder()->getStageDataHolderPtr()->setStampPickupCharType(characterType);
}

/**
 * @brief Count the collected stamps.
 * @param accessor Accessor to a valid game-data holder.
 * @return The file's answer, or 0 in single mode.
 */
int GameDataFunction::calcTotalIllustItemNum(GameDataHolderAccessor accessor) {
    if (accessor.getHolder()->isSingleMode()) {
        return 0;
    }

    return accessor.getHolder()->getPlayingFile()->calcTotalIllustItemNum();
}

/**
 * @brief Check whether the stamp of the current stage was collected.
 * @param accessor Accessor to a valid game-data holder.
 * @return The stage state, or false in single mode.
 */
bool GameDataFunction::isAcquireIllustItem(GameDataHolderAccessor accessor) {
    if (accessor.getHolder()->isSingleMode()) {
        return false;
    }

    return accessor.getHolder()->getStageDataHolder()->isAcquireIllustItem();
}

/**
 * @brief Check whether a character completed every course.
 * @param accessor Accessor to a valid game-data holder.
 * @param characterType Character to check.
 * @param isCheckFile True to check save files instead of the playing file.
 * @param fileId Save file to check, or a negative value for any file.
 * @return True when the character completed every course.
 */
bool GameDataFunction::isAcquireCharacterCompleteIllustItem(GameDataHolderAccessor accessor,
                                                            int characterType, bool isCheckFile,
                                                            int fileId) {
    if (!isCheckFile) {
        return GameDataFlagFunction::isAlreadyOpenAllClearCharacter(accessor, characterType);
    }

    if (fileId >= 0) {
        if (GameDataFlagFunction::isAlreadyOpenAllClearCharacter(accessor, characterType, fileId)) {
            return true;
        }
    } else {
        for (int i = 0; i < cFileNum; i++) {
            if (GameDataFlagFunction::isAlreadyOpenAllClearCharacter(accessor, characterType, i)) {
                return true;
            }
        }
    }

    return false;
}

/**
 * @brief Read the ghost serial number of a course.
 * @param accessor Accessor to a valid game-data holder.
 * @param courseId Course to read.
 * @return The ghost serial identifier, or -1 without a ghost or in single mode.
 */
int GameDataFunction::findGhostStageSerialId(GameDataHolderAccessor accessor, int courseId) {
    if (accessor.getHolder()->isSingleMode()) {
        return -1;
    }

    return findStageDatabaseInfo(accessor, courseId)->getGhostStageSerialId();
}

/**
 * @brief Count the courses of a world that have a ghost.
 * @param accessor Accessor to a valid game-data holder.
 * @param worldId World to count.
 * @return The number of ghost courses, or zero in single mode.
 */
int GameDataFunction::calcGhostStageNumInWorld(GameDataHolderAccessor accessor, int worldId) {
    if (accessor.getHolder()->isSingleMode()) {
        return 0;
    }

    // The world count is queried but unused by the original code.
    getWorldNum(accessor);
    const int courseNum = getCourseTotalNum(accessor);
    int ghostStageNum = 0;
    for (int courseId = 0; courseId <= courseNum; courseId++) {
        if (isInvalidCourseId(courseId) || findGhostStageSerialId(accessor, courseId) == -1) {
            continue;
        }

        int courseWorldId;
        int stageId;
        calcWorldAndStageId(accessor, &courseWorldId, &stageId, courseId);
        if (courseWorldId == worldId) {
            ghostStageNum++;
        }
    }

    return ghostStageNum;
}

/**
 * @brief Read the counter deciding when a ghost present appears.
 * @param accessor Accessor to a valid game-data holder.
 * @return The file's answer, or 0 in single mode.
 */
int GameDataFunction::getGhostPresentCounter(GameDataHolderAccessor accessor) {
    if (accessor.getHolder()->isSingleMode()) {
        return 0;
    }

    return accessor.getHolder()->getPlayingFile()->getGhostPresentCounter();
}

/**
 * @brief Read the reference time of the ghost of a stage.
 * @param accessor Accessor to a valid game-data holder.
 * @param pStageName Stage resource name.
 * @return The ghost base time, or zero in single mode.
 */
int GameDataFunction::findGhostBaseTime(GameDataHolderAccessor accessor, const char* pStageName) {
    if (accessor.getHolder()->isSingleMode()) {
        return 0;
    }

    const int worldNum = getWorldNum(accessor);
    int courseId = -1;
    int worldId = 0;
    while (worldId < worldNum) {
        worldId++;
        WorldInfo* pWorld = findWorldInfo(accessor, worldId);
        const int stageNum = pWorld->mStageCount;
        for (int i = 0; i < stageNum; i++) {
            StageDatabaseInfo* pStage = pWorld->getStageInfoByIndex(i);
            if (al::isEqualString(pStageName, pStage->getStageName())) {
                courseId = pStage->getCourseId();
                break;
            }
        }

        if (courseId >= 0) {
            break;
        }
    }

    return findStageDatabaseInfo(accessor, courseId)->getGhostBaseTime();
}

/**
 * @brief Read how many double cherry clones can exist in the playing course.
 * @param accessor Accessor to a valid game-data holder.
 * @return The maximum number of clones, or zero in single mode.
 */
int GameDataFunction::getDoubleMarioNumMax(GameDataHolderAccessor accessor) {
    if (accessor.getHolder()->isSingleMode()) {
        return 0;
    }

    return findStageDatabaseInfo(accessor, getPlayingCourseId(accessor))->getDoubleMarioNumMax();
}

/**
 * @brief Initialize the network settings.
 * @param writer Writer to a valid game-data holder.
 * @param isInit True to enable the network features.
 */
void GameDataFunction::setNetworkSettingInit(GameDataHolderWriter writer, bool isInit) {
    if (isInit) {
        setMiiverseSetting(writer, 0);
        setGhostSetting(writer, 0);
    } else {
        setMiiverseSetting(writer, 1);
        setGhostSetting(writer, 1);
    }
}

/**
 * @brief Change the Miiverse setting.
 * @param writer Writer to a valid game-data holder.
 * @param setting New Miiverse setting.
 */
void GameDataFunction::setMiiverseSetting(GameDataHolderWriter writer, int setting) {
    if (writer.getHolder()->isSingleMode()) {
        return;
    }

    writer.getHolder()->getPlayingFile()->setMiiverseSetting(setting);
}

/**
 * @brief Change the ghost setting.
 * @param writer Writer to a valid game-data holder.
 * @param setting New ghost setting.
 */
void GameDataFunction::setGhostSetting(GameDataHolderWriter writer, int setting) {
    if (writer.getHolder()->isSingleMode()) {
        return;
    }

    writer.getHolder()->getPlayingFile()->setGhostSetting(setting);
}

/**
 * @brief Read the Miiverse setting.
 * @param accessor Accessor to a valid game-data holder.
 * @return The file's answer, or 0 in single mode.
 */
int GameDataFunction::getMiiverseSetting(GameDataHolderAccessor accessor) {
    if (accessor.getHolder()->isSingleMode()) {
        return 0;
    }

    return accessor.getHolder()->getPlayingFile()->getMiiverseSetting();
}

/**
 * @brief Check whether Miiverse can be used.
 * @param accessor Accessor to a valid game-data holder.
 * @return Always false.
 */
bool GameDataFunction::isEnableMiiverse(GameDataHolderAccessor accessor) {
    return false;
}

/**
 * @brief Read the ghost setting.
 * @param accessor Accessor to a valid game-data holder.
 * @return The file's answer, or 0 in single mode.
 */
int GameDataFunction::getGhostSetting(GameDataHolderAccessor accessor) {
    if (accessor.getHolder()->isSingleMode()) {
        return 0;
    }

    return accessor.getHolder()->getPlayingFile()->getGhostSetting();
}

/**
 * @brief Change the vertical camera inversion.
 * @param writer Writer to a valid game-data holder.
 * @param isReverse True to invert the camera.
 * @return True when the setting changed.
 */
bool GameDataFunction::setCameraReverseVertical(GameDataHolderWriter writer, bool isReverse) {
    if (writer.getHolder()->isSingleMode()) {
        return false;
    }

    if (GameDataFile::getCameraReverseVertical() == isReverse) {
        return false;
    }

    setSaveRequested(writer, true);
    GameDataFile::setCameraReverseVertical(writer.getHolder(), isReverse);
    return true;
}

/**
 * @brief Read the vertical camera inversion.
 * @param writer Writer to a valid game-data holder.
 * @return True when inverted; false in single mode.
 */
bool GameDataFunction::getCameraReverseVertical(GameDataHolderWriter writer) {
    if (writer.getHolder()->isSingleMode()) {
        return false;
    }

    return GameDataFile::getCameraReverseVertical();
}

/**
 * @brief Change the horizontal camera inversion.
 * @param writer Writer to a valid game-data holder.
 * @param isReverse True to invert the camera.
 * @return True when the setting changed.
 */
bool GameDataFunction::setCameraReverseHorizontal(GameDataHolderWriter writer, bool isReverse) {
    if (writer.getHolder()->isSingleMode()) {
        return false;
    }

    if (GameDataFile::getCameraReverseHorizontal() == isReverse) {
        return false;
    }

    setSaveRequested(writer, true);
    GameDataFile::setCameraReverseHorizontal(writer.getHolder(), isReverse);
    return true;
}

/**
 * @brief Read the horizontal camera inversion.
 * @param writer Writer to a valid game-data holder.
 * @return True when inverted; false in single mode.
 */
bool GameDataFunction::getCameraReverseHorizontal(GameDataHolderWriter writer) {
    if (writer.getHolder()->isSingleMode()) {
        return false;
    }

    return GameDataFile::getCameraReverseHorizontal();
}

/**
 * @brief Change the Captain Toad vertical camera inversion.
 * @param writer Writer to a valid game-data holder.
 * @param isReverse True to invert the camera.
 */
void GameDataFunction::setKinopioBrigadeCameraReverseVertical(GameDataHolderWriter writer,
                                                              bool isReverse) {
    if (writer.getHolder()->isSingleMode()) {
        return;
    }

    if (GameDataFile::getKinopioBrigadeCameraReverseVertical() == isReverse) {
        return;
    }

    setSaveRequested(writer, true);
    GameDataFile::setKinopioBrigadeCameraReverseVertical(writer.getHolder(), isReverse);
}

/**
 * @brief Read the Captain Toad vertical camera inversion.
 * @param writer Writer to a valid game-data holder.
 * @return True when inverted; false in single mode.
 */
bool GameDataFunction::getKinopioBrigadeCameraReverseVertical(GameDataHolderWriter writer) {
    if (writer.getHolder()->isSingleMode()) {
        return false;
    }

    return GameDataFile::getKinopioBrigadeCameraReverseVertical();
}

/**
 * @brief Change the Captain Toad horizontal camera inversion.
 * @param writer Writer to a valid game-data holder.
 * @param isReverse True to invert the camera.
 */
void GameDataFunction::setKinopioBrigadeCameraReverseHorizontal(GameDataHolderWriter writer,
                                                                bool isReverse) {
    if (writer.getHolder()->isSingleMode()) {
        return;
    }

    if (GameDataFile::getKinopioBrigadeCameraReverseHorizontal() == isReverse) {
        return;
    }

    setSaveRequested(writer, true);
    GameDataFile::setKinopioBrigadeCameraReverseHorizontal(writer.getHolder(), isReverse);
}

/**
 * @brief Read the Captain Toad horizontal camera inversion.
 * @param writer Writer to a valid game-data holder.
 * @return True when inverted; false in single mode.
 */
bool GameDataFunction::getKinopioBrigadeCameraReverseHorizontal(GameDataHolderWriter writer) {
    if (writer.getHolder()->isSingleMode()) {
        return false;
    }

    return GameDataFile::getKinopioBrigadeCameraReverseHorizontal();
}

/**
 * @brief Read the initial timer of the playing course.
 * @param accessor Accessor to a valid game-data holder.
 * @return The initial timer count, or zero in single mode.
 */
int GameDataFunction::getInitStageTimer(GameDataHolderAccessor accessor) {
    if (accessor.getHolder()->isSingleMode()) {
        return 0;
    }

    return findStageDatabaseInfo(accessor, getPlayingCourseId(accessor))->getInitStageTimer();
}

/**
 * @brief Give time back to the stage timer.
 * @param writer Writer to a valid game-data holder.
 * @param count Timer count to add.
 */
void GameDataFunction::turnBackStageTimer(GameDataHolderWriter writer, int count) {
    if (writer.getHolder()->isSingleMode()) {
        return;
    }

    const int frames = StageDataHolder::calcStageTimerCountToFrame(count);
    writer.getHolder()->getStageDataHolderPtr()->addStageTimerFrame(frames);
}

/**
 * @brief Forget the passed checkpoint.
 * @param writer Writer to a valid game-data holder.
 */
void GameDataFunction::resetCheckpointPass(GameDataHolderWriter writer) {
    if (writer.getHolder()->isSingleMode()) {
        return;
    }

    writer.getHolder()->getStageDataHolderPtr()->resetCheckpointPass();
}

/**
 * @brief Record that Mario passed the checkpoint.
 * @param writer Writer to a valid game-data holder.
 */
void GameDataFunction::setCheckpointPassMario(GameDataHolderWriter writer) {
    if (writer.getHolder()->isSingleMode()) {
        return;
    }

    writer.getHolder()->getStageDataHolderPtr()->setCheckpointPass(0);
}

/**
 * @brief Record that Luigi passed the checkpoint.
 * @param writer Writer to a valid game-data holder.
 */
void GameDataFunction::setCheckpointPassLuigi(GameDataHolderWriter writer) {
    if (writer.getHolder()->isSingleMode()) {
        return;
    }

    writer.getHolder()->getStageDataHolderPtr()->setCheckpointPass(1);
}

/**
 * @brief Record that Peach passed the checkpoint.
 * @param writer Writer to a valid game-data holder.
 */
void GameDataFunction::setCheckpointPassPeach(GameDataHolderWriter writer) {
    if (writer.getHolder()->isSingleMode()) {
        return;
    }

    writer.getHolder()->getStageDataHolderPtr()->setCheckpointPass(2);
}

/**
 * @brief Record that Kinopio passed the checkpoint.
 * @param writer Writer to a valid game-data holder.
 */
void GameDataFunction::setCheckpointPassKinopio(GameDataHolderWriter writer) {
    if (writer.getHolder()->isSingleMode()) {
        return;
    }

    writer.getHolder()->getStageDataHolderPtr()->setCheckpointPass(3);
}

/**
 * @brief Record that Rosetta passed the checkpoint.
 * @param writer Writer to a valid game-data holder.
 */
void GameDataFunction::setCheckpointPassRosetta(GameDataHolderWriter writer) {
    if (writer.getHolder()->isSingleMode()) {
        return;
    }

    writer.getHolder()->getStageDataHolderPtr()->setCheckpointPass(4);
}

/**
 * @brief Check whether a checkpoint was passed.
 * @param accessor Accessor to a valid game-data holder.
 * @return The stage state, or false in single mode.
 */
bool GameDataFunction::isCheckpointPass(GameDataHolderAccessor accessor) {
    if (accessor.getHolder()->isSingleMode()) {
        return false;
    }

    return accessor.getHolder()->getStageDataHolder()->isCheckpointPass();
}

/**
 * @brief Check whether Mario passed the checkpoint.
 * @param accessor Accessor to a valid game-data holder.
 * @return True when Mario passed it; false in single mode.
 */
bool GameDataFunction::isCheckpointPassMario(GameDataHolderAccessor accessor) {
    if (accessor.getHolder()->isSingleMode()) {
        return false;
    }

    return accessor.getHolder()->getStageDataHolder()->getCheckpointPassPlayerCharacter() == 0;
}

/**
 * @brief Check whether Luigi passed the checkpoint.
 * @param accessor Accessor to a valid game-data holder.
 * @return True when Luigi passed it; false in single mode.
 */
bool GameDataFunction::isCheckpointPassLuigi(GameDataHolderAccessor accessor) {
    if (accessor.getHolder()->isSingleMode()) {
        return false;
    }

    return accessor.getHolder()->getStageDataHolder()->getCheckpointPassPlayerCharacter() == 1;
}

/**
 * @brief Check whether Peach passed the checkpoint.
 * @param accessor Accessor to a valid game-data holder.
 * @return True when Peach passed it; false in single mode.
 */
bool GameDataFunction::isCheckpointPassPeach(GameDataHolderAccessor accessor) {
    if (accessor.getHolder()->isSingleMode()) {
        return false;
    }

    return accessor.getHolder()->getStageDataHolder()->getCheckpointPassPlayerCharacter() == 2;
}

/**
 * @brief Check whether Kinopio passed the checkpoint.
 * @param accessor Accessor to a valid game-data holder.
 * @return True when Kinopio passed it; false in single mode.
 */
bool GameDataFunction::isCheckpointPassKinopio(GameDataHolderAccessor accessor) {
    if (accessor.getHolder()->isSingleMode()) {
        return false;
    }

    return accessor.getHolder()->getStageDataHolder()->getCheckpointPassPlayerCharacter() == 3;
}

/**
 * @brief Check whether Rosetta passed the checkpoint.
 * @param accessor Accessor to a valid game-data holder.
 * @return True when Rosetta passed it; false in single mode.
 */
bool GameDataFunction::isCheckpointPassRosetta(GameDataHolderAccessor accessor) {
    if (accessor.getHolder()->isSingleMode()) {
        return false;
    }

    return accessor.getHolder()->getStageDataHolder()->getCheckpointPassPlayerCharacter() == 4;
}

/**
 * @brief Check whether the stage is being restarted.
 * @param accessor Accessor to a valid game-data holder.
 * @return The stage state, or false in single mode.
 */
bool GameDataFunction::isRestartStage(GameDataHolderAccessor accessor) {
    if (accessor.getHolder()->isSingleMode()) {
        return false;
    }

    return accessor.getHolder()->getStageDataHolder()->isRestart();
}

/**
 * @brief Check whether the stage restarts from a checkpoint.
 * @param accessor Accessor to a valid game-data holder.
 * @return The stage state, or false in single mode.
 */
bool GameDataFunction::isRestartFromCheckpoint(GameDataHolderAccessor accessor) {
    if (accessor.getHolder()->isSingleMode()) {
        return false;
    }

    return accessor.getHolder()->getStageDataHolder()->isRestartFromCheckpoint();
}

/**
 * @brief Check whether the stage start demo is skipped.
 * @param accessor Accessor to a valid game-data holder.
 * @return The stage state, or false in single mode.
 */
bool GameDataFunction::isSkipStartDemo(GameDataHolderAccessor accessor) {
    if (accessor.getHolder()->isSingleMode()) {
        return false;
    }

    return accessor.getHolder()->getStageDataHolder()->isSkipStartDemo();
}

/**
 * @brief Choose whether the stage start demo is skipped.
 * @param writer Writer to a valid game-data holder.
 * @param isSkip True to skip the start demo.
 */
void GameDataFunction::setSkipStartDemo(GameDataHolderWriter writer, bool isSkip) {
    if (writer.getHolder()->isSingleMode()) {
        return;
    }

    writer.getHolder()->getStageDataHolderPtr()->setSkipStartDemo(isSkip);
}

/**
 * @brief Record that New Super Luigi U save data exists.
 * @param writer Writer to a valid game-data holder.
 */
void GameDataFunction::onExistLuigiUSaveData(GameDataHolderWriter writer) {
    writer.getHolder()->onExistLuigiUSaveData();
}

/**
 * @brief Check whether Luigi Bros. can be played.
 * @param accessor Accessor to a valid game-data holder.
 * @return True once unlocked.
 */
bool GameDataFunction::isEnablePlayLuigiBros(GameDataHolderAccessor accessor) {
    return accessor.getHolder()->isUnlockLuigiBros();
}

/**
 * @brief Check whether every save file of the current mode is new.
 * @param accessor Accessor to a valid game-data holder.
 * @return True when no save file was played.
 */
bool GameDataFunction::isAllNewFile(GameDataHolderAccessor accessor) {
    GameDataHolder* pHolder = accessor.getHolder();
    if (pHolder->isSingleMode()) {
        for (int i = 0; i < cFileNum; i++) {
            if (!pHolder->getSingleModeDataFile(i)->isNewFile()) {
                return false;
            }
        }
    } else {
        for (int i = 0; i < cFileNum; i++) {
            if (!pHolder->getGameDataFile(i)->isNewFile()) {
                return false;
            }
        }
    }

    return true;
}

/**
 * @brief Count the new save files of the current mode.
 * @param accessor Accessor to a valid game-data holder.
 * @return The number of new files.
 */
int GameDataFunction::getNewFileNum(GameDataHolderAccessor accessor) {
    GameDataHolder* pHolder = accessor.getHolder();
    int newFileNum = 0;
    if (pHolder->isSingleMode()) {
        for (int i = 0; i < cFileNum; i++) {
            newFileNum += pHolder->getSingleModeDataFile(i)->isNewFile();
        }
    } else {
        for (int i = 0; i < cFileNum; i++) {
            newFileNum += pHolder->getGameDataFile(i)->isNewFile();
        }
    }

    return newFileNum;
}

/**
 * @brief Find the first new save file of the current mode.
 * @param accessor Accessor to a valid game-data holder.
 * @return The save-file slot, or -1 when every file was played.
 */
int GameDataFunction::getFirstNewFileId(GameDataHolderAccessor accessor) {
    GameDataHolder* pHolder = accessor.getHolder();
    if (pHolder->isSingleMode()) {
        for (int i = 0; i < cFileNum; i++) {
            if (pHolder->getSingleModeDataFile(i)->isNewFile()) {
                return i;
            }
        }
    } else {
        for (int i = 0; i < cFileNum; i++) {
            if (pHolder->getGameDataFile(i)->isNewFile()) {
                return i;
            }
        }
    }

    return -1;
}

/**
 * @brief Read the slot of the active save file.
 * @param accessor Accessor to a valid game-data holder.
 * @return The save-file slot.
 */
int GameDataFunction::getFileId(GameDataHolderAccessor accessor) {
    if (accessor.getHolder()->isSingleMode()) {
        return accessor.getHolder()->getSingleFile()->getFileId();
    }

    return accessor.getHolder()->getPlayingFile()->getFileId();
}

/**
 * @brief Check whether a network account is linked.
 * @param accessor Accessor to a valid game-data holder.
 * @return Always false.
 */
bool GameDataFunction::isNetworkAccount(GameDataHolderAccessor accessor) {
    return false;
}

/**
 * @brief Read the account slot.
 * @param accessor Accessor to a valid game-data holder.
 * @return Always 0.
 */
int GameDataFunction::getAccountSlotNo(GameDataHolderAccessor accessor) {
    return 0;
}

/**
 * @brief Check whether in-game networking is forbidden.
 * @param accessor Accessor to a valid game-data holder.
 * @return Always true.
 */
bool GameDataFunction::isInGameNetworkForbidden(GameDataHolderAccessor accessor) {
    return true;
}

/**
 * @brief Check whether Miiverse is forbidden.
 * @param accessor Accessor to a valid game-data holder.
 * @return Always true.
 */
bool GameDataFunction::isMiiverseForbidden(GameDataHolderAccessor accessor) {
    return true;
}

/**
 * @brief Check whether Miiverse is read-only.
 * @param accessor Accessor to a valid game-data holder.
 * @return Always true.
 */
bool GameDataFunction::isMiiverseReadOnly(GameDataHolderAccessor accessor) {
    return true;
}
