#include "Boss/KoopaLastAttack.hpp"
#include "Boss/KoopaLastFunction.hpp"
#include "Boss/KoopaLastStateAttackFire.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"

namespace {
NERVE_DECL(KoopaLastAttackFire, Attack);
NERVES_MAKE_NOSTRUCT(KoopaLastAttackFire, Attack)
}

/**
 * @brief Creates a fire-attack actor with no attack state or parameters yet.
 * @param pName Actor name.
 */
KoopaLastAttackFire::KoopaLastAttackFire(const char* pName) : al::LiveActor(pName) {
}

/**
 * @brief Initializes the final-boss fire attack and its appearance switch.
 * @param rInfo Actor placement and scene initialization information.
 */
void KoopaLastAttackFire::init(const al::ActorInitInfo& rInfo) {
    KoopaLastFunction::initActorKoopaLastCommon(this, rInfo, nullptr, nullptr);
    mAttackParam = new KoopaLastStateAttackFireParam;
    al::tryGetArg(&mAttackParam->mAttackInterval, rInfo, "AttackInterval");
    mAttackState = new KoopaLastStateAttackFire(this, rInfo, mAttackParam);
    al::initNerve(this, &NrvKoopaLastAttackFireAttack, 1);
    al::initNerveState(this, mAttackState, &NrvKoopaLastAttackFireAttack, "炎攻撃");
    al::startAction(this, "Climb");
    al::tryListenStageSwitchAppear(this);
}

/**
 * @brief Forwards a sensor contact to the shared final-boss attack handler.
 * @param pSelf This actor's attacking sensor.
 * @param pOther Contacted sensor.
 */
void KoopaLastAttackFire::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    KoopaLastFunction::attackSensorCommon(pSelf, pOther);
}

/** @brief Advances the active fire-attack state. */
void KoopaLastAttackFire::exeAttack() {
    al::updateNerveState(this);
}

/** @brief Destroys the actor's base resources. */
KoopaLastAttackFire::~KoopaLastAttackFire() = default;
