#pragma once

#include <basis/seadTypes.h>
#include <container/seadPtrArray.h>
#include <gfx/seadCamera.h>
#include <gfx/seadProjection.h>
#include <gfx/seadViewport.h>
#include <prim/seadSafeString.h>
#include "Scene/InGameSceneBase.hpp"

namespace al {
class ActorInitInfo;
class NetworkSystem;
class NfpDirector;
class ScreenCaptureExecutor;
class SimpleLayoutAppearWait;
class StageInfo;
class WipeSimple;
}  // namespace al

namespace rc {
class StampDirector;
}  // namespace rc

class BlockChoiceWatcher;
class CameraChangeLayout;
class CameraObserver;
class CheckpointFlag;
class DemoKoopaW7;
class DokanWorldWarp;
class DrcAssistDirectorList;
class FairyHouseIllustItemWatcher;
class GameDataHolder;
class GhostPlayerDirector;
class GoalObjHolder;
class GoalPole;
class MicInputSePlayer;
class MysteryHouseChecker;
class PauseMenu;
class PlayerActor;
class PlayerAliveWatcher;
class PlayerBigBgmController;
class PlayerCrown;
class PlayerEntryMini;
class PlayerInvincibleBgmController;
class PlayerRetargettingSelector;
class ResultTimerCount;
class SnapshotLayout;
class SnapshotState;
class StageSceneLayout;
class StageSceneStateGameOver;
class StageStartEventBase;
class StageTimer;
class StageWipeKeeper;

/**
 * @brief Scene of a Super Mario 3D World course: start event, play, pause, goal, miss and the
 *        transitions to the next scene.
 */
class StageScene : public InGameSceneBase {
public:
    using PlayerArray = sead::FixedPtrArray<PlayerActor, 8>;

    explicit StageScene(StageWipeKeeper* pStageWipeKeeper);
    ~StageScene() override;

    void init(const al::SceneInitInfo& rInfo) override;
    void initPlacement(const al::ActorInitInfo& rInfo);
    void appear() override;
    void decidePlayerPlacement();
    void kill() override;
    void control() override;
    void drawMain_() const override;
    bool isDraw3D() const;
    const char* getDraw2DKitMainName() const;
    void drawSub_() const override;
    bool isGameOver() const;
    bool isRestartStage() const;
    const char* getDraw2DKitSubName() const;
    bool isNotExistStartDemo() const;
    bool isGoal() const override;
    bool isRetire() const;
    bool isReenterStage() const;
    bool isDeadGoldenExpress() const;
    bool isRestartMysteryBox() const;
    bool isTimeUpMysteryBox() const;
    bool isGameEnd() const;
    bool isGameChange() const;
    bool isLoadGame() const;
    bool isWorldWarp() const override;
    bool isUseResult() const;
    bool isRestartCheck() const override;
    void prepareDestroy() override;
    void recordClearData() override;
    void invalidatePlayerInput();
    bool isEnableOpenStartWipe() const;
    void exeStartEvent();
    void updateStartEvent();
    void exeStartBindDemo();
    void updatePlay();
    void exeStart();
    void exePlay();
    void forceKillPlayerAll();
    void exeDemoChangePlayer();
    void updateDemoChangePlayer();
    void exeDemoChangePlayerAfter();
    void updateDemoChangePlayerAfter();
    void exeDemoCamera();
    void updateDemoCamera();
    void exeDemoScene();
    void exePreGoalDemo();
    void updateDemoPlayerOnly();
    void exeGoalDemo();
    void exePauseWindowMessage();
    void exeResultTimerCount();
    void updateLayout();
    void exeKoopaDemoW7();
    void exeGoal();
    bool isTriggerPause(s32* pPort) const override;
    void exePause();
    void startLayoutPause();
    void updatePauseLayout();
    void endLayoutPause();
    void exeRetire();
    void exeReenterStage();
    void exeRestart();
    void updateMissDemo(bool isUpdateGraphics, bool isSkipVehicle);
    void exeGameOver();
    void exeGameEnd();
    void exePreTimeUp();
    void exeTimeUp();
    void exeMysteryBoxTimeUp();
    void exeWorldWarp();
    void exeIllustItemResult();
    void exeCaptureMode();
    void initPlacementCheckpoint(const al::ActorInitInfo& rInfo);
    void initPlacementPlayer(const al::StageInfo* pStageInfo, const al::ActorInitInfo& rInfo,
                             PlayerRetargettingSelector* pSelector);
    void initPlacementGoal(const al::ActorInitInfo& rInfo);
    void initPlacementObject(const al::StageInfo* pStageInfo, const al::ActorInitInfo& rInfo,
                             const char* pListName);

