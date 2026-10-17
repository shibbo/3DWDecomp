#pragma once

#include "Library/Layout/LayoutActor.hpp"
#include <prim/seadBitFlag.h>

namespace al { class LayoutInitInfo; }

/** @brief Prompt for skipping a demo with the controller plus or minus button. */
class DemoSkipLayout : public al::LayoutActor {
public:
    DemoSkipLayout(const al::LayoutInitInfo& rInfo, bool isSingleMode);
    bool isSkip(sead::BitFlag<u16> ports);
    void startHidden();
    void setForcedEnd(bool isForced);
    void end();
    void show();
    bool isVisible();
    bool isWait();
    void exeAppear();
    void exeWait();
    void exeHidden();
    void exeEnd();

private:
    bool mIsSingleMode;
    bool mIsForcedEnd = false;
};

static_assert(sizeof(DemoSkipLayout) == 0x128);
