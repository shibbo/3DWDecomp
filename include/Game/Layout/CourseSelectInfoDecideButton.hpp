#pragma once
#include "Library/Nerve/NerveStateBase.hpp"
namespace al { class LayoutActor; }
/** @brief Controls the course-selection start button animation. */
class CourseSelectInfoDecideButton : public al::NerveStateBase {
public:
    explicit CourseSelectInfoDecideButton(al::LayoutActor* pLayout);
    ~CourseSelectInfoDecideButton() override = default;
    void setDecideButtonTxt(int port);
    void startActive(bool immediate);
    void startDeactive(bool immediate);
    void startDecide();
    bool isEnableDecide() const;
    bool isEnd() const;
    void exeDeactiveWait();
    void exeDecide();
    void exeActiveAppear();
    void exeActiveWait();
    void exeDeactiveAppear();
    void exeEnd();
private:
    al::LayoutActor* mLayout;
};
static_assert(sizeof(CourseSelectInfoDecideButton) == 0x20);
