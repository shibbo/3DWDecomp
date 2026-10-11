#pragma once

#include <math/seadVector.h>

#include "Library/Layout/LayoutActor.hpp"

namespace al {
class LayoutInitInfo;
}

/**
 * @brief Island map parts drawing the trail of recent player positions.
 * @note Only what reconstructed code needs is declared so far.
 */
class MapPlayerTrackerParts : public al::LayoutActor {
public:
    MapPlayerTrackerParts(const al::LayoutInitInfo& rInfo, const char* pName,
                          const char* pPaneName, al::LayoutActor* pParent);

    void setPanePositions();
    void updateScale(const sead::Vector2f& rScale);
    bool updateTimer();
    void addPlayerPos(sead::Vector2f pos);
    void getLastKnownPos(sead::Vector2f* pPos);

private:
    u8 _121[0x150 - 0x121];
};

static_assert(sizeof(MapPlayerTrackerParts) == 0x150);
