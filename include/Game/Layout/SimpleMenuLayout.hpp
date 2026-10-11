#pragma once

#include <basis/seadTypes.h>
#include "Library/Layout/LayoutActor.hpp"

namespace al {
class LayoutInitInfo;
}

/**
 * @brief Generic vertical list of menu buttons (e.g. the pause menu's save data sub-menu).
 * @note Only the members used by already-decompiled callers are declared.
 */
class SimpleMenuLayout : public al::LayoutActor {
public:
    SimpleMenuLayout(const char* pName, const al::LayoutInitInfo& rInfo, u32 buttonNum,
                     const char* pSuffix);

    void appear(s32 port);
    void end();
    void reset(const char* pButtonName);
    bool isDecided();
    bool isDecideEnd();
    bool isEnding();

    /**
     * @brief Read the name of the last confirmed button.
     * @return The decided button's name.
     */
    const char* getDecidedButtonName() const { return mDecidedButtonName; }

private:
    u8 mUnreconstructed128[0x8];
    const char* mDecidedButtonName;
    u8 mUnreconstructed138[0x8];
};
static_assert(sizeof(SimpleMenuLayout) == 0x140);
