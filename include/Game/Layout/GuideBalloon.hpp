#pragma once

#include "Library/Layout/LayoutActor.hpp"
#include <math/seadVector.h>

namespace al {
class LayoutInitInfo;
class DemoDirector;
}

class GuideBalloon : public al::LayoutActor {
public:
    GuideBalloon(const char* pName, const al::LayoutInitInfo& rInfo,
                 const sead::Vector3f* pTrans, const sead::Vector3f& rOffset,
                 bool isFollow, al::DemoDirector* pDemoDirector);

    GuideBalloon(const char* pName, const char* pLayoutName, const al::LayoutInitInfo& rInfo,
                 const sead::Vector3f* pTrans, const sead::Vector3f& rOffset,
                 bool isFollow, al::DemoDirector* pDemoDirector);

    al::SceneCameraInfo* getSceneCameraInfo() const override { return mpSceneCameraInfo; }
    void setAppear();
    void startShowGoalItem(int count);
    void endShow();

private:
    al::SceneCameraInfo* mpSceneCameraInfo;
    u8 _130[0x28];
};

static_assert(sizeof(GuideBalloon) == 0x158);
