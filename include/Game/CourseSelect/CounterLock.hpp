#pragma once
#include "Library/Layout/LayoutActor.hpp"
#include "math/seadVector.h"
namespace al { class LayoutInitInfo; }
class CourseSelectDirector;
class ICourseSelectActorController;
class CourseSelectInfoDecideButton;
class CounterLockController;
class CounterLock : public al::LayoutActor {
public:
    CounterLock(const al::LayoutInitInfo& rInfo, const CourseSelectDirector* pDirector);
    void control() override;
    void setTouchSelect(bool isTouch);
    void startShow(ICourseSelectActorController* pController);
    void endShow();
    void startDecide();
    bool isAppear() const;
    bool isDisappear() const;
    bool isEnableEnter() const;
    void exeAppear();
    void exeWait();
    void exeReaction();
    void exeFrameOut();
    void exeEnd();
    void exeHide();
    al::SceneCameraInfo* getSceneCameraInfo() const override;
private:
    const CourseSelectDirector* mpDirector;
    int mRequiredStars = 0;
    const sead::Vector3f* mpWorldPos = nullptr;
    al::SceneCameraInfo* mpSceneCameraInfo;
    CourseSelectInfoDecideButton* mpDecideButton = nullptr;
    CounterLockController* mpController = nullptr;
    bool mTouchSelect = false;
};
static_assert(sizeof(CounterLock) == 0x160);
