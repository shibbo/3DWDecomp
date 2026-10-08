#pragma once

#include "Library/Layout/LayoutActor.hpp"
#include "System/ScenarioInfo.hpp"

namespace al {
class LayoutInitInfo;
}

class GameDataHolder;

/**
 * @brief Layout parts showing an island's name and the shines of its scenarios.
 */
class IslandCounterParts : public al::LayoutActor {
public:
    /** Number of scenario parts in the layout. */
    static constexpr s32 cScenarioPartsNum = 8;

    IslandCounterParts(const al::LayoutInitInfo& rInfo, const char* pName, const char* pPaneName,
                       al::LayoutActor* pParent, const char* pArchiveName);
    ~IslandCounterParts();

    void setIslandName(const char16_t* pName);
    void updateShineCount(s32 islandId);
    void updateShineCountOcean(const GameDataHolder* pGameData, ScenarioInfo scenarioInfo);

private:
    u8 _121[0x128 - 0x121];
    al::LayoutActor** mScenarioParts;  // 0x128
};

static_assert(sizeof(IslandCounterParts) == 0x130);
