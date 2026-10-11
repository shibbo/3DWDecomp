#include "Demo/StageStartEventTimer.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"
#include "Library/Layout/LayoutActor.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Play/Layout/WipeSimple.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/StageTimer.hpp"

namespace rc {
void requestStartDemoPlayer(const al::LiveActor* pActor);
void requestEndDemoPlayer(const al::LiveActor* pActor);
}

namespace {
NERVE_DECL(StageStartEventTimer, Appear);
NERVE_DECL(StageStartEventTimer, Move);
NERVE_DECL(StageStartEventTimer, Wait);
NERVE_DECL(StageStartEventTimer, End);
NERVES_MAKE_NOSTRUCT(StageStartEventTimer, Appear, Move, Wait, End)
}

/** @brief Creates the stage timer event. @param pName Actor name. */
StageStartEventTimer::StageStartEventTimer(const char* pName) : StageStartEventBase(pName) {}

/** @brief Assigns the stage countdown. @param pTimer Timer used for hurry-up activation. */
void StageStartEventTimer::setStageTimer(StageTimer* pTimer) {
    mStageTimer = pTimer;
}

/**
 * @brief Creates the timer layout and black wipe and initializes scene services.
 * @param rInfo Actor and layout initialization data.
 */
void StageStartEventTimer::init(const al::ActorInitInfo& rInfo) {
    al::initActorSceneInfo(this, rInfo);
    al::initActorPoseTRSV(this);
    al::initActorSRT(this, rInfo);
    al::initNerve(this, &NrvStageStartEventTimerWait, 0);
    al::initExecutorUpdate(this, rInfo, "デモオブジェクト");
    al::initActorAudioKeeperWithout3D(this, rInfo, "StageStartEventTimer", nullptr);
    makeActorDead();
    mLayout = new al::LayoutActor("タイマーデモ");
    al::initLayoutActor(mLayout, al::getLayoutInitInfo(rInfo), "StageStartEventTimer", nullptr);
    mWipe = new al::WipeSimple("黒フェード", "WipeFadeBlack", al::getLayoutInitInfo(rInfo), "Demo");
}

/** @brief Activates the event and starts the timer appearance sequence. */
void StageStartEventTimer::startDemo() {
    appear();
    al::setNerve(this, &NrvStageStartEventTimerAppear);
}

/** @brief Requests the waiting state that opens the wipe. */
void StageStartEventTimer::endDemo() {
    al::setNerve(this, &NrvStageStartEventTimerWait);
}

/** @brief Checks the ending state. @return Whether the timer demo has ended. */
bool StageStartEventTimer::isEndDemo() const {
    return al::isNerve(this, &NrvStageStartEventTimerEnd);
}

/** @brief Allows movement during the wait and end states. @return Whether movement is enabled. */
bool StageStartEventTimer::isEnableMovement() const {
    return al::isNerve(this, &NrvStageStartEventTimerWait) || al::isNerve(this, &NrvStageStartEventTimerEnd);
}

/** @brief Displays the initial timer for 120 frames while holding players in the demo. */
void StageStartEventTimer::exeAppear() {
    if (al::isFirstStep(this)) {
        rc::requestStartDemoPlayer(this);
        al::startBgm(this, "Stage", -1, 0, -1, -1);
        mStageTimer->tryStartHurryUp();
        mLayout->appear();
        al::startAction(mLayout, "Appear", nullptr);
        sead::WFormatFixedSafeString<6> timerText(u"Q%03d", GameDataFunction::getInitStageTimer(GameDataHolderAccessor(this)));
        al::setPaneString(mLayout, "TxtTimer", timerText.cstr(), 0, -1);
        al::setPaneString(mLayout, "TxtTimer_ds", timerText.cstr(), 0, -1);
        mWipe->startCloseEnd();
    }
    if (al::isGreaterEqualStep(this, 120)) {
        al::setNerve(this, &NrvStageStartEventTimerMove);
    }
}

/** @brief Moves the timer layout and waits for its action to finish. */
void StageStartEventTimer::exeMove() {
    if (al::isFirstStep(this)) {
        al::startAction(mLayout, "Move", nullptr);
    }
    if (al::isActionEnd(mLayout, nullptr)) {
        al::setNerve(this, &NrvStageStartEventTimerWait);
    }
}

/** @brief Opens the wipe, plays the hurry-up cue, and releases the players. */
void StageStartEventTimer::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(mLayout, "Wait", nullptr);
        mWipe->startOpen(20);
    }
    if (al::isStep(this, 15)) {
        al::startSe(this, "HurryUpStart");
    }
    if (al::isGreaterEqualStep(this, 20)) {
        al::setNerve(this, &NrvStageStartEventTimerEnd);
        mLayout->kill();
        rc::requestEndDemoPlayer(this);
    }
}

/** @brief Kills the event actor once the demo is finished. */
void StageStartEventTimer::exeEnd() {
    kill();
}
