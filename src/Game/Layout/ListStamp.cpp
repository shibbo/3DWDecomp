#include "Layout/ListStamp.hpp"

#include "Layout/ListStampParts.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"

namespace {
NERVE_DECL(ListStamp, End);
NERVE_DECL(ListStamp, Wait);
NERVE_DECL(ListStamp, Appear);
NERVES_MAKE_NOSTRUCT(ListStamp, End, Wait, Appear)
}

/** @brief Creates the stamp page and its collection parts.
 * @param rInfo Layout initialization context.
 * @param pGameDataHolder Save data containing collected stamps. */
ListStamp::ListStamp(const al::LayoutInitInfo& rInfo, const GameDataHolder* pGameDataHolder)
    : al::LayoutActor("はんこリスト"), mGameDataHolder(pGameDataHolder) {
    al::initLayoutActor(this, rInfo, "ListStamp", nullptr);
    mStampParts = new ListStampParts(rInfo, "はんこリストパーツ", "ParListStamp", this,
                                    pGameDataHolder);
    initNerve(&NrvListStampEnd, 0);
}

/** @brief Waits for the appearance or page-transition animation to finish. */
void ListStamp::exeAppear() {
    if (al::isActionEnd(this, nullptr))
        al::setNerve(this, &NrvListStampWait);
}
/** @brief Starts the idle animation on entry. */
void ListStamp::exeWait() {
    if (al::isFirstStep(this))
        al::startAction(this, "Wait", nullptr);
}
/** @brief Holds the end state. */
void ListStamp::exeEnd() {}

/** @brief Shows the stamp page with the requested transition.
 * @param port Controller port.
 * @param isTransition Whether to slide between pages.
 * @param isLeft Whether to use the left-side transition. */
void ListStamp::startAppear(s32 port, bool isTransition, bool isLeft) {
    mPort = port;
    al::LayoutActor::appear();
    mStampParts->appear();
    al::startAction(this, isTransition ? (isLeft ? "InLeft" : "InRight") : "Appear", nullptr);
    al::setNerve(this, &NrvListStampAppear);
}
/** @brief Starts leaving the stamp page.
 * @param isTransition Whether to slide between pages.
 * @param isLeft Whether to use the left-side transition. */
void ListStamp::startEnd(bool isTransition, bool isLeft) {
    al::startAction(this, isTransition ? (isLeft ? "OutLeft" : "OutRight") : "End", nullptr);
    al::setNerve(this, &NrvListStampEnd);
}
/** @brief Checks whether the end animation has completed.
 * @return Whether the page has finished leaving. */
bool ListStamp::isEnd() const {
    return al::isNerve(this, &NrvListStampEnd) && al::isGreaterEqualStep(this, 1) &&
           al::isActionEnd(this, nullptr);
}
/** @brief Checks whether a page-transition animation is playing.
 * @return Whether the page is sliding in or out. */
bool ListStamp::isInTransition() const {
    return al::isAnyActionPlaying(this, "Main") &&
           (al::isActionPlaying(this, "OutLeft", nullptr) ||
            al::isActionPlaying(this, "OutRight", nullptr) ||
            al::isActionPlaying(this, "InLeft", nullptr) ||
            al::isActionPlaying(this, "InRight", nullptr));
}
