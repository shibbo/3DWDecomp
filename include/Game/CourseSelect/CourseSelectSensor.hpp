#pragma once
#include "prim/seadEnum.h"

namespace al {
class HitSensor;
class LiveActor;
}
class ICourseSelectActorController;

SEAD_ENUM(CourseSelectSensorType, Touch, Lock, Dokan, RouteDokan, Category4)
inline constexpr auto cCourseSelectSensorType_Actor = CourseSelectSensorType::Touch;
inline constexpr auto cCourseSelectSensorType_Rocket = CourseSelectSensorType::Category4;

class CourseSelectSensor {
public:
    const al::HitSensor* getHitSensor() const { return mpSensor; }
    ICourseSelectActorController* getController() const { return mpController; }
    CourseSelectSensor(CourseSelectSensorType type, ICourseSelectActorController* pController);
    void setSensor(const al::HitSensor* pSensor);
    al::LiveActor* getCourseSelectLock() const;
    bool isUseCourseSelectLayoutType() const;

private:
    CourseSelectSensorType mType;
    ICourseSelectActorController* mpController;
    const al::HitSensor* mpSensor;
};

static_assert(sizeof(CourseSelectSensor) == 0x18);
