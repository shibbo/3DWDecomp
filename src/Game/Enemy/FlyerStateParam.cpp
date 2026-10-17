#include "Enemy/FlyerStateParam.hpp"

/** @brief Creates default flight damping parameters. */
FlyerStateParam::FlyerStateParam() : mFriction(0.9f), mHeightDamping(0.9f) {}

/** @brief Creates custom flight damping parameters.
 * @param friction Velocity damping factor.
 * @param heightDamping Fraction of the height difference retained each step.
 */
FlyerStateParam::FlyerStateParam(float friction, float heightDamping)
    : mFriction(friction), mHeightDamping(heightDamping) {}
