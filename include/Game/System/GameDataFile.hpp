#pragma once
#include "System/GameDataFileBase.hpp"
class StageDataHolder;
class CourseInfo;
class CourseInfoHolder;
class WorldGameData;
class WorldGameDataHolder;
class StockItemList;
class GameDataFile;

namespace GameDataFileInternal {
inline void resetKinopioHouse(GameDataFile* pFile);
} // namespace GameDataFileInternal

class GameDataFile : public GameDataFileBase {
  public:
    GameDataFile(GameDataHolder* pHolder, int fileId);
    void initializeData() override;
    void onSave() override;
    void startStage(int worldId, int stageId) override;
    void onStageStart() override;
    void onStageEnd() override;
    void restartStage() override;
    int calcClearStarLevel() override;
    bool isTwinkleData() const override;
    bool readFromStream(sead::ReadStream* pStream) override;
    void writeToStream(sead::WriteStream* pStream, bool isSkip) const override;
    bool entryPlayer(int userId, int characterType) override;
    void startOpening() override;
    void initPlayerLife(int life) override;
    int getPlayerLife() const override;
    int addPlayerLife(int life) override;
    virtual bool checkValid();

    /**
     * @brief Access the state of the stage being played.
     * @return The stage data holder.
     */
    virtual const StageDataHolder* getStageDataHolder() const { return mpStageDataHolder; }

    /**
     * @brief Access the state of the stage being played.
     * @return The stage data holder.
     */
    virtual StageDataHolder* getStageDataHolderPtr() { return mpStageDataHolder; }

    virtual void copySaveFile(const GameDataFile& rOther);
    virtual void setPlayingFile();
    virtual void recoverGameOver();
    virtual void recoverGameOverFromGoldenExpress();
    virtual void startSingleModeStage(int worldId, int stageId);
    virtual void onSaveStartInCourseSelect();
    virtual void onWorldWarp(int worldId);
    virtual void onWorldWarpDokanDemo();
    virtual void onGotoTitle(int worldId);
    virtual void updateBestScoreUser();
    virtual void setBestScoreUserId(int userId);
    virtual void clearStage();
    virtual void clearStageWorldWarp();
    virtual void retireStage();
    virtual void retireStageExitDoor();
    virtual void restartKinopioBrigade();
    virtual void restartMysteryBox();
    virtual void restartTimeupMysteryBox();
    virtual void playWorldStartDemo(int worldId);
    virtual void startEnding();
    virtual void resetCasinoRoomCounter();

    /**
     * @brief Read how many more normal stages must be cleared before the casino
     * room opens.
     * @return The casino-room counter.
     */
    virtual int getCasinoRoomCounter() const { return mCasinoRoomCounter; }

    /**
     * @brief Read how many more normal stages must be cleared before the golden
     * express opens.
     * @return The golden-express counter.
     */
    virtual int getGoldenExpressCounter() const { return mGoldenExpressCounter; }

    /**
     * @brief Read the number of lost lives.
     * @return The retry count.
     */
    virtual int getRetryCount() const { return mRetryCount; }

    /**
     * @brief Read the number of lost lives at the last save.
     * @return The saved retry count.
     */
    virtual int getRetryCountSaved() const { return mRetryCountSaved; }

    /**
     * @brief Read the counter deciding when a ghost present appears.
     * @return The ghost-present counter.
     */
    virtual int getGhostPresentCounter() const { return mGhostPresentCounter; }

    virtual const CourseInfo* getCourseInfo(int courseId) const;
    virtual CourseInfo* getCourseInfoPtr(int courseId);

    /**
     * @brief Access the per-course progress records.
     * @return The course-information holder.
     */
    virtual const CourseInfoHolder* getCourseInfoHolder() const { return mpCourseInfoHolder; }

    virtual WorldGameData* getWorldGameData(int worldId) const;
    virtual bool isGameOver() const;

    /**
     * @brief Read the player who set the best score in the last stage.
     * @return The user identifier, or -1 when there is none.
     */
    virtual int tryGetLastStageBestScoreUserID() const { return mLastStageBestScoreUserId; }

    virtual int getPlayingCourseId() const;
    virtual int getLastPlayCourseId() const;

