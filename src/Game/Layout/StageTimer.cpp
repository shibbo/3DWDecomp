#include "System/StageTimer.hpp"

#include <prim/seadSafeString.h>

#include "Layout/LayoutFontUtil.hpp"
#include "System/Data/StageDataHolder.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Layout/LayoutKeeper.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Play/Layout/SimpleLayoutAppearWait.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Project/Base/StringUtil.hpp"

namespace {
NERVE_DECL(StageTimer, CountDown);
NERVE_DECL(StageTimer, TimeUpDelay);
NERVE_DECL(StageTimer, ClearStage);
NERVE_DECL(StageTimer, TimeUp);
NERVE_DECL(StageTimer, Deactive);
NERVE_DECL(StageTimer, Stop);
NERVES_MAKE_NOSTRUCT(StageTimer, CountDown, TimeUpDelay, Stop, TimeUp, Deactive, ClearStage)

/// Timer count used when the stage has no initial timer set.
constexpr s32 cDefaultTimerCount = 400;
/// Timer count restored at a checkpoint for normal stages.
constexpr s32 cCheckpointTimerCount = 300;
/// Timer count at or below which the hurry-up music plays.
constexpr s32 cHurryUpTimerCount = 100;
/// Last timer count that plays the count-down sound.
constexpr s32 cCountDownSeCountMax = 10;
/// Frames between the timer reaching zero and the time-up layout appearing.
constexpr s32 cTimeUpDelayStep = 40;
/// Capacity of the stop requester list.
constexpr s32 cStopRequesterMax = 16;

/**
 * @brief Writes a timer count into both timer panes.
 * @param pLayout Timer layout.
 * @param count Count to display.
 */
inline void setTimerPaneCount(al::IUseLayout* pLayout, s32 count) {
    al::setPaneCounterDigit3WithIcon(pLayout, "TxtTimer", LayoutFontUtil::getIconFontClock(),
                                     count, 0);
    al::setPaneCounterDigit3WithIcon(pLayout, "TxtTimer_ds", LayoutFontUtil::getIconFontClock(),
                                     count, 0);
}
}  // namespace

/**
 * @brief Creates the stage timer as parts of its parent layout and starts the countdown.
 * @param rInfo Layout initialization context.
 * @param pName Actor name.
 * @param pPaneName Name of the layout parts pane.
 * @param pParent Parent layout actor (also used for audio and scene objects).
 * @param pStageDataHolder Stage data holding the timer frames.
 * @param pStageName Stage name (unused).
 */
StageTimer::StageTimer(const al::LayoutInitInfo& rInfo, const char* pName, const char* pPaneName,
                       al::LayoutActor* pParent, StageDataHolder* pStageDataHolder,
                       const char* pStageName)
    : al::LayoutActor(pName), mParent(pParent), mStageDataHolder(pStageDataHolder) {
    al::initLayoutPartsActor(this, pParent, rInfo, pPaneName, nullptr);
    initNerve(&NrvStageTimerCountDown, 0);
    al::setPaneString(
        this, "TxtTimer",
        sead::WFormatFixedSafeString<5>(u"%s000", LayoutFontUtil::getIconFontClock()).cstr(), 0,
        -1);
    al::setPaneString(
        this, "TxtTimer_ds",
        sead::WFormatFixedSafeString<5>(u"%s000", LayoutFontUtil::getIconFontClock()).cstr(), 0,
        -1);

    s32 count = GameDataFunction::getInitStageTimer(GameDataHolderAccessor(pParent));
    if (count < 1) {
        count = cDefaultTimerCount;
    }

    mStageDataHolder->setStageTimerFrame(StageDataHolder::calcStageTimerCountToFrame(count));
    setTimerPaneCount(this, count);
    mIsLongTimer = count > cDefaultTimerCount;
    mTimeUpLayout = new al::SimpleLayoutAppearWait("タイムアップレイアウト", "HeadTimeUp", rInfo,
                                                   nullptr);
    mStopRequesters.allocBuffer(cStopRequesterMax, nullptr);
    mTimerTextInfo.setTextBox(getLayoutKeeper()->getLayout(), "TxtTimer", 2);
    al::startAction(this, "ColorWhite", nullptr);
}

/**
 * @brief Checks whether the timer is counting down (or waiting to time up).
 * @return True while the timer runs.
 */
bool StageTimer::isCountDown() const {
    return al::isNerve(this, &NrvStageTimerCountDown) ||
           al::isNerve(this, &NrvStageTimerTimeUpDelay);
}

