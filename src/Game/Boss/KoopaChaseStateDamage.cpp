#include "Boss/KoopaChaseStateDamage.hpp"
#include "Boss/KoopaChase.hpp"
#include "Boss/KoopaChaseKoopa.hpp"
#include "Boss/KoopaChaseFunction.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Util/ScoreUtil.hpp"

namespace {
NERVE_DECL(KoopaChaseStateDamage, Wait);
NERVE_DECL(KoopaChaseStateDamage, Damage);
NERVE_DECL(KoopaChaseStateDamage, SmallDamage);
NERVES_MAKE_STRUCT(KoopaChaseStateDamage, Wait, Damage, SmallDamage)
const char* const sDamageActions[] = {"DamageBigFirst", "DamageBigSecond", "DamageBigThird"};
}

/** @brief Creates the chase damage state. @param pHost Chase actor receiving damage. */
KoopaChaseStateDamage::KoopaChaseStateDamage(KoopaChase* pHost)
    : al::NerveStateBase("クッパチェイスState[ダメージ]"), mHost(pHost) {
    initNerve(&NrvKoopaChaseStateDamage.Wait, 0);
}

/** @brief Resets the damage nerve and deactivates this state. */
void KoopaChaseStateDamage::kill() {
    al::setNerve(this, &NrvKoopaChaseStateDamage.Wait);
    al::NerveStateBase::kill();
}

/**
 * @brief Handles explosive and projectile damage while the state can accept hits.
 * @param pIsDamage Receives whether the hit changes the host's damage state.
 * @param pMsg Incoming sensor message.
 * @param pOther Attacking sensor.
 * @param pSelf Hit sensor on the host.
 * @param isJumpOrWarp Whether to defer the host's damage animation.
 * @return Whether the message was handled.
 */
bool KoopaChaseStateDamage::receiveMsg(bool* pIsDamage, const al::SensorMsg* pMsg,
    al::HitSensor* pOther, al::HitSensor* pSelf, bool isJumpOrWarp) {
    if (!al::isNerve(this, &NrvKoopaChaseStateDamage.Wait) ||
        KoopaChaseFunction::getKoopa(mHost)->isStateDamage()) {
        return false;
    }
    if (al::isMsgExplosion(pMsg) || al::isMsgExplosionCollide(pMsg)) {
        bool isCore = al::isMsgExplosion(pMsg) && al::isSensorName(pSelf, "Core");
        if (isJumpOrWarp) {
            if (isCore) doDamage(pOther, true);
            else doSmallDamage(pOther, true);
            *pIsDamage = false;
            return true;
        }
        appear();
        if (isCore) {
            doDamage(pOther, false);
            al::setNerve(this, &NrvKoopaChaseStateDamage.Damage);
        } else {
            switch (doSmallDamage(pOther, false)) {
            case DamageResult::Small: al::setNerve(this, &NrvKoopaChaseStateDamage.SmallDamage); break;
            case DamageResult::Large: al::setNerve(this, &NrvKoopaChaseStateDamage.Damage); break;
            }
        }
    } else if (al::isMsgFireBallCollide(pMsg) || al::isMsgPlayerBoomerangAttackCollide(pMsg)) {
        if (isInvalidFireBallDamage()) return false;
        switch (doFireBallDamage(pOther, isJumpOrWarp)) {
        case DamageResult::None: return false;
        case DamageResult::Small:
            if (!isJumpOrWarp) al::setNerve(this, &NrvKoopaChaseStateDamage.SmallDamage);
            break;
        case DamageResult::Large:
            if (!isJumpOrWarp) al::setNerve(this, &NrvKoopaChaseStateDamage.Damage);
            break;
        }
    } else {
        return false;
    }
    *pIsDamage = true;
    return true;
}

/**
 * @brief Applies a full hit, clears accumulated small hits, and awards score.
 * @param pOther Attacking sensor, or null when score is not awarded.
 * @param isJumpOrWarp Whether to defer the host animation.
 * @return The large-damage result.
 */
