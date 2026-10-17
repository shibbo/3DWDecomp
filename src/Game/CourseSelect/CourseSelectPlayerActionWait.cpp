#include "CourseSelect/CourseSelectPlayerActionWait.hpp"

#include "Library/Math/MathUtil.hpp"
#include "Player/IUsePlayerAnimator.hpp"
#include "Player/IUsePlayerCollision.hpp"
#include "Player/IUsePlayerHorizontalSpeedAverage.hpp"
#include "Player/IUsePlayerPanicControl.hpp"
#include "Player/IUsePlayerSubAction.hpp"
#include "Player/Normal/PlayerProperty.hpp"
#include "Player/PlayerActionArg.hpp"

CourseSelectPlayerActionWait::CourseSelectPlayerActionWait(
    const PlayerActionArg* pArg, IUsePlayerHorizontalSpeedAverage* pSpeedAverage,
    const PlayerFigureDirector* pFigureDirector, bool solveAir, IUsePlayerPanicControl* pPanicControl)
    : mArg(pArg), mSpeedAverage(pSpeedAverage), mFigureDirector(pFigureDirector),
      mSolveAir(solveAir), mPanicControl(pPanicControl) {}

void CourseSelectPlayerActionWait::move() {
    if (mSolveAir)
        mArg->mCollision->solveAir();
    else
        mArg->mCollision->snapGround();
}

void CourseSelectPlayerActionWait::update() {
    if (mArg->mCollision->isOnFloor()) {
        PlayerProperty* property = mArg->mProperty;
        al::verticalizeVec(&property->mVelocity, property->mUpDir, property->mVelocity);
    }
    if (mPanicControl) {
        if (mPanicControl->isPanic()) {
            if (!mArg->mAnimator->isAnim("CourseSelectWaitPanic"))
                mArg->mAnimator->startAnim("CourseSelectWaitPanic");
        } else {
            if (!mArg->mAnimator->isAnim("CourseSelectWait"))
                mArg->mAnimator->startAnim("CourseSelectWait");
        }
    }
}

void CourseSelectPlayerActionWait::setup() {
    mArg->mProperty->mVelocity.set(0.0f, 0.0f, 0.0f);
    if (mPanicControl && mPanicControl->isPanic()) {
        if (!mArg->mAnimator->isAnim("CourseSelectWaitPanic"))
            mArg->mAnimator->startAnim("CourseSelectWaitPanic");
    } else {
        if (!mArg->mAnimator->isAnim("CourseSelectWait"))
            mArg->mAnimator->startAnim("CourseSelectWait");
    }
    mArg->mAnimator->setAnimRate(1.0f);
}

void CourseSelectPlayerActionWait::teardown() {
    if (mArg->mSubAction)
        mArg->mSubAction->invalidateAll();
    mSpeedAverage->resetHorizontalSpeedAverage();
}
