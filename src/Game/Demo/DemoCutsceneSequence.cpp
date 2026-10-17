#include "Demo/DemoCutsceneSequence.hpp"

#include "Demo/DemoSceneActorHolder.hpp"
#include "Layout/GuideGameWindow.hpp"
#include "Layout/Switch/DemoSkipLayout.hpp"
#include "Layout/WindowMessage.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Play/Layout/WipeSimple.hpp"
#include "Library/LiveActor/Util/ActorSceneUtil.hpp"
#include "Project/Base/StringUtil.hpp"

namespace {
NERVE_DECL(DemoCutsceneSequence, SkipToUnskippableAction);
NERVE_DECL(DemoCutsceneSequence, Play);
NERVES_MAKE_NOSTRUCT(DemoCutsceneSequence, SkipToUnskippableAction, Play)
}

/** @brief Creates an empty sequence. @param pName Actor name. @param type Audio demo type. */
DemoCutsceneSequence::DemoCutsceneSequence(const char* pName, alSeFunction::DemoType type)
    : DemoCutscene(pName, type) {}

/** @brief Appends an action to the sequence. @param pName Animation name.
 * @param waitForRequest Whether advancement requires an explicit request.
 * @param pMessage Optional guide message. @param messagePage Page to wait on, or -1.
 * @param skippable Whether this action can be skipped. @param waitCount Advancement delay.
 * @param continueAction Whether an identical previous animation should keep playing. */
void DemoCutsceneSequence::addActionToSequence(const char* pName, bool waitForRequest,
    const char* pMessage, int messagePage, bool skippable, int waitCount, bool continueAction) {
    Action* action = new Action(pName, waitForRequest, pMessage, messagePage, skippable,
                                waitCount, continueAction);
    action->id = mActionCount++;
    if (!skippable)
        mAllowSkip = false;
    if (mActions) {
        Action* tail = mActions;
        while (tail->next)
            tail = tail->next;
        tail->next = action;
    } else {
        mActions = action;
    }
}

/** @brief Advances with a replacement message. @param pMessage Message for the next action. */
void DemoCutsceneSequence::tryStartNextActionWithMessage(const char* pMessage) {
    if (mCurrentAction && mCurrentAction->next)
        mCurrentAction->next->message = pMessage;
    tryStartNextAction();
}

/** @brief Counts down the advancement delay, then selects the next action. */
void DemoCutsceneSequence::tryStartNextAction() {
    if (mWaitCount - 1 >= 0) {
        mWaitCount--;
        return;
    }
    mPreviousAction = mCurrentAction;
    mCurrentAction = mCurrentAction->next;
    al::requestCaptureScreenCover(this, 4);
    if (mCurrentAction)
        mStartDelay = 2;
    else if (mFadeAtEnd)
        mWipe->startClose(mWipeFrames);
}

/** @brief Updates playback and handles skips that stop at an unskippable action. */
void DemoCutsceneSequence::exePlay() {
    DemoCutscene::exePlay();
    if (mAllowSkip || !isCurrentActionSkippable() || !getUnskippableAction())
        return;
    if (mSkipLayout->isSkip(sead::BitFlag<u16>(1 << al::getMainControllerPort())) && !isEndDemo()) {
        mWipe->startClose(-1);
        al::setNerve(this, &NrvDemoCutsceneSequenceSkipToUnskippableAction);
        if (rc::isGuideGameWindowWaitConfirm(this))
            rc::forceEndGuideGameWindow(this);
    }
}

/** @brief Finds the next action that cannot be skipped. @return Action, or null. */
DemoCutsceneSequence::Action* DemoCutsceneSequence::getUnskippableAction() {
    Action* action = mCurrentAction;
    if (!action)
        return nullptr;
    do {
        action = action->next;
    } while (action && action->skippable);
    return action;
}

/** @brief Cancels playback and closes a pending confirmation message. */
void DemoCutsceneSequence::exeCancelDemo() {
    DemoObjBase::exeCancelDemo();
    if (al::isFirstStep(this) && rc::isGuideGameWindowWaitConfirm(this))
        rc::forceEndGuideGameWindow(this);
}

