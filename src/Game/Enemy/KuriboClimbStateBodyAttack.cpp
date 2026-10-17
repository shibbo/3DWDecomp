#include "Enemy/KuriboClimbStateBodyAttack.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Nerve/NerveSetup.hpp"

namespace {
NERVE_DECL(KuriboClimbStateBodyAttack, Wait)
NERVE_DECL(KuriboClimbStateBodyAttack, AttackEnd)
NERVE_DECL(KuriboClimbStateBodyAttack, Attack)
NERVE_DECL(KuriboClimbStateBodyAttack, AttackJumpStart)
NERVE_DECL(KuriboClimbStateBodyAttack, AttackJump)
NERVE_DECL(KuriboClimbStateBodyAttack, AttackStart)
NERVE_DECL(KuriboClimbStateBodyAttack, AttackWait)
NERVES_MAKE_NOSTRUCT(KuriboClimbStateBodyAttack, AttackJumpStart, AttackJump, AttackStart, AttackWait)
NERVES_MAKE_STRUCT(KuriboClimbStateBodyAttack, Wait, AttackEnd, Attack)
}

/** @brief Creates the jumping body attack state. @param pHost Climbing enemy. */
KuriboClimbStateBodyAttack::KuriboClimbStateBodyAttack(al::LiveActor* pHost)
    : al::ActorStateBase("ネコクリボーの飛びつき攻撃ステート", pHost), mIsRequestResume(false) {
    initNerve(&NrvKuriboClimbStateBodyAttack.Wait, 0);
}

/** @brief Starts a new attack or resumes its interrupted phase. */
void KuriboClimbStateBodyAttack::appear() {
    mIsDead = false;
    if (mIsRequestResume) {
        if (al::isActionPlaying(mHostActor, "AttackEnd"))
            al::setNerve(this, &NrvKuriboClimbStateBodyAttack.AttackEnd);
        else
            al::setNerve(this, &NrvKuriboClimbStateBodyAttack.Attack);
    } else {
        al::setNerve(this, &NrvKuriboClimbStateBodyAttack.Wait);
    }
}

/** @brief Finishes the state and clears the resume request. */
void KuriboClimbStateBodyAttack::kill() {
    mIsDead = true;
    mIsRequestResume = false;
}

/** @brief Checks the initial waiting phase. @return Whether the state is waiting. */
bool KuriboClimbStateBodyAttack::isWait() const {
    return al::isNerve(this, &NrvKuriboClimbStateBodyAttack.Wait);
}

/** @brief Delays the jump by five steps. */
void KuriboClimbStateBodyAttack::exeWait() {
    al::setNerveAtStep(this, &NrvKuriboClimbStateBodyAttackAttackJumpStart, 5);
}

/** @brief Begins the upward jump and damps its velocity. */
void KuriboClimbStateBodyAttack::exeAttackJumpStart() {
    if (al::isFirstStep(this)) {
        al::startAction(mHostActor, "AttackJumpStart");
        al::setVelocityZero(mHostActor);
        al::setVelocityJump(mHostActor, 25.0f);
    }
    al::LiveActor* pHost = mHostActor;
    f32 speed = (al::getVelocity(pHost).y - 0.98f) * 0.98f;
    al::setVelocityJump(pHost, speed < 0.0f ? 0.0f : speed);
    if (al::isActionEnd(mHostActor))
        al::setNerve(this, &NrvKuriboClimbStateBodyAttackAttackJump);
}

/** @brief Continues the jump briefly before preparing the attack. */
void KuriboClimbStateBodyAttack::exeAttackJump() {
    if (al::isFirstStep(this))
        al::startAction(mHostActor, "AttackJumpLoop");
    al::LiveActor* pHost = mHostActor;
    f32 speed = (al::getVelocity(pHost).y - 0.98f) * 0.98f;
    al::setVelocityJump(pHost, speed < 0.0f ? 0.0f : speed);
    al::setNerveAtStep(this, &NrvKuriboClimbStateBodyAttackAttackStart, 3);
}

/** @brief Stops the host during its attack preparation animation. */
void KuriboClimbStateBodyAttack::exeAttackStart() {
    if (al::isFirstStep(this)) {
        al::startAction(mHostActor, "AttackStart");
        al::setVelocityZero(mHostActor);
    }
    if (al::isActionEnd(mHostActor))
        al::setNerve(this, &NrvKuriboClimbStateBodyAttackAttackWait);
}

/** @brief Pauses before lunging. */
void KuriboClimbStateBodyAttack::exeAttackWait() {
    al::setNerveAtStep(this, &NrvKuriboClimbStateBodyAttack.Attack, 10);
}

/** @brief Lunges forward and downward until the host lands. */
void KuriboClimbStateBodyAttack::exeAttack() {
    if (al::isFirstStep(this) && !al::isActionPlaying(mHostActor, "AttackLoop")) {
        al::startAction(mHostActor, "AttackLoop");
        sead::Vector3f velocity(0.0f, 0.0f, 0.0f);
        al::calcFrontDir(&velocity, mHostActor);
        velocity *= 12.5f;
        velocity.y = -10.0f;
        al::setVelocity(mHostActor, velocity);
    }
    al::addVelocityToGravity(mHostActor, 0.98f);
    al::scaleVelocityHV(mHostActor, 1.0f, 0.9f);
    if (al::isCollidedGround(mHostActor)) {
        al::setNerve(this, &NrvKuriboClimbStateBodyAttack.AttackEnd);
        al::setVelocityZero(mHostActor);
    }
}

/** @brief Finishes the landing animation. */
void KuriboClimbStateBodyAttack::exeAttackEnd() {
    if (al::isFirstStep(this))
        al::tryStartActionIfNotPlaying(mHostActor, "AttackEnd");
    if (al::isActionEnd(mHostActor))
        kill();
}
