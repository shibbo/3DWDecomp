#include "Course/CourseSelectLayoutKiosk.hpp"
#include "Layout/ButtonGroup.hpp"
#include "Layout/RCSControlGuideBar.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "System/GameDataConst.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/ControlUserUtil.hpp"

namespace {
NERVE_DECL(CourseSelectLayoutKiosk, Wait);
NERVE_DECL(CourseSelectLayoutKiosk, Appear);
NERVE_DECL(CourseSelectLayoutKiosk, Back);
NERVE_DECL(CourseSelectLayoutKiosk, Decide);
NERVES_MAKE_NOSTRUCT(CourseSelectLayoutKiosk, Wait, Appear)
NERVES_MAKE_STRUCT(CourseSelectLayoutKiosk, Back, Decide)
const CourseSelectLayoutKiosk::Course cCourses[] = {{1, 1}, {2, 5}, {3, 101}};
}

/**
 * @brief Creates the kiosk course-selection layout and its buttons.
 * @param rInfo Layout initialization context.
 */
CourseSelectLayoutKiosk::CourseSelectLayoutKiosk(const al::LayoutInitInfo& rInfo)
    : al::LayoutActor("CourseSelectLayoutKiosk") {
    al::initLayoutActor(this, rInfo, "RCS_CourseSelectKiosk", nullptr);
    initNerve(&NrvCourseSelectLayoutKioskWait, 0);
    mpButtons = new ButtonGroup(rInfo, this, "RCS_CourseSelectKiosk", nullptr, false);
}

/** @brief Resets the buttons and starts the appear transition. */
void CourseSelectLayoutKiosk::appear() {
    al::LayoutActor::appear();
    mpButtons->reset();
    mpButtons->hideCursor();
    mpButtons->invalidate();
    al::setNerve(this, &NrvCourseSelectLayoutKioskAppear);
}

/** @brief Configures the controlling player and selects the first course button. */
void CourseSelectLayoutKiosk::exeAppear() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Appear");
        mPort = rc::calcPadPortByFirstActiveUser(this);
        mpGuide->setCharacter(GameDataConst::getPlayerCharacterName(
            rc::getControlUserCharacterType(this, rc::calcControlUserIdFromPortNum(this, mPort))));
        mpGuide->overridePort(mPort);
    }
    if (al::isActionEnd(this)) {
        mpButtons->getCursorParts()->reset();
        mpButtons->select(mpButtons->getFirstTarget());
        mpButtons->showCursor();
        al::setNerve(this, &NrvCourseSelectLayoutKioskWait);
    }
}

/** @brief Handles course selection and the back button. */
void CourseSelectLayoutKiosk::exeWait() {
    if (al::isStep(this, static_cast<s32>(al::getActionFrameMax(mpButtons->getCursor(), "Appear", "Main")))) {
        mpButtons->validate();
    }
    mpButtons->updateAndCursorDefault(mPort);
    if (al::isPadTriggerB(mPort)) {
        mpButtons->invalidate();
        mpButtons->hideCursor();
        al::setNerve(this, &NrvCourseSelectLayoutKiosk.Back);
    }
    if (mpButtons->isDecideAny()) {
        mpButtons->invalidate();
        mpButtons->hideCursor();
    }
    if (mpButtons->isDecideEndAny()) {
        mSelectedCourse = cCourses[mpButtons->getDecideEndButtonIndex()];
        al::setNerve(this, &NrvCourseSelectLayoutKiosk.Decide);
    }
}

/** @brief Plays the back sound once on entry. */
void CourseSelectLayoutKiosk::exeBack() {
    if (al::isFirstStep(this)) {
        al::startSe(this, "Back");
    }
}

/** @brief Keeps the chosen course available for the owning scene. */
void CourseSelectLayoutKiosk::exeDecide() {}

/** @brief Returns whether the player chose to go back. */
bool CourseSelectLayoutKiosk::isDecideBack() const {
    return al::isNerve(this, &NrvCourseSelectLayoutKiosk.Back);
}

/** @brief Returns whether a course has been chosen. */
bool CourseSelectLayoutKiosk::isCourseDecided() const {
    return al::isNerve(this, &NrvCourseSelectLayoutKiosk.Decide);
}
