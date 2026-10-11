#include "Enemy/KuriboTowerChild.hpp"

#include <attributes.h>
#include <math/seadMathCalcCommon.h>

#include "Enemy/ActorMicRumbler.hpp"
#include "Enemy/ActorStateSupportFreeze.hpp"
#include "Enemy/EnemyStateBlowDown.hpp"
#include "Enemy/EnemyStateUtil.hpp"
#include "Enemy/KuriboTower.hpp"
#include "Enemy/TargetFinder.hpp"
#include "Enemy/WalkerStateChase.hpp"
#include "Enemy/WalkerStateFindPlayer.hpp"
#include "Enemy/WalkerStateJump.hpp"
#include "Enemy/WalkerStateParam.hpp"
#include "Enemy/WalkerStateRailMove.hpp"
#include "Enemy/WalkerStateWander.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Item/ItemUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/LiveActor/SubActorUtil.hpp"
#include "Library/LiveActor/Util/ActorAnimUtil.hpp"
#include "Library/LiveActor/Util/ActorModelUtil.hpp"
#include "Library/LiveActor/Util/ActorSceneUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Rail/RailUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/InkUtil.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ScoreUtil.hpp"

// Nerve that also ends a behavior when it is left.
#define KURIBO_TOWER_CHILD_NERVE_END_DECL(Action)                                                  \
    class KuriboTowerChildNrv##Action : public al::Nerve {                                         \
    public:                                                                                        \
        void execute(al::NerveKeeper* pKeeper) const override {                                    \
            (pKeeper->getParent<KuriboTowerChild>())->exe##Action();                               \
        }                                                                                          \
                                                                                                   \
        void executeOnEnd(al::NerveKeeper* pKeeper) const override {                               \
            (pKeeper->getParent<KuriboTowerChild>())->end##Action();                               \
        }                                                                                          \
    };