/**
 * @brief Checks whether the timer is stopped by a requester.
 * @return True while stopped.
 */
bool StageTimer::isStop() const {
    return al::isNerve(this, &NrvStageTimerStop);
}

/**
 * @brief Checks whether the time ran out.
 * @return True after the time-up.
 */
bool StageTimer::isTimeUp() const {
    return al::isNerve(this, &NrvStageTimerTimeUp);
}

/** @brief Resets the timer to the checkpoint count. */
void StageTimer::setTimerCheckpoint() {
    s32 count = mIsLongTimer ? cDefaultTimerCount : cCheckpointTimerCount;
    mStageDataHolder->setStageTimerFrame(StageDataHolder::calcStageTimerCountToFrame(count));
}

/**
 * @brief Displays the count left at the stage clear.
 * @param count Count to display.
 */
void StageTimer::setTimerCountClearStage(s32 count) {
    if (al::isNerve(this, &NrvStageTimerDeactive)) {
        return;
    }

    setTimerPaneCount(this, count);
}

/**
 * @brief Calculates the count shown by the timer.
 * @return Remaining timer count.
 */
s32 StageTimer::calcDisplayCount() const {
    return mStageDataHolder->calcStageTimerCount();
}

/** @brief Stops the timer because the stage was cleared. */
void StageTimer::clearStage() {
    if (al::isNerve(this, &NrvStageTimerDeactive)) {
        return;
    }

    al::setNerve(this, &NrvStageTimerClearStage);
}

/** @brief Deactivates the timer. */
void StageTimer::deactivate() {
    al::setNerve(this, &NrvStageTimerDeactive);
}

/** @brief Starts the hurry-up music if the timer is low enough. */
void StageTimer::tryStartHurryUp() {
    if (!mIsHurryUp && mStageDataHolder->getStageTimerFrame() <=
                           StageDataHolder::calcStageTimerCountToFrame(cHurryUpTimerCount)) {
        startHurryUp();
    }
}

/** @brief Switches the timer and the music to hurry-up mode. */
void StageTimer::startHurryUp() {
    al::startAction(this, "ColorRed", nullptr);
    al::startBgm(mParent, "HurryUp", -1, 0, -1, -1);
    al::changeBgmSituation(mParent, "ChangeNormalToHurry");
    mIsHurryUp = true;
    if (al::isExistSeKeeper(mParent)) {
        alSeFunction::setRequestKeeperVolumeSetting(mParent, "メイン", "ハリーアップ", 10, false);
    }

    mSeLikeBgmName = "HurryUp";
}

/**
 * @brief Stops the countdown on behalf of a requester.
 * @param pRequester Object requesting the stop.
 */
void StageTimer::requestStop(const void* pRequester) {
    if (!isCountDown()) {
        return;
    }

    bool isFound = false;
    for (s32 i = 0; i < mStopRequesters.size(); i++) {
        if (mStopRequesters.at(i) == pRequester) {
            isFound = true;
            break;
        }
    }

    if (!isFound) {
        mStopRequesters.pushBack(static_cast<const StopRequester*>(pRequester));
    }

    al::setNerve(this, &NrvStageTimerStop);
}

/**
 * @brief Releases a stop request; restarts the countdown once no request remains.
 * @param pRequester Object that requested the stop.
 */
void StageTimer::requestRestart(const void* pRequester) {
    if (!isStop()) {
        return;
    }

    for (s32 i = 0; i < mStopRequesters.size(); i++) {
        if (mStopRequesters.at(i) == pRequester) {
            mStopRequesters.erase(i);
            break;
        }
    }

    if (mStopRequesters.size() != 0) {
        return;
    }

    if (mStageDataHolder->getStageTimerFrame() >= 1) {
        al::setNerve(this, &NrvStageTimerCountDown);
    } else {
        al::setNerve(this, &NrvStageTimerTimeUpDelay);
    }
}

/** @brief Stops the timer for a demo. */
void StageTimer::startDemo() {
    mIsDemo = true;
    if (al::isNerve(this, &NrvStageTimerDeactive)) {
        return;
    }

    requestStop(this);
}

/** @brief Restarts the timer after a demo. */
void StageTimer::endDemo() {
    mIsDemo = false;
    if (al::isNerve(this, &NrvStageTimerDeactive)) {
        return;
    }

    if (isStop()) {
        requestRestart(this);
    }
}

/**
 * @brief Gets the layout shown when the time runs out.
 * @return The time-up layout.
 */
al::LayoutActor* StageTimer::getTimeUpLayout() const {
    return mTimeUpLayout;
}

