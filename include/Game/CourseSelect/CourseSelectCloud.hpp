#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class BgmBeatAnimeController;

class CourseSelectCloud : public al::LiveActor {
public:
    explicit CourseSelectCloud(const char* pName);
    ~CourseSelectCloud() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void endClipped() override;
    void exeWait();

private:
    BgmBeatAnimeController* mpBeatAnimeController = nullptr;
};

static_assert(sizeof(CourseSelectCloud) == 0x150);
