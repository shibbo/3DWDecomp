#pragma once

#include "Library/Layout/LayoutActor.hpp"
#include <prim/seadBitFlag.h>

namespace al { class LayoutInitInfo; }

class DemoSkipLayout : public al::LayoutActor {
public:
    DemoSkipLayout(const al::LayoutInitInfo& rInfo, bool isSingleMode);
    bool isSkip(sead::BitFlag<u16> ports);
    void startHidden();
    void end();

private:
    bool mIsSingleMode;
    bool _122;
};
