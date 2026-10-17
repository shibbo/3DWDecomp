#pragma once
#include "Library/Nerve/NerveStateBase.hpp"

namespace al { class SensorMsg; class HitSensor; }
class KoopaChaseStateThrow;
class KoopaChaseStateDamage;

// Common virtual interface reconstructed from both battle-level vtables.
class KoopaChaseBattle : public al::NerveStateBase {
public:
    KoopaChaseBattle(const char* pName) : al::NerveStateBase(pName) {}

    virtual bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) = 0;
    virtual bool isStateTurnToTarget() const = 0;
    virtual bool isStateUpdatePose() const = 0;
    virtual bool isStateUpdateMoveCount() const = 0;
    virtual bool isStateTireMove() const = 0;
    virtual bool isStateWarp() const = 0;
    virtual KoopaChaseStateThrow* getStateThrow() const = 0;
    virtual KoopaChaseStateDamage* getStateDamage() const = 0;
};
