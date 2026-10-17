#include "CourseSelect/CourseSelectStateAfterEndingEvent.hpp"
#include "CourseSelect/CourseSelectScene.hpp"
#include "CourseSelect/CourseSelectDirector.hpp"
#include "CourseSelect/CourseSelectWindowHolder.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "System/GameDataFlagFunction.hpp"
#include "System/GameDataHolderWriter.hpp"

namespace {
NERVE_DECL(CourseSelectStateAfterEndingEvent, Demo);
NERVE_DECL(CourseSelectStateAfterEndingEvent, Wipe);
NERVES_MAKE_NOSTRUCT(CourseSelectStateAfterEndingEvent, Demo, Wipe)
}

/** @brief Prepare the special-world opening event that follows the ending. */
CourseSelectStateAfterEndingEvent::CourseSelectStateAfterEndingEvent(CourseSelectScene* pScene, CourseSelectDirector* pDirector)
    : al::NerveStateBase("エンディング後のイベント"), mpScene(pScene), mpDirector(pDirector) {
    mpWindows = pDirector->getWindowHolder();
}
void CourseSelectStateAfterEndingEvent::init() {
    initNerve(&NrvCourseSelectStateAfterEndingEventDemo, 0);
}
/** @brief Start the rocket opening demo and reset this event's state. */
void CourseSelectStateAfterEndingEvent::appear() {
    al::NerveStateBase::appear();
    mpDirector->startOpenRocketDemo();
    al::setNerve(this, &NrvCourseSelectStateAfterEndingEventDemo);
}
/** @brief Apply the demo's sound mix and wait for the rocket reveal. */
void CourseSelectStateAfterEndingEvent::exeDemo() {
    if (al::isFirstStep(this))
        alSeFunction::setRequestKeeperVolumeSetting(mpScene->getAudioDirector(), "メイン", "エンディング後デモ", 0, false);
    if (mpDirector->isEndOpenRocketDemo())
        al::setNerve(this, &NrvCourseSelectStateAfterEndingEventWipe);
}
/** @brief Fade to black, then record the completed special-world reveal. */
void CourseSelectStateAfterEndingEvent::exeWipe() {
    if (al::isFirstStep(this)) {
        mpWindows->startFadeBlack(180);
        alSeFunction::stopAllSe(mpScene->getAudioDirector(), 120, nullptr);
        alSeFunction::setRequestKeeperVolumeSetting(mpScene->getAudioDirector(), "メイン", "エンディング後デモワイプ", 60, false);
    }
    if (mpWindows->isCloseFadeBlack() && !al::isLessStep(this, 240)) {
        GameDataFlagFunction::setShowSpecialWorldOpenDemo(GameDataHolderWriter(mpScene->getGameDataHolderDirect()));
        kill();
    }
}
