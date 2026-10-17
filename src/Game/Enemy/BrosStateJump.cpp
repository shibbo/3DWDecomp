#include "Enemy/BrosStateJump.hpp"
#include "Enemy/BrosFunction.hpp"
#include "Enemy/BrosMoveStepKeeper.hpp"
#include "Enemy/KuriboTower.hpp"
#include "Enemy/KuriboTowerNode.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/KeyPose/KeyPoseKeeper.hpp"
#include "Library/KeyPose/KeyPoseKeeperUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Math/ParabolicPath.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"

namespace {
NERVE_DECL(BrosStateJump, Start)
NERVE_DECL(BrosStateJump, Loop)
NERVE_DECL(BrosStateJump, Land)
NERVES_MAKE_NOSTRUCT(BrosStateJump, Start, Loop, Land)
}

/** @brief Creates the jump path and state.
 * @param pHost Bros enemy.
 * @param pKeyPoseKeeper Jump destinations.
 * @param pMoveStepKeeper Platform controller.
 */
BrosStateJump::BrosStateJump(al::LiveActor* pHost, al::KeyPoseKeeper* pKeyPoseKeeper,
                           const BrosMoveStepKeeper* pMoveStepKeeper)
    : al::ActorStateBase("\u30d6\u30ed\u30b9\u72b6\u614b[\u30b8\u30e3\u30f3\u30d7]", pHost), mKeyPoseKeeper(pKeyPoseKeeper),
      mMoveStepKeeper(pMoveStepKeeper) {
    mPath = new al::ParabolicPath;
    initNerve(&NrvBrosStateJumpStart, 0);
}

/** @brief Begins the jump anticipation. */
void BrosStateJump::appear() {
    al::setNerve(this, &NrvBrosStateJumpStart);
    al::ActorStateBase::appear();
}

/** @brief Plays the anticipation animation. */
void BrosStateJump::exeStart() {
    if (al::isFirstStep(this)) {
        if (rc::isCurrentStepKuribo(mMoveStepKeeper) && al::isExistAction(mHostActor, "TowerJumpStart")) {
            al::startAction(mHostActor, "TowerJumpStart");
        } else {
            al::startAction(mHostActor, "JumpStart");
        }
    }
    if (al::isActionEnd(mHostActor)) {
        al::setNerve(this, &NrvBrosStateJumpLoop);
    }
}

/** @brief Follows the jump arc toward the next platform. */
void BrosStateJump::exeLoop() {
    if (al::isFirstStep(this)) {
        al::startAction(mHostActor, "Jump");
        mStartPos = al::getTrans(mHostActor);
    }
    rc::turnToTargetOrDefaultDirectionNext(mHostActor, mMoveStepKeeper, 6.0f);
    mPath->initFromUpVectorAddHeight(mStartPos, al::getNextKeyTrans(mKeyPoseKeeper), sead::Vector3f::ey, 300.0f);
    mPath->calcPosition(al::getTransPtr(mHostActor), al::calcNerveRate(this, 60));
    if (al::isStep(this, 60)) {
        al::nextKeyPose(mKeyPoseKeeper);
        al::setNerve(this, &NrvBrosStateJumpLand);
    }
}

/** @brief Plays the landing animation and finishes the jump. */
void BrosStateJump::exeLand() {
    if (al::isFirstStep(this)) {
        if (rc::isCurrentStepKuribo(mMoveStepKeeper) && al::isExistAction(mHostActor, "TowerJumpStart")) {
            al::startAction(mHostActor, "TowerJumpEnd");
        } else {
            al::startAction(mHostActor, "JumpEnd");
        }
    }
    if (al::isActionEnd(mHostActor)) { kill(); }
}

/** @brief Tests whether the enemy is outside the airborne state. @return Whether it is grounded. */
bool BrosStateJump::isLand() const { return !al::isNerve(this, &NrvBrosStateJumpLoop); }

