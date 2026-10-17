#pragma once

#include "Library/LiveActor/LiveActor.hpp"

namespace al { class BlockRailPartsGroup; }
class CourseSelectRouteDokanEntranceGroup;

class CourseSelectRouteDokan : public al::LiveActor {
public:
    bool isDeactive() const { return mIsDeactive; }
    explicit CourseSelectRouteDokan(const char* pName);
    ~CourseSelectRouteDokan() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void active();
    void deactive();
    void createStopper(const al::ActorInitInfo& rInfo);
    void appearStopper(bool isAppear);
    void startAppearEffectW7();
    void setEnableInput();
    void setDisableInput();

private:
    al::BlockRailPartsGroup* mpParts = nullptr;
    CourseSelectRouteDokanEntranceGroup* mpEntrances = nullptr;
    bool mIsDeactive = false;
};

static_assert(sizeof(CourseSelectRouteDokan) == 0x160);