KoopaChaseStateDamage::DamageResult KoopaChaseStateDamage::doDamage(al::HitSensor* pOther, bool isJumpOrWarp) {
    mSmallDamageCount = 0;
    mFireBallDamageCount = 0;
    if (!isJumpOrWarp) al::startAction(mHost, sDamageActions[mDamageCount]);
    KoopaChaseFunction::getKoopa(mHost)->doDamage();
    al::startHitReaction(mHost, "大ダメージ");
    KoopaChaseFunction::vanishAllBombBounds(mHost);
    if (pOther) {
        sead::Vector3f position = al::getTrans(KoopaChaseFunction::getKoopa(mHost)) + sead::Vector3f::ey * 300.0f;
        rc::addScore(mHost, pOther, position, mDamageCount);
    }
    ++mDamageCount;
    return DamageResult::Large;
}

/**
 * @brief Accumulates a small hit and promotes the third hit to full damage.
 * @param pOther Attacking sensor.
 * @param isJumpOrWarp Whether to defer the full-damage animation.
 * @return Small or large damage, according to the accumulated hits.
 */
KoopaChaseStateDamage::DamageResult KoopaChaseStateDamage::doSmallDamage(al::HitSensor* pOther, bool isJumpOrWarp) {
    ++mSmallDamageCount;
    mFireBallDamageCount = 0;
    if (mSmallDamageCount >= 3) return doDamage(pOther, isJumpOrWarp);
    switch (mSmallDamageCount) {
    case 1: al::startHitReaction(mHost, "小ダメージ1"); break;
    case 2: al::startHitReaction(mHost, "小ダメージ2"); break;
    }
    int damageCount = mDamageCount;
    int smallDamageCount = mSmallDamageCount;
    const char* action = nullptr;
    switch (damageCount) {
    case 0:
        switch (smallDamageCount) {
        case 1: action = "DamageSmallFirst01"; break;
        case 2: action = "DamageSmallFirst02"; break;
        }
        break;
    case 1:
        switch (smallDamageCount) {
        case 1: action = "DamageSmallSecond01"; break;
        case 2: action = "DamageSmallSecond02"; break;
        }
        break;
    case 2:
        switch (smallDamageCount) {
        case 1: action = "DamageSmallThird01"; break;
        case 2: action = "DamageSmallThird02"; break;
        }
        break;
    }
    al::startAction(mHost, action);
    KoopaChaseFunction::getKoopa(mHost)->doDamageSmall();
    return DamageResult::Small;
}

/** @brief Tests the ten-frame projectile-hit cooldown. @return Whether another hit is blocked. */
bool KoopaChaseStateDamage::isInvalidFireBallDamage() const {
    return al::getNerveStep(mHost) - mLastFireBallStep < 10;
}

/**
 * @brief Accumulates a projectile hit and promotes every fifth hit to small damage.
 * @param pOther Attacking sensor.
 * @param isJumpOrWarp Whether to defer the full-damage animation.
 * @return The damage result produced by the accumulated hits.
 */
KoopaChaseStateDamage::DamageResult KoopaChaseStateDamage::doFireBallDamage(al::HitSensor* pOther, bool isJumpOrWarp) {
    al::startMclAnim(mHost, "DamageFireBall");
    al::startHitReaction(mHost, "ファイアボールヒット");
    mLastFireBallStep = al::getNerveStep(mHost);
    if (++mFireBallDamageCount >= 5) return doSmallDamage(pOther, isJumpOrWarp);
    return DamageResult::None;
}

/** @brief Consumes one life by advancing the full-hit count. */
void KoopaChaseStateDamage::decLife() { ++mDamageCount; }
/** @brief Tests whether all three full hits have been received. @return Whether the battle damage is complete. */
bool KoopaChaseStateDamage::isDamageFinish() const { return mDamageCount >= 3; }
/** @brief Waits for damage messages. */
void KoopaChaseStateDamage::exeWait() {}
/** @brief Finishes full damage after both host and Bowser recover. */
void KoopaChaseStateDamage::exeDamage() {
    if (al::isActionEnd(mHost) && !KoopaChaseFunction::getKoopa(mHost)->isStateDamage()) {
        KoopaChaseFunction::startActionWithKoopa(mHost, KoopaChaseFunction::getAnimNameRun(mHost), "Wait");
        kill();
    }
}
/** @brief Resumes running after the small-damage action finishes. */
void KoopaChaseStateDamage::exeSmallDamage() {
    if (al::isActionEnd(mHost)) {
        al::startAction(mHost, KoopaChaseFunction::getAnimNameRun(mHost));
        kill();
    }
}
