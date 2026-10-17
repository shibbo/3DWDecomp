#include "Game/Bgm/BgmBeatRateTrigger.hpp"

#include "Library/Bgm/BgmLineFunction.hpp"
#include "Library/LiveActor/LiveActor.hpp"

/**
 * @brief Creates a trigger at a specified fraction of each BGM beat.
 * @param pActor Actor whose audio keeper supplies the rhythm.
 * @param beatRate Fraction of the beat at which to trigger.
 */
BgmBeatRateTrigger::BgmBeatRateTrigger(al::LiveActor* pActor, f32 beatRate)
    : mActor(pActor), mBeatRate(beatRate) {}

/**
 * @brief Updates the trigger, arming it at the start of each enabled rhythm beat.
 */
void BgmBeatRateTrigger::update() {
    mIsTrigger = false;

    if (!al::isEnableRhythmAnim(mActor, nullptr)) {
        mIsWaiting = false;
        return;
    }

    if (al::isTriggerBeat(mActor, 1)) {
        mIsWaiting = true;
    }

    f32 beatRate = al::getBeatRate(mActor);
    if (mIsWaiting && mBeatRate <= beatRate) {
        mIsTrigger = true;
        mIsWaiting = false;
    }
}
