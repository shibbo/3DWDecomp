#pragma once

#include "Library/Layout/LayoutActor.hpp"
#include <basis/seadTypes.h>

namespace al {
class LayoutInitInfo;
} // namespace al

/**
 * @brief Control guide bar shown at the bottom of the title / file select
 * screens.
 */
class RCSControlGuideBar : public al::LayoutActor {
public:
  /** @brief Text sets the guide bar can display. */
  enum GuideBarMsgType : s32 {
    GuideBarMsgType_Default =
        0, ///< Guide text shown after a save file was chosen.
    GuideBarMsgType_ListClearStar = 1, ///< Collected stars list.
    GuideBarMsgType_ListStamp = 2,     ///< Collected stamps list.
    GuideBarMsgType_MapMenu = 3,       ///< Course map menu.
    GuideBarMsgType_PauseMenu = 4,     ///< Guide text of the pause menu.
    GuideBarMsgType_PauseMenuMap =
        5, ///< Guide text of the course select map's pause menu.
    GuideBarMsgType_ControlGuide =
        6, ///< Guide text of the pause menu's control guide.
    GuideBarMsgType_FileSelect = 7, ///< Guide text of the save file select.
    GuideBarMsgType_Title = 8,      ///< Guide text of the title screen.
    GuideBarMsgType_VoiceInfoOption = 9, ///< Voice information options.
    GuideBarMsgType_NetworkBack = 11,    ///< Network screen back button.
    GuideBarMsgType_KoopaJrDemo =
        10, ///< Guide text of the Bowser Jr. assist demo.
  };

  /** @brief Creates the guide bar and its text and player icon parts. */
  explicit RCSControlGuideBar(const al::LayoutInitInfo &rInfo);

  /** @brief Shows the guide bar in its idle pose. */
  void show();
  /** @brief Shows the title screen guide immediately. */
  void showTitle();
  /** @brief Fades to a new title screen message. */
  void changeTextTitle(GuideBarMsgType type, s32 port);
  /** @brief Reports whether the title message is fading out. */
  bool isChangingText();
  /** @brief Selects the named character icon. */
  void setCharacter(const char *pCharacterName);
  /** @brief Refreshes the message when its controller port changes. */
  void overridePort(s32 port);
  /** @brief Hides the guide bar and returns to its idle state. */
  void hide();
  /** @brief Starts the title screen guide animation. */
  void appearTitle();
  /** @brief Optionally plays the title guide exit animation. */
  void endTitle(bool isAnim);
  /** @brief Plays the player icon entrance animation. */
  void appearIcon();
  /** @brief Plays the player icon exit animation. */
  void endIcon();
  /** @brief Displays the saving overlay. */
  void startSave();
  /** @brief Displays the deleting overlay. */
  void startDelete();
  /** @brief Reports whether the file overlay animation has ended. */
  bool isOverlayFinished();
  /** @brief Changes the message immediately or through a fade. */
  void changeText(GuideBarMsgType type, s32 port, bool isForce);
  /** @brief Shows the guide with the requested message and controller port. */
  void appearWithMessage(GuideBarMsgType type, s32 port);
  /** @brief Starts the guide exit state. */
  void end();
  /** @brief Selects Mario or Bowser Jr. for the controller port. */
  void setCharacterSingleMode(s32 port);
  /** @brief Selects the brigade icon and its member glyph. */
  void setCharacterKinopioBrigade(s32 port);

  /** @brief Appears and starts the entrance state. */
  void appear() override;
  /** @brief Returns the current message selection. */
  GuideBarMsgType getMsgType() const;
  /** @brief Returns the current controller port. */
  s32 getPort() const;
  /** @brief Hides the message pane. */
  void hideText();
  /** @brief Shows the message pane and starts its entrance animation. */
  void fadeTextIn();
  /** @brief Waits for the guide entrance animation. */
  void exeAppear();
  /** @brief Refreshes the message after controller reassignment. */
  void exeWait();
  /** @brief Fades out the old message and installs its replacement. */
  void exeFadeTextOut();
  /** @brief Waits for the new message to finish appearing. */
  void exeFadeTextIn();
  /** @brief Kills the guide when its exit animation ends. */
  void exeEnd();

private:
  al::LayoutActor *mParts = nullptr;
  GuideBarMsgType mMsgType = GuideBarMsgType_Default;
  s32 mPort = -1;
};
static_assert(sizeof(RCSControlGuideBar) == 0x138);