namespace {
NERVE_DECL(KuriboTowerChild, Wait)
NERVE_DECL(KuriboTowerChild, Wander)
NERVE_DECL(KuriboTowerChild, RailMove)
NERVE_DECL(KuriboTowerChild, Chase)
NERVE_DECL(KuriboTowerChild, StandBy)
NERVE_DECL(KuriboTowerChild, FindPlayer)
NERVE_DECL(KuriboTowerChild, BlowDown)
NERVE_DECL(KuriboTowerChild, SupportFreeze)
NERVE_DECL(KuriboTowerChild, Fall)
NERVE_DECL(KuriboTowerChild, PressDown)
NERVE_DECL(KuriboTowerChild, HipDropDown)
NERVE_DECL(KuriboTowerChild, EatDown)
KURIBO_TOWER_CHILD_NERVE_END_DECL(Sleep)
NERVE_DECL(KuriboTowerChild, Land)
NERVE_DECL(KuriboTowerChild, Swallow)
NERVE_DECL(KuriboTowerChild, Surprise)
NERVE_DECL(KuriboTowerChild, SupportFreezeSync)
NERVE_DECL(KuriboTowerChild, Attack)
// Non-const nerve objects (merged into one .data block); Sleep and Land are constant.
KuriboTowerChildNrvWait NrvKuriboTowerChildWait;
KuriboTowerChildNrvWander NrvKuriboTowerChildWander;
KuriboTowerChildNrvRailMove NrvKuriboTowerChildRailMove;
KuriboTowerChildNrvChase NrvKuriboTowerChildChase;
KuriboTowerChildNrvStandBy NrvKuriboTowerChildStandBy;
KuriboTowerChildNrvFindPlayer NrvKuriboTowerChildFindPlayer;
KuriboTowerChildNrvBlowDown NrvKuriboTowerChildBlowDown;
KuriboTowerChildNrvSupportFreeze NrvKuriboTowerChildSupportFreeze;
KuriboTowerChildNrvFall NrvKuriboTowerChildFall;
KuriboTowerChildNrvPressDown NrvKuriboTowerChildPressDown;
KuriboTowerChildNrvHipDropDown NrvKuriboTowerChildHipDropDown;
KuriboTowerChildNrvEatDown NrvKuriboTowerChildEatDown;
const KuriboTowerChildNrvSleep NrvKuriboTowerChildSleep{};
const KuriboTowerChildNrvLand NrvKuriboTowerChildLand{};
KuriboTowerChildNrvSwallow NrvKuriboTowerChildSwallow;
KuriboTowerChildNrvSurprise NrvKuriboTowerChildSurprise;
KuriboTowerChildNrvSupportFreezeSync NrvKuriboTowerChildSupportFreezeSync;
KuriboTowerChildNrvAttack NrvKuriboTowerChildAttack;

TargetFinderParam sTargetFinderParam(1500.0f, 180.0f, 75.0f, 90, -1.0f, 500.0f, 500.0f, 2000.0f,
                                     true);
WalkerStateParam sWalkerStateParam(1.5f, 0.98f, 0.9f, 500.0f, 1500.0f, 70.0f, 80.0f, 150.0f);
WalkerStateWanderParam sWanderParam(10, 120, 0.2f, 3.0f, 20.0f, 500.0f, true, "Walk", "Wait");
WalkerStateFindPlayerParam sFindPlayerParam(30, 7.5f, true, "Walk");
WalkerStateParam sFindPlayerWalkerStateParam(1.0f, 0.99f, 0.9f, 500.0f, 1500.0f, 70.0f, 80.0f,
                                             150.0f);
WalkerStateJumpParam sJumpParam(14.0f, "Find", true);
WalkerStateChaseParam sChaseParam(0.8f, 130.0f, 500.0f, 2.8f, 5.0f, false, true, "Run", "Wait",
                                  -1.0f);
ActorStateSupportFreezeParam sSupportFreezeParam(true, 15, false, true, 120,
                                                 sead::Vector3f(0.0f, 150.0f, 0.0f));
EnemyStateBlowDownParam sBlowDownParam(false);
WalkerStateRailMoveParam sRailMoveParam;

/** @brief Action of the bottom Goomba and the matching action of the Goombas stacked on it. */
struct TowerActionName {
    const char* mAction;
    const char* mTowerAction;
};

const TowerActionName cTowerActionNames[] = {
    {"Run", "RunTower"},           {"Walk", "WalkTower"},          {"Wait", "WaitTower"},
    {"Fall", "FallTower"},         {"Land", "LandTower"},          {"Surprise", "SurpriseTower"},
    {"Find", "SurpriseTower"},     {"Attack", "SurpriseTower"},    {"Sleep", "SleepTower"},
};

/**
 * @brief Copies a vector one component at a time.
 * @param pDst Destination vector.
 * @param rSrc Source vector.
 */
inline void copyVector(sead::Vector3f* pDst, const sead::Vector3f& rSrc) {
    pDst->x = rSrc.x;
    pDst->y = rSrc.y;
    pDst->z = rSrc.z;
}

/**
 * @brief Calculates a random upward direction in which the item of a defeated Goomba pops out.
 * @param pDir Output direction.
 */
inline void calcItemAppearDirRandom(sead::Vector3f* pDir) {
    f32 angleH = al::getRandom(sead::Mathf::pi2());
    f32 angleV = al::getRandom(-0.7853982f, 0.7853982f) + sead::Mathf::piHalf();
    f32 sinV = sinf(angleV);
    pDir->set(sinV * cosf(angleH), cosf(angleV), sinV * sinf(angleH));
}
}  // namespace

/**
 * @brief Constructs a Goomba Tower child.
 * @param pName Actor name.
 */
KuriboTowerChild::KuriboTowerChild(const char* pName) : KuriboTowerNode(pName) {}

/** @brief Kills the Goomba in lethal areas, updates the mic rumbler and touches floors. */
void KuriboTowerChild::control() {
    if (rc::isInDeathArea(this) || rc::isCollidedDamageFire(this) ||
        (GameDataFunction::isSingleMode(GameDataHolderAccessor(this)) &&
         (rc::isCollidedInkSlow(this) || InkUtil::isInInkLimitSphere(this) ||
          rc::isInWaterArea(this)))) {
        mHost->releaseChild(this);
        al::startHitReactionDeath(this);
        kill();
        return;
    }

    if (isEnableDown()) {
        mMicRumbler->update();
    }

    if (!al::isNerve(this, &NrvKuriboTowerChildWait) && al::isCollidedGround(this) &&
        mFloorSensor != nullptr) {
        al::sendMsgEnemyFloorTouch(mFloorSensor, al::getHitSensor(this, "Body"));
    }

    al::HitSensor* floorSensor = al::tryGetCollidedGroundSensor(this);
    if ((al::getTrans(this) - mFloorTouchTrans).length() > 5.0f) {
        mFloorSensor = floorSensor;
        copyVector(&mFloorTouchTrans, al::getTrans(this));
    }

    // The original requires all three nerves at once, so this never stops the Goomba.
    if (mHost->isBottom(this) && mHost->isHipDropping() &&
        al::isNerve(this, &NrvKuriboTowerChildWander) &&
        al::isNerve(this, &NrvKuriboTowerChildRailMove) &&
        al::isNerve(this, &NrvKuriboTowerChildChase)) {
        al::setNerve(this, &NrvKuriboTowerChildWait);
    }
}

