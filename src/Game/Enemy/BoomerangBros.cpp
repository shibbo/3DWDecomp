#include "Enemy/BoomerangBros.hpp"

#include "Enemy/ActorStateSupportFreeze.hpp"
#include "Enemy/BoomerangBrosBoomerang.hpp"
#include "Enemy/BrosFunction.hpp"
#include "Enemy/BrosMoveStepKeeper.hpp"
#include "Enemy/BrosStateAttack.hpp"
#include "Enemy/BrosStateJump.hpp"
#include "Enemy/BrosStateWait.hpp"
#include "Enemy/EnemyStateBlowDown.hpp"
#include "Enemy/EnemyStateUtil.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/KeyPose/KeyPoseKeeper.hpp"
#include "Library/KeyPose/KeyPoseKeeperUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/StageSwitch/StageSwitchFunc.hpp"
#include "Library/Thread/Functor.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ScoreUtil.hpp"

namespace {
NERVE_DECL(BoomerangBros, Wait)
NERVE_DECL(BoomerangBros, Jump)
NERVE_DECL(BoomerangBros, Attack)
NERVE_DECL(BoomerangBros, BlowDown)
NERVE_DECL(BoomerangBros, PressDownBlow)
NERVE_DECL(BoomerangBros, SupportFreeze)
NERVE_DECL(BoomerangBros, CatchWait)
NERVE_DECL(BoomerangBros, Catch)
NERVE_DECL(BoomerangBros, PressDownPress)
// Non-const nerve objects: the game keeps them in .data in this order.
BoomerangBrosNrvWait NrvBoomerangBrosWait;
BoomerangBrosNrvJump NrvBoomerangBrosJump;
BoomerangBrosNrvAttack NrvBoomerangBrosAttack;
BoomerangBrosNrvBlowDown NrvBoomerangBrosBlowDown;
BoomerangBrosNrvPressDownBlow NrvBoomerangBrosPressDownBlow;
BoomerangBrosNrvSupportFreeze NrvBoomerangBrosSupportFreeze;
BoomerangBrosNrvCatchWait NrvBoomerangBrosCatchWait;
BoomerangBrosNrvCatch NrvBoomerangBrosCatch;
BoomerangBrosNrvPressDownPress NrvBoomerangBrosPressDownPress;

typedef al::FunctorV0M<BoomerangBros*, void (BoomerangBros::*)()> BoomerangBrosFunctor;

sead::Vector3f sWeaponHoldTrans(0.0f, -5.0f, 0.0f);
sead::Vector3f sWeaponHoldRotate(0.0f, 0.0f, 0.0f);
EnemyStateBlowDownParam sBlowDownParam(false);
ActorStateSupportFreezeParam sSupportFreezeParam(true, 15, false, true, 120,
                                                 sead::Vector3f(0.0f, 150.0f, 0.0f));
}  // namespace

/**
 * @brief Constructs a BoomerangBros.
 * @param pName Actor name.
 */
BoomerangBros::BoomerangBros(const char* pName) : al::LiveActor(pName) {}

/**
 * @brief Initializes the model, key poses, states, boomerangs, clipping and stage switches.
 * @param rInfo Placement info of the actor.
 */
void BoomerangBros::init(const al::ActorInitInfo& rInfo) {
    if (rInfo.getActorSceneInfo().isSingleMode) {
        al::initActorWithArchiveName(this, rInfo, "BoomerangBrosFur", nullptr);
    } else {
        al::initActor(this, rInfo);
    }

    mKeyPoseKeeper = al::createKeyPoseKeeper(rInfo);
    mMoveStepKeeper = new BrosMoveStepKeeper(this, mKeyPoseKeeper, rInfo);
    mStateAttack = new BrosStateAttack(this, mMoveStepKeeper);
    mStateJump = new BrosStateJump(this, mKeyPoseKeeper, mMoveStepKeeper);
    mPressDownBlowParam = new EnemyStateBlowDownParam(false);
    mPressDownBlowParam->mAction = "PressDownBlow";
    mStateBlowDown = new EnemyStateBlowDown(this, &sBlowDownParam);
    mStatePressDownBlow = new EnemyStateBlowDown(this, mPressDownBlowParam);
    mStateSupportFreeze = new ActorStateSupportFreeze(this, &sSupportFreezeParam);

    al::initNerve(this, &NrvBoomerangBrosWait, 6);
    al::initNerveState(this, new BrosStateWait(this), &NrvBoomerangBrosWait, "[state]待機");
    al::initNerveState(this, mStateJump, &NrvBoomerangBrosJump, "[state]ジャンプ");
    al::initNerveState(this, mStateAttack, &NrvBoomerangBrosAttack, "[state]攻撃");
    al::initNerveState(this, mStateBlowDown, &NrvBoomerangBrosBlowDown, "[state]吹き飛ばし");
    al::initNerveState(this, mStatePressDownBlow, &NrvBoomerangBrosPressDownBlow,
                       "[state]つぶれ吹き飛ばし");
    al::initNerveState(this, mStateSupportFreeze, &NrvBoomerangBrosSupportFreeze,
                       "[state]フリーズ");

    mWeapons = new WeaponArray(this, rInfo, 2, al::isSingleMode(rInfo));

    f32 radius;
    al::calcKeyMoveClippingInfo(&mClippingTrans, &radius, mKeyPoseKeeper, 300.0f);
    radius += al::getClippingRadius(this);
    al::setClippingInfo(this, radius, &mClippingTrans);

    mItemType = "コイン";
    al::tryGetStringArg(&mItemType, rInfo, "ItemType");
    al::listenStageSwitchOnKill(this, BoomerangBrosFunctor(this, &BoomerangBros::killBySwitch));
    al::offCollide(this);
    makeActorAppeared();

    const sead::Vector3f& trans = al::getTrans(this);
    mInitTrans.x = trans.x;
    mInitTrans.y = trans.y;
    mInitTrans.z = trans.z;
    const sead::Vector3f& front = al::getFront(this);
    mInitFront.x = front.x;
    mInitFront.y = front.y;
    mInitFront.z = front.z;
}

