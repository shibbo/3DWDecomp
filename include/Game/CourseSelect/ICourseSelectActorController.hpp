#pragma once

namespace al { class LiveActor; class BlockRailRider; }
class CourseSelectActorInfo;
class CourseSelectNode;
class CourseSelectPuppeteer;
class CourseSelectPuppeteerGroup;
class CourseSelectDirector;

class ICourseSelectActorController {
public:
    virtual al::LiveActor* getActor() = 0;
    virtual int getCursorLayoutType() const { return 0; }
    virtual const CourseSelectActorInfo* getCourseSelectActorInfo() const = 0;
    virtual CourseSelectNode* getCourseSelectNode() const = 0;
    virtual void startRouteDokanRider(al::BlockRailRider* pRider) {}
    virtual int calcOpenNodePriority() const { return -1; }
    virtual void startPuppetDemo(CourseSelectPuppeteerGroup* pGroup) = 0;
    virtual void endPuppetDemo() = 0;
    virtual void startBind(CourseSelectPuppeteer* pPuppeteer) = 0;
    virtual bool tryDecide(const CourseSelectDirector* pDirector) = 0;
    virtual void startOpen() = 0;
    virtual void startOpenImmediately() = 0;
    virtual bool isEndOpen() const = 0;
    virtual bool isEnableOpenRoad() const { return true; }
};
