#pragma once

#include <basis/seadTypes.h>

namespace al {
class ActorInitInfo;
class LiveActor;
}  // namespace al

class StageDatabaseInfo;

/**
 * @brief Course data of a course-select map object (miniature, dokan, rocket...), read from its
 * placement arguments.
 * @note Minimal declaration: only what the reconstructed code uses is declared so far.
 */
class CourseSelectActorInfo {
public:
    CourseSelectActorInfo(al::LiveActor* pActor, const al::ActorInitInfo& rInfo);

    bool isUseCourseInfo() const;
    bool isEnterGateKeeper() const;
    bool isEnterKinopioBrigade() const;
    bool isEnterHide() const;

    /** @brief Gets the world of the course. @return The world id. */
    s32 getWorldId() const { return mWorldId; }
    /** @brief Gets the course. @return The course id. */
    s32 getCourseId() const { return mCourseId; }
    /** @brief Gets the stage number of the course in its world. @return The stage number. */
    s32 getStageNo() const { return mStageNo; }
    /** @brief Gets the stage database entry of the course. @return The entry. */
    StageDatabaseInfo* getStageDatabaseInfo() const { return mStageDatabaseInfo; }

private:
    al::LiveActor* mActor;  // 0x0
    StageDatabaseInfo* mStageDatabaseInfo;  // 0x8
    s32 mWorldId;  // 0x10
    s32 mStageNo;  // 0x14
    s32 mCourseId;  // 0x18
};

static_assert(sizeof(CourseSelectActorInfo) == 0x20);