/** @brief Kills the boomerang held in the hand, if any. */
inline void BoomerangBros::killHoldWeapon() {
    if (mHoldWeapon != nullptr) {
        mHoldWeapon->kill();
        mHoldWeapon = nullptr;
    }
}

/** @brief Dies and makes the placed item appear. */
inline void BoomerangBros::killWithItem() {
    const char* itemType = mItemType;
    al::validateClipping(this);
    al::startHitReactionDeath(this);
    al::appearItemTiming(this, itemType);
    kill();
}

/** @brief Dies when the kill stage switch turns on. */
void BoomerangBros::killBySwitch() {
    if (al::isDead(this)) {
        return;
    }

    al::setAppearItemFactor(this, "直接攻撃", nullptr);
    killHoldWeapon();
    killWithItem();
}

/** @brief Finishes initializing the move steps once every actor is placed. */
void BoomerangBros::initAfterPlacement() {
    mMoveStepKeeper->endInit();
}

/**
 * @brief Checks whether the Boomerang Bro stands on its current key pose.
 * @return Whether it waits, attacks, catches or has landed from a jump.
 */
inline bool BoomerangBros::isOnKeyPose() const {
    const BrosStateJump* stateJump = mStateJump;
    return al::isNerve(this, &NrvBoomerangBrosWait) || al::isNerve(this, &NrvBoomerangBrosAttack) ||
           al::isNerve(this, &NrvBoomerangBrosCatch) ||
           al::isNerve(this, &NrvBoomerangBrosCatchWait) ||
           (al::isNerve(this, &NrvBoomerangBrosJump) && stateJump->isLand());
}

/** @brief Updates the move steps and keeps the actor on its current key pose. */
void BoomerangBros::control() {
    mMoveStepKeeper->update();
    if (isOnKeyPose()) {
        al::setTrans(this, al::getCurrentKeyTrans(mKeyPoseKeeper));
    }
}

/** @brief Reappears at the initial position and waits. */
void BoomerangBros::reappear() {
    makeActorAppeared();
    mKeyPoseKeeper->reset();
    al::resetPosition(this, mInitTrans, false);
    al::setFront(this, mInitFront);
    al::setNerve(this, &NrvBoomerangBrosWait);
}

/**
 * @brief Dies, making the placed item appear unless told otherwise.
 * @param isNoAppearItem Whether to die without making the item appear.
 */
void BoomerangBros::killComplete(bool isNoAppearItem) {
    if (al::isDead(this)) {
        return;
    }

    killHoldWeapon();
    if (!isNoAppearItem) {
        al::setAppearItemFactor(this, "直接攻撃", nullptr);
        const char* itemType = mItemType;
        al::validateClipping(this);
        al::appearItemTiming(this, itemType);
    }

    kill();
}

/**
 * @brief Checks whether the Boomerang Bro has been defeated.
 * @return Whether it is stomped or blown away.
 */
inline bool BoomerangBros::isDown() const {
    return al::isNerve(this, &NrvBoomerangBrosPressDownPress) ||
           al::isNerve(this, &NrvBoomerangBrosPressDownBlow) ||
           al::isNerve(this, &NrvBoomerangBrosBlowDown);
}

/**
 * @brief Attacks players, catches returning boomerangs and pushes objects.
 * @param pSelf Sensor of the Boomerang Bro.
 * @param pOther Sensor that was hit.
 */
