#include "Enemy/ImozoStateBlockRailMove.hpp"
#include "Enemy/Imozo.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Project/Collision/CollisionParts.hpp"
#include "Project/Collision/CollisionUtil.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/ProjectMsgUtil.hpp"

namespace {
NERVE_DECL(ImozoStateBlockRailMove, Move)
NERVE_DECL(ImozoStateBlockRailMove, TurnJump)
NERVE_DECL(ImozoStateBlockRailMove, Reaction)
NERVE_DECL(ImozoStateBlockRailMove, Fall)
NERVE_DECL(ImozoStateBlockRailMove, Land)
NERVES_MAKE_NOSTRUCT(ImozoStateBlockRailMove, Move, TurnJump, Reaction, Fall, Land)
}

/** @brief Creates the rail movement state. @param pName State name. @param pHost Owning enemy. */
ImozoStateBlockRailMove::ImozoStateBlockRailMove(const char* pName, Imozo* pHost)
    : HostStateBase(pName, pHost) { initNerve(&NrvImozoStateBlockRailMoveMove, 0); }

/** @brief Checks attack eligibility and repeat-hit cooldowns.
 * @param pMsg Incoming message. @param pOther Attacking sensor. @param pSelf Receiving sensor.
 * @return Whether the attack can trigger a reaction. */
bool ImozoStateBlockRailMove::isEnableReactionMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) {
    if (al::isMsgPlayerTailAttack(pMsg) || al::isMsgPlayerClimbAttack(pMsg) ||
        al::isMsgPlayerSpinAttack(pMsg) || al::isMsgPlayerBodyAttack(pMsg) || al::isMsgPlayerBodyLanding(pMsg))
        return mReactionTimer > 30 && getHost()->isHitBody(pOther, pSelf);
    if (al::isMsgPlayerBoomerangAttack(pMsg)) {
        if (getHost()->isHitBody(pOther, pSelf)) {
            s32 timer = mBoomerangTimer;
            mBoomerangTimer = 0;
            return timer > 2;
        }
        return false;
    }
    if (al::isMsgPlayerFireBallAttack(pMsg)) return getHost()->isHitBody(pOther, pSelf);
    if (GameDataFunction::isSingleMode(GameDataHolderAccessor(getHost()))) {
        if (al::isMsgBallAttack(pMsg) || al::isMsgKeyThrow(pMsg) || al::isMsgKickKouraAttack(pMsg) ||
            al::isMsgNekoAttack(pMsg) || al::isMsgPlayerKouraAttack(pMsg) || al::isMsgEnemyAttackFire(pMsg) ||
            (al::isMsgPlayerHipDropAll(pMsg) && al::isSensorKoopaJr(pOther)))
            return mReactionTimer > 30 && getHost()->isHitBody(pOther, pSelf);
    }
    return false;
}

/** @brief Starts an eligible hit reaction outside a turn jump.
 * @param pMsg Incoming message. @param pOther Attacking sensor. @param pSelf Receiving sensor.
 * @return Whether a reaction started. */