/**
 * @brief Initializes the model, states and nerves.
 * @param rInfo Placement info of the actor.
 */
void KuriboTowerChild::init(const al::ActorInitInfo& rInfo) {
    if (rInfo.getActorSceneInfo().isSingleMode) {
        al::initActorWithArchiveName(this, rInfo, "KuriboFur", nullptr);
    } else {
        al::initActorWithArchiveName(this, rInfo, "Kuribo", nullptr);
    }

    al::invalidateClipping(this);
    if (al::isExistRail(this)) {
        al::initNerve(this, &NrvKuriboTowerChildStandBy, 6);
        mStateRailMove = new WalkerStateRailMove(this, &sWalkerStateParam, &sRailMoveParam);
        al::initNerveState(this, mStateRailMove, &NrvKuriboTowerChildRailMove,
                           "[state]レール移動");
    } else {
        al::initNerve(this, &NrvKuriboTowerChildStandBy, 5);
    }

    mTargetFinder = new TargetFinder(this, &sTargetFinderParam);
    mStateWander = new WalkerStateWander(this, al::getFrontPtr(this), &sWalkerStateParam,
                                         &sWanderParam, mTargetFinder);
    mStateChase = new WalkerStateChase(this, al::getFrontPtr(this), mTargetFinder,
                                       &sWalkerStateParam, &sChaseParam, true, nullptr);
    mStateFindPlayer = new WalkerStateFindPlayer(this, al::getFrontPtr(this), mTargetFinder,
                                                 &sFindPlayerWalkerStateParam, &sFindPlayerParam,
                                                 &sJumpParam);
    mStateBlowDown = new EnemyStateBlowDown(this, &sBlowDownParam);
    mStateSupportFreeze = new ActorStateSupportFreeze(this, &sSupportFreezeParam);
    al::initNerveState(this, mStateWander, &NrvKuriboTowerChildWander, "[state]徘徊");
    al::initNerveState(this, mStateChase, &NrvKuriboTowerChildChase, "[state]追いかけ");
    al::initNerveState(this, mStateFindPlayer, &NrvKuriboTowerChildFindPlayer,
                       "[state]プレーヤー発見");
    al::initNerveState(this, mStateBlowDown, &NrvKuriboTowerChildBlowDown, "[state]吹き飛ばし");
    al::initNerveState(this, mStateSupportFreeze, &NrvKuriboTowerChildSupportFreeze,
                       "[state]フリーズ");
    mMicRumbler = new ActorMicRumbler(this, nullptr);
    al::invalidateShadow(this);
    al::createAndSetColliderSpecialPurpose(this, "MoveLimit");
    makeActorDead();
}

/** @brief Appears on top of the tower and falls onto the Goomba below. */
void KuriboTowerChild::reappear() {
    const char* actionName;
    if (mHost->getBottom()->isStopWalk()) {
        actionName = "WaitTower";
    } else {
        actionName = mHost->isBrosOnTower() ? "RunBrosOnTower" : "RunTower";
    }

    if (!al::isSklAnimPlaying(this, actionName, 0)) {
        al::startAction(this, actionName);
    }

    makeActorAppeared();
    al::offCollide(this);
    al::setNerve(this, &NrvKuriboTowerChildFall);
}

/**
 * @brief Pushes other enemies, map objects and NPCs and attacks players.
 * @param pSelf Sensor of the Goomba.
 * @param pOther Sensor that was hit.
 */
void KuriboTowerChild::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isSensorEnemyBody(pOther)) {
        if (isValidPush()) {
            al::sendMsgPushAndKillVelocityToTarget(this, pSelf, pOther);
        } else if (al::isNerve(this, &NrvKuriboTowerChildFall)) {
            al::sendMsgPush(pOther, pSelf);
        }
    }

    if (al::isSensorEnemyAttack(pSelf) &&
        (al::isSensorPlayer(pOther) || al::isSensorKoopaJr(pOther))) {
        if (!isEnableDown()) {
            return;
        }

        al::sendMsgPush(pOther, pSelf);
        if (al::sendMsgEnemyAttackForCrossoverSensor(pOther, pSelf)) {
            mHost->requestAttackReaction(pOther);
        }

        return;
    }

    if (GameDataFunction::isSingleMode(GameDataHolderAccessor(this)) &&
        al::isSensorGoalItem(pOther)) {
        al::sendMsgEnemyAttackFire(pOther, pSelf);
    }

    if (al::isSensorMapObj(pOther) ||
        (GameDataFunction::isSingleMode(GameDataHolderAccessor(this)) &&
         al::isSensorNpc(pOther))) {
        al::sendMsgPush(pOther, pSelf);
    }
}

