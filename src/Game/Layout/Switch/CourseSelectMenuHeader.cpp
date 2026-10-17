#include "Layout/Switch/CourseSelectMenuHeader.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Project/Base/StringUtil.hpp"
namespace {
NERVE_DECL(CourseSelectMenuHeader, Wait);
NERVE_DECL(CourseSelectMenuHeader, Appear);
NERVE_DECL(CourseSelectMenuHeader, End);
NERVE_DECL(CourseSelectMenuHeader, TextChange);
NERVES_MAKE_NOSTRUCT(CourseSelectMenuHeader, Wait, Appear, End, TextChange)
}
/** @brief Creates the map menu header and its title parts. */
CourseSelectMenuHeader::CourseSelectMenuHeader(const al::LayoutInitInfo& rInfo)
    : al::LayoutActor("CourseSelectMenuHeader") {
    al::initLayoutActor(this, rInfo, "RCS_CourseSelectSubHeader", nullptr);
    mHeader = new al::LayoutActor("RCS_CourseSelectSubHeaderParts");
    al::initLayoutPartsActor(mHeader, this, rInfo, "ParCourseSelectSubHeader", nullptr);
    initNerve(&NrvCourseSelectMenuHeaderWait, 0);
}
/** @brief Shows the header with its entrance animation. */
void CourseSelectMenuHeader::appear() {
    al::LayoutActor::appear();
    al::setNerve(this, &NrvCourseSelectMenuHeaderAppear);
}
/** @brief Starts the header exit animation. */
void CourseSelectMenuHeader::end() { al::setNerve(this, &NrvCourseSelectMenuHeaderEnd); }
/** @brief Selects a page and changes its title, optionally with a transition. */
void CourseSelectMenuHeader::setHeader(const char* pName, bool isAnimate) {
    al::StringTmp<32> action("SetSubHeader_%s", pName);
    const char* pAction = action.cstr();
    al::startAction(mHeader, pAction, "PageIndicator");
    mMessageName = pName;
    if (isAnimate) {
        al::startAction(mHeader, "HeaderTextChange", "Text");
        al::setNerve(this, &NrvCourseSelectMenuHeaderTextChange);
    } else {
        al::setPaneSystemMessage(mHeader, "TxtHeader", "RCS_CourseSelectSubHeaderParts", mMessageName);
    }
}
/** @brief Selects the shoulder button labels for the active controller. */
void CourseSelectMenuHeader::setButtonType(bool isJoySingle) {
    al::startAction(mHeader, isJoySingle ? "SetSubHeaderButtonsSLSR" : "SetSubHeaderButtonsLR", "HeaderButtons");
}
/** @brief Starts the entrance and button animations. */
void CourseSelectMenuHeader::exeAppear() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Appear", nullptr);
        al::startAction(mHeader, "SetSubHeaderAnimation", "ButtonAnim");
    }
}
/** @brief Starts the idle animation. */
void CourseSelectMenuHeader::exeWait() {
    if (al::isFirstStep(this)) al::startAction(this, "Wait", nullptr);
}
/** @brief Replaces the title halfway through its transition. */
void CourseSelectMenuHeader::exeTextChange() {
    if (al::isGreaterEqualStep(this, al::getActionFrameMax(mHeader, "HeaderTextChange", "Text") * 0.5f)) {
        al::setPaneSystemMessage(mHeader, "TxtHeader", "RCS_CourseSelectSubHeaderParts", mMessageName);
        al::setNerve(this, &NrvCourseSelectMenuHeaderWait);
    }
}
/** @brief Starts the exit animation. */
void CourseSelectMenuHeader::exeEnd() {
    if (al::isFirstStep(this)) al::startAction(this, "End", nullptr);
}

