#include "Game/Bgm/BgmBeatPeriodicTrigger.hpp"

#include "Library/Bgm/BgmLineFunction.hpp"
#include "Library/LiveActor/LiveActor.hpp"

/**
 * @brief Creates a trigger that divides each BGM beat into equal intervals.
 * @param pActor Actor whose audio keeper supplies the rhythm.
 * @param divisionNum Number of intervals per beat; expected to be positive.
 */
BgmBeatPeriodicTrigger::BgmBeatPeriodicTrigger(al::LiveActor* pActor, s32 divisionNum)
    : mActor(pActor) {
    mBeatRateStep = 1.0f / divisionNum;
    mNextBeatRate = mBeatRateStep;
}

/**
 * @brief Updates the trigger and its interval index from the current BGM beat.
 */
void BgmBeatPeriodicTrigger::update() {
    mIsTrigger = false;

    if (!al::isEnableRhythmAnim(mActor, nullptr)) {
        return;
    }

    if (al::isTriggerBeat(mActor, 1)) {
        mIsTrigger = true;
        mTriggerIndex = 0;
        mNextBeatRate = mBeatRateStep;
    }

    f32 beatRate = al::getBeatRate(mActor);
    if (mNextBeatRate <= beatRate) {
        mIsTrigger = true;
        mTriggerIndex++;
        mNextBeatRate += mBeatRateStep;
    }
}
