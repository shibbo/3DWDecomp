#pragma once

#include "Library/Nerve/NerveStateBase.hpp"

struct RenderMaterialIndirectParam;

class ChameleonStateMic : public al::ActorStateBase {
public:
    ChameleonStateMic(al::LiveActor* pActor, RenderMaterialIndirectParam* pParam,
                      const RenderMaterialIndirectParam* pMicParam);
    void appear() override;
    void exeMic();

private:
    RenderMaterialIndirectParam* mParam;
    const RenderMaterialIndirectParam* mMicParam;
};

static_assert(sizeof(ChameleonStateMic) == 0x30);