/**
 * @brief Checks whether the Goomba can be pushed by and push other enemies.
 * @return Whether it is moving around normally.
 */
bool KuriboTowerChild::isValidPush() const {
    bool isValid = true;
    if (al::isNerve(this, &NrvKuriboTowerChildStandBy) ||
        al::isNerve(this, &NrvKuriboTowerChildFall) ||
        al::isNerve(this, &NrvKuriboTowerChildPressDown) ||
        al::isNerve(this, &NrvKuriboTowerChildHipDropDown)) {
        isValid = false;
    } else if (al::isNerve(this, &NrvKuriboTowerChildBlowDown)) {
        isValid = false;
    }

    return isValid;
}

// Defined after its first callers (as in the original), which places its body's nerve
// references after init's in the merged nerve block.
/**
 * @brief Checks whether the Goomba can still be defeated.
 * @return Whether it is not already being defeated.
 */
bool KuriboTowerChild::isEnableDown() const {
    if (al::isNerve(this, &NrvKuriboTowerChildPressDown)) {
        return false;
    }

    if (al::isNerve(this, &NrvKuriboTowerChildHipDropDown)) {
        return false;
    }

    if (al::isNerve(this, &NrvKuriboTowerChildBlowDown)) {
        return false;
    }

    if (al::isNerve(this, &NrvKuriboTowerChildEatDown)) {
        return false;
    }

    return !al::isNerve(this, &NrvKuriboTowerChildSwallow);
}

/**
 * @brief Handles pushes, stomps, hip drops, blow downs, Piranha Plant eating and goal kills.
 * @param pMsg Received message.
 * @param pOther Sensor that sent the message.
 * @param pSelf Sensor of the Goomba.
 * @return Whether the message was handled.
 */
bool KuriboTowerChild::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                                  al::HitSensor* pSelf) {
    if (isValidPush() && al::tryReceiveMsgPushAndAddVelocity(this, pMsg, pOther, pSelf, 1.0f)) {
        return true;
    }

    if (!isEnableDown()) {
        return false;
    }

    if (!al::isSensorEnemyBody(pSelf)) {
        return false;
    }

    if (mHost->isTopChildNode(this) ||
        (!mHost->isContactTop(this) && (mHost->isContact(this) || mHost->isBottom(this)))) {
        if (al::isMsgPlayerObjHipDropAll(pMsg)) {
            mHipDropSensor = pOther;
            rc::setAppearItemFactorByMsg(this, pMsg, pOther);
            rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
            al::offSyncClippingSubActor(mHost, this);
            al::setNerve(this, &NrvKuriboTowerChildHipDropDown);
            rc::addScoreCombo(this, pOther, pMsg, 100.0f);
            return true;
        }

        if (EnemyStateUtil::tryRequestPressDownAndNextNerve(pMsg, pOther, pSelf, this,
                                                            &NrvKuriboTowerChildPressDown, true)) {
            al::offSyncClippingSubActor(mHost, this);
            rc::setAppearItemFactorByMsg(this, pMsg, pOther);
            rc::addScoreCombo(this, pOther, pMsg, 100.0f);
            return true;
        }
    } else {
        if (al::isMsgPlayerObjHipDropAll(pMsg)) {
            // Hip drops onto a Goomba in the middle of the tower are handled by the top Goomba.
        }

        if (al::isMsgBallTrample(pMsg)) {
            rc::setAppearItemFactorByMsg(this, pMsg, pOther);
            rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
            al::offSyncClippingSubActor(mHost, this);
            al::setNerve(this, &NrvKuriboTowerChildPressDown);
            rc::addScoreCombo(this, pOther, pMsg, 100.0f);
            return true;
        }
    }

    if (EnemyStateUtil::tryRequestBlowDownAndNextNerve(pMsg, pOther, pSelf, mStateBlowDown,
                                                       &NrvKuriboTowerChildBlowDown, true)) {
        al::offSyncClippingSubActor(mHost, this);
        rc::setAppearItemFactorByMsg(this, pMsg, pOther);
        rc::addScoreCombo(this, pOther, pMsg, 100.0f);
        return true;
    }

    if (al::isMsgEnemyAttack(pMsg) || rc::isMsgBubbleAttack(pMsg)) {
        al::offSyncClippingSubActor(mHost, this);
        al::onCollide(this);
        rc::startHitReactionBlowHitMessage(pMsg, this, pOther, pSelf);
        rc::setAppearItemFactorByMsg(this, pMsg, pOther);
        mStateBlowDown->setBlowDir(pOther, pSelf);
        al::setNerve(this, &NrvKuriboTowerChildBlowDown);
        return true;
    }

    if (rc::isMsgPackunEatStart(pMsg)) {
        return true;
    }

    if (rc::isMsgPackunEat(pMsg)) {
        al::offSyncClippingSubActor(mHost, this);
        mEatSensor = pOther;
        rc::setAppearItemFactorByMsg(this, pMsg, pOther);
        rc::addScoreCombo(this, pOther, pMsg, 100.0f);
        al::startAction(this, "Wait");
        al::setNerve(this, &NrvKuriboTowerChildEatDown);
        return true;
    }

    if (al::isMsgGoalKill(pMsg)) {
        al::startHitReactionDeath(this);
        al::setAppearItemFactor(this, "直接攻撃", pOther);
        al::appearItem(this);
        rc::addScoreCombo(this, pOther, pMsg, 100.0f);
        mHost->endSleep();
        kill();
        return true;
    }

    return false;
}

