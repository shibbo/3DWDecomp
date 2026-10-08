#pragma once

#include <basis/seadTypes.h>
#include <container/seadPtrArray.h>
#include <gfx/seadViewport.h>
#include <math/seadMatrix.h>
#include <prim/seadSafeString.h>
#include "Library/Draw/GraphicsInitArg.hpp"
#include "Library/Scene/IScenarioCompleteChecker.hpp"
#include "Scene/InGameSceneBase.hpp"

namespace al {
class ActorInitInfo;
class GraphicsSystemInfo;
class ISceneObj;
class LayoutInitInfo;
class LiveActor;
class NetworkSystem;
class NfpDirector;
class ScreenCaptureExecutor;
class StageInfo;
class ViewRenderer;
class WipeSimple;
}  // namespace al

namespace rc {
class StampDirector;
}  // namespace rc

class DemoCutscene;
class DrcAssistDirectorList;
class GameDataHolder;
class GuideGameWindow;
class IntroFlyOverCamera;
class IslandAreaWatcher;
class IslandDataList;
class IslandKeeper;
class IslandMap;
class IslandWarpState;
class OceanScenarioList;
class PauseMenu;
class PlayerActor;
class PlayerAliveWatcher;
class PlayerBigBgmController;
class PlayerCameraTarget;
class PlayerCrown;
class PlayerInvincibleBgmController;
class PlayerRetargettingSelector;
class RaidonSurf;
class ShadowMarioDirector;
class ShardsWatcherHolder;
class SingleModeCheckpoint;
class SingleModeDrawer3D;
class SingleModeSceneLayout;
class SnapshotLayout;
class SnapshotState;
class StageStartEventBase;
class WindowProcessing;

/**
 * @brief Base scene of Bowser's Fury: the open world of Lake Lapcat, its start demos,
 *        cutscenes, pause menus, island map and island warps.
 */
