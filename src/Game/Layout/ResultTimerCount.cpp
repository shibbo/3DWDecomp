#include "Layout/ResultTimerCount.hpp"

#include "System/StageTimer.hpp"
#include "System/Data/StageDataHolder.hpp"
#include "Library/Audio/System/AudioKeeper.hpp"
#include "Library/Audio/System/AudioKeeperFunction.hpp"
#include "Library/Nerve/NerveKeeper.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"

namespace {
NERVE_DECL(ResultTimerCount, Wait);
NERVE_DECL(ResultTimerCount, TimerCount);
NERVE_DECL(ResultTimerCount, EndWait);
NERVES_MAKE_NOSTRUCT(ResultTimerCount, Wait, TimerCount, EndWait)
}

/**
 * @brief Creates the result timer-to-score conversion controller.
 * @param pTimer Stage timer.
 * @param pStageData Team score data.
 * @param pAudio Audio director for countdown sounds.
 */
ResultTimerCount::ResultTimerCount(StageTimer* pTimer, StageDataHolder* pStageData,
                                 al::AudioDirector* pAudio)
    : mTimer(pTimer), mStageData(pStageData) {
    mNerveKeeper = new al::NerveKeeper(this, &NrvResultTimerCountWait, 0);
    mAudioKeeper = al::createAudioKeeper("StageTimerResult", pAudio);
}

/** @brief Advances the state and audio controllers. */
void ResultTimerCount::update() {
    mNerveKeeper->update();
    mAudioKeeper->update();
}

/** @brief Captures the displayed timer count and prepares for conversion. */
void ResultTimerCount::appear() {
    mInitialCount = mTimer->calcDisplayCount();
    mTimer->setTimerCountClearStage(mInitialCount);
    al::setNerve(this, &NrvResultTimerCountWait);
}

/** @brief Starts converting the remaining time into points. */
void ResultTimerCount::start() { al::setNerve(this, &NrvResultTimerCountTimerCount); }

/** @brief Checks whether conversion is idle.
 * @return Whether the wait state is active. */
bool ResultTimerCount::isWait() const { return al::isNerve(this, &NrvResultTimerCountWait); }

/** @brief Holds the idle state. */
void ResultTimerCount::exeWait() {}

/** @brief Converts up to two timer units into 50 points each on every frame. */
void ResultTimerCount::exeTimerCount() {
    int elapsed = (al::getNerveStep(this) + 1) * 2;
    int remaining = mInitialCount - elapsed;
    int count = mInitialCount > elapsed ? remaining : 0;
    int score = 100;
    if (count == 0) {
        score -= (elapsed - mInitialCount) * 50;
    }
    if (al::isFirstStep(this)) {
        if (mInitialCount == 0) {
            al::setNerve(this, &NrvResultTimerCountEndWait);
            return;
        }
        al::startSe(this, "CountDown", nullptr);
    }
    mTimer->setTimerCountClearStage(count);
    mStageData->addTeamScore(score);
    if (count == 0) {
        al::setNerve(this, &NrvResultTimerCountEndWait);
        al::stopSeByName(this, "CountDown");
        al::startSe(this, "CountDownEnd", nullptr);
    }
}

/** @brief Waits 50 frames after conversion before returning to idle. */
void ResultTimerCount::exeEndWait() {
    if (al::isGreaterEqualStep(this, 50)) {
        al::setNerve(this, &NrvResultTimerCountWait);
    }
}
