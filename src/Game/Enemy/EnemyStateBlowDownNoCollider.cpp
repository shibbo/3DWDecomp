#include "Enemy/EnemyStateBlowDownNoCollider.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"

namespace {
NERVE_DECL(EnemyStateBlowDownNoCollider, Down)
NERVES_MAKE_NOSTRUCT(EnemyStateBlowDownNoCollider, Down)
}

/** @brief Constructs knockback controlled by an external ground-contact flag.
 * @param pHost Actor moved by the state.
 * @param pParam Knockback settings.
 * @param pIsOnGround Optional flag indicating that the host has landed.
 */
EnemyStateBlowDownNoCollider::EnemyStateBlowDownNoCollider(al::LiveActor* pHost,
        const EnemyStateBlowDownParam* pParam, bool* pIsOnGround)
    : EnemyStateBlowDown("吹き飛び状態", pHost, pParam), mIsOnGround(pIsOnGround) {
    al::setNerve(this, &NrvEnemyStateBlowDownNoColliderDown);
}

/** @brief Starts knockback using the external ground-contact flag. */
void EnemyStateBlowDownNoCollider::appear() {
    EnemyStateBlowDown::appear();
    al::setNerve(this, &NrvEnemyStateBlowDownNoColliderDown);
}

/** @brief Applies airborne motion until landing and ends when the animation finishes. */
void EnemyStateBlowDownNoCollider::exeDown() {
    if (al::isFirstStep(this) && mParam->mAction) {
        al::startAction(mHostActor, mParam->mAction);
    }
    if (mIsOnGround && *mIsOnGround) {
        al::setVelocityZero(mHostActor);
    } else {
        al::addVelocityToGravity(mHostActor, mParam->mGravity);
        al::scaleVelocity(mHostActor, mParam->mFriction);
    }
    if (al::isActionEnd(mHostActor)) {
        al::validateClipping(mHostActor);
        kill();
    }
}
