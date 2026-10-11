#pragma once

#include <basis/seadTypes.h>
#include "Library/Layout/LayoutActor.hpp"

namespace al {
class LayoutInitInfo;
}  // namespace al

/**
 * @brief Control guide bar shown at the bottom of the title / file select screens.
 * @note Only the members used by already-decompiled callers are declared.
 */
class RCSControlGuideBar {
public:
    /** @brief Text sets the guide bar can display. */
    enum GuideBarMsgType : s32 {
        GuideBarMsgType_Default = 0,     ///< Guide text shown after a save file was chosen.
        GuideBarMsgType_PauseMenu = 4,   ///< Guide text of the pause menu.
        GuideBarMsgType_PauseMenuMap = 5,  ///< Guide text of the course select map's pause menu.
        GuideBarMsgType_ControlGuide = 6,  ///< Guide text of the pause menu's control guide.
        GuideBarMsgType_FileSelect = 7,  ///< Guide text of the save file select.
        GuideBarMsgType_Title = 8,  ///< Guide text of the title screen.
        GuideBarMsgType_KoopaJrDemo = 10,  ///< Guide text of the Bowser Jr. assist demo.
    };

    explicit RCSControlGuideBar(const al::LayoutInitInfo& rInfo);

    void show();
    void showTitle();
    void changeTextTitle(GuideBarMsgType type, s32 port);
    bool isChangingText();
    void setCharacter(const char* pCharacterName);
    void overridePort(s32 port);
    void hide();
    void appearTitle();
    void endTitle(bool isAnim);
    void appearIcon();
    void endIcon();
    void startSave();
    void startDelete();
    bool isOverlayFinished();
    void changeText(GuideBarMsgType type, s32 port, bool isForce);
    void appearWithMessage(GuideBarMsgType type, s32 port);
    void end();
    void setCharacterSingleMode(s32 port);
    void setCharacterKinopioBrigade(s32 port);

    /**
     * @brief Checks whether the guide bar is shown (al::LayoutActor::isAlive in the game).
     * @return True while the guide bar is alive.
     */
    bool isAlive() const { return mUnreconstructed0[0x120] != 0; }

private:
    /// Layout actor base and state; the class derives from al::LayoutActor in the game.
    u8 mUnreconstructed0[0x138];
};
static_assert(sizeof(RCSControlGuideBar) == 0x138);


