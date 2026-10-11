#pragma once

#include "Layout/CursorTarget.hpp"

namespace al {
class LayoutInitInfo;
}  // namespace al

/** @brief Touch/cursor selectable button used by the RCS file (save file) select screens. */
class ButtonRCSFileParts : public CursorTarget {
public:
    ButtonRCSFileParts(const al::LayoutInitInfo& rInfo, const char* pName, const char* pPartsName,
                       al::LayoutActor* pParent);

    void decide() override;
    void select() override;
    void wait() override;
    void enable() override;
    void disable() override;
    bool isDisable() const override;
    void setNoSoundSelect();
    bool isDecide() const override;
    bool isDecideEnd() const override;
    bool isTouch() const override;

    void exeWait();
    void exeSelect();
    void exeTouch();
    void exeOutsideHold();
    void exeDecide();
    void exeDisable();
    void exeHide();

    /** @brief Overrides the sound played on decide.
     * @param pName Sound name, or nullptr to use the default "Decide" sound. */
    void setCustomSE(const char* pName) override { mCustomSE = pName; }

    /** @brief Returns whether the button reacts to touch input.
     * @return True if touch input is accepted. */
    bool isValid() const override { return mIsValid; }

    /** @brief Stops the button from reacting to touch input. */
    void invalidate() override { mIsValid = false; }

    /** @brief Lets the button react to touch input. */
    void validate() override { mIsValid = true; }

    /** @brief Cursor navigation is not handled by this button.
     * @return Always false. */
    bool up() override { return false; }

    /** @brief Cursor navigation is not handled by this button.
     * @return Always false. */
    bool down() override { return false; }

    /** @brief Cursor navigation is not handled by this button.
     * @return Always false. */
    bool left() override { return false; }

    /** @brief Cursor navigation is not handled by this button.
     * @return Always false. */
    bool right() override { return false; }

private:
    bool isTouchInHitPane() const;

    bool mIsDisabled = false;
    bool mIsValid = true;
    bool mIsPlaySelectSe = true;
    const char* mCustomSE = nullptr;
};
