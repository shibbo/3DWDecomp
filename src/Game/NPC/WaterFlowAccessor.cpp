#include "NPC/WaterFlowAccessor.hpp"
#include "Player/BindPriority.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"

/**
 * @brief Supplies zero flow for the default water-flow accessor.
 * @param pSpeed Receives the flow velocity.
 * @param rPosition Query position; unused by the default accessor.
 */
void WaterFlowAccessor::calcSpeed(sead::Vector3f* pSpeed, const sead::Vector3f& rPosition) const {
    pSpeed->set(0.0f, 0.0f, 0.0f);
}

/**
 * @brief Ranks a sensor's player-binding category.
 * @param pSensor Sensor to rank, or nullptr for no candidate.
 * @return Priority from 1 to 9, or -1 for an absent or unrecognized sensor.
 */
int rc::getSensorPriority(const al::HitSensor* pSensor) {
    if (!pSensor) {
        return -1;
    }
    if (al::isSensorBindableGoalItem(pSensor)) {
        return 9;
    }
    if (al::isSensorBindableGigaBell(pSensor)) {
        return 8;
    }
    if (al::isSensorBindableGoal(pSensor)) {
        return 7;
    }
    if (al::isSensorBindableAllPlayer(pSensor)) {
        return 6;
    }
    if (al::isSensorBindableBubbleOutScreen(pSensor)) {
        return 5;
    }
    if (al::isSensorBindableKoura(pSensor)) {
        return 4;
    }
    if (al::isSensorBindableRouteDokan(pSensor)) {
        return 3;
    }
    if (al::isSensorBindableBubblePadInput(pSensor)) {
        return 2;
    }
    return al::isSensorBindable(pSensor) ? 1 : -1;
}

/** @brief Constructs a binding-priority comparator without initializing its unused table. */
BindPriority::BindPriority() {}

/**
 * @brief Checks whether a candidate should replace the current binding sensor.
 * @param pSensor Candidate sensor.
 * @param pOther Current binding sensor, or nullptr if there is none.
 * @return Whether the candidate wins by category or, for equal categories, sensor time.
 */
bool BindPriority::isGreater(const al::HitSensor* pSensor, const al::HitSensor* pOther) const {
    if (pSensor == pOther) {
        return false;
    }
    if (!pOther) {
        return true;
    }
    if (al::isSensorBindable(pSensor) && al::isSensorBindable(pOther)) {
        return false;
    }
    if (al::isSensorBindableRouteDokan(pSensor) && al::isSensorBindableRouteDokan(pOther)) {
        return false;
    }
    const int priority = rc::getSensorPriority(pSensor);
    const int otherPriority = rc::getSensorPriority(pOther);
    const u64 otherTime = static_cast<u64>(al::getSensorTime(pOther));
    const u64 time = static_cast<u64>(al::getSensorTime(pSensor));
    if (otherPriority < priority) {
        return true;
    }
    return otherPriority == priority && otherTime < time;
}
