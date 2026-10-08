#pragma once

#include <container/seadRingBuffer.h>
#include <math/seadVector.h>

#include "Layout/SceneLayoutBase.hpp"
#include "Library/Layout/LayoutActor.hpp"

namespace al {
class LayoutInitInfo;
class LiveActor;
class PlayerHolder;
class SceneCameraInfo;
}  // namespace al

class ButtonItemStockParts;
class CounterCoinParts;
class CounterGreenStarParts;
class CounterPlayerParts;
class CounterScoreParts;
class CounterStampParts;
class GameDataHolder;
class GreenStarKeeper;
class IllustItemKeeper;
class PlayerAliveWatcher;
class ProjectItemDirector;
class StageTimer;

typedef sead::FixedRingBuffer<sead::Vector2f, 256> HitPointBuffer;

/**
 * @brief The HUD of a normal (Super Mario 3D World) stage: timer, counters and item stock.
 */
class StageSceneLayout : public al::LayoutActor, public SceneLayoutBase {
public:
    StageSceneLayout(const al::LayoutInitInfo& rInfo, GameDataHolder* pGameDataHolder,
                     const char* pStageName, const GreenStarKeeper* pGreenStarKeeper,
                     const IllustItemKeeper* pIllustItemKeeper,
                     const al::PlayerHolder* pPlayerHolder,
                     const PlayerAliveWatcher* pPlayerAliveWatcher,
                     ProjectItemDirector* pItemDirector);

    void startDemo(bool isHideAll, bool isEndAction);
    void setNameplatesVisible(bool isVisible, bool isForce);
    void endDemo(bool isAppear, bool isResetAction);
    void startPause();
    void endPause();
    void courseClear();
    void disableItemStock();
    void stockItem(const al::LiveActor* pItem, const al::LiveActor* pPlayer,
                   s32 itemType) override;
    void stockItemSilent(const al::LiveActor* pItem, const al::LiveActor* pPlayer,
                         s32 itemType) override;
    void calcHitPane(bool* pIsHit, const char** pPaneNames, s32 paneNum,
                     const HitPointBuffer& rPoints);
    void exeWait();
    void setStageKinopioBrigade();

    al::SceneCameraInfo* getSceneCameraInfo() const override { return mSceneCameraInfo; }

    /**
     * Gets the stage timer.
     * @return The stage timer.
     */
    StageTimer* getStageTimer() const { return mStageTimer; }

private:
    CounterCoinParts* mCounterCoin = nullptr;            // 0x130
    StageTimer* mStageTimer = nullptr;                   // 0x138
    CounterGreenStarParts* mCounterGreenStar = nullptr;  // 0x140
    CounterPlayerParts* mCounterPlayer = nullptr;        // 0x148
    CounterScoreParts* mCounterScore = nullptr;          // 0x150
    CounterStampParts* mCounterStamp = nullptr;          // 0x158
    ButtonItemStockParts* mButtonItemStock;              // 0x160
    GameDataHolder* mGameDataHolder;                     // 0x168
    const al::PlayerHolder* mPlayerHolder;               // 0x170
    al::SceneCameraInfo* mSceneCameraInfo;               // 0x178
};

static_assert(sizeof(StageSceneLayout) == 0x180);
