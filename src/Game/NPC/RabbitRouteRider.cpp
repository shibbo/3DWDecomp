#include "NPC/RabbitRouteRider.hpp"
#include "NPC/RabbitRoute.hpp"
#include "NPC/RabbitRoutePoint.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include <cfloat>

namespace {
NERVE_DECL(RabbitRouteRider, Move)
NERVE_DECL(RabbitRouteRider, Jump)
NERVES_MAKE_NOSTRUCT(RabbitRouteRider, Move, Jump)
}

/**
 * @brief Places a rider at the first segment of a rabbit route.
 * @param pActor Actor used to find nearby active players.
 * @param pRoute Route containing the starting point and connected points.
 * @param isFastJump Enables the faster, higher jump settings.
 */
RabbitRouteRider::RabbitRouteRider(const al::LiveActor* pActor, const RabbitRoute* pRoute,
                                 bool isFastJump)
    : al::NerveExecutor("ウサギ移動管理"), mActor(pActor), mIsFastJump(isFastJump),
      mCurrentPoint(nullptr), mNextPoint(nullptr), mPosition(sead::Vector3f::zero),
      mDirection(sead::Vector3f::ez), mSpeed(10.0f), mJumpSteps(0), mJumpHeight(0) {
    const auto* pStart = pRoute->mStartPoint;
    updatePoint(pStart, pStart->getNextUniquePoint(0));
    initNerve(&NrvRabbitRouteRiderMove, 0);
}

/**
 * @brief Advances the segment, resets its position, and updates its direction.
 * @param pPoint New current point.
 * @param pNextPoint Explicit destination, or nullptr to choose one away from the player.
 */
void RabbitRouteRider::updatePoint(const IUseRabbitRoutePoint* pPoint,
                                  const IUseRabbitRoutePoint* pNextPoint) {
    const auto* pPrevious = mCurrentPoint;
    mCurrentPoint = pPoint;
    if (pNextPoint) {
        mNextPoint = pNextPoint;
    } else {
        updateNextPointReversePlayer(pPrevious);
    }
    mPosition.set(mCurrentPoint->getPos());
    updateRotate();
}

/** @brief Advances the movement or jump state by one step. */
void RabbitRouteRider::move() { updateNerve(); }

/** @brief Checks the current state. @return Whether the rider is jumping. */
bool RabbitRouteRider::isJumping() const { return al::isNerve(this, &NrvRabbitRouteRiderJump); }

/** @brief Checks for a newly started jump. @return Whether this is the first jump step. */
bool RabbitRouteRider::isJumpStart() const { return isJumping() && al::isFirstStep(this); }

/** @brief Gets the current route point's position. @return Reference to that position. */
const sead::Vector3f& RabbitRouteRider::getCurrentPointPos() const { return mCurrentPoint->getPos(); }

/** @brief Tests the remaining segment distance. @return Whether it is within one movement step. */
bool RabbitRouteRider::isNearNextPoint() const {
    return al::isNear(mPosition, mNextPoint->getPos(), mSpeed);
}

/** @brief Gets the destination point's position. @return Reference to that position. */
const sead::Vector3f& RabbitRouteRider::getNextPointPos() const { return mNextPoint->getPos(); }

/** @brief Reverses the segment or selects another jump destination away from the player. */
void RabbitRouteRider::reverse() {
    if (isJumping()) {
        updateNextPointReversePlayer(mNextPoint);
    } else {
        const auto* pPoint = mCurrentPoint;
        mCurrentPoint = mNextPoint;
        mNextPoint = pPoint;
    }
    updateRotate();
}

/**
 * @brief Selects an adjacent point whose direction most opposes the nearest player.
 * @param pPreviousPoint Point to exclude from the candidate list; must be non-null.
 */
