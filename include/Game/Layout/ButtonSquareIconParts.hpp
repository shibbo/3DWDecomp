#pragma once

#include "Layout/CursorTarget.hpp"

namespace al {
class LayoutActor;
class LayoutInitInfo;
}  // namespace al

/**
 * @brief Square icon button selectable by a ButtonGroup cursor.
 * @note Only the members used by already-decompiled callers are declared.
 */
class ButtonSquareIconParts : public CursorTarget {
public:
    ButtonSquareIconParts(const al::LayoutInitInfo& rInfo, const char* pName,
                          const char* pPartsName, al::LayoutActor* pParent, bool isUseIcon);

    void decide() override;
    void select() override;
    void wait() override;
    void enable() override;
    void disable() override;
    bool isDisable() const override;
    bool isValid() const override;
    void invalidate() override;
    void validate() override;
    bool isDecide() const override;
    bool isDecideEnd() const override;
    bool isTouch() const override;
    bool up() override;
    bool down() override;
    bool left() override;
    bool right() override;
};
