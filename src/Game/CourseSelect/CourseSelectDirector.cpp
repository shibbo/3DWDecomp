#include "CourseSelect/CourseSelectDirector.hpp"

#include <attributes.h>
#include <math/seadMathCalcCommon.h>

#include "CourseSelect/CourseSelectConst.hpp"
#include "CourseSelect/CourseSelectMiniature.hpp"
#include "CourseSelect/CourseSelectNode.hpp"
#include "CourseSelect/CourseSelectObjKeeper.hpp"
#include "CourseSelect/CourseSelectPlayerActor.hpp"
#include "CourseSelect/CourseSelectPuppeteer.hpp"
#include "CourseSelect/CourseSelectPuppeteerGroup.hpp"
#include "CourseSelect/CourseSelectRocket.hpp"
#include "CourseSelect/CourseSelectScene.hpp"
#include "CourseSelect/CourseSelectSensor.hpp"
#include "CourseSelect/ICourseSelectActorController.hpp"
#include "CourseSelect/MiniatureController.hpp"
#include "Demo/DemoTimerStageSwitchController.hpp"
#include "Layout/CourseSelectLayout.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/LiveActor/Util/ActorClippingUtil.hpp"
#include "Library/LiveActor/Util/ActorFlagUtil.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorMovementUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "Library/Shadow/Common/ShadowUtil.hpp"
#include "MapObj/DrcTouchChecker.hpp"
#include "MapObj/PlayerCrown.hpp"
#include "Player/IUsePlayerKeyConfig.hpp"
#include "Player/Normal/PlayerActor.hpp"
#include "Player/Normal/PlayerInput.hpp"
#include "Player/Normal/PlayerProperty.hpp"
#include "Project/Collision/CollisionUtil.hpp"
#include "Scene/SceneObjID.hpp"
#include "System/CourseInfoHolder.hpp"
#include "System/Data/StageDatabaseInfo.hpp"
#include "System/GameDataFlagFunction.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/GameDataHolderWriter.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/PlayerUtil.hpp"

namespace {
/** @brief Number of nodes followed back when looking for the crossing a road starts from. */
const s32 cCrossingSearchDepth = 10;

/** @brief Maximum number of nodes on the road between two nodes. */
const s32 cNodeListNumMax = 32;

/** @brief Maximum number of courses opened by clearing a course. */
const s32 cNextCourseNumMax = 10;

/** @brief Cosine of the widest angle between the player front and an object to select it. */
const f32 cSelectFrontDotMin = -0.70710677f;

/** @brief Position of the players on the map after the ending. */
const sead::Vector3f cAfterEndingPlayerTrans(-3436.0f, 0.0f, 1886.0f);

/**
 * @brief Gets the miniature of a course that must exist.
 * @param rMiniatures Miniatures of the map.
 * @param courseId Course id.
 * @return The miniature.
 */
inline CourseSelectMiniature* getMiniature(const sead::PtrArray<CourseSelectMiniature>& rMiniatures,
                                           s32 courseId) {
    s32 i = 0;
    while (rMiniatures[i]->getCourseId() != courseId) {
        i++;
    }

    return rMiniatures(i);
}

/**
 * @brief Places all the players side by side around a position, on the ground below it.
 * @param pPlayer Any player actor.
 * @param rTrans Center of the players.
 */
void initPlayerPosition(al::LiveActor* pPlayer, const sead::Vector3f& rTrans) {
    sead::Vector3f trans = rTrans;
    f32 groundY = trans.y;
    // Start of the ground check arrow, then position of each player.
    sead::Vector3f pos = rTrans;
    sead::Vector3f arrowDir(0.0f, -500.0f, 0.0f);
    pos.y += 100.0f;
    sead::Vector3f hitPos;
    if (alCollisionUtil::getFirstPolyOnArrow(pPlayer, &hitPos, nullptr, pos, arrowDir, nullptr,
                                             nullptr)) {
        groundY = hitPos.y;
    }

    trans.y = groundY;

    s32 playerNum = al::getPlayerNumMax(pPlayer);
    s32 aliveNum = al::getAlivePlayerNum(pPlayer);
    f32 side = CourseSelectConst::getPlayerOffsetSide();
    trans.x += side * ((aliveNum - 1.0f) * -0.5f);
    for (s32 i = 0; i < playerNum; i++) {
        auto* player = static_cast<PlayerActor*>(al::getPlayerActor(pPlayer, i));
        if (al::isAlive(player)) {
            s32 userId = rc::findControlUserId(player);
            s32 order = rc::calcActiveUserNumInOrder(GameDataHolderAccessor(player), userId);
            pos = trans;
            pos.x += order * CourseSelectConst::getPlayerOffsetSide();
            player->getProperty()->mTrans = pos;
            al::resetPosition(player, pos, false);
        } else {
            player->getProperty()->mTrans = rTrans;
            al::resetPosition(player, rTrans, false);
        }
    }
}
}  // namespace

