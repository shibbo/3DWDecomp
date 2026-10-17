#pragma once

#include <basis/seadTypes.h>

#include "Player/Giga/PlayerActionRolling.hpp"
#include "Player/Giga/PlayerActionRollingAttack.hpp"

class PlayerConstParam;

/// The tuning values of rolling from a standstill.
class WaitRollingParam : public PlayerActionRolling::IUseRollingParam {
public:
    WaitRollingParam(const PlayerConstParam* pConstParam) : mConstParam(pConstParam) {}

    f32 getMinSpeed() const override;
    f32 getNoBrakeFrame() const override;
    f32 getBrakeRate() const override;
    f32 getSideBrakeRate() const override;
    f32 getSideAccel() const override;
    f32 getSideMaxSpeed() const override;
    const char* getAnimName() const override;
    const char* getClimbAnimName() const override;
    const char* getAnimNameWithEquipment() const override;

private:
    const PlayerConstParam* mConstParam;  // 0x8
};

/// The tuning values of rolling while walking.
class NormalRollingParam : public PlayerActionRolling::IUseRollingParam {
public:
    NormalRollingParam(const PlayerConstParam* pConstParam) : mConstParam(pConstParam) {}

    f32 getMinSpeed() const override;
    f32 getNoBrakeFrame() const override;
    f32 getBrakeRate() const override;
    f32 getSideBrakeRate() const override;
    f32 getSideAccel() const override;
    f32 getSideMaxSpeed() const override;
    const char* getAnimName() const override;
    const char* getClimbAnimName() const override;
    const char* getAnimNameWithEquipment() const override;

private:
    const PlayerConstParam* mConstParam;  // 0x8
};

/// The tuning values of rolling in mid-air.
class AirRollingParam : public PlayerActionRolling::IUseRollingParam {
public:
    AirRollingParam(const PlayerConstParam* pConstParam) : mConstParam(pConstParam) {}

    f32 getMinSpeed() const override;
    f32 getNoBrakeFrame() const override;
    f32 getBrakeRate() const override;
    f32 getSideBrakeRate() const override;
    f32 getSideAccel() const override;
    f32 getSideMaxSpeed() const override;
    const char* getAnimName() const override;
    const char* getClimbAnimName() const override;
    const char* getAnimNameWithEquipment() const override;
    f32 getGravityAddition() const override;

    f32 getJumpPow() const;

private:
    const PlayerConstParam* mConstParam;  // 0x8
};

/// The tuning values of rolling while dashing.
class DashRollingParam : public PlayerActionRolling::IUseRollingParam {
public:
    DashRollingParam(const PlayerConstParam* pConstParam) : mConstParam(pConstParam) {}

    f32 getMinSpeed() const override;
    f32 getNoBrakeFrame() const override;
    f32 getBrakeRate() const override;
    f32 getSideBrakeRate() const override;
    f32 getSideAccel() const override;
    f32 getSideMaxSpeed() const override;
    const char* getAnimName() const override;
    const char* getClimbAnimName() const override;
    const char* getAnimNameWithEquipment() const override;

private:
    const PlayerConstParam* mConstParam;  // 0x8
};

/// The tuning values of the raccoon dog form's roll from a standstill.
class RaccoonDogWaitRollingParam : public PlayerActionRolling::IUseRollingParam {
public:
    RaccoonDogWaitRollingParam(const PlayerConstParam* pConstParam) : mConstParam(pConstParam) {}

    f32 getMinSpeed() const override;
    f32 getNoBrakeFrame() const override;
    f32 getBrakeRate() const override;
    f32 getSideBrakeRate() const override;
    f32 getSideAccel() const override;
    f32 getSideMaxSpeed() const override;
    const char* getAnimName() const override;
    const char* getClimbAnimName() const override;
    const char* getAnimNameWithEquipment() const override;

private:
    const PlayerConstParam* mConstParam;  // 0x8
};

/// The tuning values of the raccoon dog form's roll while walking.
class RaccoonDogNormalRollingParam : public PlayerActionRolling::IUseRollingParam {
public:
    RaccoonDogNormalRollingParam(const PlayerConstParam* pConstParam) : mConstParam(pConstParam) {}

