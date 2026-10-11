#pragma once

#include <math/seadVector.h>
#include <prim/seadBitFlag.h>

#include "Layout/GuideBalloon.hpp"

namespace al {
class DemoDirector;
class LayoutInitInfo;
}  // namespace al

/**
 * @brief Guide balloon of the Toad Brigade captain that shows which members were found.
 * @note Only what reconstructed code needs is declared so far.
 */
class GuideBalloonBrigade : public GuideBalloon {
public:
    GuideBalloonBrigade(const char* pName, const al::LayoutInitInfo& rInfo,
                        const sead::Vector3f* pTrans, const sead::Vector3f& rOffset,
                        al::DemoDirector* pDemoDirector);

    void startShowBrigade(sead::BitFlag8 collected);

private:
    u8 _158[0x20];
};

static_assert(sizeof(GuideBalloonBrigade) == 0x178);