/**
 * @brief Gets the course select director if it exists.
 * @param pUser Object with access to the scene objects.
 * @return The director, or nullptr.
 */
CourseSelectDirector*
CourseSelectDirector::tryGetCourseSelectDirector(const al::IUseSceneObjHolder* pUser) {
    return al::tryGetSceneObj<CourseSelectDirector>(pUser, SceneObjID_CourseSelectDirector);
}

/**
 * @brief Gets the course select director.
 * @param pUser Object with access to the scene objects.
 * @return The director.
 */
CourseSelectDirector*
CourseSelectDirector::getCourseSelectDirector(const al::IUseSceneObjHolder* pUser) {
    return al::tryGetSceneObj<CourseSelectDirector>(pUser, SceneObjID_CourseSelectDirector);
}

/**
 * @brief Creates the director and its object keeper.
 * @param pScene Course select scene.
 */
CourseSelectDirector::CourseSelectDirector(CourseSelectScene* pScene) : mScene(pScene) {
    mMiniatures.allocBuffer(0x100, nullptr);
    mNodes = new CourseSelectNode*[0x200];
    mObjKeeper = new CourseSelectObjKeeper(
        GameDataFunction::getWorldNum(GameDataHolderAccessor(pScene->mGameDataHolder)), 0x400);
}

/**
 * @brief Creates the puppeteers, the touch checker, the unlock actor and the crown.
 * @param rInfo Actor init info.
 */
void CourseSelectDirector::init(const al::ActorInitInfo& rInfo) {
    mPuppeteerGroup = new CourseSelectPuppeteerGroup(mScene);
    mPuppeteerGroup->init(rInfo);
    mDrcTouchChecker = new DrcTouchChecker(rInfo.getActorSceneInfo().sceneCameraInfo);

    mUnLockActor = new al::LiveActor("グリーンスターロック解除用");
    al::initActorWithArchiveName(mUnLockActor, rInfo, "GreenStar", nullptr);
    mUnLockActor->makeActorDead();
    al::invalidateClipping(mUnLockActor);
    al::hideShadow(mUnLockActor);

    mPlayerCrown = new PlayerCrown(rInfo, "CourseSelect");
}

/**
 * @brief Revives the player of a control user.
 * @param userId Control user id.
 * @param pController Controller of the object the player revives at.
 * @param isAppearDemo Whether the player plays the appear demo.
 * @return Always true.
 */
bool CourseSelectDirector::reviveUser(s32 userId, ICourseSelectActorController* pController,
                                      bool isAppearDemo) {
    mPuppeteerGroup->reviveUser(userId, pController, isAppearDemo);
    return true;
}

/**
 * @brief Makes a control user enter the map, or cancels its leave demo.
 * @param userId Control user id.
 * @param isCancelLeave Whether a playing leave demo is cancelled instead.
 * @return Always true.
 */
bool CourseSelectDirector::activateUser(s32 userId, bool isCancelLeave) {
    if (isCancelLeave && mPuppeteerGroup->isPlayLeaveDemo(userId)) {
        mPuppeteerGroup->getPuppeteer(userId)->cancelLeave();
        return true;
    }

    mPuppeteerGroup->enterUser(userId);
    return true;
}

/**
 * @brief Checks whether the player of a control user plays the leave demo.
 * @param userId Control user id.
 * @return true if it plays the leave demo.
 */
bool CourseSelectDirector::isPlayLeaveDemo(s32 userId) const {
    return mPuppeteerGroup->isPlayLeaveDemo(userId);
}

/**
 * @brief Makes a control user leave the map.
 * @param userId Control user id.
 * @return false if all the players play a demo.
 */
bool CourseSelectDirector::leaveUser(s32 userId) {
    if (mPuppeteerGroup->isPlayDemoAll()) {
        return false;
    }

    mPuppeteerGroup->leaveUser(userId);
    return true;
}

/**
 * @brief Checks whether all the players play a puppeteer demo.
 * @return true if all of them play a demo.
 */
bool CourseSelectDirector::isPlayPuppeterDemoAll() const {
    return mPuppeteerGroup->isPlayDemoAll();
}

/**
 * @brief Sets the main player of the map.
 * @param pPlayer Main player actor.
 */
void CourseSelectDirector::setMainPlayer(al::LiveActor* pPlayer) {
    mMainPlayer = pPlayer;
}

/**
 * @brief Creates the course select layouts.
 * @param rInfo Layout init info.
 * @param pWindowHolder Window holder of the scene.
 * @param pNetworkSystem Network system.
 * @param pErrorViewer Error viewer.
 * @param pHomeButton Home button.
 */
