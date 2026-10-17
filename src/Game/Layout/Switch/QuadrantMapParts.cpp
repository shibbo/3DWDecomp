#include "Layout/Switch/QuadrantMapParts.hpp"
#include <nn/ui2d/ui2d_Layout.h>
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Layout/LayoutKeeper.hpp"
namespace {
const char* const sQuadrantNames[] = {"S_Quadrant", "E_Quadrant", "N_Quadrant", "W_Quadrant"};
/**
 * @brief Creates the parts actor keeper of a layout actor with room for all of its parts panes.
 * @param pActor Layout actor whose layout was just initialized.
 */
inline void initPartsActorKeeper(al::LayoutActor* pActor) {
    const nn::util::IntrusiveListNode& partsList =
        pActor->getLayoutKeeper()->getLayout()->_48;

    if (partsList.GetNext() == &partsList) {
        return;
    }

    s32 partsNum = 0;

    for (const nn::util::IntrusiveListNode* node = partsList.GetNext(); node != &partsList;
         node = node->GetNext()) {
        partsNum++;
    }

    pActor->initLayoutPartsActorKeeper(partsNum);
}

}
/** @brief Initializes the map parts and reserves its child-parts keeper.
 * @param rInfo Layout initialization context.
 * @param pGameData Game data used by the map.
 * @param pName Actor name.
 * @param pPartsName Parts pane name.
 * @param pParent Parent layout.
 * @param pArchiveName Parts archive. */
QuadrantMapParts::QuadrantMapParts(const al::LayoutInitInfo& rInfo, const GameDataHolder* pGameData,
                                 const char* pName, const char* pPartsName,
                                 al::LayoutActor* pParent, const char* pArchiveName)
    : al::LayoutActor(pName), mGameData(pGameData) {
    al::initLayoutPartsActor(this, pParent, rInfo, pPartsName, pArchiveName);
    initPartsActorKeeper(this);
}
/** @brief Hides the currently selected quadrant. */
void QuadrantMapParts::hideActive() {
    if (mActiveQuadrant >= 0)
        al::startAction(this, "Hide", sQuadrantNames[mActiveQuadrant]);
    mActiveQuadrant = -1;
}
/** @brief Immediately hides all compass quadrants. */
void QuadrantMapParts::forceHideAll() {
    al::startFreezeActionEnd(this, "Hide", "S_Quadrant");
    al::startFreezeActionEnd(this, "Hide", "E_Quadrant");
    al::startFreezeActionEnd(this, "Hide", "N_Quadrant");
    al::startFreezeActionEnd(this, "Hide", "W_Quadrant");
    mActiveQuadrant = -1;
}
/** @brief Shows the selected quadrant and hides the previous selection.
 * @param quadrant Compass quadrant index. */
void QuadrantMapParts::showQuadrant(int quadrant) {
    if (mActiveQuadrant == quadrant)
        return;
    al::startAction(this, "Show", sQuadrantNames[quadrant]);
    if (mActiveQuadrant >= 0)
        al::startAction(this, "Hide", sQuadrantNames[mActiveQuadrant]);
    mActiveQuadrant = quadrant;
}
/** @brief Sets visibility of the root pane.
 * @param isActive Whether the root pane is visible. */
void QuadrantMapParts::setActive(bool isActive) {
    if (isActive)
        al::showPaneRootNoRecursive(this);
    else
        al::hidePaneRootNoRecursive(this);
}
