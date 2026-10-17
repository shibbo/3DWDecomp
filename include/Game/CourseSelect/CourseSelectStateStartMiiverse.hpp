#pragma once
#include "Library/Nerve/NerveStateBase.hpp"
class CourseSelectScene;
class CourseSelectWindowHolder;
class CourseSelectStateStartMiiverse : public al::HostStateBase<CourseSelectScene> {
public:
    explicit CourseSelectStateStartMiiverse(CourseSelectScene* pScene);
    void init() override;
    void appear() override;
    void exeInfoMiiverse();
    void exeInfoGhost();
    void exeInfoLocalActMiiverse();
    void exeInfoLocalActGhost();
    void exeConfirm();
    void exeConfirmLocal();
    void exeError();
    void exeDecide();
private:
    CourseSelectWindowHolder* mpWindows;
};
static_assert(sizeof(CourseSelectStateStartMiiverse) == 0x28);