void CourseSelectDirector::createLayout(const al::LayoutInitInfo& rInfo,
                                        CourseSelectWindowHolder* pWindowHolder,
                                        al::NetworkSystem* pNetworkSystem,
                                        al::ErrorViewer* pErrorViewer,
                                        al::HomeButton* pHomeButton) {
    mLayout = new CourseSelectLayout();
    mLayout->init(rInfo, mScene->mGameDataHolder, mScene->getPlayerHolder(), this, pNetworkSystem,
                  pErrorViewer, pHomeButton);
    mWindowHolder = pWindowHolder;
}

/**
 * @brief Registers a course miniature.
 * @param pMiniature Miniature to register.
 */
void CourseSelectDirector::registerStage(CourseSelectMiniature* pMiniature) {
    mMiniatures.pushBack(pMiniature);
}

/**
 * @brief Registers an object shown only while its world is active.
 * @param pActor Object actor.
 * @param worldId World of the object.
 */
void CourseSelectDirector::registerObject(al::LiveActor* pActor, s32 worldId) {
    mObjKeeper->registerObject(pActor, worldId);
}

/**
 * @brief Registers a road node and gives it its index.
 * @param pNode Node to register.
 */
void CourseSelectDirector::registerNode(CourseSelectNode* pNode) {
    mNodes[mNodeNum] = pNode;
    pNode->setNodeId(mNodeNum);
    mNodeNum++;
}

/**
 * @brief Finds the miniature of a course.
 * @param courseId Course id.
 * @return The miniature, or nullptr.
 */
CourseSelectMiniature* CourseSelectDirector::findMiniatureObj(s32 courseId) const {
    for (s32 i = 0; i < mMiniatures.size(); i++) {
        if (mMiniatures[i]->getCourseId() == courseId) {
            return mMiniatures[i];
        }
    }

    return nullptr;
}

/**
 * @brief Finds the miniature of a course if it exists.
 * @param courseId Course id.
 * @return The miniature, or nullptr.
 */
CourseSelectMiniature* CourseSelectDirector::tryFindMiniatureObj(s32 courseId) const {
    for (s32 i = 0; i < mMiniatures.size(); i++) {
        if (mMiniatures[i]->getCourseId() == courseId) {
            return mMiniatures[i];
        }
    }

    return nullptr;
}

/**
 * @brief Finds the casino room miniature of a world.
 * @param worldId World id.
 * @return The miniature, or nullptr.
 */
CourseSelectMiniature* CourseSelectDirector::tryFindCasinoRoom(s32 worldId) const {
    for (s32 i = 0; i < mMiniatures.size(); i++) {
        CourseSelectMiniature* miniature = mMiniatures[i];
        if (miniature->getWorldId() == worldId && miniature->getStageInfo()->isCasinoRoom()) {
            return miniature;
        }
    }

    return nullptr;
}

/**
 * @brief Finds the Bowser castle miniature of a world.
 * @param worldId World id.
 * @return The miniature, or nullptr.
 */
CourseSelectMiniature* CourseSelectDirector::findKoopaCastle(s32 worldId) const {
    s32 courseId = GameDataFunction::findKoopaCastleCourseId(
        GameDataHolderAccessor(mScene->mGameDataHolder), worldId);
    return tryFindMiniatureObj(courseId);
}

/**
 * @brief Finds the actor the camera looks at: the selected object, or the nearest open
 * miniature of the active world.
 * @return The actor, or nullptr.
 */
al::LiveActor* CourseSelectDirector::tryFindNearestActor() const {
    if (mMainPlayer == nullptr) {
        return nullptr;
    }

    if (mSelectedController != nullptr) {
        return mSelectedController->getActor();
    }

    if (mNearestController != nullptr) {
        return mNearestController->getActor();
    }

    const sead::Vector3f& playerTrans = al::getTrans(mMainPlayer);
    CourseSelectMiniature* nearest = nullptr;
    f32 minDistance = 100000000.0f;
    for (s32 i = 0; i < mMiniatures.size(); i++) {
        CourseSelectMiniature* miniature = mMiniatures[i];
        if (miniature->getWorldId() != mActiveWorldId) {
            continue;
        }

        if (!miniature->isGreenStarLock() && !miniature->isCourseOpen()) {
            continue;
        }

        const sead::Vector3f& trans = al::getTrans(miniature);
        f32 dx = trans.x - playerTrans.x;
        f32 dz = trans.z - playerTrans.z;
        f32 distance = dx * dx + dz * dz;
        if (distance < minDistance) {
            minDistance = distance;
            nearest = miniature;
        }
    }

    return nearest;
}

/**
 * @brief Finds the miniatures of the courses opened by clearing a course.
 * @param pMiniatures Output array of miniatures.
 * @param maxNum Size of the output array.
 * @param courseId Cleared course id.
 * @return The number of next courses.
 */