/** @brief Wakes up the whole tower. */
void KuriboTowerChild::endSleep() {
    mHost->endSleep();
}

/**
 * @brief Freezes the Goomba when it is touched on the touch screen.
 * @param pMsg Received message.
 * @param pPointer Screen pointer that touched.
 * @param pTarget Touched screen point target.
 * @return Whether the message was handled.
 */
bool KuriboTowerChild::receiveMsgScreenPoint(const al::SensorMsg* pMsg,
                                             al::ScreenPointer* pPointer,
                                             al::ScreenPointTarget* pTarget) {
    if (!mStateSupportFreeze->receiveMsgScreenPoint(pMsg, pPointer, pTarget)) {
        return false;
    }

    if (!al::isNerve(this, &NrvKuriboTowerChildSupportFreeze) &&
        !al::isNerve(this, &NrvKuriboTowerChildPressDown) &&
        !al::isNerve(this, &NrvKuriboTowerChildHipDropDown) &&
        !al::isNerve(this, &NrvKuriboTowerChildBlowDown)) {
        al::setNerve(this, &NrvKuriboTowerChildSupportFreeze);
    }

    return true;
}

/** @brief Sets the wander center and puts a sleeping tower's bottom Goomba to sleep. */
void KuriboTowerChild::endInit() {
    mStateWander->setWanderCenter(al::getTrans(this));
    if (mHost->isBottom(this) && mHost->isStartSleep()) {
        al::setNerve(this, &NrvKuriboTowerChildSleep);
    }
}

/**
 * @brief Gets the action of a stacked Goomba that matches the action of the bottom Goomba.
 * @return Name of the action to play.
 */
ALWAYS_INLINE inline const char* KuriboTowerChild::getStandByActionName() const {
    const KuriboTowerNode* bottom = mHost->getBottom();
    if (bottom->isStopWalk()) {
        return "WaitTower";
    }

    if (al::isSklAnimPlaying(bottom, "Run", 0)) {
        return mHost->isBrosOnTower() ? "RunBrosOnTower" : "RunTower";
    }

    for (s32 i = 0; i < 9; i++) {
        if (al::isSklAnimPlaying(bottom, cTowerActionNames[i].mAction, 0)) {
            return cTowerActionNames[i].mTowerAction;
        }
    }

    return "WaitTower";
}

/** @brief Rides on the tower, mirroring the action of the bottom Goomba. */
void KuriboTowerChild::exeStandBy() {
    updatePosture(true);
    const char* actionName = getStandByActionName();
    if (!al::isSklAnimPlaying(this, actionName, 0)) {
        al::startAction(this, actionName);
    }

    if (al::isSingleMode(this) && mHost->isBottom(this) && al::isFirstStep(this)) {
        updateCollider();
    }

    if ((mHost->isBottom(this) && al::isOnGround(this, 2, 0.0f)) || mHost->isContact(this)) {
        return;
    }

    al::setNerve(this, &NrvKuriboTowerChildFall);
}

