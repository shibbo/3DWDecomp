#pragma once

#include "Library/Nerve/NerveStateBase.hpp"

struct RenderMaterialIndirectParam;
namespace al { class HitSensor; }

class ChameleonStateGiantPlayer : public al::ActorStateBase {
public:
    ChameleonStateGiantPlayer(al::LiveActor* pActor, RenderMaterialIndirectParam* pParam,
                        const RenderMaterialIndirectParam* pNearParam,
                        const RenderMaterialIndirectParam* pFarParam,
                        float nearDistance, float farDistance);
    void exeGiantPlayer();

private:
    RenderMaterialIndirectParam* mParam;
    RenderMaterialIndirectParam* mTouchParam = nullptr;
    const RenderMaterialIndirectParam* mNearParam;
    const RenderMaterialIndirectParam* mFarParam;
    float mNearDistance;
    float mFarDistance;
    al::HitSensor* mPlayerSensor = nullptr;
};

static_assert(sizeof(ChameleonStateGiantPlayer) == 0x50);