s32 CourseSelectDirector::tryFindNextCourse(CourseSelectMiniature** pMiniatures, s32 maxNum,
                                            s32 courseId) const {
    CourseSelectMiniature* miniature = tryFindMiniatureObj(courseId);
    if (miniature == nullptr) {
        return 0;
    }

    s32 nextNum = miniature->getNextCourseNum();
    const s32* nextCourseIds = miniature->getNextCourseIds();
    for (s32 i = 0; i < nextNum; i++) {
        pMiniatures[i] = tryFindMiniatureObj(nextCourseIds[i]);
    }

    return nextNum;
}

/**
 * @brief Finds the miniatures of the courses whose clear opens a course.
 * @param pMiniatures Output array of miniatures.
 * @param maxNum Size of the output array.
 * @param courseId Opened course id.
 * @return The number of previous courses.
 */
s32 CourseSelectDirector::tryFindPrevCourse(CourseSelectMiniature** pMiniatures, s32 maxNum,
                                            s32 courseId) const {
    s32 num = 0;
    for (s32 i = 0; i < mMiniatures.size(); i++) {
        if (mMiniatures[i]->isNextCourse(courseId)) {
            pMiniatures[num++] = mMiniatures[i];
        }
    }

    return num;
}

/**
 * @brief Finds the nodes on the road between two nodes, following the links back from the end.
 * @param pNodes Output array of nodes, from the start to the end node.
 * @param maxNum Size of the output array.
 * @param pFrom Start node.
 * @param pTo End node.
 * @return The number of nodes, or -1 if no road links them.
 */
s32 CourseSelectDirector::tryFindNodeList(CourseSelectNode** pNodes, s32 maxNum,
                                          CourseSelectNode* pFrom, CourseSelectNode* pTo) const {
    s32 nodeNum = -1;
    if (pTo->isLinkedByTarget(pFrom)) {
        pNodes[0] = pFrom;
        pNodes[1] = pTo;
        nodeNum = 2;
    } else {
        for (s32 i = 0; i < pTo->getLinkNum(); i++) {
            s32 linkIndex = pTo->getLinkNodeIndex(i);
            s32 num = tryFindNodeList(pNodes, maxNum, pFrom, mNodes[linkIndex]);
            if (num != -1) {
                pNodes[num] = pTo;
                nodeNum = num + 1;
                break;
            }
        }
    }

    return nodeNum;
}

/**
 * @brief Follows the links back from a node until a crossing is found.
 * @param pNode Node to start from.
 * @param depth Maximum number of links followed.
 * @return The first node with several next nodes, or nullptr.
 */
CourseSelectNode* CourseSelectDirector::tryFindFirstCrossingNode(const CourseSelectNode* pNode,
                                                                 s32 depth) const {
    if (depth < 1 || pNode->getLinkNum() < 1) {
        return nullptr;
    }

    s32 linkIndex = pNode->getFrontLinkNodeIndex();
    CourseSelectNode* node = mNodes[linkIndex];
    if (node->getNextNodeNum() >= 2) {
        return node;
    }

    return tryFindFirstCrossingNode(node, depth - 1);
}

/**
 * @brief Finds the node placed at a position.
 * @param rTrans Position.
 * @return The node, or nullptr.
 */
CourseSelectNode* CourseSelectDirector::tryFindNodeFromTrans(const sead::Vector3f& rTrans) {
    for (s32 i = 0; i < mNodeNum; i++) {
        if (mNodes[i]->isEqual(rTrans)) {
            return mNodes[i];
        }
    }

    return nullptr;
}

/**
 * @brief Opens the road between two nodes without the open demo.
 * @param pFrom Start node.
 * @param pTo End node.
 */
void CourseSelectDirector::openRoadImmediately(CourseSelectNode* pFrom, CourseSelectNode* pTo) {
    if (pTo == nullptr) {
        return;
    }

    CourseSelectNode* nodes[cNodeListNumMax];
    s32 num = tryFindNodeList(nodes, cNodeListNumMax, pFrom, pTo);
    if (num <= 1) {
        CourseSelectNode* crossing = tryFindFirstCrossingNode(pFrom, cCrossingSearchDepth);
        if (crossing == nullptr) {
            return;
        }

        num = tryFindNodeList(nodes, cNodeListNumMax, crossing, pTo);
        if (num < 2) {
            return;
        }
    }

    for (s32 i = 0; i < num - 1; i++) {
        nodes[i]->openRoadToNextNode(nodes[i + 1], true);
    }
}

/**
 * @brief Checks whether a course is always open: a course no other course opens and that is not
 * a casino room.
 * @param courseId Course id.
 * @return true if the course is always open.
 */
bool CourseSelectDirector::isAllwaysOpenCourse(s32 courseId) const {
    if (tryFindMiniatureObj(courseId)->getStageInfo()->isCasinoRoom()) {
        return false;
    }

    for (s32 i = 0; i < mMiniatures.size(); i++) {
        if (mMiniatures[i]->isNextCourse(courseId)) {
            return false;
        }
    }

    return true;
}

