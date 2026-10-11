#pragma once

#include <basis/seadTypes.h>
#include "Library/Layout/LayoutActor.hpp"

namespace al {
class LayoutInitInfo;
}

/**
 * @brief Button layout of a confirmation window.
 * @note Passed as a 64-bit value by the callers (`mov x1, xzr`).
 */
enum WindowConfirmType : s64 {
    WindowConfirmType_Report = 0,  ///< Single acknowledgement button.
    WindowConfirmType_Notice = 1,  ///< Notice that closes on its own (e.g. auto power down).
    WindowConfirmType_Double = 2,  ///< Two choice (left/right) buttons.
};

/**
 * @brief Pop-up confirmation window with one or two choice buttons.
 * @note Only the members used by already-decompiled callers are declared.
 */
class WindowConfirm : public al::LayoutActor {
public:
    WindowConfirm(WindowConfirmType type, const al::LayoutInitInfo& rInfo, const char* pSuffix,
                  bool isPauseMenu);

    void appearWithSystemMessage(const char* pFileName, const char* pLabel, s32 port,
                                 const char* pArg);
    bool isDecideLeft() const;
    bool isDecideRight() const;
    bool isDecideLeftEnd() const;
    bool isDecideRightEnd() const;
    bool isEnding() const;
    void forceExit();

    /**
     * @brief Selects which button the cursor starts on when the window appears.
     * @param isLeft True to start on the left button.
     */
    void setDefaultSelectLeft(bool isLeft) { mIsDefaultSelectLeft = isLeft; }

private:
    u8 mUnreconstructed121[0x17];
    bool mIsDefaultSelectLeft;
    u8 mUnreconstructed139[0x7];
};
static_assert(sizeof(WindowConfirm) == 0x140);
