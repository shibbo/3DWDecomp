#include "Boss/TentackStateFallRock.hpp"
#include "Boss/TentackHead.hpp"
#include "Boss/TentackBase.hpp"
#include "Boss/TentackRockBase.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"

namespace {
NERVE_DECL(TentackStateFallRock, WaitStart);
NERVE_DECL(TentackStateFallRock, AttackStart);
NERVE_DECL(TentackStateFallRock, Shoot);
NERVE_DECL(TentackStateFallRock, Wait);
NERVES_MAKE_NOSTRUCT(TentackStateFallRock, WaitStart, AttackStart, Shoot, Wait)
constexpr TentackStateFallRockParam sDefaultParam(30, 25, 8, 2, 120.0f, 675.0f, 1500.0f, false, true);
}

/** @brief Initializes the default falling-rock timing and spawn parameters. */
TentackStateFallRockParam::TentackStateFallRockParam()
    : mStartWait(30), mInterval(25), mCount(8), mFollowInterval(2), mAngle(120.0f),
      mMinDistance(675.0f), mMaxDistance(1500.0f), mIsRepeat(false), mIsPlayAction(true) {}

/**
 * @brief Creates the falling-rock state with optional custom parameters.
 * @param pHead Head controlling the attack.
 * @param pParam Attack parameters, or null to use the defaults.
 */
TentackStateFallRock::TentackStateFallRock(TentackHead* pHead, const TentackStateFallRockParam* pParam)
    : al::NerveStateBase("テンタックの岩降らしステート"), mHead(pHead), mParam(pParam) {
    initNerve(&NrvTentackStateFallRockWaitStart, 0);
    if (!mParam) { mParam = &sDefaultParam; }
}

/** @brief Restarts the initial delay and resets the shot count. */
void TentackStateFallRock::appear() {
    al::NerveStateBase::appear();
    mShotCount = 0;
    al::setNerve(this, &NrvTentackStateFallRockWaitStart);
}

/** @brief Ends the state and resets the shot count. */
void TentackStateFallRock::kill() {
    al::NerveStateBase::kill();
    mShotCount = 0;
}

/** @brief Starts the attack after the configured initial delay. */
void TentackStateFallRock::exeWaitStart() {
    if (al::isGreaterEqualStep(this, mParam->mStartWait)) {
        al::setNerve(this, &NrvTentackStateFallRockAttackStart);
    }
}

/** @brief Optionally plays the head attack action and waits thirty frames. */
void TentackStateFallRock::exeAttackStart() {
    if (al::isFirstStep(this) && mParam->mIsPlayAction) {
        mHead->tryStartActionAttackRockIfWait();
    }
    if (al::isGreaterEqualStep(this, 30)) {
        al::setNerve(this, &NrvTentackStateFallRockShoot);
    }
}

/** @brief Selects player following and continues or ends the shot sequence. */
void TentackStateFallRock::exeShoot() {
    const bool isFollowShot = ((mShotCount + 1) % mParam->mFollowInterval) == 0;
    shoot((isFollowShot & !mIsDisableFollow) | (mIsValidFollowForce != 0));
    if (mParam->mIsRepeat || mParam->mCount > mShotCount) {
        al::setNerve(this, &NrvTentackStateFallRockWait);
    } else {
        kill();
    }
}

/**
 * @brief Starts a reusable rock above a player or a random point near the head.
 * @param isFollow Whether to try a position near a player first.
 */
void TentackStateFallRock::shoot(bool isFollow) {
    sead::Vector3f position(0.0f, 0.0f, 0.0f);
    sead::Vector3f headPosition = al::getTrans(mHead);
    if (!isFollow || !mHead->mHost->tryFindTransNearPlayer(&position)) {
        sead::Vector3f direction = sead::Vector3f::ez;
        al::rotateVectorDegreeY(&direction, al::getRandom(mParam->mAngle * -0.5f, mParam->mAngle * 0.5f));
        direction *= al::getRandom(mParam->mMinDistance, mParam->mMaxDistance);
        position = headPosition + direction;
    }
    position.y = headPosition.y + 2500.0f;
    if (auto* pRock = mHead->mHost->tryGetDeadRock()) {
        pRock->startFall(position);
    }
    ++mShotCount;
}

/** @brief Waits the configured interval before the next shot. */
void TentackStateFallRock::exeWait() {
    if (al::isGreaterEqualStep(this, mParam->mInterval)) {
        al::setNerve(this, &NrvTentackStateFallRockShoot);
    }
}
