#pragma once

#include <math/seadVector.h>

#include "Player/IUsePlayerSquatEnergy.hpp"
#include "Player/PlayerAction.hpp"

class IUsePlayerCeilingCheck;
class IUsePlayerCollisionSize;
struct PlayerActionArg;

/// The player's squat walk action: walking while squatting, charging a back jump and turning.
class PlayerActionSquatWalk : public PlayerAction, public IUsePlayerSquatEnergy {
    SEAD_RTTI_OVERRIDE(PlayerActionSquatWalk, PlayerAction)

public:
    /// The steps of the squat walk (also the index into getAnimNames()).
    enum class EState : u32 {
        Start = 0,
        Wait = 1,
        JumpReady = 2,
        Walk = 3,
        Turn = 4,
    };

    /// The current step and whether it changed this frame.
    struct StateHolder {
        /**
         * @brief Creates the holder.
         * @param state The initial step.
         */
        explicit StateHolder(EState state) : mState(state) {}

        /**
         * @brief Sets the step without flagging a change.
         * @param state The new step.
         */
        void reset(EState state) {
            mState = state;
            mIsChanged = false;
        }

        /**
         * @brief Changes the step, flagging a change if it differs from the current one.
         * @param state The new step.
         */
        void change(EState state) {
            if (mState != state) {
                mState = state;
                mIsChanged = true;
            }
        }

        EState mState;            // 0x0
        bool mIsChanged = false;  // 0x4
    };

    PlayerActionSquatWalk(const PlayerActionArg* pArg, const IUsePlayerCeilingCheck* pCeilingCheck,
                          IUsePlayerCollisionSize* pCollisionSize);

    void move() override;
    void update() override;
    void setup() override;
    void teardown() override;

    /** @brief Gets how charged the squat is, 0 to 1. */
    f32 getEnergy() const override { return mEnergy; }

    virtual f32 getSquatWalkSpeed();
    virtual const char* getSquatChargeSoundName() const;
    virtual const char* const* getAnimNames() const;
    virtual bool setupSquatStartAnim();

    void controlState();
    void updateAnimation();
    void updateEffectAndSound();
    void updateWalk();
    bool isStartEnd() const;
    bool checkShiftWalk();
    bool checkShiftWait();
    void updateEnergy();
    bool checkShiftTurn();
    bool checkShiftJumpReady();
    void updateTurn();
    bool isTurnEnd() const;
    void divideVelocity(sead::Vector3f* pHorizontal, sead::Vector3f* pVertical) const;

protected:
    IUsePlayerCollisionSize* mCollisionSize;       // 0x10
    const IUsePlayerCeilingCheck* mCeilingCheck;  // 0x18
    f32 mEnergy = 0.0f;                           // 0x20
    StateHolder* mState;                          // 0x28
    bool mIsEnergyFull = false;                   // 0x30
    u32 mTurnFrame = 0;                           // 0x34
    sead::Vector3f mTurnDir;                      // 0x38
    f32 mTurnAngle;                               // 0x44
    const PlayerActionArg* mArg;                  // 0x48
};
