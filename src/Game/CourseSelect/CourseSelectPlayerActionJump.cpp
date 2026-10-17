#include "CourseSelect/CourseSelectPlayerActionJump.hpp"

#include "Player/IUsePlayerAnimator.hpp"
#include "Player/IUsePlayerCollision.hpp"
#include "Player/IUsePlayerInput.hpp"
#include "Player/IUsePlayerInputArranger.hpp"
#include "Player/Normal/PlayerConstParam.hpp"
#include "Player/Normal/PlayerProperty.hpp"
#include "Player/PlayerActionAirMoveArg.hpp"
#include "Player/PlayerActionArg.hpp"
#include "Player/PlayerActionFunc.hpp"
#include "Player/PlayerContinuousJump.hpp"

CourseSelectPlayerActionJump::CourseSelectPlayerActionJump(
    PlayerActionAirMoveArg* pArg, PlayerContinuousJump* pContinuousJump,
    const IUsePlayerHorizontalSpeedAverage* pSpeedAverage, const IUsePlayerActualMove* pActualMove,
    const IUsePlayerFlag* pFlag)
    : mContinuousJump(pContinuousJump), mSpeedAverage(pSpeedAverage), mActualMove(pActualMove),
      mFlag(pFlag) {
    mArg = pArg->mActionArg;
}

void CourseSelectPlayerActionJump::move() {
    mArg->mCollision->solveAir();
}

void CourseSelectPlayerActionJump::update() {
    if (mArg->mCollision->isOnCeiling()) {
        float upwardSpeed = mArg->mProperty->mUpDir.dot(mArg->mProperty->mVelocity);
        if (upwardSpeed > 0.0f) {
            mArg->mProperty->mVelocity += mArg->mProperty->mUpDir * 2.0f;
            mArg->mProperty->mVelocity.setScaleAdd(-upwardSpeed, mArg->mProperty->mUpDir,
                                                mArg->mProperty->mVelocity);
        }
    }
    mArg->mProperty->mVelocity -= mArg->mProperty->mUpDir * 2.0f;
    updateSound();
}

void CourseSelectPlayerActionJump::updateSound() {
    if (!mJumpButtonHeld)
        return;
    if (mArg->mInput->isJumpButtonOn()) {
        ++mJumpButtonFrames;
        if (mSoundTimer <= 0)
            ++mSoundTimer;
    } else {
        mJumpButtonHeld = false;
        mJumpButtonFrames = 0;
    }
    if (mSoundTimer > 0)
        --mSoundTimer;
}

void CourseSelectPlayerActionJump::setup() {
    mContinuousJump->getContinuousCount();
    mArg->mAnimator->startAnim("CourseSelectJump");
    mArg->mInputArranger->resetPrecedingJump();
    mJumpButtonHeld = true;
    mJumpButtonFrames = 0;
    mSoundTimer = 10;
    PlayerActionFunc::setupJump(mArg->mProperty, mArg->mCollision, 21.0f, false);
}

void CourseSelectPlayerActionJump::teardown() {
    if (mContinuousJump)
        mContinuousJump->countUp();
}

int CourseSelectPlayerActionJump::getJumpPowCountMax() const {
    return mArg->mConstParam->getJumpPowCountMax();
}

float CourseSelectPlayerActionJump::getDashJumpAddition() const {
    return mArg->mConstParam->getDashJumpAddition();
}

float CourseSelectPlayerActionJump::getJumpPow() const {
    return mArg->mConstParam->getJumpPow();
}

float CourseSelectPlayerActionJump::getJumpPowLow() const {
    return mArg->mConstParam->getJumpPowLow();
}
