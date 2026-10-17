#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class CourseSelectDirector;
class CourseSelectMiniature;
class CourseSelectPuppeteer;
class CourseSelectPuppeteerGroup;
class ICourseSelectActorController;

namespace al {
class CameraInfo;
}

class DemoEventGateKeeperChecker : public al::LiveActor {
  public:
    explicit DemoEventGateKeeperChecker(const char*);
    ~DemoEventGateKeeperChecker() override;
    void init(const al::ActorInitInfo&) override;
    void initAfterPlacement() override;
    void start();
    void startPuppetDemo(CourseSelectPuppeteerGroup*);
    void endPuppetDemo();
    void startBind(CourseSelectPuppeteer*);
    void exeWait();
    void exeBindWait();
    void exeDemoStart();
    void exeCamera();
    void exeCameraStopper();
    void exeAppearStopper();
    void exeDemoEnd();

  private:
    int mType = 0;
    CourseSelectDirector* mDirector = nullptr;
    CourseSelectMiniature* mMiniature = nullptr;
    ICourseSelectActorController* mController = nullptr;
    al::LiveActor* mUnused = nullptr;
    al::CameraInfo* mCamera = nullptr;
    al::CameraInfo* mStopperCamera = nullptr;
    int mCourseId = 0;
};

static_assert(sizeof(DemoEventGateKeeperChecker) == 0x180);