    f32 getMinSpeed() const override;
    f32 getNoBrakeFrame() const override;
    f32 getBrakeRate() const override;
    f32 getSideBrakeRate() const override;
    f32 getSideAccel() const override;
    f32 getSideMaxSpeed() const override;
    const char* getAnimName() const override;
    const char* getClimbAnimName() const override;
    const char* getAnimNameWithEquipment() const override;

private:
    const PlayerConstParam* mConstParam;  // 0x8
};

/// The tuning values of the raccoon dog form's roll while dashing.
class RaccoonDogDashRollingParam : public PlayerActionRolling::IUseRollingParam {
public:
    RaccoonDogDashRollingParam(const PlayerConstParam* pConstParam) : mConstParam(pConstParam) {}

    f32 getMinSpeed() const override;
    f32 getNoBrakeFrame() const override;
    f32 getBrakeRate() const override;
    f32 getSideBrakeRate() const override;
    f32 getSideAccel() const override;
    f32 getSideMaxSpeed() const override;
    const char* getAnimName() const override;
    const char* getClimbAnimName() const override;
    const char* getAnimNameWithEquipment() const override;

private:
    const PlayerConstParam* mConstParam;  // 0x8
};

/// The tuning values of the rolling attack out of a dash roll.
class DashRollingAttackParam : public PlayerActionRollingAttack::IUseRollingAttackParam {
public:
    DashRollingAttackParam(const PlayerConstParam* pConstParam) : mConstParam(pConstParam) {}

    f32 getAttackVel() const override;
    f32 getJumpPow() const override;
    f32 getGravity() const override;
    f32 getFallSpeedMax() const override;
    const char* getAnimName() const override;
    const char* getClimbAnimName() const override;

private:
    const PlayerConstParam* mConstParam;  // 0x8
};

/// The tuning values of the rolling attack out of a walking roll.
class NormalRollingAttackParam : public PlayerActionRollingAttack::IUseRollingAttackParam {
public:
    NormalRollingAttackParam(const PlayerConstParam* pConstParam) : mConstParam(pConstParam) {}

    f32 getAttackVel() const override;
    f32 getJumpPow() const override;
    f32 getGravity() const override;
    f32 getFallSpeedMax() const override;
    const char* getAnimName() const override;
    const char* getClimbAnimName() const override;

private:
    const PlayerConstParam* mConstParam;  // 0x8
};

/// The tuning values of the rolling attack out of a standing roll.
class WaitRollingAttackParam : public PlayerActionRollingAttack::IUseRollingAttackParam {
public:
    WaitRollingAttackParam(const PlayerConstParam* pConstParam) : mConstParam(pConstParam) {}

    f32 getAttackVel() const override;
    f32 getJumpPow() const override;
    f32 getGravity() const override;
    f32 getFallSpeedMax() const override;
    const char* getAnimName() const override;
    const char* getClimbAnimName() const override;

private:
    const PlayerConstParam* mConstParam;  // 0x8
};

/// The tuning values of the raccoon dog form's rolling attack.
class RaccoonDogWaitRollingAttackParam : public PlayerActionRollingAttack::IUseRollingAttackParam {
public:
    RaccoonDogWaitRollingAttackParam(const PlayerConstParam* pConstParam)
        : mConstParam(pConstParam) {}

    f32 getAttackVel() const override;
    f32 getJumpPow() const override;
    f32 getGravity() const override;
    f32 getFallSpeedMax() const override;
    const char* getAnimName() const override;
    const char* getClimbAnimName() const override;

private:
    const PlayerConstParam* mConstParam;  // 0x8
};

/// The tuning values of the raccoon dog form's high rolling attack.
class RaccoonDogWaitRollingAttackHighParam
    : public PlayerActionRollingAttack::IUseRollingAttackParam {
public:
    RaccoonDogWaitRollingAttackHighParam(const PlayerConstParam* pConstParam)
        : mConstParam(pConstParam) {}

    f32 getAttackVel() const override;
    f32 getJumpPow() const override;
    f32 getGravity() const override;
    f32 getFallSpeedMax() const override;
    const char* getAnimName() const override;
    const char* getClimbAnimName() const override;

private:
    const PlayerConstParam* mConstParam;  // 0x8
};
