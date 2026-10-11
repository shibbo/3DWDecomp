#include "Enemy/WalkerStateParam.hpp"

/** @brief Initializes the default walking enemy motion parameters. */
WalkerStateParam::WalkerStateParam()
    : mGravity(1.0f), mAirFriction(0.99f), mGroundFriction(0.93f), mValueC(150.0f) {}

/**
 * @brief Initializes walking parameters from the original eight-argument interface.
 * @param gravity Downward acceleration.
 * @param airFriction Velocity multiplier while airborne.
 * @param groundFriction Velocity multiplier while grounded.
 * @param unused1 Unused parameter retained by the original interface.
 * @param unused2 Unused parameter retained by the original interface.
 * @param unused3 Unused parameter retained by the original interface.
 * @param unused4 Unused parameter retained by the original interface.
 * @param valueC Final parameter whose purpose is not yet identified.
 */
WalkerStateParam::WalkerStateParam(float gravity, float airFriction, float groundFriction,
        float unused1, float unused2, float unused3, float unused4, float valueC)
    : mGravity(gravity), mAirFriction(airFriction), mGroundFriction(groundFriction), mValueC(valueC) {}