/** @brief Starts the next unskippable action after the closing wipe finishes. */
void DemoCutsceneSequence::exeSkipToUnskippableAction() {
    if (!mWipe->isCloseEnd())
        return;
    mPreviousAction = mCurrentAction;
    mCurrentAction = getUnskippableAction();
    mSkipLayout->startHidden();
    startCurrentAction(true);
    al::requestCaptureScreenCover(this, 4);
    mWipe->startOpen(-1);
    al::setNerve(this, &NrvDemoCutsceneSequencePlay);
}

/** @brief Starts animation and any guide message for the selected action.
 * @param cancelPrevious Whether to cancel the prior animation before switching. */
void DemoCutsceneSequence::startCurrentAction(bool cancelPrevious) {
    mWaitCount = mCurrentAction->waitCount;
    if (!mCurrentAction->continueAction ||
        (mPreviousAction && !al::isEqualString(mCurrentAction->name, mPreviousAction->name))) {
        if (cancelPrevious && mPreviousAction)
            mDemo->tryCancel(true);
        mDemo->tryStartActionByName(mCurrentAction->name);
    }
    if (mCurrentAction->message && !rc::isGuideGameWindowActive(this)) {
        rc::appearGuideGameWindowWithConfirm(this, mCurrentAction->message, true);
        mSkipLayout->setForcedEnd(true);
    }
}

/** @brief Checks whether an action is selected. @return Whether the current action exists. */
bool DemoCutsceneSequence::isCurrentActionValid() { return mCurrentAction != nullptr; }

/** @brief Checks whether the selected action can be skipped. @return Whether skipping is allowed. */
bool DemoCutsceneSequence::isCurrentActionSkippable() {
    return mCurrentAction && mCurrentAction->skippable;
}

/** @brief Gets the selected action's index. @return Action index, or -1. */
int DemoCutsceneSequence::getCurrentActionID() const {
    return mCurrentAction ? mCurrentAction->id : -1;
}

/** @brief Enables the ending fade. @param rInfo Initialization context. @param frames Fade length. */
void DemoCutsceneSequence::setFadeAtEnd(const al::ActorInitInfo& rInfo, int frames) {
    mWipeFrames = frames;
    mFadeAtEnd = true;
}

/** @brief Covers the screen before starting the cutscene. */
void DemoCutsceneSequence::trueStartDemo() {
    al::requestCaptureScreenCover(this, 4);
    DemoCutscene::trueStartDemo();
}

/** @brief Starts the selected action, resetting to the first action outside playback. */
void DemoCutsceneSequence::startAction() {
    if (!al::isNerve(this, &NrvDemoCutsceneSequencePlay)) {
        mPreviousAction = mCurrentAction;
        mCurrentAction = mActions;
    }
    if (mCurrentAction)
        startCurrentAction(false);
}

/** @brief Advances the sequence when its message or animation completes. */
void DemoCutsceneSequence::updateDemo() {
    if (!mCurrentAction)
        return;
    if (mStartDelay - 1 >= 0) {
        mStartDelay--;
        if (mStartDelay == 0)
            startCurrentAction(false);
        return;
    }
    if (mCurrentAction->message) {
        int page = mCurrentAction->messagePage;
        bool active = rc::isGuideGameWindowActive(this);
        if (page == -1) {
            if (!active) {
                tryStartNextAction();
                return;
            }
        } else {
            if (!active) {
                tryStartNextAction();
                return;
            }
            int currentPage = rc::getGuideGameWindow(this)->getWindowMessage()->getCurrentPageNum();
            if (mCurrentAction->messagePage != currentPage) {
                tryStartNextAction();
                return;
            }
        }
    }
    if (!mCurrentAction->waitForRequest && isCurrentActionDone())
        tryStartNextAction();
}

/** @brief Checks completion of every demo actor's action. @return Whether all actions ended. */
bool DemoCutsceneSequence::isCurrentActionDone() const { return mDemo->isAllActionEnd(); }

/** @brief Checks sequence completion and finishes the optional fade. @return Whether the demo ended. */
bool DemoCutsceneSequence::checkDemoEnd() const {
    if (mCurrentAction)
        return false;
    if (mFadeAtEnd) {
        if (!mWipe->isAlive())
            mWipe->startClose(mWipeFrames);
        if (!mWipe->isCloseEnd())
            return false;
        mWipe->startOpenDelay(1, mWipeFrames);
    }
    return true;
}
