#pragma once

#include "Library/Layout/LayoutActor.hpp"

namespace al {
class LayoutInitInfo;
}  // namespace al

class CounterCoinParts;
class GameDataHolder;

/** @brief Coin counter shown after a miss that counts down the coins lost. */
class CounterCoinInfo : public al::LayoutActor {
public:
    CounterCoinInfo(const al::LayoutInitInfo& rInfo, const GameDataHolder* pHolder);

    void appear() override;

    void exeAppear();
    void exeShowMiss();
    void exeWait();
    void exeIntro();
    void exeOutro();
    void exeEnd();

private:
    CounterCoinParts* mCoinParts = nullptr;
    const GameDataHolder* mGameDataHolder;
    s32 mCoinNum;
};

static_assert(sizeof(CounterCoinInfo) == 0x140);
