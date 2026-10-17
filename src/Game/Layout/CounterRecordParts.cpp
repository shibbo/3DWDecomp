#include "Layout/CounterRecordParts.hpp"

#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"

/**
 * @brief Creates a record display within its parent layout.
 * @param rInfo Layout initialization context.
 * @param pName Actor name.
 * @param pPartsName Name of the layout parts pane.
 * @param pParent Parent layout actor.
 */
CounterRecordParts::CounterRecordParts(const al::LayoutInitInfo& rInfo, const char* pName,
                                      const char* pPartsName, al::LayoutActor* pParent)
    : al::LayoutActor(pName) {
    al::initLayoutPartsActor(this, pParent, rInfo, pPartsName, nullptr);
}

/** @brief Leaves the record display unchanged while waiting. */
void CounterRecordParts::exeWait() {}

/** @brief Plays the main pane's show action. */
void CounterRecordParts::show() {
    al::startAction(this, "Show", "Main");
}

/** @brief Plays the main pane's hide action. */
void CounterRecordParts::hide() {
    al::startAction(this, "Hide", "Main");
}

/**
 * @brief Checks whether the main pane is playing its hide action.
 * @return Whether the hide action is selected.
 */
bool CounterRecordParts::isHide() const {
    return al::isActionPlaying(this, "Hide", "Main");
}

/**
 * @brief Displays a score when it fits within six decimal digits.
 * @param score Score in the range 0 to 999999; other values leave the text unchanged.
 */
void CounterRecordParts::setPaneStringScore(s32 score) {
    if (static_cast<u32>(score) <= 999999) {
        al::setPaneString(this, "TxtRecordNum",
                          sead::WFormatFixedSafeString<7>(u"%06d", score).cstr(), 0, -1);
    }
}

/**
 * @brief Displays a time when it fits within three decimal digits.
 * @param time Time in the range 0 to 999; other values leave the text unchanged.
 */
void CounterRecordParts::setPaneStringTime(s32 time) {
    if (static_cast<u32>(time) <= 999) {
        al::setPaneString(this, "TxtRecordNum",
                          sead::WFormatFixedSafeString<4>(u"%03d", time).cstr(), 0, -1);
    }
}
