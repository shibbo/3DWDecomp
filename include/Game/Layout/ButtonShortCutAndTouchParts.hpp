#pragma once

#include <basis/seadTypes.h>
#include "Library/Layout/LayoutActor.hpp"

namespace al {
class LayoutInitInfo;
}

/**
 * @brief Button parts decided by a shortcut button (+ / -) or by touch.
 * @note Only the members used by already-decompiled callers are declared.
 */
class ButtonShortCutAndTouchParts : public al::LayoutActor {
public:
    /**
     * @brief Pad button that decides the parts.
     * @note Passed as a 64-bit value by the callers (`mov x6, xzr`).
     */
    enum ShortCutType : s64 {
        ShortCutType_Plus = 0,   ///< + button.
        ShortCutType_Minus = 1,  ///< - button.
    };

    ButtonShortCutAndTouchParts(const al::LayoutInitInfo& rInfo, const char* pName,
                                const char* pPartsName, al::LayoutActor* pParent, s32 port,
                                ShortCutType type);

    void appear() override;
    void setPort(s32 port);
    void hide();
    void end();
    void wait();
    void tryDecide();
    bool isTouch() const;
    bool isDecide() const;
    bool isDecideEnd() const;
    bool isWait() const;
    bool isHide() const;
    bool isActive() const;

    void exeDeactive();
    void exeAppear();
    void exeWait();
    void exeTouch();
    void exeDecide();
    void exeHide();
    void exeEnd();

    /**
     * @brief Sets whether the button reacts to input.
     * @param isEnable True to accept input.
     */
    void setInputEnable(bool isEnable) { mIsInputEnable = isEnable; }

private:
    s32 mPort;
    s32 mTouchPort;
    s32 mShortCutType;
    bool mIsInputEnable;
    u8 mUnreconstructed131[0xf];
};
static_assert(sizeof(ButtonShortCutAndTouchParts) == 0x140);
