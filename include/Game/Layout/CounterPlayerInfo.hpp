#pragma once
#include "Library/Layout/LayoutActor.hpp"
namespace al { class LayoutInitInfo; }
class CounterPlayerParts;
class GameDataHolder;
class GameDataHolderAccessor;
/** @brief Displays the remaining-life change for a single active player. */
class CounterPlayerInfo : public al::LayoutActor {
public:
    CounterPlayerInfo(const al::LayoutInitInfo& rInfo, const GameDataHolder* pGameData);
    void appear() override;
    void exeAppearWait();
    void exeAppear();
    void exeWait();
    void exeShowMiss();
    void exeEnd();
private:
    CounterPlayerParts* mCounter = nullptr;
    GameDataHolderAccessor* mGameData = nullptr;
    bool mIsKinopioBrigade = false;
};
static_assert(sizeof(CounterPlayerInfo) == 0x140);
