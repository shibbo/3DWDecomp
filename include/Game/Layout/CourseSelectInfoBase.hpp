#pragma once

#include "Library/Layout/LayoutActor.hpp"

namespace al { class LayoutInitInfo; }
class CourseSelectDirector;
class CourseSelectInfoDecideButton;
class CourseSelectInfoController;
class ICourseSelectActorController;

/** @brief Base layout for course details on the world map. */
class CourseSelectInfoBase : public al::LayoutActor {
public:
    CourseSelectInfoBase(const al::LayoutInitInfo& rInfo, const char* pName,
                         CourseSelectDirector* pDirector);
    /** @brief Returns the camera used to position course information.
     * @return Associated scene camera information. */
    al::SceneCameraInfo* getSceneCameraInfo() const override { return mSceneCameraInfo; }
    void control() override;
    /** @brief Updates course-specific controller information. */
    virtual void updateControllerInfo() {}
    /** @brief Updates course-specific green-star information. */
    virtual void updateGreenStars() {}

protected:
    float mOffsetY;
    CourseSelectInfoDecideButton* mDecideButton;
    CourseSelectDirector* mDirector;
    ICourseSelectActorController* mActorController;
    CourseSelectInfoController* mController;
    al::SceneCameraInfo* mSceneCameraInfo;
    bool mIsTouchSelect;
};
static_assert(sizeof(CourseSelectInfoBase) == 0x158);
