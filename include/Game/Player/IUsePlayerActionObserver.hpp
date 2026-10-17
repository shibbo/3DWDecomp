#pragma once

/** @brief Reports the player action state through the action observer interface. */
class IUsePlayerActionObserver {
public:
    virtual bool isOnGround() const = 0;
    virtual bool isDead() const = 0;
    virtual bool isAbyss() const = 0;
    virtual bool isJumpTrig() const = 0;
    virtual bool isInBind() const = 0;
    virtual bool isDeadDemo() const = 0;
    virtual bool isChangeDemo() const = 0;
    virtual bool isHipDropping() const = 0;
    virtual bool isRollingOnGround() const = 0;
    virtual bool isWaterAction() const = 0;
    virtual bool isWaterSurfaceAction() const = 0;
    virtual bool isFallBrakeTrig() const = 0;
    virtual bool isWallAction() const = 0;
    virtual bool isWallSnapAction() const = 0;
    virtual bool isChangingStatue() const = 0;
    virtual bool isSquatWalkAction() const = 0;
    virtual bool isSquatAction() const = 0;
    virtual bool isPossibleToWatch() const = 0;
    virtual bool isSliding() const = 0;
    virtual bool isTossableAction() const = 0;
    virtual bool isWaitOrPivot() const = 0;
    virtual bool isTurn() const = 0;
};
