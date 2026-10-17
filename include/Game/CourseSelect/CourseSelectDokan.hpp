#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class CourseSelectDirector;
class CourseSelectSensor;
class CourseSelectActorInfo;
class DokanController;
class CourseSelectPuppeteer;
class CourseSelectPuppeteerGroup;
class CourseSelectNode;
class CourseSelectNodeParts;

class CourseSelectDokan : public al::LiveActor {
public:
    explicit CourseSelectDokan(const char* pName);
    ~CourseSelectDokan() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void initDokanHide(const al::ActorInitInfo& rInfo);
    void initAfterPlacement() override;
    void attackSensor(al::HitSensor* pSender, al::HitSensor* pReceiver) override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    void appear() override;
    void kill() override;
    void control() override;
    void activate();
    void inactivate();
    void activateImmediately();
    void setDstDokan(CourseSelectDokan* pDokan);
    bool isHideDokan() const;
    bool isNeedSync() const;
    bool isDemo() const;
    void startPuppetDemo(CourseSelectPuppeteerGroup* pGroup);
    void endPuppetDemo();
    void startBind(CourseSelectPuppeteer* pPuppeteer);
    bool tryDecide(const CourseSelectDirector* pDirector);
    int tryGetCourseId() const;
    void syncHideEnd(const CourseSelectDokan* pDokan);
    void exeOpenPrevRoad();
    void exeWait();
    void exeHide();
    void exeHideToShow();
    void exeDemoIn();
    void exeDemoOut();
    CourseSelectDokan* getDstDokan() const { return mpDstDokan; }
    int getLinkId() const { return mLinkId; }
    CourseSelectNode* getNode() const { return mpNode; }
private:
    CourseSelectDokan* mpDstDokan = nullptr;
    int mLinkId = 0;
    bool mCameraInterpolate = false;
    bool mKillAfterDemo = false;
    bool mIsSource = false;
    bool mSyncDead = false;
    bool mActive = true;
    bool mIsHiddenType = false;
    al::LiveActor* mpClosedMiniature = nullptr;
    CourseSelectDirector* mpDirector = nullptr;
    CourseSelectSensor* mpSensor = nullptr;
    CourseSelectActorInfo* mpActorInfo = nullptr;
    DokanController* mpController = nullptr;
    CourseSelectPuppeteerGroup* mpGroup = nullptr;
    CourseSelectNode* mpNode = nullptr;
    CourseSelectNodeParts* mpPrevRoad = nullptr;
    float mPrevRoadOffset = 0.0f;
};
static_assert(sizeof(CourseSelectDokan) == 0x1a8);
