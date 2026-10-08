#pragma once
#include "Library/Scene/ISceneObj.hpp"
#include <basis/seadTypes.h>
#include "System/GameDataCommon.hpp"
namespace al {
class SceneObjHolder;
class NetworkSystem;
} // namespace al
#include <preport/PlayReportManager.h>
#include <prim/seadSafeString.h>
#include <time/seadDateTime.h>
namespace al {
class ErrorViewer;
class LayoutInitInfo;
class PlayerHolder;
} // namespace al
class GameDataCommon;
class GameDataPlayReportCommon;
class GameDataFile;
class SingleModeData;
class StageListHolder;
class StageDataHolder;
class SaveDataAccessSequence;
class OceanScenarioList;
class IslandDataList;
class CourseInfo;
class ControlUserDataHolder;
enum GameMode : int;
class GameDataHolder : public al::ISceneObj {
  public:
    explicit GameDataHolder(al::NetworkSystem* pNetworkSystem);
    void setPlayingFileId(int fileId);
    void setSingleModePlayingFileID(int fileId, bool isInitPlayTime);
    void initializeData();
    void updatePlayerFigures(int figureType);
    void initialize3DWorldData();
    bool checkValid();
    void createSaveDataAccessSequence(al::ErrorViewer* pErrorViewer,
                                      al::NetworkSystem* pNetworkSystem,
                                      const al::LayoutInitInfo& rInfo);
    void createSaveDataAccessSequenceDevelop(al::ErrorViewer* pErrorViewer,
                                             al::NetworkSystem* pNetworkSystem,
                                             const al::LayoutInitInfo& rInfo);
    void initIsPhase0();
    bool initializePlayReport(const char* pName);
    void updatePlayReport();
    void updatePlayStyle();
    bool isNewFile(int fileId) const;
    void copySaveFile(int srcFileId, int dstFileId);
    void deleteSaveFile(int fileId);
    bool isNewFile() const;
    void copySingleModeFile(int srcFileId, int dstFileId);
    int tryGetLastStageBestScoreUserID() const;
    void startOpening();
    void startEnding();
    bool isFirstPlay() const;
    int calcCharacterTypeNumMax() const;
    void readFromSaveDataBuffer();
    void writeToSaveDataBuffer(bool isSkipPlayingFile);
    void resetGameFileForTitleDemo();
    const char* getSceneObjName() const override;
    void setSceneObjHolder(al::SceneObjHolder* pHolder);
    GameDataFile* getGameDataFile(int fileId) const;
    SingleModeData* getSingleModeDataFile(int fileId) const;
    void setLastPlayedMode(GameMode mode);
    GameMode getLastPlayedMode() const;
    int getLastPlayingFileId() const;
    int getLastSingleModePlayingFileID() const;
    bool isUnlockLuigiBros() const;
    void unlockLuigiBros();
    void setGameFileForTitleDemo(GameDataFile* pFile);
    const StageDataHolder* getStageDataHolder() const;
    StageDataHolder* getStageDataHolderPtr();
    const CourseInfo* getCourseInfo(int courseId) const;
    CourseInfo* getCourseInfoPtr(int courseId);
    ControlUserDataHolder* getControlUserDataHolder() const;
    void addSessionId();
    bool beginPlayReport(preport::KeyEventType type, int eventId, int option);
    void endPlayReport();
    void setPlayReportData(preport::Key key, int value);
    void setPlayReportData(preport::Key key, float value);
    void setPlayReportData(preport::Key key, s64 value);
    void setPlayReportData(preport::Key key, sead::SafeString& rValue);
    void setPlayReportData(preport::Key key, int* pValues, int num);
    void setPlayReportData(preport::Key key, float* pValues, int num);
    void sendNetworkStatus();

    /**
     * @brief Access the course database.
     * @return The initialized course database.
     */
    StageListHolder* getStageList() const { return mpStageList; }

    /**
     * @brief Access the save-operation state machine.
     * @return The save sequence, or nullptr before creation.
     */
    SaveDataAccessSequence* getSaveAccess() const { return mpSaveAccess; }

