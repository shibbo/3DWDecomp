#include "Player/Normal/PlayerActionTypeFunc.hpp"

#include "CourseSelect/CourseSelectPlayerActionFollow.hpp"
#include "CourseSelect/CourseSelectPlayerActionGroundMove.hpp"
#include "CourseSelect/CourseSelectPlayerActionWait.hpp"
#include "Player/Giga/PlayerActionGraph.hpp"
#include "Player/Giga/PlayerActionGigaWallHit.hpp"
#include "Player/Giga/PlayerActionGroundMove.hpp"
#include "Player/Giga/PlayerActionGroundWall.hpp"
#include "Player/Giga/PlayerActionHipDrop.hpp"
#include "Player/Giga/PlayerActionInvincibleJump.hpp"
#include "Player/Giga/PlayerActionJump.hpp"
#include "Player/Giga/PlayerActionKnockDown.hpp"
#include "Player/Giga/PlayerActionLand.hpp"
#include "Player/Giga/PlayerActionLongJump.hpp"
#include "Player/Giga/PlayerActionManekinekoDrop.hpp"
#include "Player/Giga/PlayerActionNormalDie.hpp"
#include "Player/Giga/PlayerActionPivot.hpp"
#include "Player/Giga/PlayerActionPunchedJump.hpp"
#include "Player/Giga/PlayerActionRaccoonDogFall.hpp"
#include "Player/Giga/PlayerActionRolling.hpp"
#include "Player/Giga/PlayerActionRollingAttack.hpp"
#include "Player/Normal/PlayerActionSkateJump.hpp"
#include "Player/Normal/PlayerActionSlide.hpp"
#include "Player/Normal/PlayerActionSlideFall.hpp"
#include "Player/Normal/PlayerActionSlideJump.hpp"
#include "Player/Normal/PlayerActionSpinAttackJump.hpp"
#include "Player/Normal/PlayerActionSpinJump.hpp"
#include "Player/Normal/PlayerActionSquatBrake.hpp"
#include "Player/Normal/PlayerActionSquatEnd.hpp"
#include "Player/Normal/PlayerActionSquatJump.hpp"
#include "Player/Normal/PlayerActionSquatLand.hpp"
#include "Player/Normal/PlayerActionSquatWalk.hpp"
#include "Player/Normal/PlayerActionStandSwim.hpp"
#include "Player/Normal/PlayerActionStandSwimAfterBind.hpp"
#include "Player/Normal/PlayerActionStandSwimClimbAttack.hpp"
#include "Player/Normal/PlayerActionStandSwimSurface.hpp"
#include "Player/Normal/PlayerActionStoneStatue.hpp"
#include "Player/Normal/PlayerActionSurfaceSwim.hpp"
#include "Player/Normal/PlayerActionSwimDive.hpp"
#include "Player/Normal/PlayerActionSwimJump.hpp"
#include "Player/Normal/PlayerActionSwimSquat.hpp"
#include "Player/Normal/PlayerActionTrampleJump.hpp"
#include "Player/Normal/PlayerActionTurn.hpp"
#include "Player/Normal/PlayerActionTurnJump.hpp"
#include "Player/Normal/PlayerActionWait.hpp"
#include "Player/Normal/PlayerActionWall.hpp"
#include "Player/Normal/PlayerActionWallClimb.hpp"
#include "Player/Normal/PlayerActionWallClimbReady.hpp"
#include "Player/Normal/PlayerActionWallClimbReadyFromSlope.hpp"
#include "Player/Normal/PlayerActionWallClimbSlide.hpp"
#include "Player/Normal/PlayerActionWallJump.hpp"
#include "Player/PlayerActionAbyss.hpp"
#include "Player/PlayerActionAirMove.hpp"
#include "Player/PlayerActionBind.hpp"
#include "Player/PlayerActionBindJump.hpp"
#include "Player/PlayerActionBodyAttack.hpp"
#include "Player/PlayerActionBrake.hpp"
#include "Player/PlayerActionClimbAirStop.hpp"
#include "Player/PlayerActionDive.hpp"
#include "Player/PlayerActionDiveTrample.hpp"
#include "Player/PlayerActionFall.hpp"
#include "Player/PlayerActionGigaClimbAirStop.hpp"
#include "Player/PlayerActionGigaHipDrop.hpp"

