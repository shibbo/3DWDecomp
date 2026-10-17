#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class BgmBeatAnimeController;

class CourseSelectRhythmObject : public al::LiveActor {
public:
    explicit CourseSelectRhythmObject(const char* pName);
    ~CourseSelectRhythmObject() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void makeActorAppeared() override;
    void control() override;
    void endClipped() override;
    void appearBySwitch();
    void killBySwitch();

private:
    BgmBeatAnimeController* mpBeatAnimeController = nullptr;
    bool mIsActive = true;
};

static_assert(sizeof(CourseSelectRhythmObject) == 0x158);
