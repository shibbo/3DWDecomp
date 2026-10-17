#include "Layout/ListStampResult.hpp"

#include "Layout/ListStampParts.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"

namespace {
NERVE_DECL(ListStampResult, End);
NERVE_DECL(ListStampResult, Wait);
NERVE_DECL(ListStampResult, Appear);
NERVES_MAKE_NOSTRUCT(ListStampResult, End, Wait, Appear)
}

/** @brief Creates the result screen and its stamp collection parts.
 * @param rInfo Layout initialization context.
 * @param pGameData Save data containing earned stamps. */
ListStampResult::ListStampResult(const al::LayoutInitInfo& rInfo, const GameDataHolder* pGameData)
    : al::LayoutActor("ハンコ表示画面") {
    al::initLayoutActor(this, rInfo, "ListStampResult", nullptr);
    mStampParts = new ListStampParts(rInfo, "はんこリストパーツ", "ParListStamp", this, pGameData);
    initNerve(&NrvListStampResultEnd, 0);
}

/** @brief Selects the earned stamp and plays the appearance animation. */
void ListStampResult::exeAppear() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Appear", nullptr);
        if (mCourseId > 0)
            mStampParts->setAcquiredStampNameByCourseId(mCourseId);
        else if (mCharacterId != -1)
            mStampParts->setAcquiredStampNameByCharacterComplete(mCharacterId);
        mStampParts->appear();
    }
    if (al::isActionEnd(this, nullptr))
        al::setNerve(this, &NrvListStampResultWait);
}

/** @brief Waits for the earned-stamp animation to complete. */
void ListStampResult::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait", nullptr);
        mStampParts->startDecide();
    }
    if ((mCourseId > 0 && mStampParts->isDecideEnd()) ||
        (mCharacterId != -1 && mStampParts->isDecideEnd()))
        al::setNerve(this, &NrvListStampResultEnd);
}

/** @brief Starts the result screen's end animation. */
void ListStampResult::exeEnd() {
    if (al::isFirstStep(this))
        al::startAction(this, "End", nullptr);
}

/** @brief Shows the stamp earned in a course.
 * @param courseId Completed course. */
void ListStampResult::startAppear(int courseId) {
    al::LayoutActor::appear();
    mCourseId = courseId;
    al::setNerve(this, &NrvListStampResultAppear);
}

/** @brief Shows the stamp earned by completing a character's courses.
 * @param characterId Completed character. */
void ListStampResult::startAppearCharacterComplete(int characterId) {
    al::LayoutActor::appear();
    mCharacterId = characterId;
    al::setNerve(this, &NrvListStampResultAppear);
}

/** @brief Requests the end animation. */
void ListStampResult::startEnd() { al::setNerve(this, &NrvListStampResultEnd); }

/** @brief Checks whether the end animation has finished.
 * @return Whether the screen has reached the end of its end state. */
bool ListStampResult::isEnd() const {
    return al::isNerve(this, &NrvListStampResultEnd) && al::isGreaterEqualStep(this, 1) &&
           al::isActionEnd(this, nullptr);
}