/** @brief Waits until a player comes near, then walks along the rail or wanders around. */
void KuriboTowerChild::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait");
        al::setVelocityToGravity(this, sWalkerStateParam.mGravity);
    }

    updateVelocity();
    mTargetFinder->update();
    if (!isActive(2000.0f) || mHost->isHipDropping()) {
        return;
    }

    if (al::isExistRail(this)) {
        al::setNerve(this, &NrvKuriboTowerChildRailMove);
        return;
    }

    al::setNerve(this, &NrvKuriboTowerChildWander);
}

/**
 * @brief Checks whether the Goomba should be active.
 * @param distance Distance to the nearest player within which it is active.
 * @return Whether it is in the air or near a player.
 */
bool KuriboTowerChild::isActive(f32 distance) const {
    if (!al::isOnGround(this, 0, 0.0f)) {
        return true;
    }

    return al::isNearPlayer(this, distance);
}

/** @brief Sleeps until the tower is woken up. */
void KuriboTowerChild::exeSleep() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Sleep");
        al::setVelocityToGravity(this, sWalkerStateParam.mGravity);
    }

    updateVelocity();
}

/** @brief Wanders around, goes back to waiting when no player is near and falls off ledges. */
void KuriboTowerChild::exeWander() {
    al::updateNerveStateAndNextNerve(this, &NrvKuriboTowerChildFindPlayer);
    if (!isActive(2500.0f)) {
        al::setNerve(this, &NrvKuriboTowerChildWait);
        return;
    }

    mHost->revieseVelocityIfExistObstacleForward(this);
    if (!al::isOnGround(this, 2, 0.0f)) {
        al::setNerve(this, &NrvKuriboTowerChildFall);
    }
}

/** @brief Walks along the rail until a player is found. */
void KuriboTowerChild::exeRailMove() {
    al::updateNerveState(this);
    mTargetFinder->update();
    if (mTargetFinder->isFoundTarget()) {
        al::setNerve(this, &NrvKuriboTowerChildFindPlayer);
    }
}

/** @brief Turns toward a found player and then chases it. */
void KuriboTowerChild::exeFindPlayer() {
    al::updateNerveStateAndNextNerve(this, &NrvKuriboTowerChildChase);
    mHost->revieseVelocityIfExistObstacleForward(this);
}

/** @brief Chases the target and goes back to its rail or wanders around when it is lost. */
void KuriboTowerChild::exeChase() {
    if (al::updateNerveState(this)) {
        if (al::isExistRail(this)) {
            al::setNerve(this, &NrvKuriboTowerChildRailMove);
        } else {
            al::setNerve(this, &NrvKuriboTowerChildWander);
        }

        mStateWander->setWanderCenter(al::getTrans(this));
        return;
    }

    mHost->revieseVelocityIfExistObstacleForward(this);
}

/** @brief Plays the attack animation. */
void KuriboTowerChild::exeAttack() {
    if (al::isFirstStep(this)) {
        al::setVelocityZero(this);
        al::startAction(this, "Attack");
    }

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvKuriboTowerChildWait);
    }

    mHost->revieseVelocityIfExistObstacleForward(this);
}

/** @brief Falls until it lands on the ground or on the Goomba below. */
void KuriboTowerChild::exeFall() {
    if (al::isFirstStep(this)) {
        al::startAction(this, mHost->isBottom(this) ? "Fall" : "FallTower");
    }

    updateVelocity();
    restrictToTowerPosition();
    sead::Vector3f* velocity = al::getVelocityPtr(this);
    al::parallelizeVec(velocity, al::getGravity(this), *velocity);
    if ((mHost->isBottom(this) && al::isOnGround(this, 2, 0.0f)) || mHost->isContact(this)) {
        al::startAction(this, mHost->isBottom(this) ? "Land" : "LandTower");
        al::setNerve(this, &NrvKuriboTowerChildLand);
    }
}

/** @brief Lands and then starts moving (bottom Goomba) or rides on the tower. */
void KuriboTowerChild::exeLand() {
    if (al::isFirstStep(this)) {
        // The landing action is started by exeFall.
    }

    updateVelocity();
    restrictToTowerPosition();
    if (!al::isActionEnd(this)) {
        return;
    }

    if (!mHost->isBottom(this)) {
        al::setNerve(this, &NrvKuriboTowerChildStandBy);
        return;
    }

    if (al::isExistRail(this)) {
        al::setNerve(this, &NrvKuriboTowerChildRailMove);
        return;
    }

    al::setNerve(this, &NrvKuriboTowerChildWander);
}

