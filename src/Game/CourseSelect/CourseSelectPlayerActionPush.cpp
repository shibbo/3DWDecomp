#include "CourseSelect/CourseSelectPlayerActionPush.hpp"

#include "CourseSelect/CourseSelectPlayerActor.hpp"
#include "Player/IUsePlayerAnimator.hpp"
#include "Player/IUsePlayerCollision.hpp"
#include "Player/IUsePlayerSubAction.hpp"
#include "Player/Normal/PlayerProperty.hpp"
#include "Player/PlayerActionArg.hpp"

CourseSelectPlayerActionPush::CourseSelectPlayerActionPush(const PlayerActionArg* pArg,
                                                       CourseSelectPlayerActor* pPlayer)
    : mArg(pArg), mPlayer(pPlayer) {}

void CourseSelectPlayerActionPush::move() {
    mArg->mCollision->snapGround();
}

void CourseSelectPlayerActionPush::update() {}

void CourseSelectPlayerActionPush::setup() {
    mArg->mAnimator->startAnim("CourseSelectPush");
    mArg->mProperty->mVelocity = sead::Vector3f::zero;
    if (mEnableSubAction)
        mArg->mSubAction->validateAll();
    mPlayer->onPushAction();
}

void CourseSelectPlayerActionPush::teardown() {
    if (mEnableSubAction)
        mArg->mSubAction->invalidateAll();
}

bool CourseSelectPlayerActionPush::isEnd() const {
    return mArg->mAnimator->isAnimEnd();
}

bool CourseSelectPlayerActionPush::isPossibleToCancel() const {
    return true;
}
