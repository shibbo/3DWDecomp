#include "Layout/CourseSelectInfo.hpp"

#include "Layout/CourseSelectInfoLayoutFunction.hpp"

/**
 * @brief Creates the course information layout.
 * @param rInfo Layout initialization context.
 * @param pDirector World-map course selection director.
 */
CourseSelectInfo::CourseSelectInfo(const al::LayoutInitInfo& rInfo, CourseSelectDirector* pDirector)
    : CourseSelectInfoBase(rInfo, "CourseSelectInfo", pDirector) {}

/** @brief Updates the course completion indicators. */
void CourseSelectInfo::updateControllerInfo() {
    CourseSelectInfoLayoutFunction::setPaneClearPlayer(this, true);
    CourseSelectInfoLayoutFunction::setPaneStamp(this);
}

/** @brief Updates the five green-star panes. */
void CourseSelectInfo::updateGreenStars() {
    CourseSelectInfoLayoutFunction::setPaneGreenStar(this, 5);
}
