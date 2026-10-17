#pragma once

#include "Library/Layout/LayoutActor.hpp"
#include "Layout/TextBoxTextInfo.hpp"

namespace al {
class LayoutInitInfo;
}  // namespace al

class GameDataHolder;

/** @brief Coin counter parts embedded in a parent layout (coin count and miss display). */
class CounterCoinParts : public al::LayoutActor {
public:
    CounterCoinParts(const al::LayoutInitInfo& rInfo, const char* pName, const char* pPartsName,
                     al::LayoutActor* pParent);

    void control() override;
    void setCoinCount(s32 count);
    void waitMiss();
    void showMiss(const GameDataHolder* pHolder);
    void startDemo();
    void endDemo();

    void exeWait();
    void exeMiss();

private:
    s32 mCount = 0;
    al::LayoutActor* mParent;
    fix::TextBoxTextInfo mTextInfo;
    fix::TextBoxTextInfo mShadowTextInfo;
};

static_assert(sizeof(CounterCoinParts) == 0x170);
