#pragma once

#include <basis/seadTypes.h>

namespace al {
class HitSensor;
}  // namespace al

class ICourseSelectActorController;

/** @brief Kinds of course-select sensors. */
enum CourseSelectSensorType : s64 {
    cCourseSelectSensorType_Actor = 0,
    cCourseSelectSensorType_Rocket = 4,
};

/**
 * @brief Links a hit sensor of a course-select object to its controller, so the director knows
 * which object the players touch.
 * @note Minimal declaration: only what the reconstructed code uses is declared so far.
 */
class CourseSelectSensor {
public:
    CourseSelectSensor(CourseSelectSensorType type, ICourseSelectActorController* pController);

    void setSensor(const al::HitSensor* pSensor);
    bool isUseCourseSelectLayoutType() const;

    /** @brief Gets the controller of the object owning the sensor. @return The controller. */
    ICourseSelectActorController* getController() const { return mController; }
    /** @brief Gets the linked hit sensor. @return The hit sensor. */
    const al::HitSensor* getHitSensor() const { return mHitSensor; }

private:
    CourseSelectSensorType mType;  // 0x0
    ICourseSelectActorController* mController;  // 0x8
    const al::HitSensor* mHitSensor;  // 0x10
};

static_assert(sizeof(CourseSelectSensor) == 0x18);
