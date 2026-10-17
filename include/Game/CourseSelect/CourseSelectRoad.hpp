#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class CourseSelectNode;
class CourseSelectRoad : public al::LiveActor {
public:
    const sead::Vector3f& getTargetTrans() const { return mEndPos; }
    CourseSelectRoad(const char* pName, const CourseSelectNode* pNode, const sead::Vector3f& rEndPos);
    ~CourseSelectRoad() override;
    void initRoad(const al::ActorInitInfo& rInfo, const sead::Vector3f* pOffset);
    static void setRoadOpenSuperSpeed(bool isSuperSpeed);
    void kill() override;
    void exeWait();
    void exeOpen();
    void exeStop();
    void startOpen(bool isImmediately);
    void endEarly();
    bool isEndOpen() const;
    bool isWait() const;
    void subDistance(float distance);
private:
    const CourseSelectNode* mpNode;
    sead::Vector3f mEndPos;
    float mDistance = 0.0f;
    sead::Vector3f mClippingCenter = {0.0f, 0.0f, 0.0f};
    sead::Vector3f mEffectPos = {0.0f, 0.0f, 0.0f};
};
static_assert(sizeof(CourseSelectRoad) == 0x178);
