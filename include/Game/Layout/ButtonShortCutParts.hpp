#pragma once

#include "Library/Layout/LayoutActor.hpp"

namespace al { class LayoutInitInfo; }

/** @brief Menu shortcut activated by the plus or minus button. */
class ButtonShortCutParts : public al::LayoutActor {
public:
    /** @brief Shortcut button type, passed as a 64-bit value by the original ABI. */
    enum ShortCutType : s64 { Plus, Minus };

    ButtonShortCutParts(const al::LayoutInitInfo& rInfo, const char* pName,
                        const char* pLayoutName, al::LayoutActor* pParent,
                        int port, ShortCutType type);
    void appear() override;
    void end();
    void setHide();
    bool isDecide() const;
    bool isWait() const;
    bool isEndAndActionEnd() const;
    bool isHide() const;
    void exeAppear();
    void exeWait();
    void exeDecide();
    void exeHide();
    void exeEnd();

private:
    s32 mType;
    int mPort;
    bool mIsInputEnable = true;
};

static_assert(sizeof(ButtonShortCutParts) == 0x130);