bool ImozoStateBlockRailMove::tryStartReaction(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) {
    if (al::isNerve(this, &NrvImozoStateBlockRailMoveTurnJump)) return false;
    if (isEnableReactionMsg(pMsg, pOther, pSelf)) {
        mReactionTimer = 0;
        rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        al::setNerve(this, &NrvImozoStateBlockRailMoveReaction);
        return true;
    }
    return false;
}
/** @brief Resumes moving or falling according to rail contact. */
void ImozoStateBlockRailMove::appear() {
    al::NerveStateBase::appear();
    if (getHost()->isRideRail()) al::setNerve(this, &NrvImozoStateBlockRailMoveMove);
    else al::setNerve(this, &NrvImozoStateBlockRailMoveFall);
}
/** @brief Advances the capped reaction timers. */
void ImozoStateBlockRailMove::control() {
    if (mReactionTimer < 600) ++mReactionTimer;
    if (mBoomerangTimer < 600) ++mBoomerangTimer;
}
/** @brief Updates the move animation and rail movement. */
void ImozoStateBlockRailMove::exeMove() {
    if (al::isFirstStep(this)) al::startAction(getHost(), "Move");
    getHost()->addRailAccel(al::calcNerveValue(this, 0, 0.0f, mRailAccel));
    getHost()->moveOnRail();
    getHost()->rotateBody();
    if (getHost()->isTerminateRail()) {
        al::setNerve(this, &NrvImozoStateBlockRailMoveTurnJump);
        return;
    }
    if (!getHost()->isRideRail()) al::setNerve(this, &NrvImozoStateBlockRailMoveFall);
    const auto* pFilter = getHost()->getCollisionFilter();
    if (pFilter) {
        const sead::Vector3f& rPos = al::getTrans(getHost());
        sead::Vector3f dir;
        getHost()->getCollisionDir(&dir);
        auto* pParts = alCollisionUtil::getStrikeArrowCollisionParts(getHost(), nullptr, rPos, dir, pFilter, nullptr);
        if (pParts) {
            if (GameDataFunction::isSingleMode(GameDataHolderAccessor(getHost())) &&
                al::isSensorHostName(pParts->getSensor(), "\u30d0\u30e9\u30f3\u30b9\u30c8\u30ed\u30c3\u30b3")) return;
            getHost()->reverse();
            al::setNerve(this, &NrvImozoStateBlockRailMoveTurnJump);
        }
    }
}
/** @brief Updates the fall animation and rail movement. */
void ImozoStateBlockRailMove::exeFall() {
    if (al::isFirstStep(this)) al::startAction(getHost(), "Fall");
    getHost()->addRailAccel(al::calcNerveValue(this, 0, 0.0f, mRailAccel));
    getHost()->moveOnRail();
    getHost()->rotateBody();
    if (getHost()->isTerminateRail()) {
        al::setNerve(this, &NrvImozoStateBlockRailMoveTurnJump);
        return;
    }
    if (getHost()->isRideRail()) al::setNerve(this, &NrvImozoStateBlockRailMoveLand);
}
/** @brief Updates the land animation and rail movement. */
void ImozoStateBlockRailMove::exeLand() {
    if (al::isFirstStep(this)) al::startAction(getHost(), "Land");
    getHost()->addRailAccel(al::calcNerveValue(this, 0, 0.0f, mRailAccel));
    getHost()->moveOnRail();
    getHost()->rotateBody();
    if (getHost()->isTerminateRail()) {
        al::setNerve(this, &NrvImozoStateBlockRailMoveTurnJump);
        return;
    }
    if (getHost()->isRideRail()) {
        if (al::isActionEnd(getHost())) al::setNerve(this, &NrvImozoStateBlockRailMoveMove);
    } else al::setNerve(this, &NrvImozoStateBlockRailMoveFall);
}
/** @brief Updates the reaction animation and rail movement. */
void ImozoStateBlockRailMove::exeReaction() {
    if (al::isFirstStep(this)) al::startAction(getHost(), "Reaction");
    getHost()->addRailAccel(al::calcNerveValue(this, 0, 0.0f, mRailAccel));
    getHost()->moveOnRail();
    getHost()->rotateBody();
    if (getHost()->isTerminateRail()) {
        al::setNerve(this, &NrvImozoStateBlockRailMoveTurnJump);
        return;
    }
    if (al::isActionEnd(getHost())) {
        if (getHost()->isRideRail()) al::setNerve(this, &NrvImozoStateBlockRailMoveMove);
        else al::setNerve(this, &NrvImozoStateBlockRailMoveFall);
    }
}
/** @brief Jumps in place while rotating toward the reversed rail direction. */
void ImozoStateBlockRailMove::exeTurnJump() {
    if (al::isFirstStep(this)) {
        al::startAction(getHost(), "TurnJump");
        mJumpStart = al::getTrans(getHost());
        getHost()->resetRailAccel();
    }
    f32 height = al::calcNerveJumpValue(this, 10, 0, 12, 100.0f);
    al::setTrans(getHost(), height * sead::Vector3f::ey + mJumpStart);
    if (al::isLessEqualStep(this, 22)) getHost()->rotateBodyAir(0.98f);
    if (al::isGreaterEqualStep(this, 22)) al::setNerve(this, &NrvImozoStateBlockRailMoveLand);
}
/** @brief Forces a turn jump from movement or reaction. */
void ImozoStateBlockRailMove::turnJumpForce() {
    if (al::isNerve(this, &NrvImozoStateBlockRailMoveMove) || al::isNerve(this, &NrvImozoStateBlockRailMoveReaction))
        al::setNerve(this, &NrvImozoStateBlockRailMoveTurnJump);
}
/** @brief Checks whether support freezing is permitted. @return Whether no turn jump is active. */
bool ImozoStateBlockRailMove::isEnableSupportFreeze() const {
    return !al::isNerve(this, &NrvImozoStateBlockRailMoveTurnJump);
}
