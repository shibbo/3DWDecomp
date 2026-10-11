#pragma once

#include <basis/seadTypes.h>
#include <container/seadPtrArray.h>
#include <math/seadVector.h>
#include <prim/seadBitFlag.h>

#include "Library/Scene/ISceneObj.hpp"

namespace al {
class ActorInitInfo;
class ErrorViewer;
class HomeButton;
class IUseSceneObjHolder;
class LayoutInitInfo;
class LiveActor;
class NetworkSystem;
}  // namespace al

class CourseSelectLayout;
class CourseSelectMiniature;
class CourseSelectNode;
class CourseSelectObjKeeper;
class CourseSelectPuppeteerGroup;
class CourseSelectRocket;
class CourseSelectScene;
class CourseSelectSensor;
class CourseSelectWindowHolder;
class DemoOpeningSwitch;
class DemoTimerStageSwitchController;
class DrcTouchChecker;
class ICourseSelectActorController;
class PlayerCrown;

/**
 * @brief Scene object driving the course-select map: it knows the miniatures, the road nodes and
 * the players, opens the roads to the cleared courses, places the players and picks the object
 * the main player stands on so the layouts can show it and the players can enter it.
 */
class CourseSelectDirector : public al::ISceneObj {
public:
    explicit CourseSelectDirector(CourseSelectScene* pScene);

    static CourseSelectDirector* tryGetCourseSelectDirector(const al::IUseSceneObjHolder* pUser);
    static CourseSelectDirector* getCourseSelectDirector(const al::IUseSceneObjHolder* pUser);
    /** @brief Registers the opening-demo switch actor. @param pSwitch Switch controller. */
    void setDemoOpeningSwitch(DemoOpeningSwitch* pSwitch) { mDemoOpeningSwitch = pSwitch; }
    /** @brief Registers timed demo switches. @param pController Timed switch controller. */
    void setDemoTimerStageSwitchController(DemoTimerStageSwitchController* pController) {
        mDemoTimerStageSwitchController = pController;
    }
    /** @brief Registers the rocket of the map. @param pRocket Rocket actor. */
    void setRocket(CourseSelectRocket* pRocket) { mRocket = pRocket; }

    void init(const al::ActorInitInfo& rInfo);
    bool reviveUser(s32 userId, ICourseSelectActorController* pController, bool isAppearDemo);
    bool activateUser(s32 userId, bool isAppearDemo);
    bool isPlayLeaveDemo(s32 userId) const;
    bool leaveUser(s32 userId);
    bool isPlayPuppeterDemoAll() const;
    void setMainPlayer(al::LiveActor* pPlayer);
    void createLayout(const al::LayoutInitInfo& rInfo, CourseSelectWindowHolder* pWindowHolder,
                      al::NetworkSystem* pNetworkSystem, al::ErrorViewer* pErrorViewer,
                      al::HomeButton* pHomeButton);
    void registerStage(CourseSelectMiniature* pMiniature);
    void registerObject(al::LiveActor* pActor, s32 worldId);
    void registerNode(CourseSelectNode* pNode);
    CourseSelectMiniature* findMiniatureObj(s32 courseId) const;
    CourseSelectMiniature* tryFindMiniatureObj(s32 courseId) const;
    CourseSelectMiniature* tryFindCasinoRoom(s32 worldId) const;
    CourseSelectMiniature* findKoopaCastle(s32 worldId) const;
    al::LiveActor* tryFindNearestActor() const;
    s32 tryFindNextCourse(CourseSelectMiniature** pMiniatures, s32 maxNum, s32 courseId) const;
    s32 tryFindPrevCourse(CourseSelectMiniature** pMiniatures, s32 maxNum, s32 courseId) const;
    s32 tryFindNodeList(CourseSelectNode** pNodes, s32 maxNum, CourseSelectNode* pFrom,
                        CourseSelectNode* pTo) const;
    CourseSelectNode* tryFindFirstCrossingNode(const CourseSelectNode* pNode, s32 depth) const;
    CourseSelectNode* tryFindNodeFromTrans(const sead::Vector3f& rTrans);
    void openRoadImmediately(CourseSelectNode* pFrom, CourseSelectNode* pTo);
    bool isAllwaysOpenCourse(s32 courseId) const;
    bool isEnableEnterSelectedSensor() const;
    bool isTriggerDecideMainPlayer() const;
    void setWorldId(s32 worldId);
    s32 getMainPlayerPortNum() const;
    bool isPlayPuppeterDemo(s32 userId) const;
    bool isPlayPuppeterDemoAny() const;
    bool isPlayEntryDemo(s32 userId) const;
    bool isPlayEntryDemoAny() const;
    bool isInCourseSelectBubbleAny() const;
    bool isActiveActorControllerLayout(ICourseSelectActorController* pController) const;
    al::LiveActor* tryFindPlayerByUserId(s32 userId) const;
    void updateActiveWorld(const sead::BitFlag32& rWorldFlag);
    void setAfterOpeningDemo();
    void startEnterDemo(ICourseSelectActorController* pController);
    void startEnterMiiverse();
    void startEnterConfirm();
    void cancelEnter();
    void startDemo(bool isSkipLayout);
    void endDemo(bool isAppearLayout);
    bool isDemo() const;
    bool isMapPauseWorldJump() const;
    void appearLayout();
    void initAfterPlacement();
    void connectNodeLink();
    void initPlayerPositionOpening(const sead::Vector3f& rTrans);
    /** @deprecated Old guess of the signature, still used by CourseSelectScene. */
    void initPlayerPositionOpening(const char* pName);
    void initPlayerPositionStage(s32 courseId);
    void initPlayerPositionAfterEnding();
    void setPlayerPositionStage(s32 courseId);
    void setPlayerPositionWorldStart(s32 worldId);
    void startOpenRocketDemo();
    bool isEndOpenRocketDemo() const;
    bool isPlayerMove() const;
    void invalidateButtonDemo();
    void prepareFrame();
    void update();
    void touchPlayer(CourseSelectSensor* pSensor, al::LiveActor* pPlayer);
    bool checkDrcTouch(CourseSelectSensor* pSensor);
    void tryResetSelectedActorController(const ICourseSelectActorController* pController);

