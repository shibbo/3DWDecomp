#pragma once

#include <basis/seadTypes.h>
#include <container/seadPtrArray.h>
#include "Layout/ButtonCursorParts.hpp"
#include "Library/Layout/LayoutActor.hpp"

namespace al {
class ByamlIter;
class IUseAudioKeeper;
class LayoutActor;
class LayoutInitInfo;
}  // namespace al
class CursorTarget;

/**
 * @brief Layout button collection and cursor controller.
 * @note Buttons, the cursor and their directional links are read from the layout archive's
 *       "InitButtonGroup" BYAML.
 */
class alignas(8) ButtonGroup {
public:
    /** @brief Cursor movement direction, as passed to tryMove(). */
    enum Direction : s32 {
        Direction_Up = 0,
        Direction_Down = 1,
        Direction_Left = 2,
        Direction_Right = 3,
    };

    /** @brief Neighbouring buttons reached from one button with the directional input. */
    struct DestinationInfo {
        /**
         * @brief Creates the links of a button.
         * @param pButton Button the links start from.
         * @param pUp Button reached with up, or nullptr.
         * @param pDown Button reached with down, or nullptr.
         * @param pLeft Button reached with left, or nullptr.
         * @param pRight Button reached with right, or nullptr.
         */
        DestinationInfo(CursorTarget* pButton, CursorTarget* pUp, CursorTarget* pDown,
                        CursorTarget* pLeft, CursorTarget* pRight)
            : mButton(pButton), mUp(pUp), mDown(pDown), mLeft(pLeft), mRight(pRight) {}

        /**
         * @brief Replaces the links of a button.
         * @param pButton Button the links start from.
         * @param pUp Button reached with up, or nullptr.
         * @param pDown Button reached with down, or nullptr.
         * @param pLeft Button reached with left, or nullptr.
         * @param pRight Button reached with right, or nullptr.
         */
        void set(CursorTarget* pButton, CursorTarget* pUp, CursorTarget* pDown,
                 CursorTarget* pLeft, CursorTarget* pRight) {
            mButton = pButton;
            mUp = pUp;
            mDown = pDown;
            mLeft = pLeft;
            mRight = pRight;
        }

        CursorTarget* mButton;
        CursorTarget* mUp;
        CursorTarget* mDown;
        CursorTarget* mLeft;
        CursorTarget* mRight;
    };

    ButtonGroup(const al::LayoutInitInfo& rInfo, al::LayoutActor* pParent,
                const char* pLayoutName, const char* pCursorName, bool isUseIcon);
    void registerButtonLocal(CursorTarget* pButton);
    void setCursorDestination(al::ByamlIter* pIter, bool isReset);
    void update();
    CursorTarget* findButtonSelectableFromCurrent() const;
    CursorTarget* getButtonTouched() const;
    void select(CursorTarget* pButton);
    void updateAndCursorDefault(s32 port);
    void setPort(s32 port);
    bool isDecideAny() const;
    void registerButton(al::LayoutActor* pParent, CursorTarget* pButton);
    void resetDestination(const char* pButtonName, const char* pUpName, const char* pDownName,
                          const char* pLeftName, const char* pRightName, bool isReset);
    void setDestination(const char* pButtonName, const char* pUpName, const char* pDownName,
                        const char* pLeftName, const char* pRightName);
    CursorTarget* getButton(const char* pButtonName) const;
    void setDestination(CursorTarget* pButton, CursorTarget* pUp, CursorTarget* pDown,
                        CursorTarget* pLeft, CursorTarget* pRight);
    CursorTarget* findMoveTargetButton(CursorTarget* pButton, s32 direction);
    void reset();
    void select(const char* pButtonName);
    void select(s32 index);
    void showCursor();
    void showCursorAppear();
    void hideCursor();
    void invalidate();
    void validate();
    void reloadCursorDestination(const char* pLayoutName, const char* pCursorName);
    void decide(const char* pButtonName);
    bool isSelect(const char* pButtonName) const;
    bool isSelect(CursorTarget* pButton) const;
    bool isDecide(const char* pButtonName) const;
    bool isDecideEnd(const char* pButtonName) const;
    bool isDecideEndAny() const;
    const char* getDecideButton() const;
    const char* getDecideEndButton() const;
    s32 getDecideEndButtonIndex() const;
    bool isWaitCursorAppear() const;
    const char* getSelectedButtonName() const;
    bool tryMove(s32 direction);
    DestinationInfo* findDestinationInfo(CursorTarget* pButton) const;

    /** @brief Advances the button cursor layout. */
    void updateCursor() { mCursor->movement(); }

    /**
     * @brief Access the cursor layout.
     * @return The cursor layout actor.
     */
    al::LayoutActor* getCursor() const { return mCursor; }

    /**
     * @brief Access the cursor layout with its concrete type.
     * @return The button cursor parts.
     */
    ButtonCursorParts* getCursorParts() const { return mCursor; }

    /**
     * @brief Read the number of registered buttons.
     * @return The button count.
     */
    s32 getButtonNum() const { return mButtons.size(); }

    /**
     * @brief Access a registered button with a bounds check.
     * @param index Registration index of the button.
     * @return The button, or nullptr when the index is out of range.
     */
    CursorTarget* getButton(s32 index) const { return mButtons.at(index); }

    /**
     * @brief Access a registered button without a bounds check.
     * @param index Registration index of the button.
     * @return The button.
     */
    CursorTarget* getButtonUnsafe(s32 index) const { return mButtons.unsafeAt(index); }

    /**
     * @brief Access the button the cursor is currently on.
     * @return The selected button.
     */
    CursorTarget* getSelectedButton() const { return mSelectedButton; }

    /**
     * @brief Check whether a button is being held by touch, which locks pad input.
     * @return True while a touch on a button is in progress.
     */
    bool isInputLocked() const { return mIsTouching; }

private:
    ButtonCursorParts* mCursor = nullptr;
    sead::FixedPtrArray<CursorTarget, 32> mButtons;
    sead::FixedPtrArray<DestinationInfo, 32> mDestinationInfos;
    CursorTarget* mSelectedButton = nullptr;
    bool mIsTouching = false;
    CursorTarget* mTouchedButton = nullptr;
    al::IUseAudioKeeper* mAudioKeeper;
    bool mIsTouchHold = false;
};
static_assert(sizeof(ButtonGroup) == 0x250);
