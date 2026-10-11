#pragma once

class PlayerAction;
class PlayerActionGraph;

/// Queries about which kind of action the player's action graph is currently running.
namespace PlayerActionTypeFunc {
bool isDeadAction(const PlayerActionGraph* pGraph);
bool isInBind(const PlayerActionGraph* pGraph);
bool isJumpAction(const PlayerActionGraph* pGraph);
bool isRaccoonDogFallAction(const PlayerActionGraph* pGraph);
bool isAbyssAction(const PlayerActionGraph* pGraph);
bool isHipDropAction(const PlayerActionGraph* pGraph, bool isIncludeStatue);
bool isWaterAction(const PlayerActionGraph* pGraph);
bool isWaterSurfaceAction(const PlayerActionGraph* pGraph);
bool isWallAction(const PlayerActionGraph* pGraph);
bool isWallSnapAction(const PlayerActionGraph* pGraph);
bool isStatueAction(const PlayerActionGraph* pGraph);
bool isSquatWalkAction(const PlayerActionGraph* pGraph);
bool isSquatAction(const PlayerActionGraph* pGraph);
bool isWatchableAction(const PlayerActionGraph* pGraph);
bool isSlidingAction(const PlayerActionGraph* pGraph);
bool isTossableAction(const PlayerActionGraph* pGraph);
bool isWaitOrPivotAction(const PlayerActionGraph* pGraph);
bool isClimbAttackAction(const PlayerActionGraph* pGraph);
bool isBodyAttackAction(const PlayerActionGraph* pGraph);
bool isRollingAction(const PlayerActionGraph* pGraph);
bool isTrampleJumpAction(const PlayerActionGraph* pGraph);
bool isDashResetAction(const PlayerActionGraph* pGraph);
bool isKnockDownAction(const PlayerActionGraph* pGraph);
bool isPunchedJumpAction(const PlayerActionGraph* pGraph);
bool isRollingOnGround(const PlayerActionGraph* pGraph);
bool isWait(const PlayerActionGraph* pGraph);
bool isGroundMove(const PlayerActionGraph* pGraph);
bool isActionHoldable(const PlayerActionGraph* pGraph, bool isAllowStandSwimSurface);
bool isActionForceRelease(const PlayerActionGraph* pGraph, bool isAllowStandSwimSurface);
bool isSpinAttackJump(const PlayerActionGraph* pGraph);
bool isActionUseIK(const PlayerActionGraph* pGraph);
bool isActionConnectToKnockDown(const PlayerActionGraph* pGraph);
bool isActionLongJump(const PlayerActionGraph* pGraph);
bool isActionBrake(const PlayerActionGraph* pGraph);
bool isActionTurn(const PlayerActionGraph* pGraph);
bool isAirMoveAction(const PlayerActionGraph* pGraph);
bool isAirMoveAction(const PlayerAction* pAction);
}  // namespace PlayerActionTypeFunc