/** @brief Gets stomped flat and vanishes. */
void KuriboTowerChild::exePressDown() {
    if (al::isFirstStep(this)) {
        al::startAction(this, mHost->isBottom(this) ? "PressDown" : "PressDownTower");
        mHost->releaseChild(this);
        mMicRumbler->stopAndReset();
        al::changeEnvTextureStamp(this);
        al::setVelocityZero(this);
    }

    rc::tryAppearItemPressDown(this, nullptr);

    if (al::isActionEnd(this)) {
        al::startHitReactionDeath(this);
        al::resetEnvTexture(this);
        al::startSe(this, "Vanish", nullptr);
        kill();
    }
}

/** @brief Gets flattened by a hip drop, following the attacker down, and vanishes. */
void KuriboTowerChild::exeHipDropDown() {
    if (al::isFirstStep(this)) {
        mHost->releaseChild(this);
        mMicRumbler->stopAndReset();
        if (al::isSensorPlayer(mHipDropSensor)) {
            mHost->startHipDrop(al::getSensorHost(mHipDropSensor));
        }

        al::startAction(this, "PressDown");
        al::changeEnvTextureStamp(this);
        al::onCollide(this);
    }

    f32 offset = mHost->getHipDropDownOffset(this);
    const al::LiveActor* attacker = al::getSensorHost(mHipDropSensor);
    if (offset + al::getTrans(attacker).y < al::getTrans(this).y) {
        sead::Vector3f target = al::getTrans(attacker);
        target.y = offset + target.y;
        sead::Vector3f velocity = target - al::getTrans(this);
        al::parallelizeVec(&velocity, al::getGravity(this), velocity);
        al::setVelocity(this, velocity);
    } else {
        al::setVelocityZero(this);
    }

    if (!al::isActionEnd(this)) {
        return;
    }

    if (mHost->isHipDropping() && mHost->getChildNum() != 0) {
        return;
    }

    sead::Vector3f dir;
    calcItemAppearDirRandom(&dir);
    sead::Vector3f trans = al::getTrans(this);
    trans.y += 20.0f;
    al::appearItemTiming(this, "連続ヒップドロップ", trans, dir);
    al::startHitReactionDeath(this);
    al::resetEnvTexture(this);
    al::startSe(this, "Vanish", nullptr);
    kill();
}

/** @brief Gets blown away and vanishes. */
void KuriboTowerChild::exeBlowDown() {
    if (al::isFirstStep(this)) {
        mHost->releaseChild(this);
        mMicRumbler->stopAndReset();
    }

    if (al::updateNerveState(this)) {
        sead::Vector3f dir;
        calcItemAppearDirRandom(&dir);
        sead::Vector3f trans = al::getTrans(this);
        trans.y += 20.0f;
        al::appearItemTiming(this, "クリボータワー吹き飛び死亡", trans, dir);
        al::startHitReactionDeath(this);
        al::startSe(this, "Vanish", nullptr);
        kill();
    }
}

/** @brief Stays frozen and goes back to its tower behavior afterwards. */
void KuriboTowerChild::exeSupportFreeze() {
    if (al::isFirstStep(this)) {
        mMicRumbler->stopAndReset();
    }

    restrictToTowerPosition();
    if (al::updateNerveState(this)) {
        if (mHost->isBottom(this)) {
            al::setNerve(this, &NrvKuriboTowerChildWait);
        } else {
            al::setNerve(this, &NrvKuriboTowerChildStandBy);
        }
    }
}

/** @brief Stays frozen together with another frozen Goomba of the tower. */
void KuriboTowerChild::exeSupportFreezeSync() {
    if (al::isFirstStep(this)) {
        al::startAction(this, mHost->isBottom(this) ? "Fall" : "FallTower");
    }

    updateVelocity();
    restrictToTowerPosition();
    sead::Vector3f* velocity = al::getVelocityPtr(this);
    al::parallelizeVec(velocity, al::getGravity(this), *velocity);
}

/** @brief Gets pulled into the mouth of a Piranha Plant. */
void KuriboTowerChild::exeEatDown() {
    if (al::isFirstStep(this)) {
        mHost->releaseChild(this);
        mMicRumbler->stopAndReset();
        copyVector(&mEatStartTrans, al::getTrans(this));
    }

    sead::Vector3f trans = mEatStartTrans * (1.0f - al::getNerveStep(this)) +
                           al::getSensorPos(mEatSensor) * al::getNerveStep(this);
    al::setTrans(this, trans);
    if (al::isGreaterEqualStep(this, 1)) {
        al::setNerve(this, &NrvKuriboTowerChildSwallow);
        al::stopScene(this, 5, 0, false, false);
    }
}