/**
 * @brief Checks whether the players can enter the selected object.
 * @return true if they can enter it.
 */
bool CourseSelectDirector::isEnableEnterSelectedSensor() const {
    if (mSelectedController == nullptr) {
        return false;
    }

    return mLayout->isEnableEnterCourse();
}

/**
 * @brief Checks whether the main player presses the decide button.
 * @return true on the frame the button is pressed.
 */
bool CourseSelectDirector::isTriggerDecideMainPlayer() const {
    if (mMainPlayer == nullptr) {
        return false;
    }

    return static_cast<PlayerActor*>(mMainPlayer)->getKeyConfig()->isPadTriggerDecide();
}

/**
 * @brief Sets the world the main player is in.
 * @param worldId World id.
 */
void CourseSelectDirector::setWorldId(s32 worldId) {
    mLayout->setWorldId(worldId);
    if (mActiveWorldId != worldId) {
        for (s32 i = 0; i < mMiniatures.size(); i++) {
            mMiniatures[i]->changeCurrentWorldId(worldId);
        }
    }

    mActiveWorldId = worldId;
}

/**
 * @brief Gets the pad port of the main player.
 * @return The port.
 */
s32 CourseSelectDirector::getMainPlayerPortNum() const {
    return rc::getPlayerInputPort(mMainPlayer);
}

/**
 * @brief Checks whether the player of a control user plays a puppeteer demo.
 * @param userId Control user id.
 * @return true if it plays a demo.
 */
bool CourseSelectDirector::isPlayPuppeterDemo(s32 userId) const {
    return mPuppeteerGroup->isPlayDemo(userId);
}

/**
 * @brief Checks whether any player plays a puppeteer demo.
 * @return true if one of them plays a demo.
 */
bool CourseSelectDirector::isPlayPuppeterDemoAny() const {
    return mPuppeteerGroup->isPlayDemoAny();
}

/**
 * @brief Checks whether the player of a control user plays the entry demo.
 * @param userId Control user id.
 * @return true if it plays the entry demo.
 */
bool CourseSelectDirector::isPlayEntryDemo(s32 userId) const {
    return mPuppeteerGroup->isPlayEntryDemo(userId);
}

/**
 * @brief Checks whether any player plays the entry demo.
 * @return true if one of them plays the entry demo.
 */
bool CourseSelectDirector::isPlayEntryDemoAny() const {
    for (s32 i = 0; i < rc::getControlUserNumMax(); i++) {
        if (mPuppeteerGroup->isPlayEntryDemo(i)) {
            return true;
        }
    }

    return false;
}

/**
 * @brief Checks whether any alive player is in a bubble.
 * @return true if one of them is in a bubble.
 */
bool CourseSelectDirector::isInCourseSelectBubbleAny() const {
    s32 playerNum = al::getPlayerNumMax(mMainPlayer);
    for (s32 i = 0; i < playerNum; i++) {
        auto* player = static_cast<CourseSelectPlayerActor*>(al::getPlayerActor(mMainPlayer, i));
        if (al::isAlive(player) && player->isInCourseSelectBubble()) {
            return true;
        }
    }

    return false;
}

/**
 * @brief Checks whether the layout cursor shows an object.
 * @param pController Controller of the object.
 * @return true if the object is selected and the cursor is active.
 */
bool CourseSelectDirector::isActiveActorControllerLayout(
    ICourseSelectActorController* pController) const {
    if (mSelectedController != pController) {
        return false;
    }

    return mLayout->isActiveMiniatureCursor();
}

/**
 * @brief Finds the first alive player of a control user.
 * @param userId Control user id.
 * @return The player, or nullptr.
 */
al::LiveActor* CourseSelectDirector::tryFindPlayerByUserId(s32 userId) const {
    return rc::tryFindAlivePlayerActorFirstByUserId(mMainPlayer, userId);
}

/**
 * @brief Shows the objects of the active worlds only.
 * @param rWorldFlag Flags of the active worlds.
 */
void CourseSelectDirector::updateActiveWorld(const sead::BitFlag32& rWorldFlag) {
    mObjKeeper->updateActiveWorld(rWorldFlag);
}

/**
 * @brief Marks the layouts as shown after the opening demo.
 */
void CourseSelectDirector::setAfterOpeningDemo() {
    mLayout->setAfterOpening();
}

/**
 * @brief Starts the demo of the players entering an object.
 * @param pController Controller of the entered object.
 */
void CourseSelectDirector::startEnterDemo(ICourseSelectActorController* pController) {
    mPuppeteerGroup->startEnterDemo(pController);
    mSelectedController = pController;
    mIsEnter = true;
}

/**
 * @brief Starts entering the Miiverse menu.
 */
