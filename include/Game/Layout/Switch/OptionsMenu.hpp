#pragma once

#include <basis/seadTypes.h>
#include "Library/Layout/LayoutActor.hpp"

namespace al {
class CameraDirector;
class CameraDirector_RS;
class LayoutInitInfo;
}  // namespace al
class ButtonGroup;
class ButtonTextScrollParts;

/**
 * @brief Options sub-menu of the pause menu (camera inversion, camera sensitivity and the
 *        Bowser's Fury assist mode).
 * @note Built with a CameraDirector for Super Mario 3D World and with a CameraDirector_RS for
 *       Bowser's Fury, which additionally shows the sensitivity and assist mode buttons.
 */
class OptionsMenu : public al::LayoutActor {
public:
    OptionsMenu(const al::LayoutInitInfo& rInfo, al::CameraDirector* pCameraDirector,
                bool isKinopioBrigade);
    OptionsMenu(const al::LayoutInitInfo& rInfo, al::CameraDirector_RS* pCameraDirector);

    void exeAppear();
    void exeAppearKoopaJrDemo();
    void exeWait();
    void exeWaitKoopaJrDemo();
    void exeEnd();
    void exeFullEndKoopaJrDemo();
    bool isKoopaJrDemoEnd();
    void appear(s32 port);
    void appearKoopaJrDemo(s32 port);
    void forceExit();
    bool isEnding();

    /**
     * @brief Access the Bowser's Fury camera director.
     * @return The camera director passed at construction.
     */
    al::CameraDirector_RS* getCameraDirectorRS() const { return mCameraDirectorRS; }

private:
    ButtonGroup* mButtonGroup = nullptr;
    ButtonTextScrollParts* mSensitivityButton = nullptr;
    ButtonTextScrollParts* mHorizontalButton = nullptr;
    ButtonTextScrollParts* mVerticalButton = nullptr;
    ButtonTextScrollParts* mAssistModeButton = nullptr;
    s32 mPort = -1;
    s32 mMainControllerPort;
    al::CameraDirector* mCameraDirector;
    al::CameraDirector_RS* mCameraDirectorRS;
    bool mIsKinopioBrigade;
    s32 mSideInputDelay = 10;
};
static_assert(sizeof(OptionsMenu) == 0x170);
