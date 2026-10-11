#include "Boss/KoopaChaseStateWarp.hpp"
#include "Boss/KoopaChase.hpp"
#include "Boss/KoopaChaseKoopa.hpp"
#include "Boss/KoopaChaseFunction.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Util/PlayerUtil.hpp"

namespace {
NERVE_DECL(KoopaChaseStateWarp, WarpStart);
NERVE_DECL(KoopaChaseStateWarp, Warp);
NERVE_DECL(KoopaChaseStateWarp, WaitForRestart);
NERVES_MAKE_NOSTRUCT(KoopaChaseStateWarp, WarpStart, Warp, WaitForRestart)
}

/**
 * @brief Creates the chase warp state with an initial zero destination.
 * @param pHost Chase actor controlled by this state.
 * @param rInfo Actor initialization information; unused.
 */
KoopaChaseStateWarp::KoopaChaseStateWarp(KoopaChase* pHost, const al::ActorInitInfo& rInfo)
    : al::NerveStateBase("State[ワープ]"), mHost(pHost), mWarpPosition(sead::Vector3f::zero) {
    initNerve(&NrvKoopaChaseStateWarpWarpStart, 0);
}

/**
 * @brief Activates the state and records the warp destination.
 * @param rWarpPosition Destination world position.
 */
void KoopaChaseStateWarp::start(const sead::Vector3f& rWarpPosition) {
    appear();
    mWarpPosition.set(rWarpPosition);
    al::setNerve(this, &NrvKoopaChaseStateWarpWarpStart);
}

/** @brief Plays the warp-start action and advances when it ends. */
void KoopaChaseStateWarp::exeWarpStart() {
    if (al::isFirstStep(this)) {
        al::startAction(mHost, KoopaChaseFunction::getAnimNameWarpJumpStart(mHost));
        if (!KoopaChaseFunction::getKoopa(mHost)->isStateDamage()) {
            al::startAction(mHost, "WarpJumpStart");
        }
    }
    if (al::isActionEnd(mHost)) {
        al::setNerve(this, &NrvKoopaChaseStateWarpWarp);
    }
}

/** @brief Holds both actors at the destination until a player is nearby. */
void KoopaChaseStateWarp::exeWarp() {
    if (al::isFirstStep(this)) {
        al::startAction(mHost, KoopaChaseFunction::getAnimNameWarpJumpLoop(mHost));
        if (!KoopaChaseFunction::getKoopa(mHost)->isStateDamage()) {
            al::startAction(KoopaChaseFunction::getKoopa(mHost), "WarpJumpLoop");
        }
    }
    al::setTrans(mHost, mWarpPosition);
    al::setTrans(KoopaChaseFunction::getKoopa(mHost), mWarpPosition);
    al::setVelocityZero(mHost);
    al::setVelocityZero(KoopaChaseFunction::getKoopa(mHost));
    auto* pPlayer = rc::findNearestActivePlayerActor(mHost);
    if (!pPlayer || al::calcDistance(mHost, pPlayer) > 1800.0f) {
        return;
    }
    al::setNerve(this, &NrvKoopaChaseStateWarpWaitForRestart);
}

/** @brief Restarts the chase after Koopa's warp provocation finishes. */
void KoopaChaseStateWarp::exeWaitForRestart() {
    if (al::isFirstStep(this)) {
        al::startAction(mHost, KoopaChaseFunction::getAnimNameRun(mHost));
        KoopaChaseFunction::getKoopa(mHost)->setStateWarpProvocation();
    }
    if (al::isStep(this, 2)) {
        KoopaChaseFunction::tryHideWarpDummy(mHost);
    }
    if (!KoopaChaseFunction::getKoopa(mHost)->isStateWarpProvocation()) {
        KoopaChaseFunction::tryResetWarpCube(mHost);
        kill();
    }
}
