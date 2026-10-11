#include "Enemy/March.hpp"

#include "Enemy/ActorStateSupportFreeze.hpp"
#include "Enemy/EnemyAttachItem.hpp"
#include "Enemy/EnemyStateBlowDown.hpp"
#include "Enemy/EnemyStateUtil.hpp"
#include "Enemy/TargetFinder.hpp"
#include "Enemy/WalkerStateChase.hpp"
#include "Enemy/WalkerStateFindPlayer.hpp"
#include "Enemy/WalkerStateJump.hpp"
#include "Enemy/WalkerStateParam.hpp"
#include "Enemy/WalkerStateWander.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Joint/JointControllerKeeper.hpp"
#include "Library/LiveActor/ActorAreaFunction.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/LiveActor/Util/ActorSceneInfo.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/InkUtil.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ScoreUtil.hpp"

// Nerve that shares the execute function of another nerve.
#define MARCH_NERVE_SHARED_DECL(Action, ExeFunc)                                                   \
    class MarchNrv##Action : public al::Nerve {                                                    \
    public:                                                                                        \
        void execute(al::NerveKeeper* pKeeper) const override {                                    \
            (pKeeper->getParent<March>())->exe##ExeFunc();                                         \
        }                                                                                          \
    };

namespace {
NERVE_DECL(March, Parade)
NERVE_DECL(March, Wander)
NERVE_DECL(March, Chase)
NERVE_DECL(March, FindPlayer)
NERVE_DECL(March, BlowDown)
NERVE_DECL(March, SupportFreeze)
NERVE_DECL(March, SupportFreezeParade)
NERVE_DECL(March, JumpJumpPanel)
MARCH_NERVE_SHARED_DECL(JumpJumpPanelChase, JumpJumpPanel)
NERVE_DECL(March, Attack)
NERVE_DECL(March, PressDown)
NERVE_DECL(March, Wait)
NERVE_DECL(March, Puzzlement)
NERVE_DECL(March, Scatter)
// Non-const nerve objects: the game keeps them in .data in this order.
MarchNrvParade NrvMarchParade;
MarchNrvWander NrvMarchWander;
MarchNrvChase NrvMarchChase;
MarchNrvFindPlayer NrvMarchFindPlayer;
MarchNrvBlowDown NrvMarchBlowDown;
MarchNrvSupportFreeze NrvMarchSupportFreeze;
MarchNrvSupportFreezeParade NrvMarchSupportFreezeParade;
MarchNrvJumpJumpPanel NrvMarchJumpJumpPanel;
MarchNrvJumpJumpPanelChase NrvMarchJumpJumpPanelChase;
MarchNrvAttack NrvMarchAttack;
MarchNrvPressDown NrvMarchPressDown;
MarchNrvWait NrvMarchWait;
MarchNrvPuzzlement NrvMarchPuzzlement;
MarchNrvScatter NrvMarchScatter;

sead::Vector3f sItemOffset(0.0f, 220.0f, 0.0f);
sead::Vector3f sSpinItemOffset(0.0f, 290.0f, 0.0f);
TargetFinderParam sTargetFinderParam(1500.0f, 180.0f, 90.0f, 90, -1.0f, 500.0f, 500.0f, 2000.0f,
                                     false);
WalkerStateParam sWalkerStateParam(2.25f, 0.98f, 0.9f, 500.0f, 1500.0f, 70.0f, 80.0f, 150.0f);
EnemyStateBlowDownParam sBlowDownParam(false);
ActorStateSupportFreezeParam sSupportFreezeParam(true, 15, false, true, 120,
                                                 sead::Vector3f(0.0f, 110.0f, 0.0f));
ActorStateSupportFreezeParam sSupportFreezeParadeParam(false, 15, false, false, 120,
                                                       sead::Vector3f(0.0f, 110.0f, 0.0f));
WalkerStateJumpParam sJumpPanelJumpParam(45.0f, nullptr, false);
}  // namespace

