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

    void startShowGoalItem(int count);
    void endShow();

private:
    u8 _128[0x30];
};

static_assert(sizeof(GuideBalloon) == 0x158);
