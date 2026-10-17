#include "Boss/KoopaChaseFunction.hpp"
#include "Boss/KoopaChase.hpp"
#include "Boss/KoopaChaseKoopa.hpp"
#include "Boss/KoopaChaseBattle.hpp"
#include "Boss/KoopaChaseStateDamage.hpp"
#include "Boss/KoopaChaseMover.hpp"
#include "Boss/KoopaChaseMovePos.hpp"
#include "Scene/ProjectActorFactory.hpp"
#include "Library/ActorUtil.hpp"

namespace {
const char* const cRun[] = {"Run01", "Run02", "Run03"};
const char* const cJumpStart[] = {"JumpStart01", "JumpStart02", "JumpStart03"};
const char* const cJumpLoop[] = {"JumpLoop01", "JumpLoop02", "JumpLoop03"};
const char* const cJumpEnd[] = {"JumpEnd01", "JumpEnd02", "JumpEnd03"};
const char* const cWarpJumpStart[] = {"WarpJumpStart01", "WarpJumpStart02", "WarpJumpStart03"};
const char* const cWarpJumpLoop[] = {"WarpJumpLoop01", "WarpJumpLoop02", "WarpJumpLoop03"};
const char* const cWarpJumpEnd[] = {"WarpJumpEnd01", "WarpJumpEnd02", "WarpJumpEnd03"};
}

