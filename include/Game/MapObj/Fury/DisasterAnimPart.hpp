#pragma once
#include "MapObj/DisasterModeController.hpp"
#include <prim/seadSafeString.h>
class DisasterAnimPart : public DisasterModeStateListener {
public:
    DisasterAnimPart();
    void initAnimPart(al::LiveActor*, const al::ActorInitInfo&);
    void initAnimPartAfterPlacement(al::LiveActor*);
    void startDisasterModeAnim(al::LiveActor*, DisasterModeController::State);
protected:
    sead::FixedSafeString<128> mNormalAction;
    sead::FixedSafeString<128> mDisasterAction;
    const char* mNormalCubeMap = nullptr;
    const char* mDisasterCubeMap = nullptr;
    bool mSyncFarLod = false;
};
static_assert(sizeof(DisasterAnimPart) == 0x150);
