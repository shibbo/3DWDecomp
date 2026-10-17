#include "CourseSelect/CourseSelectSensor.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"

/**
 * @brief Create an unbound course-select interaction sensor.
 * @param type Interaction category used by the course-select layout.
 * @param pController Controller associated with this interaction.
 */
CourseSelectSensor::CourseSelectSensor(CourseSelectSensorType type, ICourseSelectActorController* pController)
    : mType(type), mpController(pController), mpSensor(nullptr) {}

/**
 * @brief Bind the interaction to an actor's hit sensor.
 * @param pSensor Hit sensor to retain.
 */
void CourseSelectSensor::setSensor(const al::HitSensor* pSensor) {
    mpSensor = pSensor;
}

/**
 * @brief Get the lock actor associated with the bound sensor.
 * @return The sensor's host actor; a hit sensor must already be bound.
 */
al::LiveActor* CourseSelectSensor::getCourseSelectLock() const {
    return al::getSensorHost(mpSensor);
}

/**
 * @brief Test whether the sensor belongs to a course-select layout category.
 * @return Whether the category lies in the layout range, zero through four.
 */
bool CourseSelectSensor::isUseCourseSelectLayoutType() const {
    return static_cast<unsigned int>(mType) < 5;
}
