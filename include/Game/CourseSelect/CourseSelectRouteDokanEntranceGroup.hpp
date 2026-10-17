#pragma once

namespace al {
class ActorInitInfo;
class BlockRailParts;
class BlockRailPartsGroup;
}
class CourseSelectRouteDokanEntrance;

class CourseSelectRouteDokanEntranceGroup {
public:
    CourseSelectRouteDokanEntranceGroup();
    void init(al::BlockRailPartsGroup* pParts, const al::ActorInitInfo& rInfo);
    void addEntrance(al::BlockRailParts* pParts, CourseSelectRouteDokanEntrance* pEntrance);
    void createStopper(const al::ActorInitInfo& rInfo);
    void appearStopper(bool isAppear);
    void active();
    void deactive();
    void setEnableInput();
    void setDisableInput();

private:
    CourseSelectRouteDokanEntrance** mpEntrances = nullptr;
    int mEntranceCount = 0;
    int mEntranceCapacity = 0;
};

static_assert(sizeof(CourseSelectRouteDokanEntranceGroup) == 0x10);
