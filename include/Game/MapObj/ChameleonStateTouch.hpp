#pragma once

#include "Library/Nerve/NerveStateBase.hpp"

struct RenderMaterialIndirectParam;

class ChameleonStateTouch : public al::ActorStateBase {
public:
    ChameleonStateTouch(al::LiveActor* pActor, RenderMaterialIndirectParam* pParam,
                        const RenderMaterialIndirectParam* pNearParam,
                        const RenderMaterialIndirectParam* pFarParam,
                        float nearDistance, float farDistance);
    void exeDRCTouch();

private:
    RenderMaterialIndirectParam* mParam;
    RenderMaterialIndirectParam* mTouchParam = nullptr;
    bool mIsVisible = false;
    const RenderMaterialIndirectParam* mNearParam;
    const RenderMaterialIndirectParam* mFarParam;
    float mNearDistance;
    float mFarDistance;
};

static_assert(sizeof(ChameleonStateTouch) == 0x50);