/**
 * @brief Constructs a March.
 * @param pName Actor name.
 * @param pWalkAction Action played while marching and wandering.
 * @param pItemName Name of the carried item, or nullptr for none.
 * @param itemType Type of the carried item.
 */
March::March(const char* pName, const char* pWalkAction, const char* pItemName, s32 itemType)
    : al::LiveActor(pName), mItemName(pItemName), mWalkAction(pWalkAction), mItemType(itemType) {}

/**
 * @brief Initializes the model, states, carried item, wind-up key joint and nerves.
 * @param rInfo Placement info of the actor.
 */
void March::init(const al::ActorInitInfo& rInfo) {
    if (rInfo.getActorSceneInfo().isSingleMode) {
        al::initActorWithArchiveName(this, rInfo, "MarchFur", nullptr);
    } else {
        al::initActorWithArchiveName(this, rInfo, "March", nullptr);
    }

    al::initNerve(this, &NrvMarchParade, 8);

    mWanderParam = new WalkerStateWanderParam(120, 120, 0.2f, 3.0f, 20.0f, 500.0f,
                                              mIsEnableCliffCheck, mWalkAction, "Wait");
    mChaseParam = new WalkerStateChaseParam(1.0f, 240.0f, 300.0f, 3.0f, 5.0f, false,
                                            mIsEnableCliffCheck, "Run", "Wait", -1.0f);
    mFindPlayerParam = new WalkerStateFindPlayerParam(0, 6.0f, false, mWalkAction);
    mTargetFinder = new TargetFinder(this, &sTargetFinderParam);
    mStateWander = new WalkerStateWander(this, al::getFrontPtr(this), &sWalkerStateParam,
                                         mWanderParam, nullptr);
    mStateChase = new WalkerStateChase(this, al::getFrontPtr(this), mTargetFinder,
                                       &sWalkerStateParam, mChaseParam, true, nullptr);
    mStateFindPlayer = new WalkerStateFindPlayer(this, al::getFrontPtr(this), mTargetFinder,
                                                 &sWalkerStateParam, mFindPlayerParam, nullptr);
    mStateBlowDown = new EnemyStateBlowDown(this, &sBlowDownParam);
    mStateSupportFreeze = new ActorStateSupportFreeze(this, &sSupportFreezeParam);
    mStateSupportFreezeParade = new ActorStateSupportFreeze(this, &sSupportFreezeParadeParam);
    mStateSupportFreezeParade->onKeepFreeze();
    mStateJumpJumpPanel = new WalkerStateJump(this, &sWalkerStateParam, &sJumpPanelJumpParam);

    if (mItemName != nullptr) {
        sead::Vector3f offset = sItemOffset;
        if (mItemType == cItemTypeSpin) {
            offset = sSpinItemOffset;
            mAttachItem = new EnemyAttachItem(this, "マーチアイテム", mItemName, cItemTypeSpin,
                                              al::getTransPtr(this), nullptr, offset, false);
        } else {
            mAttachItem = new EnemyAttachItem(this, "マーチアイテム", mItemName, mItemType,
                                              nullptr, al::getJointMtxPtr(this, "AllRoot"),
                                              offset, false);
        }

        al::initCreateActorWithPlacementInfo(mAttachItem, rInfo);
    }

    al::initNerveState(this, mStateWander, &NrvMarchWander, "[state]徘徊");
    al::initNerveState(this, mStateChase, &NrvMarchChase, "[state]追いかけ");
    al::initNerveState(this, mStateFindPlayer, &NrvMarchFindPlayer, "[state]プレーヤー発見");
    al::initNerveState(this, mStateBlowDown, &NrvMarchBlowDown, "[state]吹き飛ばし");
    al::initNerveState(this, mStateSupportFreeze, &NrvMarchSupportFreeze, "[state]フリーズ");
    al::initNerveState(this, mStateSupportFreezeParade, &NrvMarchSupportFreezeParade,
                       "[state]フリーズ(行列)");
    al::initNerveState(this, mStateJumpJumpPanel, &NrvMarchJumpJumpPanel,
                       "[state]ジャンプパネルジャンプ");
    al::initNerveState(this, mStateJumpJumpPanel, &NrvMarchJumpJumpPanelChase,
                       "[state]ジャンプパネルジャンプ追跡中");
    al::initJointControllerKeeper(this, 1);
    al::initJointLocalXRotator(this, &mZenmaiAngle, "Zenmai");
    makeActorAppeared();
    al::invalidateClipping(this);
    al::offCollide(this);
}

