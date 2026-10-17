#pragma once

#include "Layout/CourseSelectInfoBase.hpp"

class CourseSelectInfo : public CourseSelectInfoBase {
public:
    CourseSelectInfo(const al::LayoutInitInfo& rInfo, CourseSelectDirector* pDirector);
    void updateControllerInfo() override;
    void updateGreenStars() override;
};