    /**
     * @brief Check whether Bowser's Fury mode is active.
     * @return True for single mode.
     */
    bool isSingleMode() const { return mSingleMode; }

    /**
     * @brief Switch between the Super Mario 3D World and Bowser's Fury modes.
     * @param isSingleMode True for Bowser's Fury.
     */
    void setSingleMode(bool isSingleMode) { mSingleMode = isSingleMode; }

    /**
     * @brief Access the common play-log storage.
     * @return The allocated play-log storage.
     */
    PlayLogData* getPlayLog() const { return mpCommon->mpPlayLog; }

    /**
     * @brief Access the data shared by every save file.
     * @return The common save data.
     */
    GameDataCommon* getCommon() const { return mpCommon; }

    /**
     * @brief Access the active 3D World save file.
     * @return The active 3D World save file.
     */
    GameDataFile* getPlayingFile() const { return mpPlayingFile; }

    /**
     * @brief Access the active Bowser's Fury save file.
     * @return The active single-mode save file.
     */
    SingleModeData* getSingleFile() const { return mpSingleFile; }

    /**
     * @brief Access the ocean-quadrant scenario lists.
     * @return The ocean scenario lists.
     */
    OceanScenarioList* getOceanScenarioList() const { return mpOceanScenarioList; }

    /**
     * @brief Access the Bowser's Fury island database.
     * @return The island list.
     */
    IslandDataList* getIslandDataList() const { return mpIslandDataList; }

    /**
     * @brief Check whether the two-player assist mode is active.
     * @return True when a second player assists.
     */
    bool is2PAssistMode() const { return mIs2PAssistMode; }

    /**
     * @brief Enable or disable the two-player assist mode.
     * @param isAssist True to enable the assist mode.
     */
    void set2PAssistMode(bool isAssist) { mIs2PAssistMode = isAssist; }

    /**
     * @brief Check whether the map is enabled.
     * @return True when the map can be used.
     */
    bool isMapEnabled() const { return mIsMapEnabled; }

    /**
     * @brief Enable or disable the map.
     * @param isEnabled True to enable the map.
     */
    void setMapEnabled(bool isEnabled) { mIsMapEnabled = isEnabled; }

    /**
     * @brief Check whether the last demo was cancelled.
     * @return True when the demo was cancelled.
     */
    bool isDemoWasCancelled() const { return mIsDemoWasCancelled; }

    /**
     * @brief Record whether the last demo was cancelled.
     * @param isCancelled True when the demo was cancelled.
     */
    void setDemoWasCancelled(bool isCancelled) { mIsDemoWasCancelled = isCancelled; }

    /**
     * @brief Check whether a save has been requested.
     * @return True when the progress should be saved.
     */
    bool isSaveRequested() const { return mIsSaveRequested; }

    /**
     * @brief Request or cancel a save.
     * @param isRequested True to request a save.
     */
    void setSaveRequested(bool isRequested) { mIsSaveRequested = isRequested; }

    /**
     * @brief Enable or disable skipping the save at the start of a stage.
     * @param isSkip True while the title scene is active.
     */
    void setSkipStartSave(bool isSkip) { mIsSkipStartSave = isSkip; }

    /**
     * @brief Record that New Super Luigi U save data exists on the console.
     */
    void onExistLuigiUSaveData() { mUnknown51 = true; }

    /**
     * @brief Check whether the Bowser's Fury prologue phase is active.
     * @return True during phase 0.
     */
    bool isPhase0() const { return mIsPhase0; }

    /**
     * @brief Check whether the scene is being restarted.
     * @return True while the scene restarts.
     */
    bool isSceneRestart() const { return mIsSceneRestart; }

    /**
     * @brief Mark whether the scene is being restarted.
     * @param isRestart True while the scene restarts.
     */
    void setSceneRestart(bool isRestart) { mIsSceneRestart = isRestart; }

    /**
     * @brief Access the play-report manager.
     * @return The play-report manager.
     */
    preport::PlayReportManager* getPlayReportManager() const { return mpPlayReportManager; }

    bool isSaveDataRead() const { return mIsSaveDataRead; }

