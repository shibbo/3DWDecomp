#include "Enemy/TuccondorBase.hpp"
#include "Enemy/EnemyStateBlowDown.hpp"
#include "Enemy/EnemyStateUtil.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Collision/CollisionPartsKeeperUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Project/Collision/CollisionUtil.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/ItemUtil.hpp"

namespace {
NERVE_DECL(TuccondorBase, BlowDown)
NERVE_DECL(TuccondorBase, PressDown)
NERVE_DECL(TuccondorBase, PressDownAttackWait)
NERVES_MAKE_STRUCT(TuccondorBase, BlowDown, PressDown, PressDownAttackWait)
struct TuccondorParam {
    sead::Vector3f attackOffset = sead::Vector3f(0.0f, 50.0f, 380.0f);
    EnemyStateBlowDownParam blowDown = EnemyStateBlowDownParam(false);
};
TuccondorParam sTuccondorParam;
}

/** @brief Creates the shared Conkdor actor. @param pName Actor name. */
TuccondorBase::TuccondorBase(const char* pName) : al::LiveActor(pName) {}

/** @brief Creates and registers the knockback state. */
void TuccondorBase::initNerveState() {
    mBlowDown = new EnemyStateBlowDown(this, &sTuccondorParam.blowDown);
    al::initNerveState(this, mBlowDown, &NrvTuccondorBase.BlowDown, "吹き飛び死亡");
}

/** @brief Tests contact against the neck cylinder. @param pSelf Own sensor. @param pOther Contact sensor. @param radius Neck radius. @return Whether the neck is hit. */
bool TuccondorBase::isHitNeckSensor(const al::HitSensor* pSelf, const al::HitSensor* pOther, f32 radius) const {
    if (!al::isSensorName(pSelf, "AttackNeck"))
        return false;
    const sead::Matrix34f* pMtx = al::getJointMtxPtr(this, "Neck01");
    sead::Vector3f direction(pMtx->m[0][0], pMtx->m[1][0], pMtx->m[2][0]);
    sead::Vector3f position(pMtx->m[0][3], pMtx->m[1][3], pMtx->m[2][3]);
    if (al::normalizeOrZero(&direction))
        return false;
    return al::isHitCylinderSensor(pOther, position, direction, radius);
}

/** @brief Applies friction and gravity. */
void TuccondorBase::updateVelocity() {
    if (al::isOnGround(this, 0, 0.0f)) {
        al::scaleVelocity(this, 0.86f);
        al::addVelocity(this, sead::Vector3f::ey * -0.5f);
    } else {
        al::scaleVelocity(this, 0.98f);
        al::addVelocity(this, sead::Vector3f::ey * -2.4f);
    }
}

/** @brief Selects the appropriate flattened state. @param pMsg Message. @param pOther Sender. @param pSelf Own sensor. @param isStanding Whether standing upright. @return Whether accepted. */
bool TuccondorBase::tryRequestPressDown(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf, bool isStanding) {
    return EnemyStateUtil::tryRequestPressDownAndNextNerve(pMsg, pOther, pSelf, this,
        isStanding ? static_cast<const al::Nerve*>(&NrvTuccondorBase.PressDown) : &NrvTuccondorBase.PressDownAttackWait, true);
}

/** @brief Requests knockback defeat. @param pMsg Message. @param pOther Sender. @param pSelf Own sensor. @return Whether accepted. */
bool TuccondorBase::tryRequestBlowDownAndNextNerve(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) {
    return EnemyStateUtil::tryRequestBlowDownAndNextNerve(pMsg, pOther, pSelf, mBlowDown, &NrvTuccondorBase.BlowDown, true);
}

/** @brief Completes knockback and releases an item. */
void TuccondorBase::exeBlowDown() {
    if (al::updateNerveState(this)) {
        al::appearItem(this);
        al::startHitReactionDeath(this);
        kill();
    }
}

/** @brief Plays the upright flattening animation. */
void TuccondorBase::exePressDown() {
    if (al::isFirstStep(this)) {
        al::setVelocityZero(this);
        al::startAction(this, "PressDown");
    }
    rc::tryAppearItemPressDown(this, nullptr);
    if (al::isActionEnd(this)) {
        al::startHitReactionDeath(this);
        kill();
    }
}

/** @brief Plays the flattening animation while the neck is extended. */
void TuccondorBase::exePressDownAttackWait() {
    if (al::isFirstStep(this)) {
        al::setVelocityZero(this);
        al::startAction(this, "PressDownAttackWait");
    }
    rc::tryAppearItemPressDown(this, nullptr);
    if (al::isActionEnd(this)) {
        al::startHitReactionDeath(this);
        kill();
    }
}

/** @brief Emits a surface, water, or miss reaction at the beak. */
void TuccondorBase::requestHitReactionAttack() {
    sead::Vector3f position;
    position.setMul(*getBaseMtx(), sTuccondorParam.attackOffset);
    sead::Vector3f up = sead::Vector3f::ey;
    al::calcUpDir(&up, this);
    sead::Vector3f hitPosition;
    al::Triangle triangle;
    if (alCollisionUtil::getFirstPolyOnArrow(this, &hitPosition, &triangle, position, up * -150.0f, nullptr, nullptr)) {
        al::setMaterialCode(this, al::getMaterialCodeName(triangle));
        al::startHitReactionHitEffect(this, "攻撃[衝突]", hitPosition);
    } else {
        sead::Vector3f waterPosition = position - up * 150.0f;
        if (rc::isInWaterArea(this, waterPosition))
            al::startHitReactionHitEffect(this, "攻撃[水中]", waterPosition);
        else
            al::startHitReaction(this, "攻撃[空振り]");
    }
}

/** @brief Checks whether a defeat state is active. @return Whether flattened or knocked back. */
bool TuccondorBase::isNerveDead() const {
    return al::isNerve(this, &NrvTuccondorBase.PressDown) ||
        al::isNerve(this, &NrvTuccondorBase.PressDownAttackWait) ||
        al::isNerve(this, &NrvTuccondorBase.BlowDown);
}
