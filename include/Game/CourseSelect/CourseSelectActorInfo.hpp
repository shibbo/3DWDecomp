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

private:
    al::LiveActor* mpActor;
    StageDatabaseInfo* mpStageInfo;
    int mWorldId;
    int mStageId;
    int mCourseId;
};

static_assert(sizeof(CourseSelectActorInfo) == 0x20);