    void setSaveDataRead(bool isRead) { mIsSaveDataRead = isRead; }

    bool isCourseSelectVisited() const { return mUnknown6A; }

    void setCourseSelectVisited(bool isVisited) { mUnknown6A = isVisited; }

    bool isNeedCourseSelectPlayReport() const { return mUnknown6B; }

    void setNeedCourseSelectPlayReport(bool isNeed) { mUnknown6B = isNeed; }

    /**
     * @brief Set the ocean-quadrant scenario lists.
     * @param pList The ocean scenario lists.
     */
    void setOceanScenarioList(OceanScenarioList* pList) { mpOceanScenarioList = pList; }

    /**
     * @brief Set the Bowser's Fury island database.
     * @param pList The island list.
     */
    void setIslandDataList(IslandDataList* pList) { mpIslandDataList = pList; }

    /**
     * @brief Set the players of the current scene.
     * @param pHolder The player holder, or nullptr when the scene ends.
     */
    void setPlayerHolder(al::PlayerHolder* pHolder) { mpPlayerHolder = pHolder; }

    /**
     * @brief Set the two unknown flags at 0x61 and 0x62 (cleared by the single mode scene).
     * @param is61 Value of the flag at 0x61.
     * @param is62 Value of the flag at 0x62.
     */
    void setUnknownFlags61And62(bool is61, bool is62) {
        mUnknown62 = is62;
        mUnknown61 = is61;
    }

    /**
     * @brief Set whether the scene update is frozen (flag at 0x61).
     * @param isFreeze Whether the scene update is frozen.
     */
    void setFreezeMode(bool isFreeze) { mUnknown61 = isFreeze; }

    /**
     * @brief Set the unknown flag at 0x62 (players are not updated while frozen when set).
     * @param isSet The flag value.
     */
    void setUnknown62(bool isSet) { mUnknown62 = isSet; }

    /**
     * @brief Check whether the players are frozen (flag at 0x61, single mode).
     * @return True while the freeze mode is on.
     */
    bool isFreezeMode() const { return mUnknown61; }

    /**
     * @brief Check the unknown flag at 0x62 (players are not updated while frozen when set).
     * @return The flag value.
     */
    bool isUnknown62() const { return mUnknown62; }

    /**
     * @brief Check whether the play start report is still to be sent (flag at 0x6c).
     * @return True when the report is pending.
     */
    bool isNeedPlayStartReport() const { return mUnknown6C; }

    /**
     * @brief Set whether the play start report is still to be sent.
     * @param isNeed True when the report is pending.
     */
    void setNeedPlayStartReport(bool isNeed) { mUnknown6C = isNeed; }

  private:
    friend class PlayReport;

    GameDataCommon* mpCommon;
    GameDataPlayReportCommon* mpPlayReportCommon;
    GameDataFile** mppFiles;
    GameDataFile* mpPlayingFile;
    SingleModeData** mppSingleFiles;
    SingleModeData* mpSingleFile;
    StageListHolder* mpStageList;
    OceanScenarioList* mpOceanScenarioList;
    IslandDataList* mpIslandDataList;
    bool mIsSaveDataRead;
    bool mUnknown51;
    bool mIsSkipStartSave;
    sead::DateTime mPlayStartTime;
    bool mSingleMode;
    bool mUnknown61;
    bool mUnknown62;
    bool mIs2PAssistMode;
    bool mIsMapEnabled;
    bool mIsDemoWasCancelled;
    u8 mUnknown66;
    bool mIsSaveRequested;
    bool mIsPhase0;
    bool mIsSceneRestart;
    bool mUnknown6A;
    bool mUnknown6B;
    bool mUnknown6C;
    bool mIsEnablePlayReport;
    SaveDataAccessSequence* mpSaveAccess;
    al::NetworkSystem* mpNetwork;
    al::PlayerHolder* mpPlayerHolder;
    al::SceneObjHolder* mpSceneObjHolder;
    preport::PlayReportManager* mpPlayReportManager;
};
static_assert(sizeof(GameDataHolder) == 0x98);
