#pragma once
#include "Library/Layout/LayoutActor.hpp"
namespace al { class LayoutInitInfo; }
class ButtonGroup;
class ButtonTextScrollParts;
class WindowConfirm;
/** @brief Configures Ghost and Miiverse visibility from the pause menu. */
class WindowMiiSetUp : public al::LayoutActor {
public:
    WindowMiiSetUp(const al::LayoutInitInfo& rInfo, const char* pLayoutName);
    void appear() override;
    void appear(int port);
    bool isEnd() const;
    void exeAppear();
    void exeWait();
    void exeError();
    void exeEnd();
private:
    ButtonGroup* mButtons = nullptr;
    ButtonTextScrollParts* mGhost = nullptr;
    ButtonTextScrollParts* mMiiverse = nullptr;
    int mPort = -1;
    bool mUnknown144 = false;
    WindowConfirm* mError = nullptr;
};
static_assert(sizeof(WindowMiiSetUp) == 0x150);
