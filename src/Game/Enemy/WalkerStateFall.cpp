#include "Enemy/WalkerStateFall.hpp"
#include "Enemy/WalkerStateFunction.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Nerve/NerveSetup.hpp"

namespace {
NERVE_DECL(WalkerStateFall, Fall)
NERVE_DECL(WalkerStateFall, Land)
NERVES_MAKE_NOSTRUCT(WalkerStateFall, Fall, Land)
}

/**
 * @brief Constructs a walking enemy's falling and landing state.
 * @param pHost Walking enemy.
 * @param pParam Gravity and friction parameters.
 */
WalkerStateFall::WalkerStateFall(al::LiveActor* pHost, const WalkerStateParam* pParam)
    : al::ActorStateBase("歩行型敵落下状態", pHost), mParam(pParam) {
    initNerve(&NrvWalkerStateFallFall, 0);
}

/** @brief Starts the falling animation and activates the state. */
void WalkerStateFall::appear() {
    al::startAction(mHostActor, "Fall");
    al::setNerve(this, &NrvWalkerStateFallFall);
    al::ActorStateBase::appear();
}

/** @brief Applies airborne motion until the enemy reaches the ground. */
void WalkerStateFall::exeFall() {
    WalkerStateFunction::calcPassiveMovement(mHostActor, mParam);
    if (al::isOnGround(mHostActor, 0, 0.0f)) {
        al::setNerve(this, &NrvWalkerStateFallLand);
    }
}

/** @brief Plays the landing animation and completes the state when it ends. */
void WalkerStateFall::exeLand() {
    if (al::isFirstStep(this)) {
        al::startAction(mHostActor, "Land");
    }
    WalkerStateFunction::calcPassiveMovement(mHostActor, mParam);
    if (al::isActionEnd(mHostActor)) {
        kill();
    }
}
