#pragma once

#include "Library/Layout/LayoutActor.hpp"
#include <container/seadRingBuffer.h>
#include <math/seadVector.h>

namespace al { class LayoutInitInfo; }

class MapPlayerTrackerParts : public al::LayoutActor {
public:
    MapPlayerTrackerParts(const al::LayoutInitInfo& rInfo, const char* pName,
                          const char* pPaneName, al::LayoutActor* pParent);
    void setPanePositions();
    void updateScale(const sead::Vector2f& rScale);
    bool updateTimer();
    void addPlayerPos(sead::Vector2f position);
    bool getLastKnownPos(sead::Vector2f* pOut);

private:
    sead::RingBuffer<sead::Vector2f> mPositions;
    sead::Vector2f mLastKnownPos;
    s32 mTimer = 0;
    bool mHasLastKnownPos = false;
};

static_assert(sizeof(MapPlayerTrackerParts) == 0x150);
