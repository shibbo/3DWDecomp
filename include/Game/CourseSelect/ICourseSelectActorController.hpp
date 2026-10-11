#pragma once

#include <basis/seadTypes.h>

namespace al {
class BlockRailRider;
class LiveActor;
}  // namespace al

class CourseSelectActorInfo;
class CourseSelectDirector;
class CourseSelectNode;
class CourseSelectPuppeteer;
class CourseSelectPuppeteerGroup;

/**
 * @brief Interface of the course-select map objects a player can enter (miniatures, dokans,
 * rockets...), used by the puppeteers that drive the players into and out of them.
 */
class ICourseSelectActorController {
public:
    virtual al::LiveActor* getActor() = 0;
    virtual s32 getCursorLayoutType() const = 0;
    virtual const CourseSelectActorInfo* getCourseSelectActorInfo() const = 0;
    virtual CourseSelectNode* getCourseSelectNode() const = 0;
    virtual void startRouteDokanRider(al::BlockRailRider* pRider);
    virtual s32 calcOpenNodePriority() const;
    virtual void startPuppetDemo(CourseSelectPuppeteerGroup* pGroup) = 0;
    virtual void endPuppetDemo() = 0;
    virtual void startBind(CourseSelectPuppeteer* pPuppeteer) = 0;
    virtual bool tryDecide(const CourseSelectDirector* pDirector) = 0;
    virtual void startOpen() = 0;
    virtual void startOpenImmediately() = 0;
    virtual bool isEndOpen() const = 0;
    virtual bool isEnableOpenRoad() const;
};