void CourseSelectDirector::startEnterMiiverse() {
    mIsEnter = true;
}

/**
 * @brief Starts entering the confirm window.
 */
void CourseSelectDirector::startEnterConfirm() {
    mIsEnter = true;
}

/**
 * @brief Cancels entering an object.
 */
void CourseSelectDirector::cancelEnter() {
    mIsEnter = false;
}

/**
 * @brief Starts a demo: hides the layouts and the nameplates.
 * @param isSkipLayout Whether the layouts are hidden without animation.
 */
void CourseSelectDirector::startDemo(bool isSkipLayout) {
    mLayout->startDemo(isSkipLayout);
    mScene->setNameplatesVisible(false);
}

/**
 * @brief Ends a demo: shows the nameplates and the layouts.
 * @param isAppearLayout Whether the layouts appear with an animation.
 */
void CourseSelectDirector::endDemo(bool isAppearLayout) {
    mScene->setNameplatesVisible(true);
    mLayout->endDemo(isAppearLayout);
}

/**
 * @brief Checks whether a demo plays.
 * @return true while a demo plays.
 */
bool CourseSelectDirector::isDemo() const {
    return mLayout->isDemo();
}

/**
 * @brief Checks whether the map pause menu asked for a world jump.
 * @return true if a world jump is requested.
 */
bool CourseSelectDirector::isMapPauseWorldJump() const {
    return mScene->isMapPauseWorldJump();
}

/**
 * @brief Makes the layouts appear.
 */
void CourseSelectDirector::appearLayout() {
    mLayout->appear();
}

/**
 * @brief Gives the crown to the best player of the last stage and opens the roads of the cleared
 * courses.
 */
void CourseSelectDirector::initAfterPlacement() {
    mObjKeeper->initAfterPlacement();

    s32 bestUserId = GameDataFunction::tryGetLastStageBestScoreUserID(
        GameDataHolderAccessor(mScene->mGameDataHolder));
    if (bestUserId != -1 &&
        rc::isActiveControlUser(GameDataHolderAccessor(mPlayerCrown), bestUserId)) {
        auto* player =
            static_cast<PlayerActor*>(rc::findPlayerActorFirstByUserId(mPlayerCrown, bestUserId));
        if (player != nullptr &&
            !rc::isDeadControlUserInStage(GameDataHolderAccessor(mScene->mGameDataHolder),
                                          bestUserId)) {
            mPlayerCrown->changeHost(player);
            mPlayerCrown->appear();
        }
    }

    CourseSelectMiniature* nextMiniatures[cNextCourseNumMax] = {};
    CourseSelectNode* nodes[cNodeListNumMax] = {};
    GameDataFunction::getLastPlayCourseId(GameDataHolderAccessor(mScene->mGameDataHolder));
    for (s32 i = 0; i < mMiniatures.size(); i++) {
        CourseSelectMiniature* miniature = mMiniatures[i];
        if (GameDataFunction::isStageLastPlayAndFirstClear(
                GameDataHolderAccessor(mScene->mGameDataHolder), miniature->getCourseId())) {
            continue;
        }

        if (!CourseInfoFunction::isClear(GameDataHolderAccessor(mScene),
                                         miniature->getCourseId())) {
            continue;
        }

        s32 nextNum = tryFindNextCourse(nextMiniatures, cNextCourseNumMax, miniature->getCourseId());
        for (s32 j = 0; j < nextNum; j++) {
            CourseSelectMiniature* next = nextMiniatures[j];
            s32 num = tryFindNodeList(nodes, cNodeListNumMax, miniature->getNode(), next->getNode());
            if (num <= 1) {
                CourseSelectNode* crossing =
                    tryFindFirstCrossingNode(miniature->getNode(), cCrossingSearchDepth);
                num = tryFindNodeList(nodes, cNodeListNumMax, crossing, next->getNode());
            }

            for (s32 k = 0; k < num - 1; k++) {
                nodes[k]->openRoadToNextNode(nodes[k + 1], true);
            }
        }

        CourseSelectNode* nodeEnd = miniature->tryFindNextNodeEnd();
        if (nodeEnd != nullptr) {
            s32 num = tryFindNodeList(nodes, cNodeListNumMax, miniature->getNode(), nodeEnd);
            if (num <= 1) {
                CourseSelectNode* crossing =
                    tryFindFirstCrossingNode(miniature->getNode(), cCrossingSearchDepth);
                num = crossing != nullptr ?
                          tryFindNodeList(nodes, cNodeListNumMax, crossing, nodeEnd) :
                          0;
            }

            if (num >= 2) {
                for (s32 k = 0; k < num - 1; k++) {
                    nodes[k]->openRoadToNextNode(nodes[k + 1], true);
                }
            }
        }

        if (miniature->hasDokanLink()) {
            CourseSelectNode* dokanNode = tryFindNodeFromTrans(miniature->getDokanTrans());
            if (dokanNode != nullptr) {
                CourseSelectNode* crossing =
                    tryFindFirstCrossingNode(miniature->getNode(), cCrossingSearchDepth);
                if (crossing == nullptr) {
                    crossing = miniature->getNode();
                }

                s32 num = tryFindNodeList(nodes, cNodeListNumMax, crossing, dokanNode);
                for (s32 k = 0; k < num - 1; k++) {
                    nodes[k]->openRoadToNextNode(nodes[k + 1], true);
                }
            }
        }
    }

    if (mDemoTimerStageSwitchController != nullptr) {
        mDemoTimerStageSwitchController->prepare(!GameDataFlagFunction::isShowWorldStartDemo(
            GameDataHolderAccessor(mScene->mGameDataHolder), 8));
    }
}

