#pragma once
class CourseSelectInfoBase;
class ICourseSelectActorController;
/** @brief Exposes the course-information layout controls to the world map. */
class CourseSelectInfoController {
public:
    explicit CourseSelectInfoController(CourseSelectInfoBase* pLayout) : mLayout(pLayout) {}
    virtual void setTouchSelect(bool isTouch);
    virtual void startAppear(ICourseSelectActorController* pController);
    virtual void startDisappear();
    virtual void startDecide();
    virtual bool isAppear() const;
    virtual bool isDisappear() const;
    virtual bool isActive() const;
    virtual bool isEnableEnterCourse() const;
    virtual void startPause();
    virtual void endPause();
private:
    CourseSelectInfoBase* mLayout;
};
static_assert(sizeof(CourseSelectInfoController) == 0x10);
