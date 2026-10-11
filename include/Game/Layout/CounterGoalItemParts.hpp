#pragma once

#include "Library/Layout/LayoutActor.hpp"

namespace al {
class LayoutInitInfo;
}  // namespace al

class SingleModeSceneLayout;

/**
 * @brief Bowser's Fury HUD counter of the goal items (cat shines) needed to light a lighthouse.
 * @note Only what reconstructed code needs is declared so far.
 */
class CounterGoalItemParts : public al::LayoutActor {
public:
    CounterGoalItemParts(const al::LayoutInitInfo& rInfo, const char* pName,
                         const char* pPartsName, SingleModeSceneLayout* pParent);

    void control() override;
    void startDemo();
    void endDemo(bool isUpdate);

    void exeWait();
    void exeDemo();
    void exeDemoWaitEnd();

private:
    u8 _121[0x158 - 0x121];
};

static_assert(sizeof(CounterGoalItemParts) == 0x158);
