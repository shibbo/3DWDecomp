#pragma once
#include "Library/Layout/LayoutActor.hpp"
namespace al { class LayoutInitInfo; }
class ButtonGroup;
class RCSControlGuideBar;

class CourseSelectLayoutKiosk : public al::LayoutActor {
public:
    CourseSelectLayoutKiosk(const al::LayoutInitInfo& rInfo);
    void appear() override;
    void exeAppear();
    void exeWait();
    void exeBack();
    void exeDecide();
    bool isDecideBack() const;
    bool isCourseDecided() const;
    void setGuideBar(RCSControlGuideBar* pGuideBar) { mpGuide = pGuideBar; }
    s32 getDecidedWorldId() const { return mSelectedCourse.world; }
    s32 getDecidedStageId() const { return mSelectedCourse.course; }
    struct Course { s32 world; s32 course; };
private:
    RCSControlGuideBar* mpGuide = nullptr;
    ButtonGroup* mpButtons;
    Course mSelectedCourse;
    s32 mPort = -1;
};

static_assert(sizeof(CourseSelectLayoutKiosk) == 0x148);
