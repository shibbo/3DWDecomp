#include "NPC/ActorStateSupportStroke.hpp"
#include "MapObj/DrcAssistDirectorUtil.hpp"
#include "Util/DrcUtil.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/Movement/AnimScaleController.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"

namespace {
al::AnimScaleParam sScaleParam = [] {
    al::AnimScaleParam param;
    param._0 = 0.1f;
    param._4 = 0.9f;
    param._24 = 0.93f;
    param._28 = 15.0f;
    param._2c = 0.03f;
    return param;
}();
NERVE_DECL(ActorStateSupportStroke, Wait)
NERVE_DECL(ActorStateSupportStroke, Touch)
NERVES_MAKE_NOSTRUCT(ActorStateSupportStroke, Wait, Touch)
}

/**
 * @brief Creates the touch-support state and its scale animation controller.
 * @param pActor Actor receiving touch support.
 */
ActorStateSupportStroke::ActorStateSupportStroke(al::LiveActor* pActor)
    : al::ActorStateBase("\u30b5\u30dd\u30fc\u30c8\u306a\u3067\u306a\u3067\u72b6\u614b", pActor),
      mTouchActor(nullptr), mScaleController(nullptr), mTouchFrames(0), mStrokeGraceFrames(0),
      mStrokeFrames(0), mHasAppearedItem(false), mShouldAppearItem(false),
      mIsScaleEnabled(true), mIsEffectEnabled(true) {
    mScaleController = new al::AnimScaleController(&sScaleParam);
    mScaleController->setOriginalScale(al::getScale(mHostActor));
    initNerve(&NrvActorStateSupportStrokeWait, 0);
}

/** @brief Releases the state's nerve executor. */
ActorStateSupportStroke::~ActorStateSupportStroke() = default;

/** @brief Ends the state and resets its touch effects. */
void ActorStateSupportStroke::kill() {
    mIsDead = true;
    stopAndReset();
}

/** @brief Stops touch effects and resets the stroke counters. */
void ActorStateSupportStroke::stopAndReset() {
    if (mIsEffectEnabled) {
        al::tryDeleteEffect(mHostActor, "Touch");
        al::tryStopSe(mHostActor, "PgTouchStroked");
    }
    mStrokeGraceFrames = 0;
    mStrokeFrames = 0;
    updateScaleAnim();
    al::setNerve(this, &NrvActorStateSupportStrokeWait);
}

/**
 * @brief Accepts touch-assist messages and refreshes the touch duration.
 * @param pMessage Message to inspect.
 * @param pPointer Pointer supplying the touch actor.
 * @param pTarget Screen target; unused.
 * @return Whether the message is a touch-assist message.
 */
bool ActorStateSupportStroke::receiveMsgScreenPoint(const al::SensorMsg* pMessage,
                                                   al::ScreenPointer* pPointer,
                                                   al::ScreenPointTarget* pTarget) {
    if (al::isMsgTouchAssist(pMessage)) {
        mTouchActor = DrcFunction::tryFindDrcTouchActor(mHostActor, pPointer);
        mTouchFrames = 10;
        return true;
    }
    return false;
}

/** @brief Advances the optional scale animation and applies it to the actor. */
void ActorStateSupportStroke::updateScaleAnim() {
    if (mIsScaleEnabled) {
        mScaleController->update();
        al::setScale(mHostActor, mScaleController->getScale());
    }
}

/** @brief Waits for touch support while updating scale and item timing. */
void ActorStateSupportStroke::exeWait() {
    if (al::isFirstStep(this) && mIsEffectEnabled) {
        al::tryDeleteEffect(mHostActor, "Touch");
        al::tryStopSe(mHostActor, "PgTouchStroked");
    }
    updateAppearItemTiming();
    updateScaleAnim();
    if (mTouchFrames > 0) {
        al::setNerve(this, &NrvActorStateSupportStrokeTouch);
    }
}

/** @brief Triggers item appearance once after sufficient continuous stroking. */
void ActorStateSupportStroke::updateAppearItemTiming() {
    mShouldAppearItem = false;
    if (!mHasAppearedItem && mStrokeFrames > 120) {
        mHasAppearedItem = true;
        mShouldAppearItem = true;
    }
}

/** @brief Updates touch effects, stroke duration, and the transition back to waiting. */
void ActorStateSupportStroke::exeTouch() {
    if (al::isFirstStep(this)) {
        if (mIsEffectEnabled) {
            al::tryEmitEffect(mHostActor, "Touch", nullptr);
            al::tryStartSe(mHostActor, "PgTouchStroked");
        }
        if (mIsScaleEnabled) {
            mScaleController->startVibration();
        }
        mStrokeGraceFrames = 0;
        mStrokeFrames = 0;
    }
    updateStrokeTime();
    updateAppearItemTiming();
    updateScaleAnim();
    if (--mTouchFrames <= 0) {
        mTouchFrames = 0;
        if (mIsScaleEnabled) {
            mScaleController->startAnim();
        }
        al::setNerve(this, &NrvActorStateSupportStrokeWait);
    }
}

/** @brief Accumulates stroke time while allowing a short pause between movements. */
void ActorStateSupportStroke::updateStrokeTime() {
    if (mStrokeGraceFrames - 1 >= 0) {
        --mStrokeGraceFrames;
    }
    if (rc::tryCalcTouchPointerSlideSpeedOnScreenByPointer(mTouchActor) > 1.0f) {
        mStrokeGraceFrames = 15;
        ++mStrokeFrames;
    } else if (mStrokeGraceFrames == 0) {
        mStrokeFrames = 0;
    }
}

/**
 * @brief Checks whether touch support is currently active.
 * @return Whether the state is in its touch nerve.
 */
bool ActorStateSupportStroke::isTouch() const {
    return al::isNerve(this, &NrvActorStateSupportStrokeTouch);
}
