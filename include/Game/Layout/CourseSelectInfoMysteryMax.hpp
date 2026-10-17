#pragma once

#include "Layout/CourseSelectInfoBase.hpp"

class CourseSelectInfoMysteryMax : public CourseSelectInfoBase {
public:
    CourseSelectInfoMysteryMax(const al::LayoutInitInfo& rInfo, CourseSelectDirector* pDirector);
    void updateControllerInfo() override;
};