class SingleModeScene : public InGameSceneBase,
                        public al::ViewRendererCreator,
                        public al::IScenarioCompleteChecker {
  public:
    using PlayerArray = sead::FixedPtrArray<PlayerActor, 8>;
    using CheckpointArray = sead::PtrArray<SingleModeCheckpoint>;

    explicit SingleModeScene(const char* pName);
    ~SingleModeScene() override;

    void init(const al::SceneInitInfo& rInfo) override;
    void appear() override;
    void kill() override;
    void control() override;
    bool isValidPlacementParent(const al::PlacementInfo& rInfo) const override;
    bool isValidPlacement(const al::PlacementInfo& rInfo) const override;
    void drawMain_() const override;
    void prepareDestroy() override;
    bool isRestartCheck() const override;
    bool isGoal() const override;
    bool isTriggerPause(s32* pPort) const override;

    bool isScenarioComplete(s32 scenarioNo, s32 shineNum) override;
    al::ViewRenderer* createViewRenderer(al::GraphicsSystemInfo* pInfo) override;
    void deleteViewRenderer(al::ViewRenderer* pRenderer) override;
    virtual bool isChangePhase() const;
    virtual bool allowRestartPoint() const;
    virtual bool isIslandScene() const;
    virtual bool isBossScene() const;
    virtual bool isGameEnd() const;
    virtual bool isPhaseEnd() const;
    virtual void handlePhaseEnd();
    virtual void requestStageBgmStart();
    virtual void initIslandDataList();
    virtual void initOceanResourceKeeper();
    virtual const al::StageInfo* findOceanStageInfo();
    virtual void initOceanScenarios();
    virtual void initIslandMap(const al::LayoutInitInfo& rLayoutInfo,
                               const al::ActorInitInfo& rActorInfo);
    virtual void handleGameOver();
    virtual void preInitPlacement(const al::ActorInitInfo& rInfo);
    virtual bool doZoneIDCheck() const;
    virtual void initAreaObj(const al::ActorInitInfo& rInfo);
    virtual void initPlacement(al::ActorInitInfo& rInfo);
    virtual void initPlacementZoneHolders(const al::ActorInitInfo& rInfo);
    virtual void initPlacementObject(const al::StageInfo* pStageInfo,
                                     const al::ActorInitInfo& rInfo, const char* pListName);
    virtual void initPlacementOceanWater(al::ActorInitInfo& rInfo);
    virtual void initPlacementDisasterModeBowser(const al::ActorInitInfo& rInfo);
    virtual void initPlacementLuckyIsland(const al::ActorInitInfo& rInfo);
    virtual void initPlacementRaidonSurf(const al::StageInfo* pStageInfo,
                                         const al::ActorInitInfo& rInfo);
    virtual void initPlacementIntroCameras(const al::ActorInitInfo& rInfo);
    virtual void initIslandKeeper();
    virtual void endInitIslandKeeper();
    virtual void updateDemoCutsceneAddOn();

    void finishSave();
    void setCameraZoneID();
    void decidePlayerPlacement();
    void updateSave();
    bool isDraw3D() const;
    const char* getDraw2DKitMainName() const;
    bool isRestart() const;
    bool isReenterStage() const;
    bool isRetire() const;
    bool isGameChange() const;
    bool isLoadGame() const;
    bool isTriggerBack() const;
    bool isTriggerMapMenu(s32* pPort) const;
    bool isTriggerItemSelect(s32* pPort) const;
    bool isTriggerItemSpawn(s32 port) const;
    void invalidatePlayerInput();
    bool isEnableOpenStartWipe() const;
    void exeStartPhase0Demo();
    void updatePlay();
    void exeStartEvent();
    void updateStartEvent();
    void exeStartBindDemo();
    void exeStart();
    bool checkDemoIntroChange(bool isFromPlay);
    bool checkDemoChange();
    bool checkGameOver();
    void exePlay();
    void startSave();
    void exeDemoChangePlayer();
    void updateDemoChangePlayer();
    void exeDemoChangePlayerAfter();
    void updateDemoChangePlayerAfter();
    void exeDemoCamera();
    void exeDemoMovingCamera();
    void updateDemoMovingCamera(bool isUpdateKit);
    void finishDemoIntro();
    void exeDemoIntro();
    void exeDemoScene();
    void exeDemoCutscene();
    void updateDemoCutscene(bool isUpdatePlayer, bool isUpdateCamera);
    void exeDemoPlayerScene();
    void exeDemoKoopaJrPhaseIntro();
    void exeDemoKoopaJrPhaseIntro2Part();
    void exeDemoIslandMapPreGame();
    void updatePauseLayout();
    void updateLayout();
    void exeDemoIslandMap();
    void exeDemoIslandMap2Part();
    void exeWaitForKoopaJrOptionsIntro();
    void exeKoopaJrOptionsIntro();
    void exeDemoInGameCutscene();
    void exeGoal();
    void exeDemoFadeTransition();
    void exePauseIslandMap();
    void exePauseItemSelect();
    void exePauseWindowMessage();
    void exePause();
    void exeRestart();
    void updateMissDemo(bool isUpdateGraphics, bool isSkipVehicle);
    void exeReenterStage();
    void exeRetire();
    void exeGameEnd();
    void exePreTimeUp();
    void exeTimeUp();
    void exeCaptureMode();
    void exeIslandWarp();
    void updateIslandWarp();
    void initPlacementGoalItems(const al::ActorInitInfo& rInfo);
    void initPlacementIslandFlags(const al::ActorInitInfo& rInfo);
    void initPlacementCheckpoint(const al::ActorInitInfo& rInfo);
    void initPlacementPlayer(const al::StageInfo* pStageInfo, const al::ActorInitInfo& rInfo,
                             PlayerRetargettingSelector* pSelector);
    void initPlacementGoal(const al::ActorInitInfo& rInfo);
    void initPlacementOceanWater(const al::StageInfo* pStageInfo, al::ActorInitInfo& rInfo);
    void forceKillPlayerAll();
    void updateFreezeMode(bool isFreeze);
    const char* getDraw2DKitSubName() const;
    bool isNotExistStartDemo() const;

  protected:
    sead::FixedSafeString<0x40> mStageName;                          // 0xf8
    bool mIsPrepareDestroyed;                                        // 0x150
    bool mIsRequestSave;                                             // 0x151
    bool mIsSaving;                                                  // 0x152
    bool mIsNarrowPlace;                                             // 0x153
    s32 mStartingFigure;                                             // 0x154
    GameDataHolder* mGameDataHolder;                                 // 0x158
    IslandDataList* mIslandDataList;                                 // 0x160
    OceanScenarioList* mOceanScenarioList;                           // 0x168
    StageStartEventBase* mStartEvent;                                // 0x170
    StageStartEventBase* mPhase0StartDemo;                           // 0x178
    mutable sead::Viewport mViewport;                                // 0x180, set while drawing
    al::ISceneObj* mGoalItemHolder;                                  // 0x1a8
    al::ISceneObj* mCatGullHolder;                                   // 0x1b0
    u8 mUnknown1B8[0x1c0 - 0x1b8];                                   // Unreconstructed.
    IslandKeeper* mIslandKeeper;                                     // 0x1c0
    IntroFlyOverCamera* mIntroFlyOverCamera;                         // 0x1c8
    ShardsWatcherHolder* mShardsWatcherHolder;                       // 0x1d0
    PlayerArray mPlayers;                                            // 0x1d8
    sead::PtrArray<CheckpointArray> mCheckpointLists;                // 0x228
    s32 mLastCheckpointIndex;                                        // 0x238
    s32 mLastCheckpointZoneIndex;                                    // 0x23c
    s32 mUnlockedIslandNum;                                          // 0x240
    s32 _244;                                                        // 0x244
    PlayerCrown* mPlayerCrown;                                       // 0x248
    PlayerAliveWatcher* mPlayerAliveWatcher;                         // 0x250
    DrcAssistDirectorList* mDrcAssistDirectorList;                   // 0x258
    s32 mInvalidateInputFrames;                                      // 0x260
    IslandAreaWatcher* mIslandAreaWatcher;                           // 0x268
    RaidonSurf* mRaidonSurf;                                         // 0x270
    DemoCutscene* mDemoCutscene;                                     // 0x278
    al::WipeSimple* mWipeFadeBlack;                                  // 0x280
    al::WipeSimple* mWipeWorldJump;                                  // 0x288
    sead::Matrix34f mDemoBaseMtx;                                    // 0x290
    s32 mAudioDemoType;                                              // 0x2c0
    SingleModeSceneLayout* mSceneLayout;                             // 0x2c8
    al::WipeSimple* mWipeMiss;                                       // 0x2d0
    PlayerInvincibleBgmController* mInvincibleBgmController;         // 0x2d8
    PlayerBigBgmController* mBigBgmController;                       // 0x2e0
    PauseMenu* mPauseMenu;                                           // 0x2e8
    s32 mPausePort;                                                  // 0x2f0
    IslandMap* mIslandMap;                                           // 0x2f8
    al::ScreenCaptureExecutor* mScreenCaptureExecutor;               // 0x300
    al::NetworkSystem* mNetworkSystem;                               // 0x308
    al::NfpDirector* mNfpDirector;                                   // 0x310
    PlayerCameraTarget* mPlayerCameraTarget;                         // 0x318
    s32 _320;                                                        // 0x320
    void* _328;                                                      // 0x328
    rc::StampDirector* mStampDirector;                               // 0x330
    SnapshotState* mSnapshotState;                                   // 0x338
    SnapshotLayout* mSnapshotLayout;                                 // 0x340
    IslandWarpState* mIslandWarpState;                               // 0x348
    ShadowMarioDirector* mShadowMarioDirector;                       // 0x350
    GuideGameWindow* mGuideGameWindow;                               // 0x358
    WindowProcessing* mWindowProcessing;                             // 0x360
    s32 mUnlockedPhase;                                              // 0x368
    bool mIsLastBowserBattle;                                        // 0x36c
    bool mIsPhase0Start;                                             // 0x36d
    bool mIsInGameCutsceneSwitch;                                    // 0x36e
    SingleModeDrawer3D* mDrawer3D;                                   // 0x370
    bool mIsGigaBellUnlockCutscene;                                  // 0x378
};
static_assert(sizeof(SingleModeScene) == 0x380);
