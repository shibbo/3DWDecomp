#include "Layout/WipeCurtainStart.hpp"
#include "Layout/CounterRecordParts.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/CourseInfoHolder.hpp"
#include "Util/LayoutUtil.hpp"
namespace {
NERVE_DECL(WipeCurtainStart, Wait);
NERVE_DECL(WipeCurtainStart, Appear);
NERVE_DECL(WipeCurtainStart, End);
NERVES_MAKE_NOSTRUCT(WipeCurtainStart, Wait, Appear, End)
}
/** @brief Creates the curtain and its best-score and best-time counters. */
WipeCurtainStart::WipeCurtainStart(const al::LayoutInitInfo& rInfo)
    : al::LayoutActor("ステージ開始カーテン") {
    al::initLayoutActor(this, rInfo, "WipeCurtainStart", nullptr);
    mBestScore = new CounterRecordParts(rInfo, "ベストスコア", "ParBestScore", this);
    mBestTime = new CounterRecordParts(rInfo, "ベストタイム", "ParBestTime", this);
    initNerve(&NrvWipeCurtainStartWait, 0);
}
/** @brief Populates the course title and available records, then opens the curtain. */
void WipeCurtainStart::appearStageStart(const GameDataHolder* pGameData, al::NetworkSystem* pNetwork) {
    int courseId = GameDataFunction::getPlayingCourseId(GameDataHolderAccessor(const_cast<GameDataHolder*>(pGameData)));
    int worldId, stageId;
    GameDataFunction::calcWorldAndStageId(GameDataHolderAccessor(const_cast<GameDataHolder*>(pGameData)), &worldId, &stageId, courseId);
    rc::setPaneWorldStageString(this, this, "TxtWorld", "WipeCurtainStart", "WipeCurtainStart_World",
                               worldId, stageId, pGameData, "TxtWorld_ds", false);
    rc::setPaneStageNameString(this, this, "TxtCourseName", pGameData, courseId);
    if (CourseInfoFunction::isClear(GameDataHolderAccessor(const_cast<GameDataHolder*>(pGameData)), courseId)) {
        mBestScore->setPaneStringScore(CourseInfoFunction::getBestScore(GameDataHolderAccessor(const_cast<GameDataHolder*>(pGameData)), courseId));
        mBestScore->show();
        int bestTime = CourseInfoFunction::getBestTime(GameDataHolderAccessor(const_cast<GameDataHolder*>(pGameData)), courseId);
        if (GameDataFunction::isEnableShowBestTimeStage(GameDataHolderAccessor(const_cast<GameDataHolder*>(pGameData)), courseId) && bestTime >= 0) {
            mBestTime->show();
            mBestTime->setPaneStringTime(bestTime);
        } else mBestTime->hide();
    } else {
        mBestScore->hide();
        mBestTime->hide();
    }
    al::startAction(this, "Appear", nullptr);
    if (GameDataFunction::isCurrentStageKinopioBrigade(GameDataHolderAccessor(const_cast<GameDataHolder*>(pGameData))))
        al::startAction(this, "KinopioWipe", "WipeType");
    else al::startAction(this, "NormalWipe", "WipeType");
    appear();
    al::setNerve(this, &NrvWipeCurtainStartAppear);
}
/** @brief Ghost Mii icons are unavailable in this version. */
bool WipeCurtainStart::tryAppearGhostMiiIcon(al::NetworkSystem* pNetwork, const GameDataHolder* pGameData, int courseId) { return false; }
/** @brief Reports whether the curtain is waiting. */
bool WipeCurtainStart::isWait() const { return al::isNerve(this, &NrvWipeCurtainStartWait); }
/** @brief Allows closing after the minimum display interval. */
bool WipeCurtainStart::isEnableEnd() const { return isWait() && al::isGreaterEqualStep(this, 135); }
/** @brief Starts the closing sequence. */
void WipeCurtainStart::end() { al::setNerve(this, &NrvWipeCurtainStartEnd); }
/** @brief Plays the opening sound and waits for the entrance animation. */
void WipeCurtainStart::exeAppear() {
    if (al::isFirstStep(this)) al::startSe(this, "Appear", nullptr);
    if (al::isActionEnd(this, nullptr)) al::setNerve(this, &NrvWipeCurtainStartWait);
}
/** @brief Starts the idle animation. */
void WipeCurtainStart::exeWait() {
    if (al::isFirstStep(this)) al::startAction(this, "Wait", nullptr);
}
/** @brief Plays the closing animation and sound, then hides the curtain. */
void WipeCurtainStart::exeEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "End", nullptr);
        al::startSe(this, "End", nullptr);
    }
    if (al::isActionEnd(this, nullptr)) kill();
}
