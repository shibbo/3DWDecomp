#pragma once

#include "Layout/Switch/SingleModeCounterBase.hpp"

namespace al {
class LayoutInitInfo;
}  // namespace al

/**
 * @brief Bowser's Fury HUD counter of the cat shines collected for the current scenario.
 * @note Only what reconstructed code needs is declared so far.
 */
class ScenarioShineCounterParts : public SingleModeCounterBase {
public:
    ScenarioShineCounterParts(const al::LayoutInitInfo& rInfo, const char* pName,
                              const char* pPartsName, al::LayoutActor* pParent);

    void control() override;
    void show() override;
    void forceHide();
    void forceEndAppear(s32 islandId);
    void updateString(s32 count);
    void endDemo();
    void handleIslandWarp();
    void updateCount(s32 count);
    void addShine();

private:
    u8 _122[0x168 - 0x122];
};

static_assert(sizeof(ScenarioShineCounterParts) == 0x168);