    /**
     * @brief Read the last played course at the last save.
     * @return The saved course identifier.
     */
    virtual int getLastPlayCourseIdSaved() const { return mLastPlayCourseIdSaved; }

    virtual bool isLastPlayCourseClear() const;
    virtual bool isLastPlayCourseClearWorldWarp() const;
    virtual bool isLastPlayCourseFirstClear() const;
    virtual bool isLastPlayCourseFirstAcquireIllustItem() const;
    virtual bool isLastPlayCourseUpdateBestScore() const;
    virtual bool isLastPlayCourseUpdateBestTime() const;
    virtual bool isNeedOpenCasinoRoom() const;
    virtual bool isNeedOpenGoldenExpress() const;
    virtual bool isNeedSave() const;
    virtual bool isClearNormalEnding() const;
    virtual void setMiiverseSetting(int setting);

    /**
     * @brief Read the Miiverse setting.
     * @return The Miiverse setting; zero enables posting.
     */
    virtual int getMiiverseSetting() const { return mMiiverseSetting; }

    virtual void setGhostSetting(int setting);

    /**
     * @brief Read the ghost setting.
     * @return The ghost setting.
     */
    virtual int getGhostSetting() const { return mGhostSetting; }

    /**
     * @brief Read the progression flags.
     * @return The progression flag bits.
     */
    virtual u64 getGameFlag() const { return mGameFlag; }

    /**
     * @brief Read the network setting.
     * @return The network setting.
     */
    virtual int getNetworkSetting() const { return mNetworkSetting; }

    /**
     * @brief Read the Green Star total computed when the last stage started.
     * @return The acquired Green Star count.
     */
    virtual int getLastTotalAcquireGreenStarNum() const { return mLastTotalAcquireGreenStarNum; }

    virtual int calcTotalAcquireGreenStarNum() const;
    virtual int calcTotalIllustItemNum() const;

    /**
     * @brief Read the number of stars shown on the file.
     * @return The clear star level from 0 through 5.
     */
    virtual int getClearStarLevel() const { return mClearStarLevel; }

    virtual bool isAllClearWithCharacter(int characterType) const;
    virtual bool isRosettaPlayable() const;
    virtual bool isShowBestTime() const;
    virtual bool isOpenWorldStar() const;
    virtual bool isOpenWorldArrange() const;
    virtual bool isOpenWorldChampionship() const;
    virtual bool isShowWorldJumpMenuInfo() const;
    virtual bool isShowWorldJumpMenuInfo2nd() const;
    virtual bool isLastClearStageKinopioBrigade() const;
    virtual bool isLastClearStageUseDrc() const;
    virtual bool isShowCharacterChangeExplain() const;
    virtual bool isOpenNetworkSetting() const;
    virtual bool isShowNetworkGuide() const;
    virtual bool isShowAmiiboGuide() const;
    virtual bool isShowSnapshotGuide() const;
    virtual bool isEnableCancelBossDemo() const;

    /**
     * @brief Remember that the boss demo was already shown in this stage.
     */
    virtual void setAlreadyShowBossDemo() { mIsAlreadyShowBossDemo = true; }

    /**
     * @brief Check whether the player is inside a superb-view area.
     * @return True inside a superb-view area.
     */
    virtual bool isIsInsideSuperbView() const { return mIsInsideSuperbView; }

    /**
     * @brief Mark the player as inside a superb-view area.
     */
    virtual void setIsInsideSuperbView() { mIsInsideSuperbView = true; }

    /**
     * @brief Mark the player as outside any superb-view area.
     */
    virtual void resetIsInsideSuperbView() { mIsInsideSuperbView = false; }

    /**
     * @brief Access the play-log flags of the current stage.
     * @return The stage's play-log flag bits.
     */
    virtual u32* getPlayLogStageFlag() { return &mPlayLogStageFlag; }

    virtual bool read3DWorldFromStream(sead::ReadStream* pStream);

    void playReportOptionsEvent();
    void playReportStageEvent(int eventId);
    void trySaveCourseGreenStars(bool isPlayLog);
    void resetStageScore();
    void reenterStage();
    int getPlayerLifeSaved() const;
    bool isShowTouchGuide() const;