/**
 * @brief Pushes other enemies and map objects and attacks players.
 * @param pSelf Sensor of the March.
 * @param pOther Sensor that was hit.
 */
void March::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (GameDataFunction::isSingleMode(this) && al::isSensorName(pOther, "EnemyEar")) {
        return;
    }

    if (al::isSensorEnemyBody(pOther)) {
        al::sendMsgPushAndKillVelocityToTarget(this, pSelf, pOther);
    }

    if (al::isSensorEnemyAttack(pSelf) &&
        (al::isSensorPlayer(pOther) || al::isSensorKoopaJr(pOther))) {
        if (!isEnableDown()) {
            return;
        }

        al::sendMsgPush(pOther, pSelf);
        if (al::sendMsgEnemyAttackForCrossoverSensor(pOther, pSelf) &&
            !al::isNerve(this, &NrvMarchAttack) && !al::isNerve(this, &NrvMarchParade)) {
            al::setNerve(this, &NrvMarchAttack);
        }

        return;
    }

    if (al::isSensorMapObj(pOther) || al::isSensorKickKoura(pOther) || al::isSensorNpc(pOther) ||
        al::isSensorDoorKey(pOther)) {
        if (isEnableDown()) {
            al::sendMsgPush(pOther, pSelf);
        }
    }
}

/**
 * @brief Checks whether the March can be knocked down.
 * @return Whether it is neither stomped nor blown away.
 */
bool March::isEnableDown() const {
    if (al::isNerve(this, &NrvMarchPressDown)) {
        return false;
    }

    return !al::isNerve(this, &NrvMarchBlowDown);
}

/**
 * @brief Handles pushes, stomps, blow downs, bobsled attacks and jump panels.
 * @param pMsg Received message.
 * @param pOther Sensor that sent the message.
 * @param pSelf Sensor of the March.
 * @return Whether the message was handled.
 */
bool March::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) {
    if (!al::isNerve(this, &NrvMarchParade) && isEnableDown() &&
        al::tryReceiveMsgPushAndAddVelocity(this, pMsg, pOther, pSelf, 6.0f)) {
        return true;
    }

    bool isEar = GameDataFunction::isSingleMode(this) && al::isSensorName(pSelf, "EnemyEar");
    if (isEnableDown() && al::isSensorEnemyBody(pSelf)) {
        if (EnemyStateUtil::tryRequestPressDownAndNextNerve(pMsg, pOther, pSelf, this,
                                                            &NrvMarchPressDown, true)) {
            if (isEar) {
                return true;
            }

            al::invalidateClipping(this);
            rc::addScoreCombo(this, pOther, pMsg, 100.0f);
            return true;
        }

        if (EnemyStateUtil::tryRequestBlowDownAndNextNerve(pMsg, pOther, pSelf, mStateBlowDown,
                                                           &NrvMarchBlowDown, true)) {
            rc::addScoreCombo(this, pOther, pMsg, 100.0f);
            return true;
        }

        if (rc::isMsgBobsledBodyAttack(pMsg)) {
            if (mActorSceneInfo->isSingleMode) {
                rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
            }

            rc::startHitReactionBlowHitMessage(pMsg, this, pOther, pSelf);
            rc::setAppearItemFactorByMsg(al::getSensorHost(pSelf), pMsg, pOther);
            mStateBlowDown->setBlowDir(al::getSensorHost(pOther));
            al::setNerve(this, &NrvMarchBlowDown);
            rc::addScoreCombo(this, pOther, pMsg, 100.0f);
            return true;
        }
    }

    if (al::isSingleMode(this) && rc::isMsgJumpPanelAction(pMsg)) {
        if (al::isNerve(this, &NrvMarchChase)) {
            al::setNerve(this, &NrvMarchJumpJumpPanelChase);
        } else if (al::isNerve(this, &NrvMarchWait) || al::isNerve(this, &NrvMarchWander) ||
                   al::isNerve(this, &NrvMarchFindPlayer) ||
                   al::isNerve(this, &NrvMarchSupportFreeze)) {
            al::setNerve(this, &NrvMarchJumpJumpPanel);
        }
    }

    return false;
}

