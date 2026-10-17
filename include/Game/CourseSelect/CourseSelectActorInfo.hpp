#pragma once

namespace al {
class LiveActor;
class ActorInitInfo;
}
class StageDatabaseInfo;

class CourseSelectActorInfo {
public:
    CourseSelectActorInfo(al::LiveActor* pActor, const al::ActorInitInfo& rInfo);
    bool isUseCourseInfo() const;
    bool isEnterGateKeeper() const;
    bool isEnterKinopioBrigade() const;
    bool isEnterHide() const;

    const StageDatabaseInfo* getStageInfo() const { return mpStageInfo; }
    int getWorldId() const { return mWorldId; }
    int getStageNo() const { return mStageId; }
    StageDatabaseInfo* getStageDatabaseInfo() const { return mpStageInfo; }
    int getCourseId() const { return mCourseId; }
private:
    al::LiveActor* mpActor;
    StageDatabaseInfo* mpStageInfo;
    int mWorldId;
    int mStageId;
    int mCourseId;
};

static_assert(sizeof(CourseSelectActorInfo) == 0x20);