namespace rc {
/** @brief Turns toward the player or the platform's fixed direction.
 * @param pActor Bros enemy. @param pMoveStepKeeper Platform controller. @param degree Turning speed.
 */
void turnToTargetOrDefaultDirectionCurrent(al::LiveActor* pActor, const BrosMoveStepKeeper* pMoveStepKeeper, f32 degree) {
    if (isFaceToDefaultDirectionCurrent(pActor, pMoveStepKeeper)) {
        sead::Vector3f dir;
        al::calcQuatFront(&dir, al::getCurrentKeyQuat(pMoveStepKeeper->getKeyPoseKeeper()));
        al::turnToDirection(pActor, dir, degree);
    } else {
        al::turnToTarget(pActor, al::findNearestPlayerPos(pActor), degree);
    }
}

/** @brief Checks whether the platform requires its default facing.
 * @param pActor Bros enemy. @param pMoveStepKeeper Platform controller.
 * @return Whether to use the platform's facing direction.
 */
bool isFaceToDefaultDirectionCurrent(const al::LiveActor* pActor, const BrosMoveStepKeeper* pMoveStepKeeper) {
    if (!pMoveStepKeeper) { return false; }
    if (pMoveStepKeeper->isCurrentStepPoseFixed()) { return true; }
    if (pMoveStepKeeper->isCurrentStepRouteDokan()) {
        sead::Vector3f targetOffset;
        sead::Vector3f planarFront;
        sead::Vector3f keyFront;
        al::calcQuatFront(&keyFront, al::getCurrentKeyQuat(pMoveStepKeeper->getKeyPoseKeeper()));
        const sead::Vector3f& rKeyTrans = al::getCurrentKeyTrans(pMoveStepKeeper->getKeyPoseKeeper());
        const sead::Vector3f& rGravity = al::getGravity(pActor);
        targetOffset.set(al::findNearestPlayerPos(pActor));
        targetOffset -= rKeyTrans;
        al::verticalizeVec(&targetOffset, rGravity, targetOffset);
        al::verticalizeVec(&planarFront, rGravity, keyFront);
        return al::isNearAngleDegree(targetOffset, planarFront, 60.0f);
    }
    return false;
}

/** @brief Gets the platform rotation. @param pMoveStepKeeper Platform controller. @return Key pose rotation. */
const sead::Quatf& getCurrentKeyQuat(const BrosMoveStepKeeper* pMoveStepKeeper) {
    return al::getCurrentKeyQuat(pMoveStepKeeper->getKeyPoseKeeper());
}
/** @brief Gets the platform position. @param pMoveStepKeeper Platform controller. @return Key pose position. */
const sead::Vector3f& getCurrentKeyTrans(const BrosMoveStepKeeper* pMoveStepKeeper) {
    return al::getCurrentKeyTrans(pMoveStepKeeper->getKeyPoseKeeper());
}

/** @brief Checks whether the platform requires its default facing.
 * @param pActor Bros enemy. @param pMoveStepKeeper Platform controller.
 * @return Whether to use the platform's facing direction.
 */
bool isFaceToDefaultDirectionNext(const al::LiveActor* pActor, const BrosMoveStepKeeper* pMoveStepKeeper) {
    if (!pMoveStepKeeper) { return false; }
    if (pMoveStepKeeper->isNextStepPoseFixed()) { return true; }
    if (pMoveStepKeeper->isNextStepRouteDokan()) {
        sead::Vector3f targetOffset;
        sead::Vector3f planarFront;
        sead::Vector3f keyFront;
        al::calcQuatFront(&keyFront, al::getNextKeyQuat(pMoveStepKeeper->getKeyPoseKeeper()));
        const sead::Vector3f& rKeyTrans = al::getNextKeyTrans(pMoveStepKeeper->getKeyPoseKeeper());
        const sead::Vector3f& rGravity = al::getGravity(pActor);
        targetOffset.set(al::findNearestPlayerPos(pActor));
        targetOffset -= rKeyTrans;
        al::verticalizeVec(&targetOffset, rGravity, targetOffset);
        al::verticalizeVec(&planarFront, rGravity, keyFront);
        return al::isNearAngleDegree(targetOffset, planarFront, 60.0f);
    }
    return false;
}

/** @brief Gets the platform rotation. @param pMoveStepKeeper Platform controller. @return Key pose rotation. */
const sead::Quatf& getNextKeyQuat(const BrosMoveStepKeeper* pMoveStepKeeper) {
    return al::getNextKeyQuat(pMoveStepKeeper->getKeyPoseKeeper());
}
/** @brief Gets the platform position. @param pMoveStepKeeper Platform controller. @return Key pose position. */
const sead::Vector3f& getNextKeyTrans(const BrosMoveStepKeeper* pMoveStepKeeper) {
    return al::getNextKeyTrans(pMoveStepKeeper->getKeyPoseKeeper());
}

/** @brief Turns toward the player or the next platform's fixed direction.
 * @param pActor Bros enemy. @param pMoveStepKeeper Platform controller. @param degree Turning speed.
 */
void turnToTargetOrDefaultDirectionNext(al::LiveActor* pActor, const BrosMoveStepKeeper* pMoveStepKeeper, f32 degree) {
    sead::Vector3f dir;
    if (isFaceToDefaultDirectionNext(pActor, pMoveStepKeeper)) {
        al::calcQuatFront(&dir, al::getNextKeyQuat(pMoveStepKeeper->getKeyPoseKeeper()));
    } else {
        dir = al::findNearestPlayerPos(pActor) - al::getNextKeyTrans(pMoveStepKeeper->getKeyPoseKeeper());
        if (al::normalizeOrZero(&dir)) { return; }
    }
    al::turnToDirection(pActor, dir, degree);
}

/** @brief Checks whether the enemy needs to turn back to its platform direction.
 * @param pActor Bros enemy. @param pMoveStepKeeper Platform controller. @return Whether a turn is needed.
 */
bool isTurnToDefaultDirectionCurrent(const al::LiveActor* pActor, const BrosMoveStepKeeper* pMoveStepKeeper) {
    if (pMoveStepKeeper->isCurrentStepPoseFixed()) {
        sead::Vector3f front;
        sead::Vector3f planarKey;
        sead::Vector3f keyFront;
        al::calcQuatFront(&keyFront, al::getCurrentKeyQuat(pMoveStepKeeper->getKeyPoseKeeper()));
        const sead::Vector3f& rGravity = al::getGravity(pActor);
        front.set(al::getFront(pActor));
        al::verticalizeVec(&front, rGravity, front);
        al::verticalizeVec(&planarKey, rGravity, keyFront);
        return !al::isNearAngleDegree(front, planarKey, 10.0f);
    }
    return false;
}
/** @brief Tests for a Goomba platform. @param pMoveStepKeeper Platform controller. @return Whether the current step is a Goomba tower. */
bool isCurrentStepKuribo(const BrosMoveStepKeeper* pMoveStepKeeper) { return pMoveStepKeeper->isCurrentStepKuribo(); }
/** @brief Tests for a fixed pose. @param pMoveStepKeeper Platform controller. @return Whether the current pose is fixed. */
bool isCurrentStepPoseFixed(const BrosMoveStepKeeper* pMoveStepKeeper) { return pMoveStepKeeper->isCurrentStepPoseFixed(); }

/** @brief Places a weapon relative to its parent matrix.
 * @param pActor Weapon actor. @param pParentMtx Parent pose. @param rTrans Local translation. @param rRotate Local rotation in degrees.
 */
void updateWeaponPose(al::LiveActor* pActor, const sead::Matrix34f* pParentMtx, const sead::Vector3f& rTrans, const sead::Vector3f& rRotate) {
    sead::Matrix34f rotateMtx;
    rotateMtx.makeR(rRotate * sead::Mathf::deg2rad(1.0f));
    sead::Matrix34f transMtx;
    transMtx.makeRT({0.0f, 0.0f, 0.0f}, rTrans);
    sead::Matrix34f localMtx;
    localMtx.setMul(rotateMtx, transMtx);
    sead::Matrix34f poseMtx;
    poseMtx.setMul(*pParentMtx, localMtx);
    al::updatePoseMtx(pActor, &poseMtx);
}

/** @brief Attacks every Goomba in every platform tower.
 * @param pMoveStepKeeper Platform controller. @param pSender Attack sensor.
 */
void killStepKuriboAll(const BrosMoveStepKeeper* pMoveStepKeeper, al::HitSensor* pSender) {
    for (s32 i = 0; i < al::getKeyPoseCount(pMoveStepKeeper->getKeyPoseKeeper()); ++i) {
        KuriboTower* pTower = pMoveStepKeeper->tryGetStepKuribo(i);
        if (pTower) {
            for (s32 j = 0; j < pTower->getChildNum(); ++j) {
                KuriboTowerNode* pChild = pTower->getChild(j);
                al::sendMsgEnemyAttack(al::getHitSensor(pChild, 0), pSender);
                al::turnRandomVector(al::getFrontPtr(pChild), al::getFront(pChild), 0.5f);
            }
        }
    }
}
/** @brief Counts the current tower's Goombas. @param pMoveStepKeeper Platform controller. @return Number of Goombas. */
s32 getCurrentStepKuriboNum(const BrosMoveStepKeeper* pMoveStepKeeper) {
    KuriboTower* pTower = pMoveStepKeeper->tryGetStepKuribo(pMoveStepKeeper->getKeyPoseKeeper()->getKeyPoseCurrentIdx());
    if (pTower) { return pTower->getChildNum(); }
    return 0;
}

/** @brief Updates synchronized assist freezing for the current tower. @param pMoveStepKeeper Platform controller. */
void tryRequestSupportFreezeSyncCurrentStepKuribo(const BrosMoveStepKeeper* pMoveStepKeeper) {
    KuriboTower* pTower = pMoveStepKeeper->tryGetStepKuribo(pMoveStepKeeper->getKeyPoseKeeper()->getKeyPoseCurrentIdx());
    if (pTower) { pTower->setSupportFreezeSync(true); }
}

/** @brief Updates synchronized assist freezing for the current tower. @param pMoveStepKeeper Platform controller. */
void tryRequestEndSupportFreezeSyncCurrentStepKuribo(const BrosMoveStepKeeper* pMoveStepKeeper) {
    KuriboTower* pTower = pMoveStepKeeper->tryGetStepKuribo(pMoveStepKeeper->getKeyPoseKeeper()->getKeyPoseCurrentIdx());
    if (pTower) { pTower->setSupportFreezeSync(false); }
}

} // namespace rc
