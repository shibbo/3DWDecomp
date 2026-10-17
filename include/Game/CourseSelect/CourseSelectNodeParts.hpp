#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class CourseSelectNodeParts : public al::LiveActor {
public:
    bool isCurve() const { return mIsRoadCurve; }
    CourseSelectNodeParts(const char* pName, const char* pGrowAction, const char* pWaitAction);
    ~CourseSelectNodeParts() override;
    void initPartsWithArchiveName(const al::ActorInitInfo& rInfo, const char* pArchiveName);
    void startOpen();
    void startOpenImmediately();
    bool isEndGrow() const;
    void exeDeactive();
    void exeGrow();
    void exeWait();
private:
    const char* mpGrowAction;
    const char* mpWaitAction;
    bool mIsRoadCurve = false;
};
static_assert(sizeof(CourseSelectNodeParts) == 0x160);
