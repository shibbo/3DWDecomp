#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>

#include "Player/PlayerAction.hpp"

class IUsePlayerAttack;
class IUsePlayerCollisionSize;
class PlayerFigureDirector;
class PlayerTrigger;
struct PlayerActionArg;

/// The player's rolling attack action.
class PlayerActionRollingAttack : public PlayerAction {
    SEAD_RTTI_OVERRIDE(PlayerActionRollingAttack, PlayerAction)

public:
    /// The tuning values of one kind of rolling attack.
    class IUseRollingAttackParam {
    public:
        virtual f32 getAttackVel() const = 0;
        virtual f32 getJumpPow() const = 0;
        virtual f32 getGravity() const = 0;
        virtual f32 getFallSpeedMax() const = 0;
        virtual const char* getAnimName() const = 0;
        virtual f32 getAnimRate() const { return 1.0f; }
        virtual const char* getClimbAnimName() const = 0;
    };

    PlayerActionRollingAttack(const PlayerActionArg* pArg,
                              IUsePlayerCollisionSize* pCollisionSize,
                              const PlayerTrigger* pTrigger, const IUseRollingAttackParam* pParam,
                              const PlayerFigureDirector* pFigureDirector,
                              IUsePlayerAttack* pAttack);

    void move() override;
    void update() override;
    void setup() override;
    void teardown() override;

    void controlHVelocity(sead::Vector3f& rVelocity);

private:
    const PlayerActionArg* mArg;                    // 0x8
    IUsePlayerCollisionSize* mCollisionSize;        // 0x10
    const IUseRollingAttackParam* mParam;           // 0x18
    const PlayerTrigger* mTrigger;                  // 0x20
    const PlayerFigureDirector* mFigureDirector;    // 0x28
    IUsePlayerAttack* mAttack;                      // 0x30
    bool mIsTailAttack;                             // 0x38
};