/** @brief Switches the timer and the music back from hurry-up mode. */
void StageTimer::endHurryUp() {
    al::startAction(this, "ColorWhite", nullptr);
    al::stopBgm(mParent, "HurryUp", -1, -1);
    al::changeBgmSituation(mParent, "ChangeHurryToNormal");
    mIsHurryUp = false;
    if (al::isExistSeKeeper(mParent)) {
        alSeFunction::setRequestKeeperVolumeSetting(mParent, "メイン", "通常", 30, false);
    }

    mSeLikeBgmName = nullptr;
}

/** @brief Restores the normal volume once the hurry-up music is no longer playing. */
void StageTimer::tryRevertVolumeForSeLikeBgm() {
    if (!mIsHurryUp || mSeLikeBgmName == nullptr) {
        return;
    }

    const char* playingName = al::getCurPlayingBgmPlayName(mParent);
    if (playingName != nullptr && !al::isEqualString(playingName, mSeLikeBgmName)) {
        if (al::isExistSeKeeper(mParent)) {
            alSeFunction::setRequestKeeperVolumeSetting(mParent, "メイン", "通常", 60, false);
        }

        playingName = nullptr;
    }

    mSeLikeBgmName = playingName;
}

/** @brief Counts the timer down and handles hurry-up, the count-down sound and the time-up. */
void StageTimer::exeCountDown() {
    if (al::isFirstStep(this)) {
        mPrevTimerFrame = mStageDataHolder->getStageTimerFrame();
    }

    mStageDataHolder->decStageTimerFrame(1);
    al::setPaneCounterDigit3WithIcon(this, "TxtTimer", LayoutFontUtil::getIconFontClock(),
                                     calcDisplayCount(), 0);
    al::setPaneCounterDigit3WithIcon(this, "TxtTimer_ds", LayoutFontUtil::getIconFontClock(),
                                     calcDisplayCount(), 0);

    s32 frame = mStageDataHolder->getStageTimerFrame();
    if (!mIsHurryUp && frame <= StageDataHolder::calcStageTimerCountToFrame(cHurryUpTimerCount)) {
        startHurryUp();
    } else if (mIsHurryUp &&
               frame > StageDataHolder::calcStageTimerCountToFrame(cHurryUpTimerCount)) {
        endHurryUp();
    }

    tryRevertVolumeForSeLikeBgm();
    if (mPrevTimerFrame < frame) {
        al::startAction(this, "Signal", "Signal");
    }

    mPrevTimerFrame = frame;
    for (s32 i = 1; i <= cCountDownSeCountMax; i++) {
        if (frame == StageDataHolder::calcStageTimerCountToFrame(i)) {
            al::startSe(mParent, "StageTimerCountDown");
            break;
        }
    }

    if (frame <= 0) {
        al::setNerve(this, &NrvStageTimerTimeUpDelay);
    }
}

/** @brief Waits a moment after the timer reached zero before the time-up. */
void StageTimer::exeTimeUpDelay() {
    if (mStageDataHolder->getStageTimerFrame() >= 1) {
        al::startAction(this, "Signal", "Signal");
        al::setNerve(this, &NrvStageTimerCountDown);
        return;
    }

    if (al::isStep(this, cTimeUpDelayStep)) {
        al::setNerve(this, &NrvStageTimerTimeUp);
    }
}

/** @brief Shows the time-up layout. */
void StageTimer::exeTimeUp() {
    if (al::isStep(this, 0)) {
        mTimeUpLayout->appear();
    }
}

/** @brief Does nothing after the stage was cleared. */
void StageTimer::exeClearStage() {}

/** @brief Does nothing while stopped. */
void StageTimer::exeStop() {}

/** @brief Shows an empty timer and only counts the play time. */
void StageTimer::exeDeactive() {
    if (al::isFirstStep(this)) {
        al::setPaneString(
            this, "TxtTimer",
            sead::WFormatFixedSafeString<6>(u"%s000", LayoutFontUtil::getIconFontClock()).cstr(),
            0, -1);
        al::setPaneString(
            this, "TxtTimer_ds",
            sead::WFormatFixedSafeString<6>(u"%s000", LayoutFontUtil::getIconFontClock()).cstr(),
            0, -1);
    }

    if (!mIsDemo) {
        mStageDataHolder->countUpPlayTime(1);
    }
}

/** @brief Re-applies the font fix to the timer text every frame. */
void StageTimer::control() {
    mTimerTextInfo.applyFix();
}
