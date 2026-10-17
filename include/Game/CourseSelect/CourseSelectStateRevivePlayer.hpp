#pragma once
#include "Library/Nerve/NerveStateBase.hpp"
class CourseSelectScene;

class CourseSelectStateRevivePlayer : public al::NerveStateBase {
public:
    CourseSelectStateRevivePlayer(CourseSelectScene* pScene, bool isEntryDemo);
    void init() override;
    void appear() override;
    void exeStart();
    void exePlay();
    void exeEnd();
private:
    CourseSelectScene* mpScene;
    int _020 = 0;
    int mDeadPlayerCount = 0;
    bool mIsEntryDemo;
};
static_assert(sizeof(CourseSelectStateRevivePlayer) == 0x30);