void BoomerangBros::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isSensorEnemyAttack(pSelf) &&
        (al::isSensorPlayer(pOther) || al::isSensorKoopaJr(pOther))) {
        if (isDown()) {
            return;
        }

        al::sendMsgPush(pOther, pSelf);
        al::sendMsgEnemyAttackForCrossoverSensor(pOther, pSelf);
        return;
    }

    if (al::isNerve(this, &NrvBoomerangBrosCatchWait)) {
        for (s32 i = 0; i < mWeapons->size(); i++) {
            if (al::getSensorHost(pOther) == mWeapons->getWeapon(i)) {
                mCatchWeapon = mWeapons->getWeapon(i);
                al::setNerve(this, &NrvBoomerangBrosCatch);
                return;
            }
        }
    }

    if (!GameDataFunction::isSingleMode(this)) {
        return;
    }

    if ((al::isSensorEnemyAttack(pSelf) &&
         (al::isSensorNpc(pOther) || al::isSensorEnemyBody(pOther))) ||
        al::isSensorDoorKey(pOther) || al::isSensorHostName(pOther, "コウラ") ||
        al::isSensorHostName(pOther, "パックンフラワー（鉢植えあり）") ||
        al::isSensorHostName(pOther, "BallNeko")) {
        if (isDown()) {
            return;
        }

        al::sendMsgPush(pOther, pSelf);
        rc::sendMsgPushConnected(pOther, pSelf);
    }
}

/**
 * @brief Handles pushes, stomps, boomerang hits, blow downs and thrown keys.
 * @param pMsg Received message.
 * @param pOther Sensor that sent the message.
 * @param pSelf Sensor of the Boomerang Bro.
 * @return Whether the message was handled.
 */
bool BoomerangBros::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                               al::HitSensor* pSelf) {
    if (al::tryReceiveMsgPushAndAddVelocity(this, pMsg, pOther, pSelf, 1.0f)) {
        return true;
    }

    if (isDown()) {
        return false;
    }

    if (!al::isSensorEnemyBody(pSelf)) {
        return false;
    }

    if (al::isMsgPlayerObjHipDropReflectAll(pMsg)) {
        rc::setAppearItemFactorByMsg(this, pMsg, pOther);
        rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        al::setNerve(this, &NrvBoomerangBrosPressDownPress);
        rc::tryRequestEndSupportFreezeSyncCurrentStepKuribo(mMoveStepKeeper);
        killHoldWeapon();
        rc::addScoreCombo(this, pOther, pMsg, 0.0f);
        mStatePressDownBlow->setBlowDir(al::getSensorHost(pOther));
        return true;
    }

    if (EnemyStateUtil::tryRequestPressDownAndNextNerve(pMsg, pOther, pSelf, this,
                                                        &NrvBoomerangBrosPressDownPress, true)) {
        rc::tryRequestEndSupportFreezeSyncCurrentStepKuribo(mMoveStepKeeper);
        killHoldWeapon();
        rc::addScoreCombo(this, pOther, pMsg, 0.0f);
        mStatePressDownBlow->setBlowDir(al::getSensorHost(pOther));
        return true;
    }

    if (al::isMsgEnemyAttackBoomerang(pMsg)) {
        for (s32 i = 0; i < mWeapons->size(); i++) {
            if (al::getSensorHost(pOther) == mWeapons->getWeapon(i)) {
                if (al::isNerve(this, &NrvBoomerangBrosCatchWait) ||
                    !mWeapons->getWeapon(i)->isMoveBack()) {
                    return false;
                }
            }
        }

        rc::tryRequestEndSupportFreezeSyncCurrentStepKuribo(mMoveStepKeeper);
        killHoldWeapon();
        rc::addScoreCombo(this, pOther, pMsg, 0.0f);
        EnemyStateUtil::requestBlowDown(pMsg, pOther, pSelf, mStateBlowDown, true);
        al::setNerve(this, &NrvBoomerangBrosBlowDown);
        return true;
    }

    if (EnemyStateUtil::tryRequestBlowDownAndNextNerve(pMsg, pOther, pSelf, mStateBlowDown,
                                                       &NrvBoomerangBrosBlowDown, true)) {
        rc::tryRequestEndSupportFreezeSyncCurrentStepKuribo(mMoveStepKeeper);
        killHoldWeapon();
        rc::addScoreCombo(this, pOther, pMsg, 0.0f);
        return true;
    }

    if (al::isMsgKeyThrow(pMsg) && GameDataFunction::isSingleMode(this)) {
        killHoldWeapon();
        EnemyStateUtil::requestBlowDown(pMsg, pOther, pSelf, mStateBlowDown, true);
        rc::tryRequestEndSupportFreezeSyncCurrentStepKuribo(mMoveStepKeeper);
        al::setNerve(this, &NrvBoomerangBrosBlowDown);
        return true;
    }

    return false;
}

