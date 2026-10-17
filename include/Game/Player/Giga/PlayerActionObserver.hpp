#pragma once

#include "Game/Player/IUsePlayerActionObserver.hpp"

class IUsePlayerCollision;
class IUsePlayerLifeControl;
class IUsePlayerModelChanger;
class IUsePlayerSubAction;
class Player;
class PlayerActionGraph;

/// Answers questions about what the player's action graph is currently doing.
class PlayerActionObserver : public IUsePlayerActionObserver {
public:
    PlayerActionObserver(const Player*, PlayerActionGraph*, IUsePlayerCollision*,
                         const IUsePlayerModelChanger*, const IUsePlayerSubAction*,
                         const IUsePlayerLifeControl*);

    void update();

    bool isOnGround() const override;
    bool isDead() const override;
    bool isAbyss() const override;
    bool isJumpTrig() const override;
    bool isInBind() const override;
    bool isDeadDemo() const override;
    bool isChangeDemo() const override;
    bool isHipDropping() const override;
    bool isRollingOnGround() const override;
    bool isWaterAction() const override;
    bool isWaterSurfaceAction() const override;
    bool isFallBrakeTrig() const override;
    bool isWallAction() const override;
    bool isWallSnapAction() const override;
    bool isChangingStatue() const override;
    bool isSquatWalkAction() const override;
    bool isSquatAction() const override;
    bool isPossibleToWatch() const override;
    bool isSliding() const override;
    bool isTossableAction() const override;
    bool isWaitOrPivot() const override;
    bool isTurn() const override;
};
