#pragma once

#include <basis/seadTypes.h>
#include "Library/Layout/LayoutActor.hpp"

namespace al {
class LayoutInitInfo;
}

/**
 * @brief "Back" button parts of a menu layout, decided with the cancel button or by touch.
 * @note Only the members used by already-decompiled callers are declared.
 */
class ButtonBackParts : public al::LayoutActor {
public:
    ButtonBackParts(const al::LayoutInitInfo& rInfo, const char* pName, const char* pPartsName,
                    al::LayoutActor* pParent, s32 port);

    void appear() override;
    void tryAppear();
    bool isActive() const;
    void reactivate();
    void setPort(s32 port);
    void trySetEndIfActive();
    bool isDecide() const;
    bool isDecideEnd() const;

    void exeDeactive();
    void exeAppear();
    void exeWait();
    void exeTouch();
    void exeDecide();
    void exeEnd();

    /**
     * @brief Sets whether the button reacts to input.
     * @param isEnable True to accept input.
     */
    void setInputEnable(bool isEnable) { mIsInputEnable = isEnable; }

private:
    s32 mPort;
    s32 mTouchPort;
    bool mIsInputEnable;
};
static_assert(sizeof(ButtonBackParts) == 0x130);
