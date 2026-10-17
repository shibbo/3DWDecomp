#pragma once

#include <basis/seadTypes.h>

#include "Player/PlayerAction.hpp"

/// The player's rolling action.
class PlayerActionRolling : public PlayerAction {
    SEAD_RTTI_OVERRIDE(PlayerActionRolling, PlayerAction)

public:
    /// The tuning values of one kind of roll.
    class IUseRollingParam {
    public:
        virtual f32 getMinSpeed() const = 0;
        virtual f32 getNoBrakeFrame() const = 0;
        virtual f32 getBrakeRate() const = 0;
        virtual f32 getSideBrakeRate() const = 0;
        virtual f32 getSideAccel() const = 0;
        virtual f32 getSideMaxSpeed() const = 0;
        virtual const char* getAnimName() const = 0;
        virtual f32 getAnimRate() const { return 1.0f; }
        virtual const char* getClimbAnimName() const = 0;
        virtual const char* getAnimNameWithEquipment() const = 0;
        virtual f32 getGravityAddition() const { return 0.0f; }
    };
};
