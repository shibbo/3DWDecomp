#pragma once
#include "System/Data/StageUserData.hpp"
class GameDataHolder;
class StockItemList;
class CourseGreenStarInfo;
class StageDataHolder {
  public:
    explicit StageDataHolder(GameDataHolder* pHolder);
    void initializeData();
    void startStage(int courseId);
    void setGreenStarAcquireFlag(const CourseGreenStarInfo& rStars);
    void startTitle();
    void restartStage();
    void restartMysteryBox();
    void restartTimeupMysteryBox();
    void reenterStage();
    void resetStageScore();
    void gameOverStage();
    void clearStage(bool firstClear, bool newBestScore, bool newBestTime);
    void clearStageWorldWarp();
    int getStageBestScore() const;
    int getStageBestTime() const;
    int getTotalScore() const;
    int tryCalcLastStageBestScoreUserID() const;
    void resetStockItems();
    void acquireGreenStar(int starIndex);
    bool isAcquireGreenStar(int starIndex) const;
    void setCheckpointPass(int characterType);
    bool isContinuousMysteryBox() const;
    bool isRestartFromCheckpoint() const;
    bool isAcquireIllustItem() const;
    void setContinuousMysteryBox();
    void retireStage();
    void acquireIllustItem();
    void setUseAssistBlock();
    void resetCheckpointPass();
    bool isCheckpointPass() const;
    void setAcquireIllustItem(bool acquired);
    void setStagePlayerNumForAssistBlock(int count);
    int getStagePlayerNumForAssistBlock() const;
    void addTeamScore(int score);
    void addScore(int score, int userId);
    int getScore(int userId) const;
    bool isAlive(int userId) const;
    bool isGoalSuccess(int userId) const;
    float getGoalHeight(int userId) const;
    bool isGoalLeader(int userId) const;
    int getPlayerFigureType(int userId) const;
    void setAlive(int userId, bool alive);
    void setPlayerFigureType(int userId, int figureType);
    void setGoalState(int userId, bool success, float height, bool leader);
    bool stockItem(int itemId);
    void useStockItem();
    int getStockItem(int index) const;
    void setStampPickupCharType(int characterType);
    const CourseGreenStarInfo* getGreenStarAcquireFlag() const;
    int getCheckpointPassPlayerCharacter() const;
    void setStageTimerFrame(int frames);
    void addStageTimerFrame(int frames);
    void decStageTimerFrame(int frames);
    void countUpPlayTime(int frames);
    int calcStageTimerCount() const;
    int calcTimeAttackCount() const;
    static int calcStageTimerCountToFrame(int count);

    /**
     * @brief Read the course being played.
     * @return The playing course identifier.
     */
    int getCourseId() const { return mCourseId; }

    /**
     * @brief Check whether the last stage was cleared.
     * @return True when the stage was cleared.
     */
    bool isCleared() const { return mCleared; }

    /**
     * @brief Check whether the last stage was cleared through a world warp.
     * @return True when the stage was cleared by a warp.
     */
    bool isWorldWarpClear() const { return mWorldWarpClear; }

    /**
     * @brief Check whether the last clear was the first one.
     * @return True for a first clear.
     */
    bool isFirstClear() const { return mFirstClear; }

    /**
     * @brief Check whether the last stage awarded its stamp for the first time.
     * @return True when the stamp was newly acquired.
     */
    bool isFirstStamp() const { return mFirstStamp; }

    /**
     * @brief Check whether the last clear set a new best score.
     * @return True for a new best score.
     */
    bool isNewBestScore() const { return mNewBestScore; }

    /**
     * @brief Check whether the last clear set a new best time.
     * @return True for a new best time.
     */
    bool isNewBestTime() const { return mNewBestTime; }

    /**
     * @brief Check whether the stage is being restarted.
     * @return True while restarting.
     */
    bool isRestart() const { return mRestart; }

    /**
     * @brief Check whether the stage was retired.
     * @return True when the player retired.
     */
    bool isRetired() const { return mRetired; }

    /**
     * @brief Check whether the assist block is in use in the current attempt.
     * @return True when the assist block was used.
     */
    bool isUseAssistBlock() const { return mAssistCurrent; }

    /**
     * @brief Access the stage's stock items.
     * @return The stock-item list of the playing stage.
     */
    StockItemList* getStockItemList() const { return mpStockItems; }

    /**
     * @brief Check whether the stage start demo should be skipped.
     * @return True when the start demo is skipped.
     */
    bool isSkipStartDemo() const { return mUnknown16; }

    /**
     * @brief Choose whether the stage start demo should be skipped.
     * @param isSkip True to skip the start demo.
     */
    void setSkipStartDemo(bool isSkip) { mUnknown16 = isSkip; }

    /**
     * @brief Read the remaining stage timer frames.
     * @return The remaining timer frames.
     */
    int getStageTimerFrame() const { return mTimerFrames; }

  private:
    /**
     * @brief Clamp the stage timer to its supported range.
     * @param frames Requested number of timer frames.
     * @return A frame count from 0 through 43956.
     */
    static int clampTimer(int frames) {
        const int capped = frames < 43956 ? frames : 43956;
        return capped < 0 ? 0 : capped;
    }

    GameDataHolder* mpHolder;
    int mCourseId;
    bool mPlaying;
    bool mCleared;
    bool mWorldWarpClear;
    bool mFirstClear;
    bool mFirstStamp;
    bool mNewBestScore;
    bool mNewBestTime;
    bool mRestart;
    bool mMysteryBox;
    bool mRestartFromCheckpoint;
    bool mUnknown16;
    bool mRetired;
    bool mStampEntry;
    bool mStampCurrent;
    bool mStampCheckpoint;
    bool mAssistCurrent;
    bool mAssistCheckpoint;
    int mCheckpointCharacter;
    int mUnknown24;
    int mUnknown28;
    u64 mUnknown30;
    int mTimerFrames;
    int mPlayFrames;
    int mTeamScore;
    int mAssistPlayerCount;
    StageUserData* mpUsers;
    StockItemList* mpStockItems;
    CourseGreenStarInfo* mpStarsCurrent;
    CourseGreenStarInfo* mpStarsCheckpoint;
    int mStampCharacter;
};
static_assert(sizeof(StageDataHolder) == 0x70);
