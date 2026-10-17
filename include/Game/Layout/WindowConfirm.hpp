#pragma once

#include "Library/Layout/LayoutActor.hpp"
#include <basis/seadTypes.h>
#include <prim/seadEnum.h>

namespace al {
class LayoutInitInfo;
}

class ButtonGroup;

/** @brief Button layout of a confirmation window. */
SEAD_ENUM(WindowConfirmType, Report, OneButton, TwoButton)
constexpr auto WindowConfirmType_Report = WindowConfirmType::Report;
constexpr auto WindowConfirmType_Notice = WindowConfirmType::OneButton;
constexpr auto WindowConfirmType_Double = WindowConfirmType::TwoButton;

/**
 * @brief Pop-up confirmation window with one or two choice buttons.
 */
class WindowConfirm : public al::LayoutActor {
public:
  /** @brief Creates the requested window and its button group. */
  WindowConfirm(WindowConfirmType type, const al::LayoutInitInfo &rInfo,
                const char *pSuffix, bool isPauseMenu);

  /** @brief Shows localized message text and button labels. */
  void appearWithSystemMessage(const char *pFileName, const char *pLabel,
                               s32 port, const char *pArg);
  /** @brief Checks whether the left button was selected. */
  bool isDecideLeft() const;
  /** @brief Checks whether the right button was selected. */
  bool isDecideRight() const;
  /** @brief Checks whether the left selection animation has ended. */
  bool isDecideLeftEnd() const;
  /** @brief Checks whether the right selection animation has ended. */
  bool isDecideRightEnd() const;
  /** @brief Checks whether a normal or forced exit is active. */
  bool isEnding() const;
  /** @brief Closes an idle window regardless of its keep-open setting. */
  void forceExit();

  /**
   * @brief Controls whether the window remains open after a decision.
   * @param isLeft True to wait for the owner to close the window.
   * @note The legacy setter name is retained for existing callers.
   */
  void setDefaultSelectLeft(bool isLeft) { mIsKeepOpen = isLeft; }

  /** @brief Shows supplied text with an acknowledgement icon. */
  void appearWithCustomMessage(const char16_t *pMessage, s32 port);
  /** @brief Checks whether the window has one button. */
  bool isButtonOne() const;
  /** @brief Checks whether the window has two buttons. */
  bool isButtonTwo() const;
  /** @brief Checks whether the window has any buttons. */
  bool isExistButton() const;
  /** @brief Checks whether the single button was selected. */
  bool isDecideOne() const;
  /** @brief Checks whether the single button selection animation has ended. */
  bool isDecideOneEnd() const;
  /** @brief Enables buttons after the window entrance animation. */
  void exeAppear();
  /** @brief Shows the controller-specific acknowledgement icon. */
  void exeAppearAIcon();
  /** @brief Processes button input or waits to reveal the acknowledgement icon.
   */
  void exeWait();
  /** @brief Waits for the acknowledgement input. */
  void exeWaitAIcon();
  /** @brief Plays the exit animation and kills the window unless held open. */
  void exeEnd();

private:
  ButtonGroup *mButtons = nullptr;
  s32 mPort = -1;
  s32 mIconDelay = 0;
  bool mIsKeepOpen = false;
  bool mIsPauseMenu;
};
static_assert(sizeof(WindowConfirm) == 0x140);
