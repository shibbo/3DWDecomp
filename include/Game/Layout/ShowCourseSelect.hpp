#pragma once
#include "Library/Layout/LayoutActor.hpp"
namespace al { class LayoutInitInfo; }
class ButtonGroup;
/** @brief Course selection menu used for the E3 show. */
class ShowCourseSelect : public al::LayoutActor {
public:
    explicit ShowCourseSelect(const al::LayoutInitInfo& rInfo);
    void appear() override;
    void setControllerPort(int port);
    int getDecideCourseIndex() const;
    bool isDecide() const;
    bool isBack() const;
    void exeAppear();
    void exeWaitCursor();
    void exeWait();
    void exeEnd();
    void lockButtonEachOther();
    int findDecideCourseIndex() const;
private:
    ButtonGroup* mButtons = nullptr;
    int mPort;
    int mDecideCourseIndex = -1;
    bool mHasPlayedDecideSound = false;
};
static_assert(sizeof(ShowCourseSelect) == 0x140);
