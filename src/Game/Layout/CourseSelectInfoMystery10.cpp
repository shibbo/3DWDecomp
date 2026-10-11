#include "Layout/CourseSelectInfoMystery10.hpp"

#include "Layout/CourseSelectInfoLayoutFunction.hpp"

/**
 * @brief Creates the course information layout.
 * @param rInfo Layout initialization context.
 * @param pDirector World-map course selection director.
 */
CourseSelectInfoMystery10::CourseSelectInfoMystery10(const al::LayoutInitInfo& rInfo, CourseSelectDirector* pDirector)
    : CourseSelectInfoBase(rInfo, "CourseSelectInfoMystery10", pDirector) {}

/** @brief Updates the course completion indicators. */
void CourseSelectInfoMystery10::updateControllerInfo() {
    CourseSelectInfoLayoutFunction::setPaneGreenStar(this, 5);
    CourseSelectInfoLayoutFunction::setPaneClearPlayer(this, true);
}
