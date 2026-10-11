#pragma once

#include "Layout/CourseSelectInfoBase.hpp"

class CourseSelectInfoA : public CourseSelectInfoBase {
public:
    CourseSelectInfoA(const al::LayoutInitInfo& rInfo, CourseSelectDirector* pDirector);
    void updateControllerInfo() override;
    void setOffsetDefault();
    void setOffsetDokan();
    void setOffsetRouteDokan();
};
