#pragma once

#include "Library/Layout/LayoutActor.hpp"

namespace al {
class LayoutInitInfo;
}
class ButtonGroup;

/** @brief Save-result window with an acknowledgement button. */
class WindowSave : public al::LayoutActor {
public:
    explicit WindowSave(const al::LayoutInitInfo& rInfo);
    void appearWindow(int padPort);
    void exeAppear();
    void exeWait();
    void exeEnd();

private:
    ButtonGroup* mButtonGroup = nullptr;
    s32 mPadPort = -1;
};
static_assert(sizeof(WindowSave) == 0x138);