/**
 * @brief Freezes the March when it is touched on the screen.
 * @param pMsg Received message.
 * @param pPointer Screen pointer that touched.
 * @param pTarget Touched screen point target.
 * @return Whether the message was handled.
 */
bool March::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                  al::ScreenPointTarget* pTarget) {
    if (!mStateSupportFreeze->receiveMsgScreenPoint(pMsg, pPointer, pTarget)) {
        return false;
    }

    if (!al::isNerve(this, &NrvMarchSupportFreeze) &&
        !al::isNerve(this, &NrvMarchSupportFreezeParade) &&
        !al::isNerve(this, &NrvMarchBlowDown) && !al::isNerve(this, &NrvMarchPressDown)) {
        al::setNerve(this, &NrvMarchSupportFreeze);
    }

    return true;
}

/** @brief Updates the attack timer and the shadow, and dies in lava, water or ink. */
void March::control() {
    if (mAttackInvalidTime > 0) {
        mAttackInvalidTime--;
    }

    if (!al::isSingleMode(this)) {
        al::setShadowDropDirActorDown(this);
    }

    bool isInDeadArea = GameDataFunction::isSingleMode(this) &&
                        (al::isInWaterArea(this) || InkUtil::isInInkLimitSphere(this));
    bool isKillArea = EnemyStateUtil::isKillByAreaOrMaterialCode(this);
    if (isInDeadArea || isKillArea) {
        al::startHitReaction(this, "溶岩接触");
        kill();
    }
}

/** @brief Clips the carried item together with the March. */
void March::startClipped() {
    al::LiveActor::startClipped();
    if (mAttachItem != nullptr && al::isAlive(mAttachItem) && !al::isClipped(mAttachItem)) {
        mAttachItem->startClipped();
    }
}

/** @brief Unclips the carried item together with the March. */
void March::endClipped() {
    al::LiveActor::endClipped();
    if (mAttachItem != nullptr && al::isAlive(mAttachItem) && al::isClipped(mAttachItem)) {
        mAttachItem->endClipped();
    }
}

/** @brief Appears again and rejoins the parade. */
void March::reappear() {
    appear();
    al::setNerve(this, &NrvMarchParade);
}

/** @brief Marches in the parade, turning the wind-up key. */
void March::exeParade() {
    if (al::isFirstStep(this)) {
        al::startAction(this, mWalkAction);
    }

    if (mItemType == cItemTypeSpin) {
        rotateItem();
    }

    rotateZenmai(2.5f);
}

/** @brief Spins the carried item around the vertical axis. */
void March::rotateItem() {
    mItemRotate.y += 3.0f;
    mItemRotate.y = al::wrapAngle(mItemRotate.y);
    al::rotateActor(mAttachItem, mItemRotate);
}

/**
 * @brief Turns the wind-up key.
 * @param speed Rotation speed in degrees per frame.
 */
void March::rotateZenmai(f32 speed) {
    mZenmaiAngle = al::wrapAngle(mZenmaiAngle + speed);
}

/** @brief Jumps back in surprise after the parade was disturbed. */
void March::exeScatter() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Surprise");
        sead::Vector3f velocity(0.0f, 12.0f, 0.0f);
        velocity = velocity - al::getFront(this) * 6.5f;
        al::setVelocity(this, velocity);
        if (mAttachItem != nullptr) {
            al::validateClipping(mAttachItem);
        }
    }

    if (mItemType == cItemTypeSpin) {
        rotateItem();
    }

    rotateZenmai(10.0f);
    al::addVelocityToGravity(this, 0.7f);
    if (al::isOnGround(this, 0, 0.0f) && al::getVelocity(this).y < 0.0f) {
        al::setNerve(this, &NrvMarchPuzzlement);
    }
}

