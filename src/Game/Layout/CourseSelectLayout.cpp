#include "Layout/CourseSelectLayout.hpp"
#include "Layout/CourseSelectMiniatureCursor.hpp"
#include "Layout/CourseSelectSceneLayout.hpp"
#include "Layout/PlayerEntryMini.hpp"
#include "Layout/GuideCourse.hpp"
#include "Layout/GuideGameWindow.hpp"
#include "CourseSelect/CourseSelectDirector.hpp"
#include "CourseSelect/CourseSelectMiniature.hpp"
#include "System/GameDataFunction.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
namespace {
NERVE_DECL(CourseSelectLayout, Play);
NERVE_DECL(CourseSelectLayout, Demo);
NERVE_DECL(CourseSelectLayout, AfterOpening);
NERVES_MAKE_NOSTRUCT(CourseSelectLayout, Play, Demo, AfterOpening)
}
/** @brief Creates the course-select layout coordinator. */
CourseSelectLayout::CourseSelectLayout() : al::NerveExecutor("コース選択レイアウト") {}
/** @brief Creates the cursor, map HUD, entry window, and first-course guide.
 * @param rInfo Layout initialization context.
 * @param pGameDataHolder Game data.
 * @param pPlayerHolder Player actors.
 * @param pDirector Course-select director.
 * @param pNetworkSystem Network context.
 * @param pErrorViewer Error display.
 * @param pHomeButton Home-button controller. */
void CourseSelectLayout::init(const al::LayoutInitInfo& rInfo, GameDataHolder* pGameDataHolder,
                             al::PlayerHolder* pPlayerHolder, CourseSelectDirector* pDirector,
                             al::NetworkSystem* pNetworkSystem, al::ErrorViewer* pErrorViewer,
                             al::HomeButton* pHomeButton) {
    mCursor = new CourseSelectMiniatureCursor(rInfo, pDirector, pNetworkSystem);
    mSceneLayout = new CourseSelectSceneLayout(rInfo, pGameDataHolder, mCursor, pDirector,
                                               pNetworkSystem, pErrorViewer, pHomeButton);
    mPlayerEntry = new PlayerEntryMini(rInfo, pGameDataHolder, pPlayerHolder, 2);
    al::setSceneObj(mPlayerEntry, new GuideGameWindow(rInfo, false), 28);
    mGuide = new GuideCourse(rInfo, "W1-1矢印");
    initNerve(&NrvCourseSelectLayoutPlay, 0);
}
/** @brief Starts demo behavior for the cursor and HUD.
 * @param isSkipAnim Whether to skip the HUD animation. */
void CourseSelectLayout::startDemo(bool isSkipAnim) {
    mIsDemo = true;
    mCursor->startDemo();
    mPlayerEntry->startDemo();
    mSceneLayout->startDemo(isSkipAnim);
    al::setNerve(this, &NrvCourseSelectLayoutDemo);
}
/** @brief Restores the HUD and starts entry or the opening guide as needed.
 * @param isSkipAnim Whether to restore player-entry controls immediately. */