/**
 * @brief Links each node to the nodes placed at its next node positions.
 */
void CourseSelectDirector::connectNodeLink() {
    for (s32 i = 0; i < mNodeNum; i++) {
        for (s32 j = 0; j < mNodes[i]->getNextNodeNum(); j++) {
            sead::Vector3f nextPos = *mNodes[i]->getNextNodePos(j);
            for (s32 k = 0; k < mNodeNum; k++) {
                if (i == k) {
                    continue;
                }

                if (mNodes[k]->isEqual(nextPos)) {
                    mNodes[k]->addLinkedList(i);
                    mNodes[i]->addTargetNodeList(k);
                    break;
                }
            }
        }
    }
}

/**
 * @brief Places the players for the opening demo.
 * @param rTrans Center of the players.
 */
void CourseSelectDirector::initPlayerPositionOpening(const sead::Vector3f& rTrans) {
    initPlayerPosition(mMainPlayer, rTrans);
}

/**
 * @brief Places the players in front of the miniature of a course.
 * @param courseId Course id.
 */
void CourseSelectDirector::initPlayerPositionStage(s32 courseId) {
    sead::Vector3f trans = {0.0f, 0.0f, 0.0f};
    CourseSelectMiniature* miniature = tryFindMiniatureObj(courseId);
    f32 offset;
    if (GameDataFunction::isStageKoopaCastle(GameDataHolderAccessor(mScene->mGameDataHolder),
                                             courseId)) {
        offset = CourseSelectConst::getPlayerOffsetKoopa();
    } else {
        offset = CourseSelectConst::getPlayerOffsetMiniature();
    }

    trans = sead::Vector3f(0.0f, 0.0f, offset) + al::getTrans(miniature);
    initPlayerPosition(mMainPlayer, trans);
}

/**
 * @brief Places the players at their position after the ending.
 */
void CourseSelectDirector::initPlayerPositionAfterEnding() {
    initPlayerPosition(mMainPlayer, cAfterEndingPlayerTrans);
}

/**
 * @brief Warps the players to the miniature of a course.
 * @param courseId Course id.
 */
void CourseSelectDirector::setPlayerPositionStage(s32 courseId) {
    mPuppeteerGroup->startWarpToCourse(getMiniature(mMiniatures, courseId)->getController());
}

/**
 * @brief Warps the players to the first course of a world.
 * @param worldId World id.
 */
void CourseSelectDirector::setPlayerPositionWorldStart(s32 worldId) {
    s32 courseId = GameDataFunction::tryCalcCourseId(
        GameDataHolderAccessor(mScene->mGameDataHolder), worldId, 1);
    if (GameDataFunction::isInvalidCourseId(courseId)) {
        return;
    }

    GameDataFunction::onWorldWarp(GameDataHolderWriter(mScene->mGameDataHolder), courseId);

    mPuppeteerGroup->startWarpToCourse(getMiniature(mMiniatures, courseId)->getController());
}

/**
 * @brief Starts the demo of the rocket opening.
 */
void CourseSelectDirector::startOpenRocketDemo() {
    startDemo(true);
    mPuppeteerGroup->startHidePlayerDemo();
    mRocket->startOpenDemo();
}

/**
 * @brief Checks whether the rocket open demo ended.
 * @return true once the demo ended.
 */
bool CourseSelectDirector::isEndOpenRocketDemo() const {
    return !mRocket->isDemo();
}

/**
 * @brief Checks whether the main player moves.
 * @return true if it is bound or has a horizontal velocity.
 */
bool CourseSelectDirector::isPlayerMove() const {
    if (mMainPlayer == nullptr) {
        return false;
    }

    if (rc::isPlayerBinded(mMainPlayer)) {
        return true;
    }

    const sead::Vector3f& velocity = rc::getPlayerVelocity(mMainPlayer);
    return !al::isNearZero(velocity.x) || !al::isNearZero(velocity.z);
}

/**
 * @brief Disables the layout buttons during a demo.
 */
void CourseSelectDirector::invalidateButtonDemo() {
    mLayout->invalidateButton();
}

