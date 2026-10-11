#pragma once

#include <container/seadRingBuffer.h>
#include <math/seadVector.h>

#include "Layout/SceneLayoutBase.hpp"
#include "Library/Layout/LayoutActor.hpp"
#include "Library/Scene/ISceneObj.hpp"
#include "System/ScenarioInfo.hpp"

namespace al {
class HitSensor;
class LayoutInitInfo;
class LiveActor;
class PlayerHolder;
class SceneCameraInfo;
class WipeSimple;
}  // namespace al

class AreaNameParts;
class ChallengeTimerParts;
class CounterCoinParts;
class CounterGoalItemParts;
class CounterTimerGate;
class GameDataHolder;
class GreenStarKeeper;
class GuideFrameOutSingleMode;
class ItemStockTray;
class PlayerAliveWatcher;
class ProjectItemDirector;
class ScenarioShineCounterParts;
class ShardCounterParts;
class TimerGate;

/**
 * @brief Which buttons the menu / map guides of the HUD show.
 */
enum MenuGuideState {
    MenuGuideState_None = -1,        ///< Not set yet; forces the next update.
    MenuGuideState_Normal = 0,       ///< Single player or full controllers: + menu, - map.
    MenuGuideState_SameJoyLeft = 1,  ///< 2P assist mode with two left Joy-Cons: - for both.
    MenuGuideState_SameJoyRight = 2, ///< 2P assist mode with two right Joy-Cons: + for both.
};

typedef sead::FixedRingBuffer<sead::Vector2f, 256> HitPointBuffer;

/**
 * @brief The HUD layout of Bowser's Fury.
 */
class SingleModeSceneLayout : public al::LayoutActor, public SceneLayoutBase, public al::ISceneObj {
public:
    SingleModeSceneLayout(const al::LayoutInitInfo& rInfo, GameDataHolder* pGameDataHolder,
                          const char* pName, const GreenStarKeeper* pGreenStarKeeper,
                          const al::PlayerHolder* pPlayerHolder,
                          const PlayerAliveWatcher* pAliveWatcher,
                          ProjectItemDirector* pItemDirector);

    static SingleModeSceneLayout* tryGetSingleModeSceneLayout(const al::IUseSceneObjHolder* pHolder);
    static void tryDisablePause(const al::IUseSceneObjHolder* pHolder, bool isDisable);

    void updateCounters(s32 islandId);
    void setInDemo(bool isInDemo);
    void appear() override;
    void updateMenuGuideState(MenuGuideState state);
    void control() override;
    void startDemo(bool isHideAll, bool isEndAction);
    void endDemo(bool isAppear, bool isUpdateItems);
    void prepEndDemo();
    void startPause(bool isShowItemStock);
    void endPause();
    void courseClear();
    void setAreaNameWipeFadeWhite(al::WipeSimple* pWipe);
    void updateAreaName(s32 islandId);
    void setAreaNamePhaseStart(bool isPhaseStart);
    void fadeOutAreaName(bool isForce);
    void disableItemStock();
    void stockItem(const al::LiveActor* pItem, const al::LiveActor* pPlayer,
                   s32 itemType) override;
    void stockItemSilent(const al::LiveActor* pItem, const al::LiveActor* pPlayer,
                         s32 itemType) override;
    void appearItemStock(s32 port);
    bool isItemStockStartClose() const;
    bool isItemStockEnd() const;
    bool canSpawnItem() const;
    s32 getItemStockControllerID() const;
    void spawnItemStockCoins(u8 count, al::HitSensor* pSensor);
    void calcHitPane(bool* pIsHit, const char** pPaneNames, s32 paneNum,
                     const HitPointBuffer& rPoints);
    void addShard(s32 islandId, s32 shardIndex, bool isComplete, bool isDemo);
    void showShardCounter();
    void hideShardCounter();
    void startShardDemo();
    void showShineCounter();
    void hideShineCounter();
    void forceShineCounterEndAppear();
    void forceHideShineCounter();
    void addShine();
    void exeWait();
    void setBowserGuideFrameOut(GuideFrameOutSingleMode* pGuide);
    void showTimer();
    void setTimer(s32 frames);
    void startTimer();
    void hideTimer(bool isForce);
    void setTimerRed();
    void pauseTimer(bool isPause);
    void displayTimer();
    bool isTimerActive();
    void showPlessieTimer();
    void setPlessieTimer(s32 frames, s32 maxFrames);
    void startPlessieTimer();
    void hidePlessieTimer(bool isForce);
    void pausePlessieTimer(bool isPause);
    void setPlessieTimerGate(TimerGate* pTimerGate);
    void setEnableKoopaJrGuideFrameOut(bool isEnable);
    void setDisableAreaName(bool isDisable);
    void handleIslandWarp();