void CourseSelectLayout::endDemo(bool isSkipAnim) {
    mCursor->endDemo();
    if (isSkipAnim) {
        mIsDemo = false;
        mPlayerEntry->endDemo();
        validateButton();
    }
    mSceneLayout->endDemo(false);
    if (mIsAfterOpening) {
        al::setNerve(this, &NrvCourseSelectLayoutAfterOpening);
        mIsAfterOpening = false;
    } else if (!mHasStartedEntry && isSkipAnim) {
        mHasStartedEntry = true;
        mPlayerEntry->startPlayerEntry(true);
    }
}
/** @brief Enables entry-window controls. */
void CourseSelectLayout::validateButton() { mPlayerEntry->endDemo(); }
/** @brief Reports whether demo behavior is active. */
bool CourseSelectLayout::isDemo() const { return mIsDemo; }
/** @brief Pauses the HUD, cursor, and guide window. */
void CourseSelectLayout::startPause() {
    mSceneLayout->startPause();
    mPlayerEntry->hide();
    if (mGuide && mGuide->isShow()) al::startAction(mGuide, "Hide", "Visibility");
    mCursor->startPause();
    rc::hideGuideGameWindow(mPlayerEntry);
}
/** @brief Resumes the HUD, cursor, and guide window. */
void CourseSelectLayout::endPause() {
    mSceneLayout->endPause();
    mCursor->endPause();
    rc::unHideGuideGameWindow(mPlayerEntry);
    if (mGuide && mGuide->isShow()) al::startAction(mGuide, "Show", "Visibility");
}
/** @brief Disables entry-window controls. */
void CourseSelectLayout::invalidateButton() { mPlayerEntry->startDemo(); }
/** @brief Reports whether the cursor has decided a course. */
bool CourseSelectLayout::isDecide() const { return mCursor->isDecide(); }
/** @brief Reports whether the selected course can be entered. */
bool CourseSelectLayout::isEnableEnterCourse() const { return mCursor->isEnableEnterCourse(); }
/** @brief Reports whether the miniature cursor is active. */
bool CourseSelectLayout::isActiveMiniatureCursor() const { return mCursor->isActiveCursor(); }
/** @brief Reports whether the opening play guide is active. */
bool CourseSelectLayout::isShowPlayGuide() const { return al::isNerve(this, &NrvCourseSelectLayoutAfterOpening); }
/** @brief Selects the actor controlled by the miniature cursor.
 * @param pController Selected actor controller.
 * @param isDrc Whether selection came from the DRC.
 * @param isForce Whether to force the selection. */
bool CourseSelectLayout::setSelectedActorController(ICourseSelectActorController* pController,
                                                   bool isDrc, bool isForce) {
    return mCursor->setSelectedActorController(pController, isDrc, isForce);
}
/** @brief Decides the selected course and ends its visible guide. */
void CourseSelectLayout::startDecide() {
    mCursor->startDecide();
    if (mGuide && mGuide->isShow()) mGuide->startEnd();
}
/** @brief Selects the world shown by the HUD. @param worldId World identifier. */
void CourseSelectLayout::setWorldId(s32 worldId) { mSceneLayout->setWorldId(worldId); }
/** @brief Starts the player-entry window. */
void CourseSelectLayout::appear() { mPlayerEntry->startPlayerEntry(true); }
/** @brief Updates the cursor and coordinator state. */
void CourseSelectLayout::update() { mCursor->update(); updateNerve(); }
/** @brief Moves a player's controller assignment. @param srcPort Source port. @param dstPort Destination port. */
void CourseSelectLayout::changeUserPort(int srcPort, int dstPort) { mPlayerEntry->changeUserPort(srcPort, dstPort); }
/** @brief Shows the opening guide and initializes the first-course entry prompt. */
void CourseSelectLayout::exeAfterOpening() {
    if (al::isFirstStep(this)) {
        rc::appearGuideGameWindow(mPlayerEntry, "GuideMessage", "WorldMapStart", -1, 0.0f);
        mSceneLayout->startDemo(true);
    }
    if (!al::isGreaterEqualStep(this, 3600)) return;
    rc::disappearGuideGameWindow(mPlayerEntry);
    if (mHasStartedEntry) return;
    mHasStartedEntry = true;
    mPlayerEntry->startPlayerEntry(true);
    mSceneLayout->endDemo(false);
    int courseId = GameDataFunction::calcCourseId(GameDataHolderAccessor(mPlayerEntry), 1, 1);
    auto* pMiniature = CourseSelectDirector::getCourseSelectDirector(mPlayerEntry)->tryFindMiniatureObj(courseId);
    mGuide->startAppear(pMiniature);
}
/** @brief Keeps demo coordination idle. */
void CourseSelectLayout::exeDemo() {}
/** @brief Keeps normal play coordination idle. */
void CourseSelectLayout::exePlay() {}
/** @brief Checks whether the opening guide is active or pending. */
bool CourseSelectLayout::isAfterOpeningState() {
    return al::isNerve(this, &NrvCourseSelectLayoutAfterOpening) || mIsAfterOpening;
}