void RabbitRouteRider::updateNextPointReversePlayer(const IUseRabbitRoutePoint* pPreviousPoint) {
    sead::FixedPtrArray<IUseRabbitRoutePoint, 8> candidates;
    for (int i = 0; i < mCurrentPoint->getNextPointNum(); ++i) {
        const auto* pPoint = mCurrentPoint->getNextPoint(i);
        if (pPoint != pPreviousPoint) {
            candidates.pushBack(const_cast<IUseRabbitRoutePoint*>(pPoint));
        }
    }
    pPreviousPoint->isRoutePoint();
    sead::Vector3f playerDirection(sead::Vector3f::zero);
    if (rc::calcActivePlayerNum(mActor) > 0) {
        const auto* pPlayer = rc::findNearestActivePlayerActor(mActor);
        playerDirection.setSub(al::getTrans(pPlayer), mCurrentPoint->getPos());
    }
    if (!al::normalizeOrZero(&playerDirection)) {
        float bestDot = FLT_MAX;
        for (auto it = candidates.begin(); it != candidates.end(); ++it) {
            sead::Vector3f direction = it->getPos() - mCurrentPoint->getPos();
            al::normalize(&direction);
            const float dot = playerDirection.dot(direction);
            if (dot < bestDot) {
                bestDot = dot;
                mNextPoint = &*it;
            }
        }
        return;
    }
    mNextPoint = candidates[al::getRandom(candidates.size())];
}

/** @brief Normalizes the direction from the current point to the next point. */
void RabbitRouteRider::updateRotate() {
    mDirection.setSub(mNextPoint->getPos(), mCurrentPoint->getPos());
    al::normalize(&mDirection);
}

/**
 * @brief Sets the distance traveled per movement step.
 * @param speed Movement speed, also used for ordinary jumps.
 */
void RabbitRouteRider::setSpeed(float speed) { mSpeed = speed; }

/** @brief Starts a jump when both segment endpoints request one. @return Whether a jump started. */
bool RabbitRouteRider::tryStartJump() {
    if (!mCurrentPoint->isActionJump()) {
        return false;
    }
    if (!mNextPoint->isActionJump()) {
        return false;
    }
    al::setNerve(this, &NrvRabbitRouteRiderJump);
    return true;
}

/** @brief Moves along the segment, carrying unused movement into the next segment. */
void RabbitRouteRider::exeMove() {
    if (al::isFirstStep(this) && tryStartJump()) {
        return;
    }
    sead::Vector3f offset = mNextPoint->getPos() - mPosition;
    const float distance = offset.length();
    if (mSpeed < distance) {
        al::normalize(&mDirection, offset);
        mPosition += mDirection * mSpeed;
    } else {
        updatePoint(mNextPoint, nullptr);
        if (tryStartJump()) {
            return;
        }
        mPosition += mDirection * (mSpeed - distance);
    }
}

/** @brief Interpolates a jump arc and changes segments when its duration elapses. */
void RabbitRouteRider::exeJump() {
    const sead::Vector3f start(mCurrentPoint->getPos());
    const sead::Vector3f end(mNextPoint->getPos());
    if (al::isFirstStep(this)) {
        const float difference = sead::Mathf::abs(end.y - start.y);
        const bool isFast = mIsFastJump;
        mJumpHeight = static_cast<int>(sead::Mathf::max(difference * 0.8f, isFast ? 300.0f : 200.0f));
        const float speed = mIsFastJump ? 45.0f : mSpeed;
        mJumpSteps = static_cast<int>((start - end).length() / speed);
    }
    if (al::isStep(this, mJumpSteps)) {
        updatePoint(mNextPoint, nullptr);
        if (!tryStartJump()) {
            al::setNerve(this, &NrvRabbitRouteRiderMove);
        }
    } else {
        al::lerpVec(&mPosition, start, end, al::calcNerveRate(this, mJumpSteps));
        const int halfSteps = (mJumpSteps - 4) / 2;
        mPosition.y += al::calcNerveJumpValue(this, halfSteps, 4, halfSteps,
                                            static_cast<float>(mJumpHeight));
    }
}
