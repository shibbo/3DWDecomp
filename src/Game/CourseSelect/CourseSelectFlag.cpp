#include "CourseSelect/CourseSelectFlag.hpp"
#include "CourseSelect/CourseSelectMiniature.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "System/CourseInfoHolder.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/Data/StageDatabaseInfo.hpp"
namespace {
NERVE_DECL(CourseSelectFlag, ClearDemo);
NERVE_DECL(CourseSelectFlag, Wait);
NERVE_DECL(CourseSelectFlag, Hide);
NERVE_DECL(CourseSelectFlag, Appear);
NERVE_DECL(CourseSelectFlag, WaitClearDemo);
NERVES_MAKE_NOSTRUCT(CourseSelectFlag, ClearDemo, Wait, Hide, Appear, WaitClearDemo)
}
/** @brief Create the flag for a course miniature. @param pName Actor name. @param pMiniature Associated course. */
CourseSelectFlag::CourseSelectFlag(const char* pName, CourseSelectMiniature* pMiniature)
    : al::LiveActor(pName), mpMiniature(pMiniature) {}
/** @brief Initialize the hidden flag model. @param rInfo Actor initialization data. */
void CourseSelectFlag::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "CourseSelectFlag", nullptr);
    al::initNerve(this, &NrvCourseSelectFlagHide, 0);
    makeActorDead();
}
/** @brief Apply miniature visibility without showing a hidden flag. @param isVisible Whether the miniature is visible. */
void CourseSelectFlag::updateVisibility(bool isVisible) {
    if (isVisible) {
        if (!al::isNerve(this, &NrvCourseSelectFlagHide)) appear();
    } else kill();
}
/** @brief Refresh course progress and show the appropriate flag. */
void CourseSelectFlag::startAppear() {
    mFlagState = calcFlagState();
    if (mFlagState) {
        appear();
        al::setNerve(this, &NrvCourseSelectFlagAppear);
    } else {
        al::setNerve(this, &NrvCourseSelectFlagHide);
        kill();
    }
}
/** @brief Show the course flag unless a special course's first-clear demo suppresses it. */
void CourseSelectFlag::startShow() {
    if (GameDataFunction::isStageLastPlayAndFirstClear(GameDataHolderAccessor(this), mpMiniature->getCourseId())) {
        const StageDatabaseInfo* pInfo = mpMiniature->getStageInfo();
        if (pInfo->isGateKeeperGoalPole() || pInfo->isContinuousMysteryBox() || pInfo->isKinopioBrigade()) {
            al::setNerve(this, &NrvCourseSelectFlagHide);
            kill();
            return;
        }
    }
    startAppear();
}
/** @brief Prepare an ordinary course flag for its clear animation. */
void CourseSelectFlag::startWaitClearDemo() {
    mFlagState = calcFlagState();
    const StageDatabaseInfo* pInfo = mpMiniature->getStageInfo();
    if (pInfo->isGateKeeperGoalPole() || pInfo->isContinuousMysteryBox() || pInfo->isKinopioBrigade()) {
        al::setNerve(this, &NrvCourseSelectFlagHide);
        kill();
        return;
    }
    appear();
    al::tryStartActionIfNotPlaying(this, "CloseWait");
    al::startMtpAnimAndSetFrameAndStop(this, "Color", 0.0f);
    al::setNerve(this, &NrvCourseSelectFlagWaitClearDemo);
}
/** @brief Refresh saved progress and play the flag's clear demo. */
void CourseSelectFlag::startClearDemo() {
    appear();
    mFlagState = calcFlagState();
    al::setNerve(this, &NrvCourseSelectFlagClearDemo);
}
/** @brief Remain hidden until requested. */
void CourseSelectFlag::exeHide() {}
/** @brief Enter the flag's idle state. */
void CourseSelectFlag::exeAppear() { al::setNerve(this, &NrvCourseSelectFlagWait); }
/** @brief Apply the saved appearance when entering idle. */
void CourseSelectFlag::exeWait() {
    if (al::isFirstStep(this)) updateFlagAction(mFlagState, false);
}
/** @brief Wait for the clear demo to start. */
void CourseSelectFlag::exeWaitClearDemo() {}
/** @brief Play the clear animation and return to idle when it finishes. */
void CourseSelectFlag::exeClearDemo() {
    if (al::isFirstStep(this)) updateFlagAction(mFlagState, true);
    if (al::isActionEnd(this)) al::setNerve(this, &NrvCourseSelectFlagWait);
}
/** @brief Encode visibility, gold flag, character, and star appearance from saved progress. @return Packed flag state. */
u32 CourseSelectFlag::calcFlagState() const {
    int courseId = mpMiniature->getCourseId();
    const StageDatabaseInfo* pInfo = mpMiniature->getStageInfo();
    FlagState result{true, false, 0, 0};
    if (pInfo->isKinopioBrigade()) {
        if (!CourseInfoFunction::isClear(GameDataHolderAccessor(this), courseId)) return {};
        result.character = 7;
        result.gold = true;
        result.star = GameDataFunction::isEnableFlagGoldStar(GameDataHolderAccessor(this)) ? 3 : 2;
    } else if (pInfo->isContinuousMysteryBox()) {
        if (!CourseInfoFunction::isAcquireGreenStarAll(GameDataHolderAccessor(this), courseId)) return {};
        result.character = CourseInfoFunction::getClearTopCharacter(GameDataHolderAccessor(this), courseId) + 1;
        result.gold = true;
        result.star = CourseInfoFunction::isClearComplete(GameDataHolderAccessor(this), courseId) ? 3 : 2;
    } else if (CourseInfoFunction::isClear(GameDataHolderAccessor(this), courseId)) {
        result.character = CourseInfoFunction::getClearTopCharacter(GameDataHolderAccessor(this), courseId) + 1;
        result.gold = CourseInfoFunction::isClearFlagTop(GameDataHolderAccessor(this), courseId);
        result.star = CourseInfoFunction::isAcquireGreenStarAll(GameDataHolderAccessor(this), courseId)
            ? (CourseInfoFunction::isClearComplete(GameDataHolderAccessor(this), courseId) ? 3 : 2) : 1;
    } else if (pInfo->isGateKeeperGoalPole()) return {};
    return result;
}
/** @brief Apply a packed state to the flag's actions and texture frames. @param state Saved flag state. @param isClearDemo Whether to play the clear animation. */
void CourseSelectFlag::updateFlagAction(u32 state, bool isClearDemo) {
    if (!(state & 0xff)) return;
    u32 character = (state >> 16) & 0xff;
    if (!character) {
        al::tryStartActionIfNotPlaying(this, "CloseWait");
        return;
    }
    u32 star = state >> 24;
    if (star == 1) {
        al::tryStartActionIfNotPlaying(this, isClearDemo ? "ClearDemo" : "ClearWait");
    } else {
        al::tryStartActionIfNotPlaying(this, isClearDemo ? "CompleteClearDemo" : "CompleteWait");
        if (star == 2) al::startMclAnimAndSetFrameAndStop(this, "ColorStar", 0.0f);
        else if (star == 3) al::startMclAnimAndSetFrameAndStop(this, "ColorStar", 1.0f);
    }
    al::startMtpAnimAndSetFrameAndStop(this, (state & 0xff00) ? "ColorGold" : "Color", static_cast<float>(character));
}
/** @brief Destroy the course flag. */
CourseSelectFlag::~CourseSelectFlag() = default;
