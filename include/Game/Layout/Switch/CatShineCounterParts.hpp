#pragma once

#include "Layout/Switch/SingleModeCounterBase.hpp"
#include "Layout/TextBoxTextInfo.hpp"

namespace al {
class LayoutInitInfo;
}  // namespace al

/** @brief HUD counter showing the cat shines collected on the current island. */
class CatShineCounterParts : public SingleModeCounterBase {
public:
    CatShineCounterParts(const al::LayoutInitInfo& rInfo, const char* pName,
                         const char* pPartsName, al::LayoutActor* pParent);

    void control() override;
    void updateCount(s32 islandId, s32 scenarioId, bool isHideScenario);

private:
    fix::TextBoxTextInfo mShineTextInfo;
    fix::TextBoxTextInfo mShineOceanTextInfo;
};

static_assert(sizeof(CatShineCounterParts) == 0x168);