/** @brief Runs around in random directions for a while, then faces a player and wanders. */
void March::exePuzzlement() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Run");
        getRandomDir(&mPuzzlementDir);
    } else if (al::getNerveStep(this) % 20 == 0) {
        al::rotateVectorDegreeY(&mPuzzlementDir, 75.0f);
    }

    if (mItemType == cItemTypeSpin) {
        rotateItem();
    }

    rotateZenmai(10.0f);
    al::walkAndTurnToDirection(this, mPuzzlementDir, 1.0f, sWalkerStateParam.mGravity,
                               sWalkerStateParam.mGroundFriction, 4.0f, true);
    if (al::isGreaterEqualStep(this, 100)) {
        al::faceToTarget(this, al::findNearestPlayerPos(this));
        al::setNerve(this, &NrvMarchWander);
        al::validateClipping(this);
    }
}

/**
 * @brief Picks a random horizontal direction.
 * @param pDir Output direction (X axis if the random vector was vertical).
 */
void March::getRandomDir(sead::Vector3f* pDir) const {
    al::getRandomVector(pDir, 1.0f);
    al::verticalizeVec(pDir, sead::Vector3f::ey, *pDir);
    al::normalizeOrZero(pDir);
    if (al::isNearZero(*pDir, 0.001f)) {
        pDir->e = sead::Vector3f::ex.e;
    }
}

/** @brief Wanders around, looks for a player and goes back to waiting when none is near. */
void March::exeWander() {
    al::updateNerveState(this);
    if (!isActive(20500.0f)) {
        al::setNerve(this, &NrvMarchWait);
        return;
    }

    if (mItemType == cItemTypeSpin) {
        rotateItem();
    }

    rotateZenmai(2.5f);
    mTargetFinder->update();
    if (mTargetFinder->isExistTarget() &&
        (mTargetFinder->mIsFound || al::isGreaterEqualStep(this, mWanderParam->mWaitTime)) &&
        al::isOnGround(this, 0, 0.0f)) {
        al::setNerve(this, &NrvMarchFindPlayer);
    }
}

/**
 * @brief Checks whether the March should be active.
 * @param distance Distance to the nearest player within which it is active.
 * @return Whether it is in the air or near a player.
 */
bool March::isActive(f32 distance) const {
    if (!al::isOnGround(this, 0, 0.0f)) {
        return true;
    }

    return al::isNearPlayer(this, distance);
}

/** @brief Reacts to a found player and then chases it. */
void March::exeFindPlayer() {
    if (mItemType == cItemTypeSpin) {
        rotateItem();
    }

    rotateZenmai(10.0f);
    al::updateNerveStateAndNextNerve(this, &NrvMarchChase);
}

/** @brief Chases the target and wanders around the place where it was lost. */
void March::exeChase() {
    if (mItemType == cItemTypeSpin) {
        rotateItem();
    }

    rotateZenmai(10.0f);
    if (al::updateNerveStateAndNextNerve(this, &NrvMarchWander)) {
        mStateWander->setWanderCenter(al::getTrans(this));
    }
}

/** @brief Attacks a player and waits afterwards. */
void March::exeAttack() {
    if (al::isFirstStep(this)) {
        al::setVelocityZero(this);
        al::startAction(this, "Attack");
    }

    if (mItemType == cItemTypeSpin) {
        rotateItem();
    }

    rotateZenmai(10.0f);
    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvMarchWait);
    }
}

/** @brief Waits until a player comes near. */
void March::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait");
        al::setVelocityToGravity(this, sWalkerStateParam.mGravity);
    }

    if (mItemType == cItemTypeSpin) {
        rotateItem();
    }

    rotateZenmai(2.5f);
    mTargetFinder->update();
    if (isActive(20000.0f)) {
        al::setNerve(this, &NrvMarchWander);
    }
}