namespace KoopaChaseFunction {

/**
 * @brief Creates a linked actor and leaves it inactive.
 * @param rInfo Source placement information.
 * @param pLinkName Link category to resolve.
 * @param index Child index within that category.
 * @return Created actor, or nullptr if no linked actor is available.
 */
al::LiveActor* tryCreateLinkObj(const al::ActorInitInfo& rInfo, const char* pLinkName, int index) {
    if (al::calcLinkChildNum(rInfo, pLinkName) < 1) {
        return nullptr;
    }
    ProjectActorFactory factory;
    auto* pActor = al::createLinksActorFromFactory(factory, rInfo, pLinkName, index);
    if (pActor) {
        pActor->makeActorDead();
    }
    return pActor;
}

/**
 * @brief Starts the chase action and updates Koopa unless he is damaged.
 * @param pActor Chase actor.
 * @param pAction Action to start on the chase actor.
 */
void startActionWithKoopa(KoopaChase* pActor, const char* pAction) {
    al::startAction(pActor, pAction);
    if (!pActor->mKoopa->isStateDamage()) {
        al::startAction(pActor->mKoopa, pAction);
    }
}

/**
 * @brief Starts the chase action and updates Koopa unless he is damaged.
 * @param pActor Chase actor.
 * @param pAction Action to start on the chase actor.
 * @param pKoopaAction Action to start on Koopa.
 */
void startActionWithKoopa(KoopaChase* pActor, const char* pAction, const char* pKoopaAction) {
    al::startAction(pActor, pAction);
    if (!pActor->mKoopa->isStateDamage()) {
        al::startAction(pActor->mKoopa, pKoopaAction);
    }
}

/**
 * @brief Returns the associated chase actor pointer.
 * @param pActor Chase actor.
 * @return Stored actor pointer, which may be nullptr.
 */
KoopaChaseKoopa* getKoopa(KoopaChase* pActor) {
    return pActor->mKoopa;
}

/**
 * @brief Returns the associated chase actor pointer.
 * @param pActor Chase actor.
 * @return Stored actor pointer, which may be nullptr.
 */
al::LiveActor* getTargetPlayer(KoopaChase* pActor) {
    return pActor->mTargetPlayer;
}

/**
 * @brief Returns the associated chase actor pointer.
 * @param pActor Chase actor.
 * @return Stored actor pointer, which may be nullptr.
 */
al::LiveActor* getLookAtTargetPlayer(KoopaChase* pActor) {
    return pActor->mLookAtTargetPlayer;
}

/**
 * @brief Reads the active battle damage count.
 * @param pActor Chase actor with an initialized battle.
 * @return Number of damage events recorded by the battle.
 */
int getDamageCount(const KoopaChase* pActor) {
    return pActor->mBattle->getStateDamage()->mDamageCount;
}

/**
 * @brief Positions Koopa at the vehicle attachment joint.
 * @param pActor Chase actor.
 */
void updateKoopaPose(KoopaChase* pActor) {
    al::calcJointPos(al::getTransPtr(pActor->mKoopa), pActor, "KoopaPosition");
}

/**
 * @brief Turns Koopa toward the current target player when one exists.
 * @param pActor Chase actor.
 * @return True if a target player was available.
 */
bool tryLookAtTargetPlayer(KoopaChase* pActor) {
    if (!pActor->mTargetPlayer) {
        return false;
    }
    al::turnToTarget(pActor->mKoopa, al::getTrans(pActor->mTargetPlayer), 1.0f);
    return true;
}

/**
 * @brief Checks whether the mover passed its point.
 * @param pActor Chase actor.
 * @return Whether the point has been passed.
 */
bool isOverPoint(const KoopaChase* pActor) {
    return pActor->mMover->mIsOverPoint;
}

/**
 * @brief Checks whether the previous movement point is a jump point.
 * @param pActor Chase actor with initialized movement points.
 * @return Whether the movement point has the requested type.
 */
bool isPreviousPointJump(const KoopaChase* pActor) {
    return pActor->mMover->mPreviousPoint->mType == 1;
}

/**
 * @brief Checks whether the current movement point is a jump point.
 * @param pActor Chase actor with initialized movement points.
 * @return Whether the movement point has the requested type.
 */
bool isCurrentPointJump(const KoopaChase* pActor) {
    return pActor->mMover->mCurrentPoint->mType == 1;
}

/**
 * @brief Checks whether the mover has passed a jump point.
 * @param pActor Chase actor.
 * @return Whether a point of the requested type has been passed.
 */
bool isOverPointJump(const KoopaChase* pActor) {
    return pActor->mMover->mIsOverPoint && pActor->mMover->mPreviousPoint->mType == 1;
}

/**
 * @brief Checks whether the previous movement point is a warp point.
 * @param pActor Chase actor with initialized movement points.
 * @return Whether the movement point has the requested type.
 */
bool isPreviousPointWarp(const KoopaChase* pActor) {
    return pActor->mMover->mPreviousPoint->mType == 2;
}

/**
 * @brief Checks whether the current movement point is a warp point.
 * @param pActor Chase actor with initialized movement points.
 * @return Whether the movement point has the requested type.
 */
bool isCurrentPointWarp(const KoopaChase* pActor) {
    return pActor->mMover->mCurrentPoint->mType == 2;
}

/**
 * @brief Checks whether the mover has passed a warp point.
 * @param pActor Chase actor.
 * @return Whether a point of the requested type has been passed.
 */
bool isOverPointWarp(const KoopaChase* pActor) {
    return pActor->mMover->mIsOverPoint && pActor->mMover->mPreviousPoint->mType == 2;
}

/**
 * @brief Copies the current movement point position.
 * @param pOut Destination world position.
 * @param pActor Chase actor with an initialized current point.
 */
void calcCurrentPointPos(sead::Vector3f* pOut, const KoopaChase* pActor) {
    *pOut = sead::Vector3f(pActor->mMover->mCurrentPoint->mTrans);
}

/**
 * @brief Measures the distance to the current movement point.
 * @param pActor Chase actor.
 * @return Distance reported by the mover.
 */
float calcDistanceToCurrentPoint(const KoopaChase* pActor) {
    return pActor->mMover->calcDistanceToCurrentPoint();
}

/**
 * @brief Removes the active warp helper through the mover.
 * @param pActor Chase actor.
 */
void tryResetWarpCube(KoopaChase* pActor) {
    pActor->mMover->tryKillWarpCubeIfAlive();
}

/**
 * @brief Removes the active warp helper through the mover.
 * @param pActor Chase actor.
 */
void tryHideWarpDummy(KoopaChase* pActor) {
    pActor->mMover->tryKillWarpDummyIfAlive();
}

/**
 * @brief Selects the Run animation for the damage level.
 * @param damageCount Damage level; must be in [0, 2].
 * @return Animation resource name.
 */
const char* getAnimNameRun(int damageCount) {
    return cRun[damageCount];
}

/**
 * @brief Selects the Run animation for the current damage level.
 * @param pActor Chase actor whose damage count is in [0, 2].
 * @return Animation resource name.
 */
const char* getAnimNameRun(const KoopaChase* pActor) {
    return cRun[getDamageCount(pActor)];
}

/**
 * @brief Selects the JumpStart animation for the current damage level.
 * @param pActor Chase actor whose damage count is in [0, 2].
 * @return Animation resource name.
 */
const char* getAnimNameJumpStart(const KoopaChase* pActor) {
    return cJumpStart[getDamageCount(pActor)];
}

/**
 * @brief Selects the JumpLoop animation for the current damage level.
 * @param pActor Chase actor whose damage count is in [0, 2].
 * @return Animation resource name.
 */
const char* getAnimNameJumpLoop(const KoopaChase* pActor) {
    return cJumpLoop[getDamageCount(pActor)];
}

/**
 * @brief Selects the JumpEnd animation for the current damage level.
 * @param pActor Chase actor whose damage count is in [0, 2].
 * @return Animation resource name.
 */
const char* getAnimNameJumpEnd(const KoopaChase* pActor) {
    return cJumpEnd[getDamageCount(pActor)];
}

/**
 * @brief Selects the WarpJumpStart animation for the current damage level.
 * @param pActor Chase actor whose damage count is in [0, 2].
 * @return Animation resource name.
 */
const char* getAnimNameWarpJumpStart(const KoopaChase* pActor) {
    return cWarpJumpStart[getDamageCount(pActor)];
}

/**
 * @brief Selects the WarpJumpLoop animation for the damage level.
 * @param damageCount Damage level; must be in [0, 2].
 * @return Animation resource name.
 */
const char* getAnimNameWarpJumpLoop(int damageCount) {
    return cWarpJumpLoop[damageCount];
}

/**
 * @brief Selects the WarpJumpLoop animation for the current damage level.
 * @param pActor Chase actor whose damage count is in [0, 2].
 * @return Animation resource name.
 */
const char* getAnimNameWarpJumpLoop(const KoopaChase* pActor) {
    return cWarpJumpLoop[getDamageCount(pActor)];
}

/**
 * @brief Selects the WarpJumpEnd animation for the damage level.
 * @param damageCount Damage level; must be in [0, 2].
 * @return Animation resource name.
 */
const char* getAnimNameWarpJumpEnd(int damageCount) {
    return cWarpJumpEnd[damageCount];
}
}