namespace {
/**
 * @brief Checks whether an action is of the given type or derives from it.
 * @param pAction The action to check (may be null).
 * @return true if the action is a T.
 */
template <typename T>
inline bool isType(const PlayerAction* pAction) {
    return sead::IsDerivedFrom<T>(pAction);
}
}  // namespace

namespace PlayerActionTypeFunc {

/**
 * @brief Checks whether the player is dying or falling into the abyss.
 * @param pGraph The player's action graph.
 * @return true if the player is dying or falling into the abyss.
 */
bool isDeadAction(const PlayerActionGraph* pGraph) {
    const PlayerAction* action = pGraph->getAction();

    return isType<PlayerActionNormalDie>(action) || isType<PlayerActionAbyss>(action);
}

/**
 * @brief Checks whether the player is bound by another object.
 * @param pGraph The player's action graph.
 * @return true if the player is bound by another object.
 */
bool isInBind(const PlayerActionGraph* pGraph) {
    return isType<PlayerActionBind>(pGraph->getAction());
}

/**
 * @brief Checks whether the player is in any kind of jump.
 * @param pGraph The player's action graph.
 * @return true if the current action is a jump.
 */
bool isJumpAction(const PlayerActionGraph* pGraph) {
    const PlayerAction* action = pGraph->getAction();

    if (isType<PlayerActionBindJump>(action)) {
        return static_cast<const PlayerActionBindJump*>(action)->isJumpAction();
    }

    return isType<PlayerActionJump>(action) || isType<PlayerActionTurnJump>(action) ||
           isType<PlayerActionLongJump>(action) || isType<PlayerActionWallJump>(action) ||
           isType<PlayerActionSquatJump>(action) || isType<PlayerActionRollingAttack>(action) ||
           isType<PlayerActionSwimJump>(action) || isType<PlayerActionInvincibleJump>(action) ||
           isType<PlayerActionSlideJump>(action) || isType<PlayerActionSpinJump>(action) ||
           isType<PlayerActionSkateJump>(action);
}

/**
 * @brief Checks whether the player is gliding down in the tanooki suit.
 * @param pGraph The player's action graph.
 * @return true if the player is gliding down in the tanooki suit.
 */
bool isRaccoonDogFallAction(const PlayerActionGraph* pGraph) {
    return isType<PlayerActionRaccoonDogFall>(pGraph->getAction());
}

/**
 * @brief Checks whether the player is falling into the abyss.
 * @param pGraph The player's action graph.
 * @return true if the player is falling into the abyss.
 */
bool isAbyssAction(const PlayerActionGraph* pGraph) {
    return isType<PlayerActionAbyss>(pGraph->getAction());
}

/**
 * @brief Checks whether the player is dropping down onto the ground.
 * @param pGraph The player's action graph.
 * @param isIncludeStatue Whether falling as a statue counts as a hip drop.
 * @return true if the current action is a hip drop or a dive.
 */
bool isHipDropAction(const PlayerActionGraph* pGraph, bool isIncludeStatue) {
    const PlayerAction* action = pGraph->getAction();

    return isType<PlayerActionHipDrop>(action) ||
           (isType<PlayerActionStoneStatue>(action) && isIncludeStatue) ||
           isType<PlayerActionDive>(action) || isType<PlayerActionSwimDive>(action) ||
           isType<PlayerActionGigaHipDrop>(action);
}

/**
 * @brief Checks whether the player is swimming under water.
 * @param pGraph The player's action graph.
 * @return true if the player is swimming under water.
 */
bool isWaterAction(const PlayerActionGraph* pGraph) {
    const PlayerAction* action = pGraph->getAction();

    return isType<PlayerActionStandSwim>(action) ||
           isType<PlayerActionStandSwimAfterBind>(action) ||
           isType<PlayerActionSwimDive>(action) || isType<PlayerActionDive>(action) ||
           isType<PlayerActionSwimSquat>(action) || isType<PlayerActionDiveTrample>(action);
}

/**
 * @brief Checks whether the player is swimming on the water surface.
 * @param pGraph The player's action graph.
 * @return true if the player is swimming on the water surface.
 */
bool isWaterSurfaceAction(const PlayerActionGraph* pGraph) {
    const PlayerAction* action = pGraph->getAction();

    return isType<PlayerActionSurfaceSwim>(action) || isType<PlayerActionStandSwimSurface>(action);
}

/**
 * @brief Checks whether the player is sliding down a wall.
 * @param pGraph The player's action graph.
 * @return true if the player is sliding down a wall.
 */
bool isWallAction(const PlayerActionGraph* pGraph) {
    return isType<PlayerActionWall>(pGraph->getAction());
}

/**
 * @brief Checks whether the player is snapped to a wall.
 * @param pGraph The player's action graph.
 * @return true if the player is snapped to a wall.
 */
bool isWallSnapAction(const PlayerActionGraph* pGraph) {
    const PlayerAction* action = pGraph->getAction();

    return isType<PlayerActionWall>(action) || isType<PlayerActionWallClimb>(action) ||
           isType<PlayerActionWallClimbReady>(action) ||
           isType<PlayerActionWallClimbReadyFromSlope>(action) ||
           isType<PlayerActionWallClimbSlide>(action);
}

/**
 * @brief Checks whether the player is a statue.
 * @param pGraph The player's action graph.
 * @return true if the player is a statue.
 */
bool isStatueAction(const PlayerActionGraph* pGraph) {
    const PlayerAction* action = pGraph->getAction();

    return isType<PlayerActionStoneStatue>(action) || isType<PlayerActionManekinekoDrop>(action);
}

/**
 * @brief Checks whether the player is walking while squatting.
 * @param pGraph The player's action graph.
 * @return true if the player is walking while squatting.
 */
bool isSquatWalkAction(const PlayerActionGraph* pGraph) {
    return isType<PlayerActionSquatWalk>(pGraph->getAction());
}

/**
 * @brief Checks whether the player is in a squatting action.
 * @param pGraph The player's action graph.
 * @return true if the player is in a squatting action.
 */
bool isSquatAction(const PlayerActionGraph* pGraph) {
    const PlayerAction* action = pGraph->getAction();

    return isType<PlayerActionRolling>(action) || isType<PlayerActionRollingAttack>(action) ||
           isType<PlayerActionSlide>(action) || isType<PlayerActionSlideFall>(action) ||
           isType<PlayerActionSquatBrake>(action) || isType<PlayerActionSquatJump>(action) ||
           isType<PlayerActionSquatWalk>(action);
}

/**
 * @brief Checks whether the player is calm enough to look around.
 * @param pGraph The player's action graph.
 * @return true if the player is calm enough to look around.
 */
bool isWatchableAction(const PlayerActionGraph* pGraph) {
    const PlayerAction* action = pGraph->getAction();

    return isType<PlayerActionGroundMove>(action) || isType<PlayerActionWait>(action) ||
           isType<PlayerActionStandSwim>(action) ||
           isType<PlayerActionStandSwimAfterBind>(action) ||
           isType<CourseSelectPlayerActionWait>(action) ||
           isType<CourseSelectPlayerActionGroundMove>(action) ||
           isType<CourseSelectPlayerActionFollow>(action);
}

/**
 * @brief Checks whether the player is sliding.
 * @param pGraph The player's action graph.
 * @return true if the player is sliding.
 */
bool isSlidingAction(const PlayerActionGraph* pGraph) {
    const PlayerAction* action = pGraph->getAction();

    return isType<PlayerActionSlide>(action) || isType<PlayerActionSlideFall>(action);
}

/**
 * @brief Checks whether the player can toss what it is holding.
 * @param pGraph The player's action graph.
 * @return true if the player can toss what it is holding.
 */
bool isTossableAction(const PlayerActionGraph* pGraph) {
    const PlayerAction* action = pGraph->getAction();

    return isType<PlayerActionSquatBrake>(action) || isType<PlayerActionSquatWalk>(action);
}

/**
 * @brief Checks whether the player is waiting or pivoting.
 * @param pGraph The player's action graph.
 * @return true if the player is waiting or pivoting.
 */
bool isWaitOrPivotAction(const PlayerActionGraph* pGraph) {
    const PlayerAction* action = pGraph->getAction();

    return isType<PlayerActionWait>(action) || isType<PlayerActionPivot>(action);
}

/**
 * @brief Checks whether the player is doing a climbing attack.
 * @param pGraph The player's action graph.
 * @return true if the player is doing a climbing attack.
 */
bool isClimbAttackAction(const PlayerActionGraph* pGraph) {
    const PlayerAction* action = pGraph->getAction();

    return isType<PlayerActionClimbAirStop>(action) ||
           isType<PlayerActionStandSwimClimbAttack>(action) ||
           isType<PlayerActionGigaClimbAirStop>(action);
}

/**
 * @brief Checks whether the player is doing a body attack.
 * @param pGraph The player's action graph.
 * @return true if the player is doing a body attack.
 */
bool isBodyAttackAction(const PlayerActionGraph* pGraph) {
    return isType<PlayerActionBodyAttack>(pGraph->getAction());
}

/**
 * @brief Checks whether the player is rolling.
 * @param pGraph The player's action graph.
 * @return true if the player is rolling.
 */
bool isRollingAction(const PlayerActionGraph* pGraph) {
    const PlayerAction* action = pGraph->getAction();

    return isType<PlayerActionRollingAttack>(action) || isType<PlayerActionRolling>(action) ||
           isType<PlayerActionLongJump>(action);
}

/**
 * @brief Checks whether the player is bouncing off a trampled enemy.
 * @param pGraph The player's action graph.
 * @return true if the player is bouncing off a trampled enemy.
 */
bool isTrampleJumpAction(const PlayerActionGraph* pGraph) {
    return isType<PlayerActionTrampleJump>(pGraph->getAction());
}

/**
 * @brief Checks whether the current action resets the player's dash.
 * @param pGraph The player's action graph.
 * @return true unless the player is moving on the ground, jumping or falling.
 */
bool isDashResetAction(const PlayerActionGraph* pGraph) {
    const PlayerAction* action = pGraph->getAction();

    return !(isType<PlayerActionGroundMove>(action) || isType<PlayerActionJump>(action) ||
             isType<PlayerActionFall>(action));
}

/**
 * @brief Checks whether the player is knocked down.
 * @param pGraph The player's action graph.
 * @return true if the player is knocked down.
 */
bool isKnockDownAction(const PlayerActionGraph* pGraph) {
    return isType<PlayerActionKnockDown>(pGraph->getAction());
}

/**
 * @brief Checks whether the player was punched into the air.
 * @param pGraph The player's action graph.
 * @return true if the player was punched into the air.
 */
bool isPunchedJumpAction(const PlayerActionGraph* pGraph) {
    return isType<PlayerActionPunchedJump>(pGraph->getAction());
}

/**
 * @brief Checks whether the player is rolling on the ground.
 * @param pGraph The player's action graph.
 * @return true if the player is rolling on the ground.
 */
bool isRollingOnGround(const PlayerActionGraph* pGraph) {
    return isType<PlayerActionRolling>(pGraph->getAction());
}

/**
 * @brief Checks whether the player is waiting.
 * @param pGraph The player's action graph.
 * @return true if the player is waiting.
 */
bool isWait(const PlayerActionGraph* pGraph) {
    return isType<PlayerActionWait>(pGraph->getAction());
}

/**
 * @brief Checks whether the player is moving on the ground.
 * @param pGraph The player's action graph.
 * @return true if the player is moving on the ground.
 */
bool isGroundMove(const PlayerActionGraph* pGraph) {
    return isType<PlayerActionGroundMove>(pGraph->getAction());
}

/**
 * @brief Checks whether the player can keep holding an object in the current action.
 * @param pGraph The player's action graph.
 * @param isAllowStandSwimSurface Whether swimming upright on the water surface allows holding
 *                                (also rejects the giant wall hit when set).
 * @return true if the current action allows holding an object.
 */
bool isActionHoldable(const PlayerActionGraph* pGraph, bool isAllowStandSwimSurface) {
    const PlayerAction* action = pGraph->getAction();

    if (isAllowStandSwimSurface) {
        if (isType<PlayerActionStandSwimSurface>(action)) {
            return true;
        }

        if (isType<PlayerActionGigaWallHit>(action)) {
            return false;
        }
    }

    if (isDeadAction(pGraph) ||
        isWaterAction(pGraph) || isWaterSurfaceAction(pGraph) ||
        isType<PlayerActionRolling>(action) || isType<PlayerActionRollingAttack>(action) ||
        isType<PlayerActionSquatBrake>(action) || isType<PlayerActionSquatEnd>(action) ||
        isType<PlayerActionSquatJump>(action) || isType<PlayerActionSquatLand>(action) ||
        isType<PlayerActionSquatWalk>(action) || isType<PlayerActionTurnJump>(action) ||
        isType<PlayerActionPunchedJump>(action) || isType<PlayerActionGroundWall>(action) ||
        isWallSnapAction(pGraph) || isBodyAttackAction(pGraph)) {
        return false;
    }

    return !isKnockDownAction(pGraph);
}

/**
 * @brief Checks whether the current action forces the player to let go of a held object.
 * @param pGraph The player's action graph.
 * @param isAllowStandSwimSurface Whether swimming upright on the water surface allows holding.
 * @return true if the held object has to be released.
 */
bool isActionForceRelease(const PlayerActionGraph* pGraph, bool isAllowStandSwimSurface) {
    const PlayerAction* action = pGraph->getAction();

    if (isType<PlayerActionPunchedJump>(action)) {
        return false;
    }

    if (isAllowStandSwimSurface && isType<PlayerActionStandSwimSurface>(action)) {
        return false;
    }

    return !isActionHoldable(pGraph, isAllowStandSwimSurface);
}

/**
 * @brief Checks whether the player is doing a spin attack jump.
 * @param pGraph The player's action graph.
 * @return true if the player is doing a spin attack jump.
 */
bool isSpinAttackJump(const PlayerActionGraph* pGraph) {
    return isType<PlayerActionSpinAttackJump>(pGraph->getAction());
}

/**
 * @brief Checks whether the current action places the feet with IK.
 * @param pGraph The player's action graph.
 * @return true if the current action places the feet with IK.
 */
bool isActionUseIK(const PlayerActionGraph* pGraph) {
    const PlayerAction* action = pGraph->getAction();

    return isType<PlayerActionGroundMove>(action) || isType<PlayerActionWait>(action) ||
           isType<PlayerActionLand>(action) || isType<CourseSelectPlayerActionWait>(action) ||
           isType<CourseSelectPlayerActionGroundMove>(action);
}

/**
 * @brief Checks whether the current action can shift into being knocked down.
 * @param pGraph The player's action graph.
 * @return true if the current action can shift into being knocked down.
 */
bool isActionConnectToKnockDown(const PlayerActionGraph* pGraph) {
    const PlayerAction* action = pGraph->getAction();

    return isType<PlayerActionGroundMove>(action) || isType<PlayerActionWait>(action) ||
           isType<PlayerActionPivot>(action);
}

/**
 * @brief Checks whether the player is long jumping.
 * @param pGraph The player's action graph.
 * @return true if the player is long jumping.
 */
bool isActionLongJump(const PlayerActionGraph* pGraph) {
    return isType<PlayerActionLongJump>(pGraph->getAction());
}

/**
 * @brief Checks whether the player is braking.
 * @param pGraph The player's action graph.
 * @return true if the player is braking.
 */
bool isActionBrake(const PlayerActionGraph* pGraph) {
    return isType<PlayerActionBrake>(pGraph->getAction());
}

/**
 * @brief Checks whether the player is turning around.
 * @param pGraph The player's action graph.
 * @return true if the player is turning around.
 */
bool isActionTurn(const PlayerActionGraph* pGraph) {
    return isType<PlayerActionTurn>(pGraph->getAction());
}

/**
 * @brief Checks whether the player is moving through the air.
 * @param pGraph The player's action graph.
 * @return true if the player is moving through the air.
 */
bool isAirMoveAction(const PlayerActionGraph* pGraph) {
    return isType<PlayerActionAirMove>(pGraph->getAction());
}

/**
 * @brief Checks whether an action moves the player through the air.
 * @param pAction The action to check.
 * @return true if the action is an air move action.
 */
bool isAirMoveAction(const PlayerAction* pAction) {
    return isType<PlayerActionAirMove>(pAction);
}

}  // namespace PlayerActionTypeFunc
