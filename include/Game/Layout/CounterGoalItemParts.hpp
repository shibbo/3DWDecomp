#pragma once

#include "Library/Layout/LayoutActor.hpp"
#include "Layout/TextBoxTextInfo.hpp"

namespace al {
class LayoutInitInfo;
}  // namespace al

class SingleModeSceneLayout;

/**
 * @brief Bowser's Fury HUD counter of the goal items (cat shines) needed to light a lighthouse.
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
    int mCount = 0;
    SingleModeSceneLayout* mParent;
    bool mHasPendingAdd = false;
    fix::TextBoxTextInfo mTextInfo;
};

static_assert(sizeof(CounterGoalItemParts) == 0x158);
