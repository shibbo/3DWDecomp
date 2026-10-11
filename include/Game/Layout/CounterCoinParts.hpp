#pragma once

#include "Library/Layout/LayoutActor.hpp"

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
    // TODO: layout not decompiled yet.
    u8 _128[0x170 - 0x128];
};

static_assert(sizeof(CounterCoinParts) == 0x170);
