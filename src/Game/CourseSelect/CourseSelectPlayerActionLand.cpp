#include "CourseSelect/CourseSelectPlayerActionLand.hpp"

#include "Player/IUsePlayerAnimator.hpp"
#include "Player/IUsePlayerCollision.hpp"
#include "Player/IUsePlayerSubAction.hpp"
#include "Player/Normal/PlayerProperty.hpp"
#include "Player/PlayerActionArg.hpp"

CourseSelectPlayerActionLand::CourseSelectPlayerActionLand(const PlayerActionArg* pArg)
    : mArg(pArg) {}

void CourseSelectPlayerActionLand::move() {
    mArg->mCollision->snapGround();
}

void CourseSelectPlayerActionLand::update() {}

void CourseSelectPlayerActionLand::setup() {
    mArg->mAnimator->startAnim("CourseSelectLand");
    mArg->mProperty->mVelocity = sead::Vector3f::zero;
    if (mEnableSubAction)
        mArg->mSubAction->validateAll();
}

void CourseSelectPlayerActionLand::teardown() {
    if (mEnableSubAction)
        mArg->mSubAction->invalidateAll();
}

bool CourseSelectPlayerActionLand::isEnd() const {
    return mArg->mAnimator->isAnimEnd();
}

bool CourseSelectPlayerActionLand::isPossibleToCancel() const {
    return true;
}