    static u16 getOptions();
    static void setOptions(u16 options);
    static bool getCameraReverseHorizontal();
    static bool getCameraReverseVertical();
    static void initCameraSettings();
    static void setCameraReverseVertical(GameDataHolder* pHolder, bool reverse);
    static void setCameraReverseHorizontal(GameDataHolder* pHolder, bool reverse);
    static void setKinopioBrigadeCameraReverseVertical(GameDataHolder* pHolder, bool reverse);
    static bool getKinopioBrigadeCameraReverseVertical();
    static void setKinopioBrigadeCameraReverseHorizontal(GameDataHolder* pHolder, bool reverse);
    static bool getKinopioBrigadeCameraReverseHorizontal();

    /**
     * @brief Access the saved stock items of the file.
     * @return The file's stock-item list.
     */
    StockItemList* getStockItemList() const { return mpStockItems; }

    /**
     * @brief Read the green star total recorded at the last save.
     * @return The saved green star total.
     */
    s32 getTotalAcquireGreenStarNumSaved() const { return mTotalAcquireGreenStarNumSaved; }

    /**
     * @brief Count a stage restart chosen from the pause menu.
     */
    void incMenuRestartCount() { mUnknown98++; }

    /**
     * @brief Set progression flag bits.
     * @param flag Progression flag bits to set.
     */
    void onGameFlag(u32 flag) { mGameFlag |= flag; }

    /**
     * @brief Clear progression flag bits.
     * @param flag Progression flag bits to clear.
     */
    void offGameFlag(u32 flag) { mGameFlag &= ~flag; }

    /**
     * @brief Check whether the file was resumed after a game over.
     * @return True after a game over.
     */
    bool isAfterGameOver() const { return mUnknownAC; }

    /**
     * @brief Forget that the file was resumed after a game over.
     */
    void resetAfterGameOver() { mUnknownAC = false; }

    /**
     * @brief Check whether the ending was started.
     * @return True after the ending.
     */
    bool isAfterEnding() const { return mIsStartEnding; }

    /**
     * @brief Forget that the ending was started.
     */
    void resetAfterEnding() { mIsStartEnding = false; }

    /** @brief Requests a return to the world map when the current stage is left. */
    void requestReturnToMap() { mUnknownF3 = true; }

    /** @brief Requests a restart of the current stage when it is left. */
    void requestRestartStage() { mUnknownF4 = true; }

  private:
    friend inline void GameDataFileInternal::resetKinopioHouse(GameDataFile* pFile);
    friend class GameDataHolder;

    static u8 sCameraSettings;

    s32 mPlayerLife;
    s32 mPlayerLifeSaved;
    s32 mCasinoRoomCounter;
    s32 mGoldenExpressCounter;
    s32 mRetryCount;
    s32 mRetryCountSaved;
    s32 mUnknown98;
    s32 mLastPlayCourseId;
    s32 mLastPlayCourseIdSaved;
    s32 mTotalAcquireGreenStarNumSaved;
    u32 mGameFlag;
    bool mUnknownAC;
    bool mIsStartEnding;
    s32 mMiiverseSetting;
    s32 mGhostSetting;
    u16 mNetworkSetting;
    CourseInfoHolder* mpCourseInfoHolder;
    WorldGameDataHolder* mpWorldGameDataHolder;
    StockItemList* mpStockItems;
    StageDataHolder* mpStageDataHolder;
    s32 mLastTotalAcquireGreenStarNum;
    s32 mLastStageBestScoreUserId;
    s32 mGhostPresentCounter;
    s32 mClearStarLevel;
    bool mIsInsideSuperbView;
    bool mIsAlreadyShowBossDemo;
    bool mIsGameOver;
    bool mUnknownF3;
    bool mUnknownF4;
    s32 mPlayWorldId;
    s32 mMissCount;
    u32 mPlayLogStageFlag;
    nn::TimeSpan mStageStartActiveTime;
    s64 mUnknown110;
    sead::DateTime mStageStartTime;
    s32 mGoalCharacters[4];
    s32 mGoalCharacterNum;
};
static_assert(sizeof(GameDataFile) == 0x138);
