#include "Layout/TouchEffect.hpp"

#include "MapObj/DrcTouchAssistInfo.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"

namespace {
const char* const sCursorActions[] = {
    "Cursor0", "Cursor1", "Cursor2", "Cursor3", "Cursor4",
    "Cursor5", "Cursor6", "Cursor7", "Cursor8", "Cursor10",
};
}  // namespace

/**
 * @brief Creates a hidden touch cursor.
 * @param rInfo Layout initialization context.
 * @param pTouchInfo Position data for the touch cursor.
 */
TouchEffect::TouchEffect(const al::LayoutInitInfo& rInfo, const DrcTouchAssistInfo* pTouchInfo)
    : al::LayoutActor("タッチエフェクト"), mTouchInfo(pTouchInfo) {
    al::initLayoutActor(this, rInfo, "TouchEffect", nullptr);
    kill();
}

/** @brief Changes the character cursor if its selection has changed.
 * @param character Cursor-table index, or -1 for the neutral cursor. */
void TouchEffect::setCharacter(int character) {
    if (mCharacter == character) {
        return;
    }
    mCharacter = character;
    setIcon();
}

/** @brief Selects the stamp, character, or neutral cursor action. */
void TouchEffect::setIcon() {
    if (mIsStamp) {
        al::startAction(this, "Cursor9", nullptr);
    } else if (mCharacter != -1) {
        al::startAction(this, sCursorActions[mCharacter], nullptr);
    } else {
        al::startAction(this, "CursorNeutralB", nullptr);
    }
}

/** @brief Changes whether the stamp cursor overrides the character cursor.
 * @param isStamp Whether to display the stamp cursor. */
void TouchEffect::setStampIcon(bool isStamp) {
    if (mIsStamp == isStamp) {
        return;
    }
    mIsStamp = isStamp;
    setIcon();
}

/** @brief Removes the character-specific cursor selection. */
void TouchEffect::setInvalidChar() { setCharacter(-1); }

/** @brief Refreshes the cursor action and shows the layout. */
void TouchEffect::appear() {
    setIcon();
    al::LayoutActor::appear();
}

/** @brief Updates cursor position and opacity from the touch-assist state. */
void TouchEffect::movement() {
    sead::Vector2f pos(mTouchInfo->getLayoutPos());
    pos.y += 0.0f;
    al::setLocalTrans(this, pos);
    al::setPaneLocalAlpha(this, "TouchEffect", mAlpha * 255.0f);
}

/** @brief Selects translucent or fully opaque cursor rendering.
 * @param isTransparent Whether to use 30 percent opacity. */
void TouchEffect::setTransparent(bool isTransparent) {
    mAlpha = isTransparent ? 0.3f : 1.0f;
}