/**
 * @brief Freezes the Boomerang Bro when it is touched on the screen.
 * @param pMsg Received message.
 * @param pPointer Screen pointer that touched.
 * @param pTarget Touched screen point target.
 * @return Whether the message was handled.
 */
bool BoomerangBros::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                          al::ScreenPointTarget* pTarget) {
    if (!mStateSupportFreeze->receiveMsgScreenPoint(pMsg, pPointer, pTarget)) {
        return false;
    }

    if (isOnKeyPose()) {
        al::setNerve(this, &NrvBoomerangBrosSupportFreeze);
    }

    return true;
}

/** @brief Jumps to the next key pose, or attacks again when there is only one. */
inline void BoomerangBros::startNextMove() {
    if (al::getKeyPoseCount(mKeyPoseKeeper) <= 1) {
        al::setNerve(this, &NrvBoomerangBrosAttack);
    } else {
        al::setNerve(this, &NrvBoomerangBrosJump);
    }
}

/** @brief Waits and then moves on. */
void BoomerangBros::exeWait() {
    if (al::updateNerveState(this)) {
        al::invalidateClipping(this);
        startNextMove();
    }
}

/** @brief Takes the next boomerang in the hand. */
inline void BoomerangBros::holdNextWeapon() {
    BoomerangBrosBoomerang* weapon = mWeapons->rotateNext();
    mHoldWeapon = weapon;
    weapon->attach(al::getJointMtxPtr(this, "HandR"), sWeaponHoldTrans, sWeaponHoldRotate);
    al::startAction(weapon, "JumpStart");
}

/** @brief Jumps to the next key pose holding a boomerang, then attacks. */
void BoomerangBros::exeJump() {
    if (al::isFirstStep(this)) {
        holdNextWeapon();
    }

    if (al::updateNerveState(this)) {
        al::validateClipping(this);
        al::setNerve(this, &NrvBoomerangBrosAttack);
    }
}

/** @brief Takes a boomerang on the sign and throws it forward, then waits for it to return. */
void BoomerangBros::exeAttack() {
    if (mStateAttack->isSignStep() && mHoldWeapon == nullptr) {
        holdNextWeapon();
    }

    if (mStateAttack->isAttackStep()) {
        sead::Vector3f dir;
        al::calcFrontDir(&dir, this);
        mHoldWeapon->shoot(dir * 20.0f);
        mHoldWeapon = nullptr;
    }

    al::updateNerveStateAndNextNerve(this, &NrvBoomerangBrosCatchWait);
}

/** @brief Waits until every boomerang is back, then moves on. */
void BoomerangBros::exeCatchWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait");
    }

    if (mWeapons->isAnyAlive()) {
        return;
    }

    startNextMove();
}

/** @brief Catches a returning boomerang. */
void BoomerangBros::exeCatch() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Catch");
        if (mCatchWeapon != nullptr) {
            mCatchWeapon->caught(al::getJointMtxPtr(this, "HandR"), sWeaponHoldTrans,
                                 sWeaponHoldRotate);
            mCatchWeapon = nullptr;
        }
    }

    if (al::isActionEnd(this)) {
        if (mWeapons->isAnyAlive()) {
            al::setNerve(this, &NrvBoomerangBrosCatchWait);
        } else {
            startNextMove();
        }
    }
}

/** @brief Gets stomped flat, then gets blown away. */
void BoomerangBros::exePressDownPress() {
    if (al::isFirstStep(this)) {
        al::setVelocityZero(this);
        al::startAction(this, "PressDownPress");
    }

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvBoomerangBrosPressDownBlow);
    }
}

/** @brief Gets blown away after being stomped and dies. */
void BoomerangBros::exePressDownBlow() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "PressDownBlow");
    }

    if (al::updateNerveState(this)) {
        killWithItem();
    }
}

/** @brief Gets blown away and dies. */
void BoomerangBros::exeBlowDown() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "BlowDown");
    }

    if (al::updateNerveState(this)) {
        killWithItem();
    }
}

/** @brief Stays frozen together with the Goombas of the current step, then drops the boomerangs. */
void BoomerangBros::exeSupportFreeze() {
    if (al::isFirstStep(this)) {
        rc::tryRequestSupportFreezeSyncCurrentStepKuribo(mMoveStepKeeper);
    }

    if (al::updateNerveState(this)) {
        rc::tryRequestEndSupportFreezeSyncCurrentStepKuribo(mMoveStepKeeper);
        al::setNerve(this, &NrvBoomerangBrosWait);
        if (mCatchWeapon != nullptr) {
            mCatchWeapon->kill();
            mCatchWeapon = nullptr;
        }

        killHoldWeapon();
    }
}
