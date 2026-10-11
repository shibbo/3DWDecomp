#pragma once

#include "Layout/CourseSelectInfoBase.hpp"

class CourseSelectInfoMystery10 : public CourseSelectInfoBase {
public:
    CourseSelectInfoMystery10(const al::LayoutInitInfo& rInfo, CourseSelectDirector* pDirector);
    void updateControllerInfo() override;
};
