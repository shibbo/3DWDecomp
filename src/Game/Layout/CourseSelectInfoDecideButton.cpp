#include "Layout/CourseSelectInfoDecideButton.hpp"
#include "Library/Layout/LayoutActor.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Util/LayoutUtil.hpp"
namespace {
NERVE_DECL(CourseSelectInfoDecideButton, DeactiveWait);
NERVE_DECL(CourseSelectInfoDecideButton, Decide);
NERVE_DECL(CourseSelectInfoDecideButton, ActiveAppear);
NERVE_DECL(CourseSelectInfoDecideButton, ActiveWait);
NERVE_DECL(CourseSelectInfoDecideButton, DeactiveAppear);
NERVE_DECL(CourseSelectInfoDecideButton, End);
NERVES_MAKE_NOSTRUCT(CourseSelectInfoDecideButton, DeactiveWait, Decide, ActiveAppear, ActiveWait, DeactiveAppear, End)
}
/** @brief Creates an initially inactive start button controller. */
CourseSelectInfoDecideButton::CourseSelectInfoDecideButton(al::LayoutActor* pLayout)
    : al::NerveStateBase("CourseSelectInfoDecideButton"), mLayout(pLayout) {
    initNerve(&NrvCourseSelectInfoDecideButtonDeactiveWait, 0);
}
/** @brief Uses the decide icon for the selected controller. */
void CourseSelectInfoDecideButton::setDecideButtonTxt(int port) {
    rc::setPaneDecideIconFont(mLayout, "TxtStart", port);
}
/** @brief Changes the button availability, optionally skipping its transition. */
void CourseSelectInfoDecideButton::startActive(bool immediate) {
    if (al::isNerve(this, &NrvCourseSelectInfoDecideButtonDecide) || al::isNerve(this, &NrvCourseSelectInfoDecideButtonActiveAppear) || al::isNerve(this, &NrvCourseSelectInfoDecideButtonActiveWait)) return;
    if (immediate) al::setNerve(this, &NrvCourseSelectInfoDecideButtonActiveWait);
    else al::setNerve(this, &NrvCourseSelectInfoDecideButtonActiveAppear);
}
/** @brief Changes the button availability, optionally skipping its transition. */
void CourseSelectInfoDecideButton::startDeactive(bool immediate) {
    if (al::isNerve(this, &NrvCourseSelectInfoDecideButtonDecide) || al::isNerve(this, &NrvCourseSelectInfoDecideButtonDeactiveAppear) || al::isNerve(this, &NrvCourseSelectInfoDecideButtonDeactiveWait)) return;
    if (immediate) al::setNerve(this, &NrvCourseSelectInfoDecideButtonDeactiveWait);
    else al::setNerve(this, &NrvCourseSelectInfoDecideButtonDeactiveAppear);
}
/** @brief Starts the confirmation animation once. */
void CourseSelectInfoDecideButton::startDecide() {
    if (al::isNerve(this, &NrvCourseSelectInfoDecideButtonDecide)) return;
    al::setNerve(this, &NrvCourseSelectInfoDecideButtonDecide);
}
/** @brief Enables confirmation once the activation has advanced far enough. */
bool CourseSelectInfoDecideButton::isEnableDecide() const {
    return al::isNerve(this, &NrvCourseSelectInfoDecideButtonActiveWait) ||
        (al::isNerve(this, &NrvCourseSelectInfoDecideButtonActiveAppear) && al::isGreaterStep(this, 10));
}
/** @brief Reports completion of the confirmation animation. */
bool CourseSelectInfoDecideButton::isEnd() const { return al::isNerve(this, &NrvCourseSelectInfoDecideButtonEnd); }
/** @brief Runs the ActiveAppear animation state. */
void CourseSelectInfoDecideButton::exeActiveAppear() {
    if (al::isFirstStep(this)) al::startAction(mLayout, "Active", "Start");
    if (al::isActionEnd(mLayout, "Start")) al::setNerve(this, &NrvCourseSelectInfoDecideButtonActiveWait);
}
/** @brief Runs the DeactiveAppear animation state. */
void CourseSelectInfoDecideButton::exeDeactiveAppear() {
    if (al::isFirstStep(this)) al::startAction(mLayout, "Deactive", "Start");
    if (al::isActionEnd(mLayout, "Start")) al::setNerve(this, &NrvCourseSelectInfoDecideButtonDeactiveWait);
}
/** @brief Runs the Decide animation state. */
void CourseSelectInfoDecideButton::exeDecide() {
    if (al::isFirstStep(this)) al::startAction(mLayout, "Decide", "Start");
    if (al::isActionEnd(mLayout, "Start")) al::setNerve(this, &NrvCourseSelectInfoDecideButtonEnd);
}
/** @brief Runs the ActiveWait animation state. */
void CourseSelectInfoDecideButton::exeActiveWait() {
    if (al::isFirstStep(this)) al::startAction(mLayout, "ActiveWait", "Start");
}
/** @brief Runs the DeactiveWait animation state. */
void CourseSelectInfoDecideButton::exeDeactiveWait() {
    if (al::isFirstStep(this)) al::startAction(mLayout, "DeactiveWait", "Start");
}
/** @brief Runs the End animation state. */
void CourseSelectInfoDecideButton::exeEnd() {
}
