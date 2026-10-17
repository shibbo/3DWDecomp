#pragma once
#include "Library/Layout/LayoutActor.hpp"
namespace al { class LayoutInitInfo; class NetworkSystem; }
class CounterRecordParts;
class GameDataHolder;
/** @brief Displays the stage name and records on the opening curtain. */
class WipeCurtainStart : public al::LayoutActor {
public:
    explicit WipeCurtainStart(const al::LayoutInitInfo& rInfo);
    void appearStageStart(const GameDataHolder* pGameData, al::NetworkSystem* pNetwork);
    bool tryAppearGhostMiiIcon(al::NetworkSystem* pNetwork, const GameDataHolder* pGameData, int courseId);
    bool isWait() const;
    bool isEnableEnd() const;
    void end();
    void exeAppear();
    void exeWait();
    void exeEnd();
private:
    CounterRecordParts* mBestScore = nullptr;
    CounterRecordParts* mBestTime = nullptr;
};
static_assert(sizeof(WipeCurtainStart) == 0x138);
