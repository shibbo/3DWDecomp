#include "Layout/CourseSelectInfoA.hpp"

#include "Layout/CourseSelectInfoLayoutFunction.hpp"

/**
 * @brief Creates the course information layout.
 * @param rInfo Layout initialization context.
 * @param pDirector World-map course selection director.
 */
CourseSelectInfoA::CourseSelectInfoA(const al::LayoutInitInfo& rInfo, CourseSelectDirector* pDirector)
    : CourseSelectInfoBase(rInfo, "CourseSelectInfoA", pDirector) {}

/** @brief Updates the course completion indicators. */
void CourseSelectInfoA::updateControllerInfo() {
}

/** @brief Sets the vertical offset for this course marker type. */
void CourseSelectInfoA::setOffsetDefault() {
    mOffsetY = 250.0f;
}

/** @brief Sets the vertical offset for this course marker type. */
void CourseSelectInfoA::setOffsetDokan() {
    mOffsetY = 200.0f;
}

/** @brief Sets the vertical offset for this course marker type. */
void CourseSelectInfoA::setOffsetRouteDokan() {
    mOffsetY = 120.0f;
}
