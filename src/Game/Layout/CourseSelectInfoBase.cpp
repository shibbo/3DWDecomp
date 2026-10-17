#include "Layout/CourseSelectInfoBase.hpp"
#include "Layout/CourseSelectInfoController.hpp"
#include "Layout/CourseSelectInfoDecideButton.hpp"
#include "Layout/CourseSelectMiniatureCursor.hpp"
#include "CourseSelect/CourseSelectDirector.hpp"
#include "CourseSelect/ICourseSelectActorController.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
namespace {
NERVE_DECL(CourseSelectInfoBase, End);
NERVE_DECL(CourseSelectInfoBase, Decide);
NERVE_DECL(CourseSelectInfoBase, Appear);
NERVE_DECL(CourseSelectInfoBase, AppearWait);
NERVE_DECL(CourseSelectInfoBase, Hide);
NERVE_DECL(CourseSelectInfoBase, FrameOut);
struct {
    CourseSelectInfoBaseNrvEnd End;
    CourseSelectInfoBaseNrvDecide Decide;
    CourseSelectInfoBaseNrvAppear Appear;
    CourseSelectInfoBaseNrvAppearWait AppearWait;
    CourseSelectInfoBaseNrvHide Hide;
    CourseSelectInfoBaseNrvFrameOut FrameOut;
} NrvCourseSelectInfoBase;
}
/** @brief Creates the course information layout and its input controller. */
CourseSelectInfoBase::CourseSelectInfoBase(const al::LayoutInitInfo& rInfo, const char* pName, CourseSelectDirector* pDirector)
    : al::LayoutActor(pName), mOffsetY(250.0f), mDecideButton(nullptr), mDirector(pDirector),
      mActorController(nullptr), mSceneCameraInfo(rInfo.getSceneCameraInfo()), mIsTouchSelect(false) {
    al::initLayoutActor(this, rInfo, pName, nullptr);
    initNerve(&NrvCourseSelectInfoBase.End, 0);
    kill();
    mController = new CourseSelectInfoController(this);
    mDecideButton = new CourseSelectInfoDecideButton(this);
    updateGreenStars();
}
/** @brief Updates position and start-button availability. */
void CourseSelectInfoBase::control() {
    updateScreenPos();
    bool active = !al::isNerve(this, &NrvCourseSelectInfoBase.Decide) && !al::isNerve(this, &NrvCourseSelectInfoBase.End);
    if (al::isNerve(this, &NrvCourseSelectInfoBase.Appear) && al::isLessEqualStep(this, 10)) active = false;
    if (active) {
        if (mIsTouchSelect) mDecideButton->startDeactive(false);
        else mDecideButton->startActive(false);
    }
    mDecideButton->update();
}
/** @brief Follows the selected map actor and handles leaving the visible frame. */
void CourseSelectInfoBase::updateScreenPos() {
    sead::Vector2f pos;
    bool outside = CourseSelectMiniatureCursor::calcMiniatureLayoutPos(&pos, this, mActorController->getActor(), sead::Vector3f(0.0f, mOffsetY, 0.0f), 50.0f);
    if (outside) {
        if (!al::isNerve(this, &NrvCourseSelectInfoBase.End) && !al::isNerve(this, &NrvCourseSelectInfoBase.Decide)) {
            bool hidden = al::isNerve(this, &NrvCourseSelectInfoBase.Hide);
            bool frameOut = al::isNerve(this, &NrvCourseSelectInfoBase.FrameOut);
            if (!hidden && !frameOut) al::setNerve(this, &NrvCourseSelectInfoBase.FrameOut);
        } else al::isNerve(this, &NrvCourseSelectInfoBase.FrameOut);
    } else if (al::isNerve(this, &NrvCourseSelectInfoBase.FrameOut)) {
        if (mActorController) al::setNerve(this, &NrvCourseSelectInfoBase.Appear);
        else kill();
        return;
    }
    al::setLocalTrans(this, pos);
}
/** @brief Sets whether selection comes from touch input. */
void CourseSelectInfoBase::setTouchSelect(bool isTouch) { mIsTouchSelect = isTouch; }
/** @brief Opens information for the selected actor. */
void CourseSelectInfoBase::startAppear(ICourseSelectActorController* pController) {
    appear();
    al::hidePaneRoot(this);
    mActorController = pController;
    al::setNerve(this, &NrvCourseSelectInfoBase.Appear);
}
/** @brief Starts the closing animation. */
void CourseSelectInfoBase::startDisappear() { al::setNerve(this, &NrvCourseSelectInfoBase.End); }
/** @brief Starts course confirmation. */
void CourseSelectInfoBase::startDecide() { al::setNerve(this, &NrvCourseSelectInfoBase.Decide); }
/** @brief Reports opening or visible information. */
bool CourseSelectInfoBase::isAppear() const { return al::isNerve(this, &NrvCourseSelectInfoBase.Appear) || al::isNerve(this, &NrvCourseSelectInfoBase.AppearWait); }
/** @brief Reports hidden or inactive information. */
bool CourseSelectInfoBase::isDisappear() const { return al::isNerve(this, &NrvCourseSelectInfoBase.Hide) || !isAlive(); }
/** @brief Reports when the start button accepts confirmation. */
bool CourseSelectInfoBase::isEnableEnterCourse() const { return mDecideButton->isEnableDecide(); }
/** @brief Initializes content, then plays the opening animation. */
void CourseSelectInfoBase::exeAppear() {
    if (al::isFirstStep(this)) {
        al::showPaneRoot(this);
        al::startFreezeAction(this, "Appear", 0.0f, nullptr);
        mDecideButton->setDecideButtonTxt(mDirector->getMainPlayerPortNum());
        if (mIsTouchSelect) mDecideButton->startDeactive(true);
        else mDecideButton->startActive(true);
        updateControllerInfo();
    }
    if (al::isStep(this, 1)) {
        updateGreenStars();
        al::startAction(this, "Appear", nullptr);
    }
    if (al::isActionEnd(this, nullptr)) al::setNerve(this, &NrvCourseSelectInfoBase.AppearWait);
}
/** @brief Holds the visible information. */
void CourseSelectInfoBase::exeAppearWait() { al::isFirstStep(this); }
/** @brief Waits for confirmation before closing the information. */
void CourseSelectInfoBase::exeDecide() {
    if (al::isFirstStep(this)) mDecideButton->startDecide();
    if (mDecideButton->isEnd()) al::setNerve(this, &NrvCourseSelectInfoBase.End);
}
/** @brief Closes the information panel. */
void CourseSelectInfoBase::exeFrameOut() {
    if (al::isFirstStep(this)) al::startAction(this, "End", nullptr);
    if (al::isActionEnd(this, nullptr)) al::setNerve(this, &NrvCourseSelectInfoBase.Hide);
}
/** @brief Closes the information panel. */
void CourseSelectInfoBase::exeEnd() {
    if (al::isFirstStep(this)) al::startAction(this, "End", nullptr);
    if (al::isActionEnd(this, nullptr)) al::setNerve(this, &NrvCourseSelectInfoBase.Hide);
}
/** @brief Deactivates the hidden layout. */
void CourseSelectInfoBase::exeHide() { kill(); }
