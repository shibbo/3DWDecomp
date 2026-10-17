#pragma once

#include "Library/Layout/LayoutActor.hpp"

namespace al { class LayoutInitInfo; }

/** @brief Player icon attached to the player's current world on the map menu. */
class MapMenuPlayerParts : public al::LayoutActor {
public:
    MapMenuPlayerParts(const al::LayoutInitInfo& rInfo, const char* pName,
                       const char* pPartsName, al::LayoutActor* pParent);
    void exeAppear();
    void exeWait();
    void exeEnd();
    void exeHide();
    void startAppear(const char* pCharacterName, const al::IUseLayout* pTarget);
    void startEnd();
    void hide();
    void show();

private:
    const al::IUseLayout* mTarget = nullptr;
    const char* mCharacterName = nullptr;
};

static_assert(sizeof(MapMenuPlayerParts) == 0x138);