/**
 * @brief Clears the sensors touched during the last frame.
 */
void CourseSelectDirector::prepareFrame() {
    mPlayerTouchSensors.clear();
    mDrcTouchSensors.clear();
    mDrcTouchChecker->update();
}

/**
 * @brief Selects the object the main player stands on or touched on the touch screen, and
 * enters it when the main player decides.
 */
void CourseSelectDirector::update() {
    mLayout->update();
    if (mIsEnter) {
        return;
    }

    auto* mainPlayer = static_cast<PlayerActor*>(mMainPlayer);
    s32 playerNum = al::getPlayerNumMax(mainPlayer);
    for (s32 i = 0; i < playerNum; i++) {
        auto* player = static_cast<PlayerActor*>(al::getPlayerActor(mMainPlayer, i));
        if (player != nullptr) {
            player->getInput()->enableJumpButton();
        }
    }

    if (mIsEnter || (mPlayerTouchSensors.size() == 0 && mDrcTouchSensors.size() == 0)) {
        mLayout->setSelectedActorController(nullptr, false, false);
        mSelectedController = nullptr;
        mNearestController = nullptr;
        return;
    }

    const sead::Vector3f& playerTrans = al::getTrans(mMainPlayer);
    ICourseSelectActorController* controller = nullptr;
    s32 nearestIndex = -1;
    f32 minDistance = sead::Mathf::maxNumber();
    for (s32 i = 0; i < mPlayerTouchSensors.size(); i++) {
        const sead::Vector3f& pos = al::getSensorPos(mPlayerTouchSensors(i)->getHitSensor());
        f32 dx = playerTrans.x - pos.x;
        f32 dz = playerTrans.z - pos.z;
        f32 distance = dx * dx + dz * dz;
        if (distance < minDistance) {
            minDistance = distance;
            nearestIndex = i;
        }
    }

    if (nearestIndex != -1 && mPlayerTouchSensors[nearestIndex]->isUseCourseSelectLayoutType()) {
        controller = mPlayerTouchSensors(nearestIndex)->getController();
        mNearestController = controller;
        if (controller != nullptr) {
            al::LiveActor* actor = controller->getActor();
            const sead::Vector3f& front = rc::getPlayerFront(mainPlayer);
            const sead::Vector3f& actorTrans = al::getTrans(actor);
            const sead::Vector3f& trans = al::getTrans(mainPlayer);
            sead::Vector3f dir(actorTrans.x - trans.x, 0.0f, actorTrans.z - trans.z);
            bool isFront = al::normalizeOrZero(&dir) || dir.dot(front) > cSelectFrontDotMin;
            if (!isFront) {
                controller = nullptr;
            }
        }
    } else {
        mNearestController = nullptr;
    }

    bool isDrc = false;
    if (mDrcTouchSensors.size() != 0) {
        ICourseSelectActorController* drcController = mDrcTouchSensors(0)->getController();
        if (drcController != controller && drcController->getCursorLayoutType() != 5) {
            controller = mDrcTouchSensors(0)->getController();
            isDrc = true;
        }
    }

    if (mLayout->setSelectedActorController(controller, isDrc, mIsForceSelect)) {
        mSelectedController = controller;
        mIsForceSelect = false;
    }

    if (controller == nullptr || isDrc) {
        return;
    }

    mainPlayer->getInput()->disableJumpButton();
    if (controller->tryDecide(this)) {
        mLayout->startDecide();
    }
}

/**
 * @brief Registers a sensor touched by the main player.
 * @param pSensor Touched sensor.
 * @param pPlayer Player touching it.
 */
void CourseSelectDirector::touchPlayer(CourseSelectSensor* pSensor, al::LiveActor* pPlayer) {
    if (mMainPlayer != pPlayer) {
        return;
    }

    if (rc::isPlayerBinded(pPlayer)) {
        return;
    }

    mPlayerTouchSensors.pushBack(pSensor);
}

/**
 * @brief Registers a sensor if it is touched on the touch screen.
 * @param pSensor Sensor to check.
 * @return true if it is touched.
 */
bool CourseSelectDirector::checkDrcTouch(CourseSelectSensor* pSensor) {
    const al::HitSensor* hitSensor = pSensor->getHitSensor();
    f32 distance;
    if (mDrcTouchChecker->isTouchSphere(al::getSensorPos(hitSensor),
                                        al::getSensorRadius(hitSensor), &distance)) {
        mDrcTouchSensors.pushBack(pSensor);
        return true;
    }

    return false;
}

/**
 * @brief Forces the layouts to update the selected object if it is the given one.
 * @param pController Controller of the object.
 */
void CourseSelectDirector::tryResetSelectedActorController(
    const ICourseSelectActorController* pController) {
    if (mSelectedController == pController) {
        mIsForceSelect = true;
    }
}
