#pragma once

#include "Library/Nerve/NerveStateBase.hpp"

struct RenderMaterialIndirectParam;

class ChameleonStateHipDrop : public al::ActorStateBase {
public:
    ChameleonStateHipDrop(al::LiveActor*, RenderMaterialIndirectParam*,
                          const RenderMaterialIndirectParam*, const RenderMaterialIndirectParam*);
    void appear() override;
    void exeWait();
    void exeAppear();
    void exeAppearWait();
    void exeEnd();
    void resetAppearDelay() { mAppearDelay = 0; }

private:
    RenderMaterialIndirectParam* mParam;
    const RenderMaterialIndirectParam* mAppearParam;
    const RenderMaterialIndirectParam* mEndParam;
    int mAppearDelay = 30;
};

static_assert(sizeof(ChameleonStateHipDrop) == 0x40);
