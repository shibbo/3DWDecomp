#pragma once
#include "Library/Nerve/NerveStateBase.hpp"
class CourseSelectScene;
class CourseSelectDirector;
class CourseSelectWindowHolder;

class CourseSelectStateAfterEndingEvent : public al::NerveStateBase {
public:
    CourseSelectStateAfterEndingEvent(CourseSelectScene* pScene, CourseSelectDirector* pDirector);
    void init() override;
    void appear() override;
    void exeDemo();
    void exeWipe();
private:
    CourseSelectScene* mpScene;
    CourseSelectDirector* mpDirector;
    CourseSelectWindowHolder* mpWindows = nullptr;
};
static_assert(sizeof(CourseSelectStateAfterEndingEvent) == 0x30);
