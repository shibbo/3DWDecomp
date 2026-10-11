#pragma once

#include <basis/seadTypes.h>

#include "Library/Layout/LayoutActor.hpp"

namespace al {
class IUseLayout;
class LayoutInitInfo;
}  // namespace al

/**
 * @brief Player icon of the map menu, placed on the button of the world the player is in.
 * @note Minimal declaration: only what the reconstructed code uses is declared so far.
 */
class MapMenuPlayerParts : public al::LayoutActor {
public:
    MapMenuPlayerParts(const al::LayoutInitInfo& rInfo, const char* pName, const char* pPartsName,
                       al::LayoutActor* pParent);

    void startAppear(const char* pCharacterName, const al::IUseLayout* pTarget);
    void startEnd();
    void show();
    void hide();

private:
    u8 _128[0x138 - 0x128];
};

static_assert(sizeof(MapMenuPlayerParts) == 0x138);