    /**
     * Gets the id of the course this scene plays.
     * @return The course id.
     */
    s32 getCourseId() const { return mCourseId; }

private:
    StageWipeKeeper* mStageWipeKeeper;                           // 0xe8
    sead::FixedSafeString<0x40> mStageName;                      // 0xf0
    bool mIsTimerDisabled = false;                               // 0x148
    bool mIsPrepareDestroyed = false;                            // 0x149
    bool mIsNarrowPlace = false;                                 // 0x14a
    bool mIsControlled = false;                                  // 0x14b
    s32 mWorldId = 0;                                            // 0x14c
    s32 mStageId = 0;                                            // 0x150
    s32 mCourseId = 0;                                           // 0x154
    GameDataHolder* mGameDataHolder = nullptr;                   // 0x158
    CameraObserver* mCameraObserver = nullptr;                   // 0x160
    StageStartEventBase* mStartEvent = nullptr;                  // 0x168
    DemoKoopaW7* mDemoKoopaW7 = nullptr;                         // 0x170
    mutable sead::Viewport mMainViewport;                        // 0x178, set while drawing
    sead::Viewport mSubViewport;                                 // 0x1a0
    GoalObjHolder* mGoalObjHolder = nullptr;                     // 0x1c8
    GoalPole* mGoalPole = nullptr;                               // 0x1d0
    DokanWorldWarp* mDokanWorldWarp = nullptr;                   // 0x1d8
    MysteryHouseChecker* mMysteryHouseChecker = nullptr;         // 0x1e0
    BlockChoiceWatcher* mBlockChoiceWatcher = nullptr;           // 0x1e8
    bool mIsKinopioBrigade = false;                              // 0x1f0
    CheckpointFlag* mCheckpointFlag = nullptr;                   // 0x1f8
    PlayerArray mPlayers;                                        // 0x200
    PlayerCrown* mPlayerCrown = nullptr;                         // 0x250
    PlayerAliveWatcher* mPlayerAliveWatcher = nullptr;           // 0x258
    DrcAssistDirectorList* mDrcAssistDirectorList = nullptr;     // 0x260
    GhostPlayerDirector* mGhostPlayerDirector = nullptr;         // 0x268
    bool mIsGhostRecording = false;                              // 0x270
    StageSceneStateGameOver* mStateGameOver = nullptr;           // 0x278
    StageSceneLayout* mStageSceneLayout = nullptr;               // 0x280
    CameraChangeLayout* mCameraChangeLayout = nullptr;           // 0x288
    StageTimer* mStageTimer = nullptr;                           // 0x290
    PlayerEntryMini* mPlayerEntryMini = nullptr;                 // 0x298
    ResultTimerCount* mResultTimerCount = nullptr;               // 0x2a0
    al::SimpleLayoutAppearWait* mMissLayout = nullptr;           // 0x2a8
    al::WipeSimple* mMissWipe = nullptr;                         // 0x2b0
    void* _2b8 = nullptr;                                        // 0x2b8
    MicInputSePlayer* mMicInputSePlayer = nullptr;               // 0x2c0
    PlayerInvincibleBgmController* mInvincibleBgmController = nullptr;  // 0x2c8
    PlayerBigBgmController* mBigBgmController = nullptr;         // 0x2d0
    PauseMenu* mPauseMenu = nullptr;                             // 0x2d8
    s32 mPausePort = -1;                                         // 0x2e0
    al::ScreenCaptureExecutor* mScreenCaptureExecutor = nullptr; // 0x2e8
    al::NetworkSystem* mNetworkSystem = nullptr;                 // 0x2f0
    FairyHouseIllustItemWatcher* mIllustItemWatcher = nullptr;   // 0x2f8
    al::NfpDirector* mNfpDirector = nullptr;                     // 0x300
    s32 mInvalidateInputFrame = 0;                               // 0x308
    sead::Viewport mStereoMainViewport;                          // 0x310
    sead::Viewport mStereoSubViewport;                           // 0x338
    sead::DirectProjection mStereoProjectionLeft;                // 0x360
    sead::DirectProjection mStereoProjectionRight;               // 0x458
    sead::DirectCamera mStereoCameraLeft;                        // 0x550
    sead::DirectCamera mStereoCameraRight;                       // 0x5b8
    bool mIsWaitEffectBeforeKill = false;                        // 0x620
    s32 mCameraModeBeforeCapture = -1;                           // 0x624
    void* _628 = nullptr;                                        // 0x628
    rc::StampDirector* mStampDirector = nullptr;                 // 0x630
    SnapshotState* mSnapshotState = nullptr;                     // 0x638
    SnapshotLayout* mSnapshotLayout = nullptr;                   // 0x640
};

static_assert(sizeof(StageScene) == 0x648);
