#pragma once

#include <basis/seadTypes.h>
#include "Library/Layout/LayoutActor.hpp"

class ButtonGroup;
namespace al {
class LayoutInitInfo;
}

/**
 * @brief Generic vertical list of menu buttons (e.g. the pause menu's save data sub-menu).
 */
class SimpleMenuLayout : public al::LayoutActor {
public:
    SimpleMenuLayout(const char* pName, const al::LayoutInitInfo& rInfo, u32 buttonNum,
                     const char* pSuffix);

    void appear(s32 port);
    void exeAppear();
    void exeWait();
    void exeDecided();
    void exeEnd();
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
    ButtonGroup* mButtons = nullptr;
    const char* mDecidedButtonName = nullptr;
    int mPort = -1;
    bool mHasAppeared = false;
};
static_assert(sizeof(SimpleMenuLayout) == 0x140);
