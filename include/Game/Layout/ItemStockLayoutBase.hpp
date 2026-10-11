#pragma once

namespace al {
class LayoutActor;
}  // namespace al

/**
 * @brief Common part of the layouts that show the stocked items (the Bowser's Fury item tray and
 * the item stock button). The blur effects of stocked items fly towards these layouts.
 * @note Only what reconstructed code needs is declared so far.
 */
class ItemStockLayoutBase {
public:
    /**
     * @brief Constructs the base with the layout that owns the tray.
     * @param pParentLayout Parent layout actor (used for sounds and effects).
     */
    ItemStockLayoutBase(al::LayoutActor* pParentLayout) : mParentLayout(pParentLayout) {}

    /**
     * @brief Get the parent layout of the tray.
     * @return The parent layout actor.
     */
    al::LayoutActor* getParentLayout() const { return mParentLayout; }

protected:
    al::LayoutActor* mParentLayout;  // 0x0
};
