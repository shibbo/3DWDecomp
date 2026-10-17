#pragma once
#include "Layout/GuideBalloon.hpp"
#include <prim/seadBitFlag.h>

class GuideBalloonBrigade : public GuideBalloon {
public:
    GuideBalloonBrigade(const char* pName, const al::LayoutInitInfo& rInfo,
                       const sead::Vector3f* pTrans, const sead::Vector3f& rOffset,
                       al::DemoDirector* pDemoDirector);
    void startShowBrigade(sead::BitFlag<u8> flags);
private:
    al::LayoutActor* mpBrigade1;
    al::LayoutActor* mpBrigade2;
    al::LayoutActor* mpBrigade3;
    al::LayoutActor* mpBrigade4;
};
