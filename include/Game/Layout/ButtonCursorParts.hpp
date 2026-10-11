#pragma once

#include "Library/Layout/LayoutActor.hpp"

namespace al {
class IUseLayout;
class LayoutInitInfo;
}  // namespace al

/**
 * @brief Selection cursor layout driven by a ButtonGroup.
 * @note Only the members used by already-decompiled callers are declared.
 */
class ButtonCursorParts : public al::LayoutActor {
public:
    ButtonCursorParts(const al::LayoutInitInfo& rInfo, const char* pName, const char* pPartsName,
                      al::LayoutActor* pParent);
    void set(const al::IUseLayout* pTarget);
    void setAppear(const al::IUseLayout* pTarget);
    bool isWaitAppear() const;
    void hide();
    void reset();
    bool isWait();

private:
    u8 mUnreconstructed128[0x8];
};
static_assert(sizeof(ButtonCursorParts) == 0x130);
