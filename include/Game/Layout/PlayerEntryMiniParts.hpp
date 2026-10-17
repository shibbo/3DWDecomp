#pragma once

#include "Library/Layout/LayoutActor.hpp"

namespace al { class LayoutInitInfo; }

/** @brief Compact player-entry indicator with controller and character icons. */
class PlayerEntryMiniParts : public al::LayoutActor {
public:
    PlayerEntryMiniParts(const al::LayoutInitInfo& rInfo, const char* pName,
                         const char* pPartsName, al::LayoutActor* pParent, int port);
    void exeWait();
    void setupController(int characterId);
    void setStageKinopioBrigade();
    void appearWithAnimName(int characterId, const char* pAnimName);

private:
    int mPort;
    bool mIsKinopioBrigade = false;
};

static_assert(sizeof(PlayerEntryMiniParts) == 0x130);
