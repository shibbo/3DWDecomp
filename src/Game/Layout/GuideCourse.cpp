#include "Layout/GuideCourse.hpp"

#include "Layout/CourseSelectMiniatureCursor.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"

namespace {
NERVE_DECL(GuideCourse, Hide);
NERVE_DECL(GuideCourse, Appear);
NERVE_DECL(GuideCourse, End);
NERVE_DECL(GuideCourse, Wait);
NERVES_MAKE_NOSTRUCT(GuideCourse, Hide, Appear, End, Wait)
}

/** @brief Creates an initially hidden course guide.
 * @param rInfo Layout and camera initialization context.
 * @param pName Actor name. */
GuideCourse::GuideCourse(const al::LayoutInitInfo& rInfo, const char* pName)
    : al::LayoutActor(pName), mSceneCameraInfo(rInfo.getSceneCameraInfo()) {
    al::initLayoutActor(this, rInfo, "GuideCourse", nullptr);
    initNerve(&NrvGuideCourseHide, 0);
    kill();
}

/** @brief Updates the guide's projected screen position. */
void GuideCourse::control() { updateScreenPos(); }

/** @brief Projects a point above the selected miniature into the layout. */
void GuideCourse::updateScreenPos() {
    sead::Vector2f position;
    CourseSelectMiniatureCursor::calcMiniatureLayoutPos(
        &position, this, mMiniature, sead::Vector3f(0.0f, 250.0f, 0.0f), 50.0f);
    al::setLocalTrans(this, position);
}

/** @brief Attaches the guide to a miniature and requests its appearance.
 * @param pMiniature Miniature to follow. */
void GuideCourse::startAppear(al::LiveActor* pMiniature) {
    appear();
    al::hidePaneRootNoRecursive(this);
    mMiniature = pMiniature;
    al::setNerve(this, &NrvGuideCourseAppear);
}
/** @brief Starts the end animation. */
void GuideCourse::startEnd() { al::setNerve(this, &NrvGuideCourseEnd); }
/** @brief Checks whether the guide is appearing or idle.
 * @return Whether the guide is in a visible state. */
bool GuideCourse::isShow() const {
    return al::isNerve(this, &NrvGuideCourseAppear) || al::isNerve(this, &NrvGuideCourseWait);
}
/** @brief Reveals the root pane and plays the appearance animation. */
void GuideCourse::exeAppear() {
    if (al::isFirstStep(this)) {
        al::showPaneRootNoRecursive(this);
        al::startAction(this, "Appear", nullptr);
    }
    if (al::isActionEnd(this, nullptr))
        al::setNerve(this, &NrvGuideCourseWait);
}
/** @brief Starts the idle animation. */
void GuideCourse::exeWait() {
    if (al::isFirstStep(this))
        al::startAction(this, "Wait", nullptr);
}
/** @brief Plays the end animation before hiding the guide. */
void GuideCourse::exeEnd() {
    if (al::isFirstStep(this))
        al::startAction(this, "End", nullptr);
    if (al::isActionEnd(this, nullptr))
        al::setNerve(this, &NrvGuideCourseHide);
}
/** @brief Deactivates the hidden guide. */
void GuideCourse::exeHide() { kill(); }