/** @brief Jumps on a jump panel and goes back to chasing or waiting. */
void March::exeJumpJumpPanel() {
    if (al::isFirstStep(this)) {
        mAttackInvalidTime = 20;
    }

    if (mItemType == cItemTypeSpin) {
        rotateItem();
    }

    if (al::updateNerveState(this)) {
        if (al::isNerve(this, &NrvMarchJumpJumpPanelChase)) {
            al::setNerve(this, &NrvMarchChase);
        } else {
            al::setNerve(this, &NrvMarchWait);
        }
    }
}

/** @brief Gets stomped flat, drops the carried item and dies. */
void March::exePressDown() {
    if (al::isFirstStep(this)) {
        al::setVelocityZero(this);
        al::changeEnvTextureStamp(this);
        al::startAction(this, "PressDown");
        if (mAttachItem != nullptr) {
            mAttachItem->endAttach(false);
        }
    }

    rc::tryAppearItemPressDown(this, nullptr);
    if (al::isActionEnd(this)) {
        al::resetEnvTexture(this);
        al::validateClipping(this);
        al::startHitReactionDeath(this);
        kill();
    }
}

/** @brief Gets blown away, drops the carried item and dies. */
void March::exeBlowDown() {
    if (al::isFirstStep(this) && mAttachItem != nullptr) {
        mAttachItem->endAttach(false);
    }

    if (al::updateNerveState(this)) {
        al::appearItem(this);
        al::startHitReactionDeath(this);
        kill();
    }
}

/** @brief Stays frozen together with the rest of the parade. */
void March::exeSupportFreezeParade() {
    al::updateNerveState(this);
}

/** @brief Stays frozen and goes back to the parade or to waiting afterwards. */
void March::exeSupportFreeze() {
    if (al::updateNerveState(this)) {
        if (al::isNoCollide(this)) {
            al::setNerve(this, &NrvMarchParade);
        } else {
            al::setNerve(this, &NrvMarchWait);
        }
    }
}

/**
 * @brief Breaks out of the parade and jumps back from a position.
 * @param rTarget Position to face, e.g. the player that disturbed the parade.
 */
void March::startScatter(const sead::Vector3f& rTarget) {
    if (al::isNerve(this, &NrvMarchParade) || al::isNerve(this, &NrvMarchSupportFreezeParade) ||
        al::isNerve(this, &NrvMarchSupportFreeze)) {
        al::setVelocityZero(this);
        al::onCollide(this);
        al::setNerve(this, &NrvMarchScatter);
        al::startAction(this, "Wait");
        al::faceToTarget(this, rTarget);
    }
}

/**
 * @brief Freezes the March together with the rest of the parade.
 * @param pTouchActor Actor that froze the parade.
 */
void March::startFreeze(const al::LiveActor* pTouchActor) {
    mStateSupportFreezeParade->forceSetTouchActor(const_cast<al::LiveActor*>(pTouchActor));
    al::setNerve(this, &NrvMarchSupportFreezeParade);
}

/**
 * @brief Gets the actor that froze the March.
 * @return Touch actor of the active freeze state.
 */
al::LiveActor* March::getFreezeTouchActor() const {
    const ActorStateSupportFreeze* state = al::isNerve(this, &NrvMarchSupportFreeze) ?
                                               mStateSupportFreeze :
                                               mStateSupportFreezeParade;
    return state->getTouchActor();
}

/** @brief Starts marching in the parade. */
void March::startParade() {
    al::setNerve(this, &NrvMarchParade);
}

/**
 * @brief Checks whether the March is frozen.
 * @return Whether it is frozen alone or together with the parade.
 */
bool March::isFreeze() const {
    return al::isNerve(this, &NrvMarchSupportFreezeParade) ||
           al::isNerve(this, &NrvMarchSupportFreeze);
}

/**
 * @brief Gets the actor linked to the March.
 * @return Item actor of the carried item, or nullptr without an item.
 */
al::LiveActor* March::getLinkedActor() {
    return mAttachItem != nullptr ? mAttachItem->getItemActor() : nullptr;
}
