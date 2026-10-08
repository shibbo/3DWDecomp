#pragma once

#include <basis/seadTypes.h>
#include "Layout/CursorTarget.hpp"

namespace al {
class LayoutInitInfo;
}  // namespace al

/**
 * @brief Cursor-selectable option button that scrolls left/right through a list of message labels.
 */
class ButtonTextScrollParts : public CursorTarget {
public:
    ButtonTextScrollParts(const al::LayoutInitInfo& rInfo, const char* pName,
                          const char* pPartsName, al::LayoutActor* pParent,
                          const char* pMessageArchive, const char** pLabels, s32 labelNum);

    void control() override;

    void exeWait();
    void exeSelect();
    void exeTouchCenter();
    void exeTouchRight();
    void exeTouchLeft();
    void exeOutsideHold();
    void exeTextScrollOut();
    void exeTextScrollIn();

    void select() override;
    bool isEnableControl() const;
    void decide() override;
    void wait() override;
    void enable() override;
    void disable() override;
    bool isDisable() const override;

    void exeDecide();
    void exeDisable();
    void exeHide();

    bool isDecide() const override;
    bool isDecideEnd() const override;
    bool isTouch() const override;
    bool left() override;
    bool right() override;
    void setLabelIdx(s32 index);

    /**
     * @brief Returns whether the button reacts to touch input while waiting.
     * @return True if touch input is accepted.
     */
    bool isValid() const override { return mIsValid; }

    /** @brief Stops the button from reacting to touch input while waiting. */
    void invalidate() override { mIsValid = false; }

    /** @brief Lets the button react to touch input while waiting. */
    void validate() override { mIsValid = true; }

    /**
     * @brief Vertical cursor navigation is not handled by this button.
     * @return Always false.
     */
    bool up() override { return false; }

    /**
     * @brief Vertical cursor navigation is not handled by this button.
     * @return Always false.
     */
    bool down() override { return false; }

    /**
     * @brief Read the index of the currently shown label.
     * @return The label index.
     */
    s32 getLabelIdx() const { return mLabelIdx; }

private:
    bool mIsValid = true;
    bool mIsScrollRight = true;
    al::LayoutActor* mMessageLayout;
    const char* mMessageArchive;
    const char** mLabels;
    s32 mLabelNum;
    s32 mLabelIdx = -1;
    bool mIsSelectStarted = false;
};
static_assert(sizeof(ButtonTextScrollParts) == 0x158);
