#pragma once

#include "Library/Layout/LayoutActor.hpp"

namespace al { class LayoutInitInfo; }

/** @brief Displays the world and stage number on the course-start curtain. */
class CourseSelectStartInfo : public al::LayoutActor {
public:
    explicit CourseSelectStartInfo(const al::LayoutInitInfo& rInfo);
    void exeAppear();
    void exeWait();
    void exeEnd();
};
