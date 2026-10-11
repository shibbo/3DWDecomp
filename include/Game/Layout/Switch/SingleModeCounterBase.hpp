#pragma once

#include "Library/Layout/LayoutActor.hpp"

namespace al {
class LayoutInitInfo;
}  // namespace al

/** @brief Base class of the Bowser's Fury HUD counters (appear / wait / hide states). */
class SingleModeCounterBase : public al::LayoutActor {
public:
    SingleModeCounterBase(const al::LayoutInitInfo& rInfo, const char* pName,
                          const char* pPartsName, al::LayoutActor* pParent);

    virtual void show();
    virtual void hide();
    bool isHide();

    void exeAppear();
    void exeWait();
    void exeHide();

private:
    bool mIsHide;
};

static_assert(sizeof(SingleModeCounterBase) == 0x128);
