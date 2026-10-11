#pragma once

#include <math/seadVector.h>

#include "Library/Layout/LayoutActor.hpp"
#include "System/ScenarioInfo.hpp"

namespace al {
class LayoutInitInfo;
class LiveActor;
}  // namespace al

class GameDataHolder;

/**
 * @brief Island map parts showing the ocean (non-island) shines as icons.
 * @note Only what reconstructed code needs is declared so far.
 */
class MapOceanShineParts : public al::LayoutActor {
public:
    MapOceanShineParts(const al::LayoutInitInfo& rInfo, const char* pName, const char* pPaneName,
                       al::LayoutActor* pParent, const GameDataHolder* pGameData);

    void setPanePositions(sead::Vector2f scale, f32 iconScale);
    void updateScale(const sead::Vector2f& rScale);
    s32 getNearestIconIdx(f32& rMinDistance);
    al::LayoutActor* getIcon(s32 index);
    void snapIcon(s32 index);
    ScenarioInfo getScenarioInfo(s32 index);
    void addSpecialShineLocation(al::LiveActor* pActor, sead::Vector3f trans,
                                 ScenarioInfo scenarioInfo);
    void removeSpecialShineLocation(al::LiveActor* pActor);
    void setSpecialShineIconComplete(al::LiveActor* pActor, bool isComplete);
    void startIconBlink();
    void endIconBlink();
    bool isBlinkAnimDone();

private:
    u8 _121[0x178 - 0x121];
};

static_assert(sizeof(MapOceanShineParts) == 0x178);
