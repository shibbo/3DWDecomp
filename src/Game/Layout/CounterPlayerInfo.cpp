#include "Layout/CounterPlayerInfo.hpp"
#include "Layout/CounterPlayerParts.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/ControlUserUtil.hpp"
namespace {
NERVE_DECL(CounterPlayerInfo, Wait);
NERVE_DECL(CounterPlayerInfo, AppearWait);
NERVE_DECL(CounterPlayerInfo, Appear);
NERVE_DECL(CounterPlayerInfo, ShowMiss);
NERVE_DECL(CounterPlayerInfo, End);
NERVES_MAKE_NOSTRUCT(CounterPlayerInfo, Wait, AppearWait, Appear, ShowMiss, End)
}
/** @brief Creates the life information layout and its counter parts. */
CounterPlayerInfo::CounterPlayerInfo(const al::LayoutInitInfo& rInfo, const GameDataHolder* pGameData)
    : al::LayoutActor("プレイヤー残機情報") {
    mGameData = new GameDataHolderAccessor(const_cast<GameDataHolder*>(pGameData));
    al::initLayoutActor(this, rInfo, "CounterPlayerInfo", nullptr);
    initNerve(&NrvCounterPlayerInfoWait, 0);
    mCounter = new CounterPlayerParts(rInfo, "残機", "ParCounterPlayer", this, nullptr, true);
}
/** @brief Configures the course style and shows the counter for single-player control. */
void CounterPlayerInfo::appear() {
    int courseId = GameDataFunction::getPlayingCourseId(*mGameData);
    mIsKinopioBrigade = false;
    mCounter->setKinopioBrigade(false);
    if (!GameDataFunction::isInvalidCourseId(courseId) && GameDataFunction::isStageKinopioBrigade(*mGameData, courseId)) {
        mIsKinopioBrigade = true;
        mCounter->setKinopioBrigade(true);
    }
    if (rc::getActiveControlUserNum(*mGameData) != 1) return;
    mCounter->initShowMissData(mGameData);
    al::LayoutActor::appear();
    al::setNerve(this, &NrvCounterPlayerInfoAppearWait);
}
/** @brief Holds the first entrance frame before starting the animation. */
void CounterPlayerInfo::exeAppearWait() {
    if (al::isFirstStep(this)) al::startFreezeAction(this, "Appear", 0.0f, nullptr);
    if (al::isGreaterStep(this, 0)) al::setNerve(this, &NrvCounterPlayerInfoAppear);
}
/** @brief Plays the entrance animation. */
void CounterPlayerInfo::exeAppear() {
    if (al::isFirstStep(this)) al::startAction(this, "Appear", nullptr);
    if (al::isActionEnd(this, nullptr)) al::setNerve(this, &NrvCounterPlayerInfoWait);
}
/** @brief Pauses briefly before displaying the lost life. */
void CounterPlayerInfo::exeWait() {
    if (al::isFirstStep(this)) al::startAction(this, "Wait", nullptr);
    if (al::isGreaterStep(this, 16)) al::setNerve(this, &NrvCounterPlayerInfoShowMiss);
}
/** @brief Shows the life decrease and plays its sound before exiting. */
void CounterPlayerInfo::exeShowMiss() {
    if (al::isFirstStep(this)) {
        mCounter->showMiss(mGameData);
        al::startSe(this, "Decrease", nullptr);
    }
    if (al::isGreaterStep(this, 40)) al::setNerve(this, &NrvCounterPlayerInfoEnd);
}
/** @brief Plays the exit animation and hides the layout. */
void CounterPlayerInfo::exeEnd() {
    if (al::isFirstStep(this)) al::startAction(this, "End", nullptr);
    if (al::isActionEnd(this, nullptr)) kill();
}
