#include "Layout/CourseSelectInfoMysteryMax.hpp"

#include "Layout/CourseSelectInfoLayoutFunction.hpp"

/**
 * @brief Creates the course information layout.
 * @param rInfo Layout initialization context.
 * @param pDirector World-map course selection director.
 */
CourseSelectInfoMysteryMax::CourseSelectInfoMysteryMax(const al::LayoutInitInfo& rInfo, CourseSelectDirector* pDirector)
    : CourseSelectInfoBase(rInfo, "CourseSelectInfoMysteryMax", pDirector) {}

/** @brief Updates the course completion indicators. */
void CourseSelectInfoMysteryMax::updateControllerInfo() {
    CourseSelectInfoLayoutFunction::setPaneGreenStar(this, 15);
    CourseSelectInfoLayoutFunction::setPaneClearPlayer(this, false);
    CourseSelectInfoLayoutFunction::setPaneStamp(this);
}
