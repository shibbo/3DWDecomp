#pragma once

#include <math/seadMatrix.h>

#include "Layout/HealthBar.hpp"

namespace al {
class LayoutInitInfo;
class LiveActor;
}  // namespace al

/** @brief Health bar shown over Fury Bowser during his battles. */
class SuperBowserHealthBar : public rc::HealthBar {
public:
    /** @brief Which variant of the bar is shown. */
    enum HealthBarType : s32 {
        HealthBarType_Single = 0,
        HealthBarType_Double = 1,
        HealthBarType_V2 = 2,
    };

    /** @brief Whether the bar is shown at its normal or offset position. */
    enum OffsetState : s32 {
        OffsetState_Normal = 0,
        OffsetState_Offset = 1,
    };

    SuperBowserHealthBar(const al::LiveActor* pActor, const al::LayoutInitInfo& rInfo,
                         f32 (*pGetRate)(const al::LiveActor*), HealthBarType type,
                         const sead::Matrix34f* pFaceMtx, const sead::Matrix34f* pSpikeMtx,
                         f32 offsetX, f32 offsetY);

    void setState(OffsetState state, f32 frame);

private:
    u8 _128[0x188 - 0x128];
};
static_assert(sizeof(SuperBowserHealthBar) == 0x188);
