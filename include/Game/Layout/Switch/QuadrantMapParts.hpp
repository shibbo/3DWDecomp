#pragma once
#include "Library/Layout/LayoutActor.hpp"
namespace al { class LayoutInitInfo; }
class GameDataHolder;
/** @brief Map parts that highlight the active compass quadrant. */
class QuadrantMapParts : public al::LayoutActor {
public:
    QuadrantMapParts(const al::LayoutInitInfo& rInfo, const GameDataHolder* pGameData,
                     const char* pName, const char* pPartsName, al::LayoutActor* pParent,
                     const char* pArchiveName);
    void hideActive();
    void forceHideAll();
    void showQuadrant(int quadrant);
    void setActive(bool isActive);
private:
    const GameDataHolder* mGameData;
    int mActiveQuadrant = -1;
    int mUnusedValue = 1;
};
static_assert(sizeof(QuadrantMapParts) == 0x138);
