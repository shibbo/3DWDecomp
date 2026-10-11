#pragma once

#include <math/seadVector.h>

#include "Library/Layout/LayoutActor.hpp"

namespace al {
class DemoDirector;
class LayoutInitInfo;
}  // namespace al

/**
 * @brief Counter shown over a locked Giga Bell with the Cat Shines still needed to unlock it.
 * @note Only what reconstructed code needs is declared so far.
 */
class CounterLockGigaBell : public al::LayoutActor {
public:
    CounterLockGigaBell(const char* pName, const al::LayoutInitInfo& rInfo,
                        const sead::Vector3f* pTrans, al::DemoDirector* pDemoDirector);

    void startShow(s32 count);
    void endShow();

private:
    u8 _128[0x140 - 0x128];
};

static_assert(sizeof(CounterLockGigaBell) == 0x140);
