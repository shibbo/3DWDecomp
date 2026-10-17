#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class CourseSelectBgmEventController : public al::LiveActor {
public:
    explicit CourseSelectBgmEventController(const char* pName);
    ~CourseSelectBgmEventController() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void exeCheckWorldStartDemo();

private:
    int mEventType = -1;
    int mWorldId = 0;
};

static_assert(sizeof(CourseSelectBgmEventController) == 0x150);