    /**
     * @brief Gets the name of the scene object.
     * @return The name.
     */
    const char* getSceneObjName() const override { return "コースセレクトデータ管理"; }

    /** @brief Gets the course select scene. @return The scene. */
    CourseSelectScene* getScene() const { return mScene; }
    /** @brief Gets the message windows of the map. @return The window holder. */
    CourseSelectWindowHolder* getWindowHolder() const { return mWindowHolder; }
    /** @brief Gets the controller of the object the main player stands on. @return The controller. */
    ICourseSelectActorController* getSelectedController() const { return mSelectedController; }
    /** @brief Gets the puppeteers of the players. @return The puppeteer group. */
    CourseSelectPuppeteerGroup* getPuppeteerGroup() const { return mPuppeteerGroup; }
    /** @brief Gets the course select layouts. @return The layout holder. */
    CourseSelectLayout* getLayout() const { return mLayout; }
    /** @brief Gets the course select nodes. @return The node array. */
    CourseSelectNode** getNodes() const { return mNodes; }
    /** @brief Gets the number of course select nodes. @return The node count. */
    s32 getNodeNum() const { return mNodeNum; }
    /** @brief Gets the opening-demo switch actor. @return The switch actor, or nullptr. */
    DemoOpeningSwitch* getDemoOpeningSwitch() const { return mDemoOpeningSwitch; }
    /** @brief Gets the world the main player is in. @return The world id, or -1. */
    s32 getActiveWorldId() const { return mActiveWorldId; }
    /** @brief Gets whether the event gate keeper is talking. @return true while it talks. */
    bool isEventGateKeeper() const { return mIsEventGateKeeper; }
    /** @brief Gets the main player of the map. @return The main player actor. */
    al::LiveActor* getMainPlayer() const { return mMainPlayer; }
    /** @brief Gets the crown of the best-score player. @return The crown actor. */
    PlayerCrown* getPlayerCrown() const { return mPlayerCrown; }
    /** @brief Gets the lock shown when a road is unlocked. @return The lock actor. */
    al::LiveActor* getUnLockActor() const { return mUnLockActor; }

private:
    /** @brief Maximum number of sensors touched in one frame. */
    static constexpr s32 cTouchSensorNumMax = 16;

    typedef sead::FixedPtrArray<CourseSelectSensor, cTouchSensorNumMax> TouchSensorArray;

    CourseSelectScene* mScene;  // 0x8
    CourseSelectLayout* mLayout = nullptr;  // 0x10
    CourseSelectWindowHolder* mWindowHolder;  // 0x18
    TouchSensorArray mPlayerTouchSensors;  // 0x20
    TouchSensorArray mDrcTouchSensors;  // 0xb0
    sead::PtrArray<CourseSelectMiniature> mMiniatures;  // 0x140
    CourseSelectNode** mNodes = nullptr;  // 0x150
    s32 mNodeNum = 0;  // 0x158
    al::LiveActor* mMainPlayer = nullptr;  // 0x160
    PlayerCrown* mPlayerCrown = nullptr;  // 0x168
    bool mIsForceSelect = false;  // 0x170
    ICourseSelectActorController* mSelectedController = nullptr;  // 0x178
    ICourseSelectActorController* mNearestController = nullptr;  // 0x180
    CourseSelectPuppeteerGroup* mPuppeteerGroup = nullptr;  // 0x188
    CourseSelectObjKeeper* mObjKeeper = nullptr;  // 0x190
    bool mIsEnter = false;  // 0x198
    CourseSelectRocket* mRocket = nullptr;  // 0x1a0
    DrcTouchChecker* mDrcTouchChecker = nullptr;  // 0x1a8
    al::LiveActor* mUnLockActor = nullptr;  // 0x1b0
    DemoTimerStageSwitchController* mDemoTimerStageSwitchController = nullptr;  // 0x1b8
    DemoOpeningSwitch* mDemoOpeningSwitch = nullptr;  // 0x1c0
    s32 mActiveWorldId = -1;  // 0x1c8
    bool mIsEventGateKeeper = false;  // 0x1cc
};

static_assert(sizeof(CourseSelectDirector) == 0x1d0);

namespace rc {
s32 getSelectMiniatureCourseId(CourseSelectDirector* pDirector);
bool isPlayerMove(CourseSelectDirector* pDirector);
}  // namespace rc
