#pragma once

#include <basis/seadTypes.h>
#include <container/seadPtrArray.h>
#include <prim/seadSafeString.h>

#include "Library/Scene/Scene.hpp"

namespace sead {
class Viewport;
}  // namespace sead

namespace al {
class ActorInitInfo;
class LiveActor;
class PlayerHolder;
class ScreenCaptureExecutor;
class StageInfo;
}  // namespace al

namespace rc {
class StampDirector;
}  // namespace rc

class CounterGreenStarAppend;
class CourseSelectDirector;
class CourseSelectMenuHeader;
class CourseSelectMiniature;
class CourseSelectStateAfterEndingEvent;
class CourseSelectStateClearDemo;
class CourseSelectStateRevivePlayer;
class CourseSelectStateStageEnter;
class CourseSelectStateStageExit;
class CourseSelectStateWorldStartDemo;
class CourseSelectWindowHolder;
class DemoOpeningState;
class DrcAssistDirectorList;
class GameDataHolder;
class IUsePlayerPanicControl;
class ListClearStar;
class ListStamp;
class ListStampResult;
class MapMenu;
class PauseMenuMap;
class PlayerActor;
class ProductStageStartParam;
class RCSControlGuideBar;
class SnapshotLayout;
class SnapshotState;

/**
 * @brief Scene of the course select map (the world map between the courses).
 */
class CourseSelectScene : public al::Scene {
    friend class CourseSelectDirector;

public:
    explicit CourseSelectScene(bool isAfterEndingEvent);
    ~CourseSelectScene() override;

    void init(const al::SceneInitInfo& rInfo) override;
    void initPlacement(const al::ActorInitInfo& rInfo);
    void appear() override;
    void control() override;
    void update();
    bool isInvalidCameraCtrl() const;
    bool isPause() const;
    s32 getMainPlayerInputPort() const;
    al::PlayerHolder* getPlayerHolder() const;
    void updatePauseLayout();
    bool isPauseStart() const;
    void updateWorldClipArea();
    void drawMain_() const override;
    bool isPauseDraw3D() const;
    void drawSub_() const override;
    GameDataHolder* getGameDataHolder() const;
    void getSelectedCourseInfo(ProductStageStartParam* pParam) const;
    CourseSelectDirector* getCourseSelectDirector() const;
    s32 getSelectedCourseId() const;
    bool isGotoTitle() const;
    bool isLoadGame() const;
    bool isControllerChange() const;
    bool isMapPauseWorldJump() const;
    void enterStage(CourseSelectMiniature* pMiniature);
    void setNameplatesVisible(bool isVisible);
    bool isMainPlayerInputPortDrc() const;
    void invalidatePlayerInput(s32 frame);

    void exeStageExit();
    void exeGreenStarInfo();
    void exeStampInfo();
    void exeOpening();
    void exeAfterEndingEvent();
    void exeStart();
    void exeClearDemo();
    void exeWorldStartDemo();
    void exeRevivePlayer();
    void exeSnapshot();
    void exeSave();
    void exeLoadGame();
    bool isTriggerPause(s32* pPort) const;
    bool isTriggerMapMenu(s32* pPort) const;
    void exePauseStart();
    void exePause();
    bool checkEnableOpenMenu();
    void exeMapPauseStart();
    void exeMapPause();
    void exeMapPauseEnd();
    void exeStarList();
    void exeStampList();
    void exePlay();
    void exeStageEnter();
    void exeEventGateKeeper();
    void exeGotoTitle();

    void initPlacementPlayer(const al::StageInfo* pStageInfo, const al::ActorInitInfo& rInfo);
    void initPlacementObject(const al::StageInfo* pStageInfo, const al::ActorInitInfo& rInfo,
                             const char* pListName);
    void initPlacementNodeItem(const al::ActorInitInfo& rInfo);

    /** @brief Gets the world the map shows. @return The world id. */
    s32 getWorldId() const { return mWorldId; }

private:
    /** Number of control users that can idle until they are made to leave. */
    static constexpr s32 cControlUserNum = 4;

    bool mIsAfterEndingEvent;                                // 0xe1
    bool mIsFirstPlayFrame = true;                           // 0xe2
    bool mIsNeedSave = false;                                // 0xe3
    bool mIsUpdated = false;                                 // 0xe4
    sead::FixedSafeString<64> mStageName;                    // 0xe8
    GameDataHolder* mGameDataHolder = nullptr;               // 0x140
    void* _148 = nullptr;
    IUsePlayerPanicControl* mPanicControl = nullptr;         // 0x150
    PauseMenuMap* mPauseMenuMap = nullptr;                   // 0x158
    MapMenu* mMapMenu = nullptr;                             // 0x160
    ListClearStar* mListClearStar = nullptr;                 // 0x168
    ListStamp* mListStamp = nullptr;                         // 0x170
    CounterGreenStarAppend* mCounterGreenStar = nullptr;     // 0x178
    ListStampResult* mListStampResult = nullptr;             // 0x180
    RCSControlGuideBar* mControlGuideBar = nullptr;          // 0x188
    s32 mMenuPort = -1;                                      // 0x190
    PlayerActor* mMainPlayer = nullptr;                      // 0x198
    s32 mIdleFrames[cControlUserNum];                        // 0x1a0
    s32 mWorldId = 1;                                        // 0x1b0
    CourseSelectMiniature* mSelectedMiniature = nullptr;     // 0x1b8
    s32 mSelectedWorldId = 0;                                // 0x1c0
    s32 mSelectedStageId = 0;                                // 0x1c4
    sead::Viewport* mMainViewport;                           // 0x1c8
    sead::Viewport* mSubViewport;                            // 0x1d0
    CourseSelectDirector* mDirector = nullptr;               // 0x1d8
    DemoOpeningState* mOpeningState = nullptr;              // 0x1e0
    CourseSelectStateAfterEndingEvent* mAfterEndingEventState;  // 0x1e8
    CourseSelectStateClearDemo* mClearDemoState = nullptr;   // 0x1f0
    CourseSelectStateWorldStartDemo* mWorldStartDemoState = nullptr;  // 0x1f8
    CourseSelectStateStageEnter* mStageEnterState = nullptr;  // 0x200
    CourseSelectStateStageExit* mStageExitState = nullptr;   // 0x208
    CourseSelectStateRevivePlayer* mRevivePlayerState = nullptr;  // 0x210
    CourseSelectWindowHolder* mWindowHolder = nullptr;       // 0x218
    al::ScreenCaptureExecutor* mScreenCaptureExecutor = nullptr;  // 0x220
    sead::FixedPtrArray<al::LiveActor, 100> mActors;         // 0x228
    CourseSelectMenuHeader* mMenuHeader = nullptr;           // 0x558
    bool mIsJoySingle = false;                               // 0x560
    s32 mCameraMode = -1;                                    // 0x564
    rc::StampDirector* mStampDirector = nullptr;             // 0x568
    SnapshotState* mSnapshotState = nullptr;                 // 0x570
    SnapshotLayout* mSnapshotLayout = nullptr;               // 0x578
    DrcAssistDirectorList* mDrcAssistDirectorList;           // 0x580
};

static_assert(sizeof(CourseSelectScene) == 0x588);