    al::SceneCameraInfo* getSceneCameraInfo() const override { return mSceneCameraInfo; }
    const char* getSceneObjName() const override { return "SingleModeSceneLayout"; }

    /**
     * @brief Get the full-screen wipe used to hide cutscene skips.
     * @return The wipe.
     */
    al::WipeSimple* getWipe() const { return mWipe; }

    /**
     * @brief Get the black full-screen wipe.
     * @return The black wipe.
     */
    al::WipeSimple* getWipeBlack() const { return mWipeBlack; }

    /**
     * @brief Get the number of frames the wipe takes to close or open.
     * @return The wipe duration in frames.
     */
    s32 getWipeFrames() const { return mWipeFrames; }

    /**
     * @brief Set whether the pause menu is disabled.
     * @param isDisable Whether the pause menu is disabled.
     */
    void setDisablePause(bool isDisable) { mIsDisablePause = isDisable; }

    /**
     * @brief Check whether opening the pause menu is disabled.
     * @return True while the pause menu cannot be opened.
     */
    bool isDisablePause() const { return mIsDisablePause; }

private:
    CounterCoinParts* mCounterCoin = nullptr;             // 0x138
    CounterGoalItemParts* mCounterGoalItem;               // 0x140
    AreaNameParts* mAreaName;                             // 0x148
    ScenarioInfo mScenarioInfo = {-1, -1};                // 0x150
    GameDataHolder* mGameDataHolder;                      // 0x158
    const al::PlayerHolder* mPlayerHolder;                // 0x160
    al::WipeSimple* mWipeBlack = nullptr;                 // 0x168
    al::WipeSimple* mWipe = nullptr;                      // 0x170
    s32 mWipeFrames = 15;                                 // 0x178
    al::SceneCameraInfo* mSceneCameraInfo;                // 0x180
    ItemStockTray* mItemStockTray = nullptr;              // 0x188
    ShardCounterParts* mShardCounter = nullptr;           // 0x190
    ScenarioShineCounterParts* mScenarioShineCounter;     // 0x198
    ChallengeTimerParts* mChallengeTimer = nullptr;       // 0x1a0
    CounterTimerGate* mCounterTimerGate = nullptr;        // 0x1a8
    bool _1b0 = false;                                    // 0x1b0
    bool mIsShowMap = false;                              // 0x1b1
    s32 mMapHideTimer = 90;                               // 0x1b4
    MenuGuideState mMenuGuideState = MenuGuideState_None;  // 0x1b8
    GuideFrameOutSingleMode* mKoopaJrGuide = nullptr;     // 0x1c0
    GuideFrameOutSingleMode* mBowserGuide = nullptr;      // 0x1c8
    bool mIsInDemo = true;                                // 0x1d0
    bool mIsEndDemoReady = false;                         // 0x1d1
    bool mIsDisableAreaName = false;                      // 0x1d2
    bool mIsDisablePause = false;                         // 0x1d3
};

static_assert(sizeof(SingleModeSceneLayout) == 0x1d8);