/** @brief Shrinks inside the mouth of a Piranha Plant and vanishes. */
void KuriboTowerChild::exeSwallow() {
    sead::Vector3f trans = al::getSensorPos(mEatSensor);
    al::setTrans(this, trans);
    f32 scale = al::getNerveStep(this) * -0.1f + 1.0f;
    al::setScale(this, scale, scale, scale);
    if (scale <= 0.0f) {
        al::appearItem(this);
        kill();
    }
}

/** @brief Plays the surprise reaction and then starts moving again. */
void KuriboTowerChild::exeSurprise() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Surprise");
        sead::Vector3f* velocity = al::getVelocityPtr(this);
        al::parallelizeVec(velocity, al::getGravity(this), *velocity);
    }

    updateVelocity();
    if (al::isActionEnd(this)) {
        if (al::isExistRail(this)) {
            al::setNerve(this, &NrvKuriboTowerChildRailMove);
            return;
        }

        al::setNerve(this, &NrvKuriboTowerChildWander);
        return;
    }

    if (!al::isOnGround(this, 2, 0.0f)) {
        al::setNerve(this, &NrvKuriboTowerChildFall);
    }
}

/**
 * @brief Checks whether the Goomba is being knocked down by a hip drop.
 * @return Whether it is in its hip drop down behavior.
 */
bool KuriboTowerChild::isNerveHipDropDown() const {
    return al::isNerve(this, &NrvKuriboTowerChildHipDropDown);
}

/**
 * @brief Checks whether the Goomba is frozen by the support player.
 * @return Whether it is in its support freeze behavior.
 */
bool KuriboTowerChild::isNerveSupportFreeze() const {
    return al::isNerve(this, &NrvKuriboTowerChildSupportFreeze);
}

/** @brief Becomes the bottom of the tower and starts moving. */
void KuriboTowerChild::requestRootBehavior() {
    al::onCollide(this);
    al::validateShadow(this);
    if (!al::isNerve(this, &NrvKuriboTowerChildStandBy)) {
        return;
    }

    if (al::isExistRail(this)) {
        al::setNerve(this, &NrvKuriboTowerChildRailMove);
        return;
    }

    al::setNerve(this, &NrvKuriboTowerChildWander);
}

/** @brief Becomes a free Goomba after leaving the tower. */
void KuriboTowerChild::requestRelease() {
    al::onCollide(this);
    al::validateShadow(this);
}

/**
 * @brief Plays the surprise reaction unless the Goomba is being defeated.
 * @return Whether the reaction started.
 */
bool KuriboTowerChild::requestSurprise() {
    if (!isEnableDown()) {
        return false;
    }

    al::setNerve(this, &NrvKuriboTowerChildSurprise);
    return true;
}

/** @brief Freezes together with another frozen Goomba of the tower. */
void KuriboTowerChild::requestSupportFreezeSync() {
    if (!isEnableDown() || al::isNerve(this, &NrvKuriboTowerChildSupportFreeze) ||
        al::isNerve(this, &NrvKuriboTowerChildSupportFreezeSync)) {
        return;
    }

    al::setNerve(this, &NrvKuriboTowerChildSupportFreezeSync);
}

/** @brief Ends a synchronized freeze. */
void KuriboTowerChild::requestEndSupportFreezeSync() {
    if (!al::isNerve(this, &NrvKuriboTowerChildSupportFreezeSync)) {
        return;
    }

    if (mHost->isBottom(this)) {
        al::setNerve(this, &NrvKuriboTowerChildWait);
        return;
    }

    al::setNerve(this, &NrvKuriboTowerChildStandBy);
}

/**
 * @brief Faces an attacked sensor and plays the attack animation as the bottom Goomba.
 * @param pSensor Sensor that was attacked.
 */
void KuriboTowerChild::requestAttackReaction(al::HitSensor* pSensor) {
    al::faceToTarget(this, al::getSensorPos(pSensor));
    if (mHost->isBottom(this) && isEnableDown()) {
        al::setNerve(this, &NrvKuriboTowerChildAttack);
    }
}

/**
 * @brief Gets the height that the Goomba occupies in the tower.
 * @return The height offset to the next node.
 */
f32 KuriboTowerChild::getOffsetY() const {
    return 110.0f;
}
