#include "Enemy/ActorRailBrakeMover.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Rail/RailUtil.hpp"
#include <math/seadMathCalcCommon.h>

namespace {

/** @brief Parameters used by movers created without explicit parameters. */
ActorRailBrakeMoverParam sDefaultParam;

}  // namespace

/** @brief Creates the default brake parameters. */
ActorRailBrakeMoverParam::ActorRailBrakeMoverParam()
    : mAccel(0.1f), mBrakeLength(150.0f), mBrake(0.1f), mMinSpeed(1.0f), mShortRailLength(300.0f),
      mShortAccel(0.05f), mShortBrakeLength(100.0f), mShortBrake(0.05f) {}

/** @brief Creates brake parameters from explicit values.
 * @param accel Acceleration applied while far from the goal.
 * @param brakeLength Distance from the goal at which braking starts.
 * @param brake Deceleration applied while braking.
 * @param minSpeed Lowest speed reachable by braking.
 * @param shortRailLength Rails shorter than this use the short-rail parameters.
 * @param shortAccel Acceleration used on short rails.
 * @param shortBrakeLength Braking distance used on short rails.
 * @param shortBrake Deceleration used on short rails.
 */
ActorRailBrakeMoverParam::ActorRailBrakeMoverParam(float accel, float brakeLength, float brake,
                                                   float minSpeed, float shortRailLength,
                                                   float shortAccel, float shortBrakeLength,
                                                   float shortBrake)
    : mAccel(accel), mBrakeLength(brakeLength), mBrake(brake), mMinSpeed(minSpeed),
      mShortRailLength(shortRailLength), mShortAccel(shortAccel),
      mShortBrakeLength(shortBrakeLength), mShortBrake(shortBrake) {}

/** @brief Creates a rail mover for an actor.
 * @param pActor Actor moved along its rail.
 * @param maxSpeed Highest speed reached by acceleration.
 * @param pParam Brake parameters, or nullptr to use the defaults.
 */
ActorRailBrakeMover::ActorRailBrakeMover(al::LiveActor* pActor, float maxSpeed,
                                         const ActorRailBrakeMoverParam* pParam)
    : mActor(pActor), mSpeed(0.0f), mMaxSpeed(maxSpeed), mLength(0.0f), mRateStep(1.0f),
      mRate(0.0f), mParam(pParam != nullptr ? pParam : &sDefaultParam) {}

/** @brief Moves the actor along its rail, accelerating and braking before the goal.
 * @return Whether the actor reached the end of the rail.
 */
bool ActorRailBrakeMover::moveSyncRailBrake() {
    if (al::isLoopRail(mActor)) {
        al::moveSyncRailLoop(mActor, mMaxSpeed);
        return false;
    }

    float toGoalLength = al::calcRailToGoalLength(mActor);
    float accel = mParam->mAccel;
    float brakeLength = mParam->mBrakeLength;
    float brake = mParam->mBrake;
    if (al::getRailTotalLength(mActor) < mParam->mShortRailLength) {
        accel = mParam->mShortAccel;
        brakeLength = mParam->mShortBrakeLength;
        brake = mParam->mShortBrake;
    }

    if (al::getRailTotalLength(mActor) < brakeLength) {
        return al::moveSyncRail(mActor, mMaxSpeed);
    }

    if (toGoalLength < brakeLength) {
        mSpeed = sead::Mathf::max(mParam->mMinSpeed, mSpeed - brake);
    } else if (brakeLength < toGoalLength && mSpeed < mMaxSpeed) {
        mSpeed = sead::Mathf::min(accel + mSpeed, mMaxSpeed);
    }

    if (al::moveSyncRail(mActor, mSpeed)) {
        mSpeed = 0.0f;
        return true;
    }

    return false;
}

/** @brief Advances the rail position by time with an ease-in curve.
 * @return Whether the end of the movement was reached.
 */
bool ActorRailBrakeMover::moveSyncRailByTime() {
    mRate += mRateStep;
    bool isEnd = false;
    if (mRate >= 1.0f) {
        mRate = 1.0f;
        isEnd = true;
    }

    al::setRailPosToCoord(mActor, al::easeIn(mRate) * mLength);
    al::syncRailTrans(mActor);
    return isEnd;
}

/** @brief Gets the rail position for the current rate.
 * @param pPos Output position.
 */
void ActorRailBrakeMover::getPosition(sead::Vector3f* pPos) {
    al::setRailPosToCoord(mActor, al::easeInOut(mRate) * mLength);
    *pPos = al::getRailPos(mActor);
}

/** @brief Gets the rail position for a rate of the remaining rail length.
 * @param pPos Output position.
 * @param rate Rate along the remaining rail length.
 */
void ActorRailBrakeMover::getPosition(sead::Vector3f* pPos, float rate) {
    al::setRailPosToCoord(mActor, al::easeInOut(rate) * al::calcRailToGoalLength(mActor));
    *pPos = al::getRailPos(mActor);
}

/** @brief Advances the rail position by time, applying each rail part's accelerations.
 * @param pActor Actor whose rail part data is used.
 * @param pPartIndex Output index of the current rail part.
 * @param pPartRate Output rate within the current rail part.
 * @param isSyncTrans Whether to sync the actor's translation to the rail.
 * @return Whether the end of the movement was reached.
 */
bool ActorRailBrakeMover::moveSyncRailByTimeInOut(al::LiveActor* pActor, int* pPartIndex,
                                                  float* pPartRate, bool isSyncTrans) {
    float accelStart = 0.0f;
    float accelEnd = 0.0f;
    mRate += mRateStep;
    *pPartIndex = al::getRailPartIndex(pActor);
    al::getRailPartAccels(pActor, *pPartIndex, &accelStart, &accelEnd);
    float partRate =
        al::getRailPartRate(pActor, *pPartIndex, al::easeInOut(mRate) * mLength);
    *pPartRate = partRate;
    mRate += accelStart * (1.0f - partRate) + accelEnd * partRate;
    bool isEnd = false;
    if (mRate >= 1.0f) {
        mRate = 1.0f;
        isEnd = true;
    }

    al::setRailPosToCoord(mActor, al::easeInOut(mRate) * mLength);
    if (isSyncTrans) {
        al::syncRailTrans(mActor);
    }

    return isEnd;
}

/** @brief Sets the rail position from a rate with an ease-in-out curve.
 * @param pActor Unused.
 * @param rate Rate along the movement length.
 * @return Whether the end of the movement was reached.
 */
bool ActorRailBrakeMover::moveSyncRailByPos(al::LiveActor* pActor, float rate) {
    mRate = rate;
    bool isEnd = false;
    if (mRate >= 1.0f) {
        mRate = 1.0f;
        isEnd = true;
    }

    al::setRailPosToCoord(mActor, al::easeInOut(mRate) * mLength);
    return isEnd;
}

/** @brief Stops the mover. */
void ActorRailBrakeMover::resetSpeed() {
    mSpeed = 0.0f;
}

/** @brief Sets up a timed movement over the remaining rail length.
 * @param time Number of steps the movement takes.
 * @param isStop Whether to set up the movement without advancing.
 */
void ActorRailBrakeMover::setSpeedByTime(int time, bool isStop) {
    mLength = al::calcRailToGoalLength(mActor);
    mRate = 0.0f;
    float rateStep = isStop ? 0.0f : 1.0f / time;
    mRateStep = rateStep;
    mSpeed = mLength * rateStep;
}
