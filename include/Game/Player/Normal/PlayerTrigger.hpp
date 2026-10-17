#pragma once

#include <basis/seadTypes.h>

/// Collects the one-frame events (sensor hits and collision touches) the player got.
class PlayerTrigger {
public:
    /// Sensor events; cSensorTriggerNum means none.
    enum ESensorTrigger { cDamage = 0, cTrample = 1, cSensorTriggerNum = 23 };
    /// Collision events; cCollisionTriggerNum means none.
    enum ECollisionTrigger { cCollisionDamage = 4, cCollisionTriggerNum = 6 };

    PlayerTrigger();
    bool isOn(ESensorTrigger) const;
    bool isOn(ECollisionTrigger) const;
    void set(ESensorTrigger);
    void set(ECollisionTrigger);
    void clearSensorTrigger();
    void clearCollisionTrigger();

private:
    u32 mSensorTrigger;     // 0x0, one bit per ESensorTrigger
    u32 mCollisionTrigger;  // 0x4, one bit per ECollisionTrigger
};
