#pragma once

#include <math/seadMatrix.h>

#include "Library/Nerve/NerveStateBase.hpp"

namespace al {
class ActorInitInfo;
}  // namespace al

class DarkBowser;

/** @brief Fury Bowser's appearance demo state. */
class DarkBowserStateDemo : public al::NerveStateBase {
public:
    DarkBowserStateDemo(DarkBowser* pDarkBowser, const al::ActorInitInfo& rInfo);

    /**
     * @brief Sets the matrix the demo is placed relative to.
     * @param pMtx Base matrix (usually the player's).
     */
    void setBaseMtx(const sead::Matrix34f* pMtx) { mBaseMtx = pMtx; }

private:
    u8 _18[0x30 - 0x18];
    const sead::Matrix34f* mBaseMtx;  // 0x30
    u8 _38[0x60 - 0x38];
};
static_assert(sizeof(DarkBowserStateDemo) == 0x60);
