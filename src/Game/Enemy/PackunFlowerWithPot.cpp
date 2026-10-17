#include "Enemy/PackunFlowerWithPot.hpp"

#include <math/seadVector.h>

#include "Enemy/ActorMicRumbler.hpp"
#include "Enemy/ActorStateSupportFreeze.hpp"
#include "Enemy/EnemyStateBlowDown.hpp"
#include "Enemy/EnemyStateUtil.hpp"
#include "Enemy/PackunFlowerHead.hpp"
#include "Enemy/PackunStateHold.hpp"
#include "Layout/GuideGameWindow.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Collision/PartsConnectorUtil.hpp"
#include "Library/Connector/MtxConnector.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Player/Normal/PlayerActor.hpp"
#include "Project/Action/Common/ActionEffectCtrl.hpp"
#include "Project/Action/Common/ActionSeCtrl.hpp"
#include "Project/Action/Common/ActorActionKeeper.hpp"
#include "Project/Collision/CollisionUtil.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ScoreUtil.hpp"

namespace {
NERVE_DECL(PackunFlowerWithPot, Sleep)
NERVE_DECL(PackunFlowerWithPot, BlowDown)
NERVE_DECL(PackunFlowerWithPot, SupportFreeze)
NERVE_DECL(PackunFlowerWithPot, Hold)
NERVE_DECL(PackunFlowerWithPot, Wait)
NERVE_DECL(PackunFlowerWithPot, Land)
NERVE_DECL(PackunFlowerWithPot, PressDown)
NERVE_DECL(PackunFlowerWithPot, Turn)
NERVE_DECL(PackunFlowerWithPot, Attack)
NERVE_DECL(PackunFlowerWithPot, DieDown)
NERVE_DECL(PackunFlowerWithPot, Respawn)
NERVE_DECL(PackunFlowerWithPot, Release)
NERVE_DECL(PackunFlowerWithPot, Push)
NERVE_DECL(PackunFlowerWithPot, Find)
NERVE_DECL(PackunFlowerWithPot, AfterAttack)
NERVE_DECL(PackunFlowerWithPot, TurnFast)
// Non-const nerve objects: the game keeps them together in .data in this order.
PackunFlowerWithPotNrvSleep NrvPackunFlowerWithPotSleep;
PackunFlowerWithPotNrvBlowDown NrvPackunFlowerWithPotBlowDown;
PackunFlowerWithPotNrvSupportFreeze NrvPackunFlowerWithPotSupportFreeze;
PackunFlowerWithPotNrvHold NrvPackunFlowerWithPotHold;
PackunFlowerWithPotNrvWait NrvPackunFlowerWithPotWait;
PackunFlowerWithPotNrvLand NrvPackunFlowerWithPotLand;
PackunFlowerWithPotNrvPressDown NrvPackunFlowerWithPotPressDown;
PackunFlowerWithPotNrvTurn NrvPackunFlowerWithPotTurn;
PackunFlowerWithPotNrvAttack NrvPackunFlowerWithPotAttack;
PackunFlowerWithPotNrvDieDown NrvPackunFlowerWithPotDieDown;
PackunFlowerWithPotNrvRespawn NrvPackunFlowerWithPotRespawn;
PackunFlowerWithPotNrvRelease NrvPackunFlowerWithPotRelease;
PackunFlowerWithPotNrvPush NrvPackunFlowerWithPotPush;
PackunFlowerWithPotNrvFind NrvPackunFlowerWithPotFind;
PackunFlowerWithPotNrvAfterAttack NrvPackunFlowerWithPotAfterAttack;
PackunFlowerWithPotNrvTurnFast NrvPackunFlowerWithPotTurnFast;

EnemyStateBlowDownParam sBlowDownParam(false);
ActorStateSupportFreezeParam sSupportFreezeParam(true, 15, false, true, 120,
                                                 sead::Vector3f(0.0f, 150.0f, 0.0f));
}  // namespace

/**
 * @brief Constructs the flower pot.
 * @param pName Actor name.
 * @param pParent Plant growing in the pot.
 */
PackunFlowerWithPot::Pot::Pot(const char* pName, PackunFlowerWithPot* pParent)
    : al::LiveActor(pName), mParent(pParent) {}

/**
 * @brief Hides the pot unless the plant is hidden while it is held.
 * @return Whether the pot was hidden.
 */
bool PackunFlowerWithPot::Pot::hideActor() {
    if (mParent->mIsHiddenWhileHeld) {
        return false;
    }

    return al::LiveActor::hideActor();
}

/**
 * @brief Constructs the potted Piranha Plant.
 * @param pName Actor name.
 */
PackunFlowerWithPot::PackunFlowerWithPot(const char* pName) : al::LiveActor(pName) {}

/**
 * @brief Initializes the model, pot, heads, states and collision connector.
 * @param rInfo Actor placement and scene information.
 */
void PackunFlowerWithPot::init(const al::ActorInitInfo& rInfo) {
    if (rInfo.getActorSceneInfo().isSingleMode) {
        al::initActorWithArchiveName(this, rInfo, "PackunFlowerWithPotFur", nullptr);
    } else {
        al::initActorWithArchiveName(this, rInfo, "PackunFlowerWithPot", nullptr);
    }

    al::tryGetArg(&mIsEnableReset, rInfo, "IsEnableReset");
    mIsSingleMode = al::isSingleMode(rInfo);
    mHeadJointMtx = al::getJointMtxPtr(this, "Head");

    mPot = new Pot("パックンフラワー土台", this);
    al::initActorWithArchiveNameWithPlacementInfo(mPot, rInfo, "FlowerPot", nullptr);
    // The game makes a discarded virtual getName() call after initializing each part.
    mPot->getName();
    al::trySetShadowLength(mPot, rInfo, nullptr);
    al::trySetShadowLength(this, rInfo, nullptr);

    mHeads = new PackunFlowerHead*[3];
    for (s32 i = 0; i < 3; i++) {
        mHeads[i] = new PackunFlowerHead("パックンフラワー頭");
        mHeads[i]->init(rInfo);
        al::trySetShadowLength(mHeads[i], rInfo, nullptr);
        mHeads[i]->setFollowMtx(mHeadJointMtx);
        mHeads[i]->getName();
    }

    mStateBlowDown = new EnemyStateBlowDown(this, &sBlowDownParam);
    mStateSupportFreeze = new ActorStateSupportFreeze(this, &sSupportFreezeParam);
    mStateHold = new PackunStateHold(this, mHeads);
    al::initNerve(this, &NrvPackunFlowerWithPotSleep, 3);
    al::initNerveState(this, mStateBlowDown, &NrvPackunFlowerWithPotBlowDown,
                       "[state]吹き飛ばし");
    al::initNerveState(this, mStateSupportFreeze, &NrvPackunFlowerWithPotSupportFreeze,
                       "[state]フリーズ");
    al::initNerveState(this, mStateHold, &NrvPackunFlowerWithPotHold,
                       "[state]プレイヤーに持たれる");
    mStateHold->initColliderControl();
    al::setEffectFollowMtxPtr(this, "FallLeaf", &mEffectMtx);
    mMtxConnector = al::createMtxConnector(this);
    al::offCollide(this);
    mMicRumbler = new ActorMicRumbler(this, nullptr);

    if (mIsEnableReset) {
        mInitTrans = al::getTrans(this);
        mInitQuat = al::getQuat(this);
    }

    makeActorAppeared();
}

/** @brief Attaches the plant to the collision it was placed on. */
void PackunFlowerWithPot::initAfterPlacement() {
    al::attachMtxConnectorToCollision(mMtxConnector, this, false);
}

/** @brief Re-attaches the plant to the collision below its current pose. */
inline void PackunFlowerWithPot::attachToCollision() {
    mMtxConnector->clear();
    mMtxConnector->setBaseQuatTrans(al::getQuat(this), al::getTrans(this));
    al::attachMtxConnectorToCollision(mMtxConnector, this, false);
}

/**
 * @brief Checks for ground collision right below the plant.
 * @return Whether collision was found.
 */
inline bool PackunFlowerWithPot::isOnGroundCollision() const {
    return alCollisionUtil::getFirstPolyOnArrow(
        this, nullptr, nullptr, al::getTrans(this) + sead::Vector3f::ey * 50.0f,
        sead::Vector3f::ey * -100.0f, static_cast<const al::CollisionPartsFilterBase*>(nullptr),
        nullptr);
}

/** @brief Emits the falling-leaf effect at the current base matrix. */
inline void PackunFlowerWithPot::emitFallLeafEffect() {
    sead::Matrix34f* effectMtx = &mEffectMtx;
    *effectMtx = *getBaseMtx();
    al::tryEmitEffect(this, "FallLeaf", nullptr);
}

/** @brief Updates guide messages, water/death checks, the pot pose and the mic rumble. */
void PackunFlowerWithPot::control() {
    if (mIsSingleMode && mIsHiddenWhileHeld &&
        (al::isNerve(this, &NrvPackunFlowerWithPotWait) ||
         al::isNerve(this, &NrvPackunFlowerWithPotLand))) {
        sead::Vector3f frontDir;
        al::calcFrontDir(&frontDir, this);
        mStateBlowDown->setBlowDir(frontDir);
        al::setNerve(this, &NrvPackunFlowerWithPotPressDown);
    }

    if (rc::isInDeathArea(this)) {
        kill();
    }

    if (GameDataFunction::isSingleMode(GameDataHolderAccessor(this))) {
        if (!rc::isCurrentGuideGameWindowUser(this)) {
            mIsShowGuide = false;
        }

        if (al::isNerve(this, &NrvPackunFlowerWithPotWait) ||
            al::isNerve(this, &NrvPackunFlowerWithPotTurn) ||
            al::isNerve(this, &NrvPackunFlowerWithPotSleep) ||
            al::isNerve(this, &NrvPackunFlowerWithPotAttack)) {
            if (mIsCarryable) {
                auto* player = static_cast<PlayerActor*>(al::tryFindNearestPlayerActor(this));

                if (player != nullptr && player->getHoldingSensor() == nullptr && !mIsShowGuide &&
                    !rc::isPlayerEquipHeadgear(player)) {
                    if (al::isPadTypeJoySingle(al::getMainControllerPort())) {
                        mIsShowGuide = rc::appearGuideGameWindowWithPriority(
                            this, "SingleMode_GuideMessage", "PickupGuide_SingleJoycons",
                            GuideMessagePriority(3), -1, 0.0f);
                    } else {
                        mIsShowGuide = rc::appearGuideGameWindowWithPriority(
                            this, "SingleMode_GuideMessage", "PickupGuide_DualJoycons",
                            GuideMessagePriority(3), -1, 0.0f);
                    }
                }
            } else {
                rc::disappearGuideGameWindow(this);
                mIsShowGuide = false;
            }
        }

        mIsCarryable = false;
    }

    if (al::isNerve(this, &NrvPackunFlowerWithPotBlowDown) ||
        al::isNerve(this, &NrvPackunFlowerWithPotDieDown) ||
        al::isNerve(this, &NrvPackunFlowerWithPotPressDown)) {
        return;
    }

    if (mIsSingleMode && EnemyStateUtil::isKillByAreaOrMaterialCode(this)) {
        setBlowFromWater();
        return;
    }

    if (mIsSingleMode && rc::isInWaterArea(this) &&
        !al::isNerve(this, &NrvPackunFlowerWithPotRespawn) &&
        !al::isNerve(this, &NrvPackunFlowerWithPotRelease)) {
        if (!al::isNerve(this, &NrvPackunFlowerWithPotHold)) {
            setBlowFromWater();
            return;
        }

        mStateHold->requestRelease();
        al::setNerve(this, &NrvPackunFlowerWithPotRelease);
        return;
    }

    al::setTrans(mPot, al::getTrans(this) - sead::Vector3f::ey * 33.0f + al::getVelocity(this));
    mIsPushed = false;

    if (al::isNerve(this, &NrvPackunFlowerWithPotHold) ||
        al::isNerve(this, &NrvPackunFlowerWithPotRelease) ||
        al::isNerve(this, &NrvPackunFlowerWithPotPush)) {
        return;
    }

    al::connectPoseTrans(this, mMtxConnector, al::getConnectBaseTrans(mMtxConnector));
    al::addTransOffsetLocalDir(this, 33.0f, 1);

    if (al::isNerve(this, &NrvPackunFlowerWithPotSupportFreeze) ||
        al::isNerve(this, &NrvPackunFlowerWithPotPressDown) ||
        al::isNerve(this, &NrvPackunFlowerWithPotBlowDown) ||
        al::isNerve(this, &NrvPackunFlowerWithPotDieDown) ||
        al::isNerve(this, &NrvPackunFlowerWithPotRelease) ||
        al::isNerve(this, &NrvPackunFlowerWithPotLand)) {
        return;
    }

    if (!al::isNerve(this, &NrvPackunFlowerWithPotSupportFreeze) &&
        !al::isNerve(this, &NrvPackunFlowerWithPotPressDown) &&
        !al::isNerve(this, &NrvPackunFlowerWithPotBlowDown) &&
        !al::isNerve(this, &NrvPackunFlowerWithPotDieDown)) {
        mMicRumbler->update();
    }

    if (!mIsSingleMode) {
        return;
    }

    if (al::isNerve(this, &NrvPackunFlowerWithPotSleep) ||
        al::isNerve(this, &NrvPackunFlowerWithPotFind) ||
        al::isNerve(this, &NrvPackunFlowerWithPotWait) ||
        al::isNerve(this, &NrvPackunFlowerWithPotTurn) ||
        al::isNerve(this, &NrvPackunFlowerWithPotAttack) ||
        al::isNerve(this, &NrvPackunFlowerWithPotAfterAttack)) {
        if (!isOnGroundCollision()) {
            al::setNerve(this, &NrvPackunFlowerWithPotRelease);
        }
    }
}

/** @brief Blows the plant away out of water, reflecting its velocity upwards. */
void PackunFlowerWithPot::setBlowFromWater() {
    al::onCollide(this);
    sead::Vector3f dir = al::getVelocity(this);

    if (al::isNearZero(dir, 0.001f)) {
        dir = sead::Vector3f::ez;
    } else {
        al::normalize(&dir);
    }

    al::calcReflectionVector(&dir, sead::Vector3f::ey, 1.0f, 0.0f);
    mStateBlowDown->setBlowDirScale(dir);
    al::setAppearItemFactor(this, "直接攻撃", nullptr);
    al::setNerve(this, &NrvPackunFlowerWithPotBlowDown);
}

/**
 * @brief Checks whether the plant can currently attack.
 * @return Whether the plant is neither down nor being released or landing.
 */
bool PackunFlowerWithPot::isEnableAttack() {
    return !al::isNerve(this, &NrvPackunFlowerWithPotPressDown) &&
           !al::isNerve(this, &NrvPackunFlowerWithPotBlowDown) &&
           !al::isNerve(this, &NrvPackunFlowerWithPotDieDown) &&
           !al::isNerve(this, &NrvPackunFlowerWithPotRelease) &&
           !al::isNerve(this, &NrvPackunFlowerWithPotLand);
}

/** @brief Makes the plant and its pot appear. */
void PackunFlowerWithPot::makeActorAppeared() {
    mPot->appear();
    al::resetPosition(mPot, false);
    al::LiveActor::makeActorAppeared();
}

/** @brief Appears asleep, attached to the collision below. */
void PackunFlowerWithPot::appear() {
    al::setNerve(this, &NrvPackunFlowerWithPotSleep);
    al::LiveActor::appear();
    attachToCollision();
}

/** @brief Makes the plant and its pot dead. */
void PackunFlowerWithPot::makeActorDead() {
    mPot->kill();
    al::LiveActor::makeActorDead();
}

/**
 * @brief Hides the plant, unless it is held (then it is only flagged as hidden).
 * @return Whether the plant was hidden.
 */
bool PackunFlowerWithPot::hideActor() {
    if (al::isNerve(this, &NrvPackunFlowerWithPotHold)) {
        mIsHiddenWhileHeld = true;
        return false;
    }

    return al::LiveActor::hideActor();
}

/** @brief Kills the plant, pot and heads, or respawns the plant when resetting is enabled. */
void PackunFlowerWithPot::kill() {
    rc::disappearGuideGameWindow(this);
    mIsShowGuide = false;

    if (mIsEnableReset && !mIsHiddenWhileHeld) {
        startRespawn();
        return;
    }

    mIsHiddenWhileHeld = false;
    mPot->kill();

    for (s32 i = 0; i < 3; i++) {
        mHeads[i]->kill();
    }

    al::LiveActor::kill();
}

/** @brief Vanishes and waits to respawn at the initial position. */
void PackunFlowerWithPot::startRespawn() {
    getActorActionKeeper()->getEffectCtrl()->stopActionAndKillParticles();
    getActorActionKeeper()->getSeCtrl()->stopAction();
    mStateHold->kill();
    mPot->kill();
    al::startSe(this, "HrVanish");
    al::offCollide(this);
    al::invalidateClipping(this);
    al::invalidateHitSensors(this);
    al::setVelocityZero(this);
    al::hideSilhouetteModel(this);
    al::hideModelIfShow(this);
    al::setNerve(this, &NrvPackunFlowerWithPotRespawn);
}

/** @brief Updates the collider, letting the hold state handle it while held. */
void PackunFlowerWithPot::updateCollider() {
    PackunStateHold* stateHold = mStateHold;

    if (stateHold->isDead()) {
        al::LiveActor::updateCollider();
        return;
    }

    stateHold->updateCollider(al::getHitSensor(this, "Body"));
}

/**
 * @brief Pushes and attacks other actors depending on the current state.
 * @param pSelf Sensor of the plant.
 * @param pOther Sensor of the other actor.
 */
void PackunFlowerWithPot::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isNerve(this, &NrvPackunFlowerWithPotPressDown) ||
        al::isNerve(this, &NrvPackunFlowerWithPotBlowDown) ||
        al::isNerve(this, &NrvPackunFlowerWithPotDieDown)) {
        return;
    }

    if (al::isNerve(this, &NrvPackunFlowerWithPotRelease) ||
        al::isNerve(this, &NrvPackunFlowerWithPotLand)) {
        if (al::isSensorMapObj(pOther) || al::isSensorNpc(pOther)) {
            rc::sendMsgPackunPush(pOther, pSelf);

            if (!mIsSingleMode) {
                return;
            }

            al::sendMsgBallItemGet(pOther, pSelf);
            al::sendMsgBallTrample(pOther, pSelf, nullptr);
            rc::sendMsgPackunThrowAttack(pOther, pSelf);
            return;
        }

        if (mIsSingleMode) {
            if (al::isSensorEnemyBody(pOther)) {
                if (rc::sendMsgPackunPush(pOther, pSelf)) {
                    if (al::isNerve(this, &NrvPackunFlowerWithPotRelease)) {
                        sead::Vector3f dir;
                        al::calcDirBetweenSensors(&dir, pOther, pSelf);
                        f32 speed = al::getVelocity(this).length();
                        al::setVelocity(this, speed * dir * 0.8f);
                    }

                    return;
                }

                al::sendMsgBallTrample(pOther, pSelf, nullptr);
            }

            if (al::isSensorKickKoura(pOther) || al::isSensorBindableGoal(pOther) ||
                al::isSensorBindableGoalItem(pOther)) {
                rc::sendMsgPackunPush(pOther, pSelf);
                rc::sendMsgPackunThrowAttack(pOther, pSelf);
            }
        }
    }

    if (al::isNerve(this, &NrvPackunFlowerWithPotRelease)) {
        return;
    }

    if (al::isNerve(this, &NrvPackunFlowerWithPotHold)) {
        mStateHold->attackSensor(pSelf, pOther);

        if (!al::isSensorKoopaJr(pOther)) {
            return;
        }

        if (al::isSensorName(pSelf, "Root") || al::isSensorName(pSelf, "Body") ||
            al::isSensorName(pSelf, "Push")) {
            al::sendMsgPush(pOther, pSelf);
        }

        return;
    }

    if (al::isSensorEnemyBody(pOther)) {
        al::sendMsgPushAndKillVelocityToTarget(this, pSelf, pOther);
        return;
    }

    if ((al::isSensorPlayer(pOther) || al::isSensorKoopaJr(pOther) || al::isSensorNpc(pOther) ||
         al::isSensorHostName(pOther, "Shards")) &&
        al::isSensorEnemyAttack(pSelf)) {
        al::sendMsgPush(pOther, pSelf);
        al::sendMsgEnemyAttack(pOther, pSelf);
        return;
    }

    if (al::isSensorName(pSelf, "Root") && (!mIsSingleMode || !al::isSensorPlessie(pOther))) {
        al::sendMsgPush(pOther, pSelf);
        return;
    }

    if (!mIsSingleMode) {
        return;
    }

    if (al::isSensorEnemyAttack(pSelf) && !al::isSensorPlessie(pOther)) {
        al::sendMsgPush(pOther, pSelf);
        return;
    }

    if (!mIsSingleMode) {
        return;
    }

    if (al::isSensorMapObj(pOther) && al::isSensorHostName(pOther, "トランポリン★") &&
        al::isSensorName(pSelf, "Body") && rc::sendMsgEnemyFloorTouchTrampoline(pOther, pSelf)) {
        al::setVelocity(this, 0.0f, 15.0f, 0.0f);
        al::setNerve(this, &NrvPackunFlowerWithPotPush);
    }
}

/**
 * @brief Checks whether the plant has been knocked down.
 * @return Whether the plant is pressed, blown or dying.
 */
bool PackunFlowerWithPot::isDown() {
    return al::isNerve(this, &NrvPackunFlowerWithPotPressDown) ||
           al::isNerve(this, &NrvPackunFlowerWithPotBlowDown) ||
           al::isNerve(this, &NrvPackunFlowerWithPotDieDown);
}

/**
 * @brief Gets blown away by an enemy attack.
 * @param pMsg Received attack message.
 * @param pOther Sensor of the attacker.
 * @param pSelf Sensor of the plant.
 */
inline void PackunFlowerWithPot::blowDownByAttack(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                                                  al::HitSensor* pSelf) {
    al::onCollide(this);
    mStateBlowDown->setBlowDir(al::getSensorHost(pOther));
    rc::startHitReactionBlowHitMessage(pMsg, this, pOther, pSelf);
    rc::setAppearItemFactorByMsg(this, pMsg, pOther);
    rc::addScoreCombo(this, pOther, pMsg, 100.0f);
    al::setNerve(this, &NrvPackunFlowerWithPotBlowDown);
}

/**
 * @brief Handles attacks, pushes, carrying and the hold state's messages.
 * @param pMsg Received message.
 * @param pOther Sensor of the sender.
 * @param pSelf Sensor of the plant.
 * @return Whether the message was handled.
 */
bool PackunFlowerWithPot::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                                     al::HitSensor* pSelf) {
    if (al::isNerve(this, &NrvPackunFlowerWithPotPressDown) ||
        al::isNerve(this, &NrvPackunFlowerWithPotBlowDown) ||
        al::isNerve(this, &NrvPackunFlowerWithPotDieDown) ||
        al::isNerve(this, &NrvPackunFlowerWithPotRespawn)) {
        return false;
    }

    if (al::isNerve(this, &NrvPackunFlowerWithPotRelease) && al::isLessEqualStep(this, 20)) {
        if (!mIsSingleMode || !al::isGreaterEqualStep(this, 5)) {
            return false;
        }

        if (al::tryReceiveMsgPushAndAddVelocity(this, pMsg, pOther, pSelf, 5.0f) ||
            rc::tryReceiveMsgPushDirAndAddVelocity(this, pMsg, 5.0f)) {
            mIsPushed = true;
            return true;
        }

        return false;
    }

    if (al::isNerve(this, &NrvPackunFlowerWithPotHold)) {
        if (al::isMsgPlayerHideItem(pMsg)) {
            al::hideModelIfShow(this);
            al::hideModelIfShow(mPot);

            for (s32 i = 0; i < 3; i++) {
                al::hideModelIfShow(mHeads[i]);
            }
        } else if (al::isMsgPlayerShowItem(pMsg)) {
            al::showModelIfHide(this);
            al::showModelIfHide(mPot);
        }

        if (mStateHold->receiveMsg(pMsg, pOther, pSelf)) {
            if (al::isMsgHoldCancelWarp(pMsg)) {
                startRespawn();
                return true;
            }

            al::setNerve(this, &NrvPackunFlowerWithPotRelease);
            return true;
        }

        if (!mIsSingleMode || !al::isMsgEnemyAttackFire(pMsg)) {
            return false;
        }

        if (!al::isSensorName(pSelf, "Body") && !al::isSensorName(pSelf, "Root")) {
            return false;
        }

        mStateHold->requestRelease();
        blowDownByAttack(pMsg, pOther, pSelf);
        return true;
    }

    if (al::isSensorName(pSelf, "Body") || al::isSensorName(pSelf, "Root")) {
        if (EnemyStateUtil::tryRequestBlowDownAndNextNerve(pMsg, pOther, pSelf, mStateBlowDown,
                                                           &NrvPackunFlowerWithPotBlowDown,
                                                           false)) {
            rc::setAppearItemFactorByMsg(this, pMsg, pOther);
            rc::addScoreCombo(this, pOther, pMsg, 100.0f);
            return true;
        }

        if (al::isMsgTrampleAll(pMsg) || al::isMsgPlayerObjHipDropAll(pMsg)) {
            rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
            rc::setAppearItemFactorByMsg(this, pMsg, pOther);
            rc::addScoreCombo(this, pOther, pMsg, 100.0f);
            al::setNerve(this, &NrvPackunFlowerWithPotPressDown);
            return true;
        }

        if (al::isMsgEnemyAttack(pMsg)) {
            blowDownByAttack(pMsg, pOther, pSelf);
            return true;
        }

        if (rc::isMsgPackunEatStart(pMsg)) {
            return true;
        }

        if (rc::isMsgPackunEat(pMsg)) {
            rc::setAppearItemFactorByMsg(this, pMsg, pOther);
            rc::addScoreCombo(this, pOther, pMsg, 100.0f);
            kill();
            return true;
        }

        if (mIsSingleMode && rc::isMsgJumpPanelAction(pMsg)) {
            al::setVelocity(this, 0.0f, rc::isMsgJumpPanelActionAndSuperJump(pMsg) ? 50.0f : 30.0f,
                            0.0f);
            al::setNerve(this, &NrvPackunFlowerWithPotPush);
            return true;
        }

        if (mIsSingleMode && al::isMsgPlayerCanCarry(pMsg) && al::isSensorName(pSelf, "Root")) {
            mIsCarryable = true;
        }
    }

    if (mIsSingleMode) {
        if ((al::isSensorName(pSelf, "Push") &&
             al::tryReceiveMsgPushAndAddVelocity(this, pMsg, pOther, pSelf,
                                                 al::isMsgPushVeryStrong(pMsg) ? 10.0f : 5.0f)) ||
            rc::tryReceiveMsgPushDirAndAddVelocity(this, pMsg, 5.0f)) {
            mIsPushed = true;
            return true;
        }

        if (rc::tryReceiveMsgPushConnectedAndAddVelocity(this, pMsg, pOther, pSelf, 5.0f)) {
            mIsPushed = true;

            if (!al::isNerve(this, &NrvPackunFlowerWithPotRelease) ||
                !al::isNerve(this, &NrvPackunFlowerWithPotPush)) {
                al::setNerve(this, &NrvPackunFlowerWithPotPush);
            }

            return true;
        }
    } else if (al::isSensorName(pSelf, "Push") &&
               al::tryReceiveMsgPushAndAddVelocity(this, pMsg, pOther, pSelf, 0.5f)) {
        return true;
    }

    if (al::isSensorName(pSelf, "Hold") && mStateHold->tryStartCarry(pMsg, pOther)) {
        mHolderSensor = pOther;

        for (s32 i = 0; i < 3; i++) {
            mHeads[i]->setHolderSensor(mHolderSensor);
        }

        al::startSe(this, "HoldItem");
        al::setVelocityZero(this);
        al::setNerve(this, &NrvPackunFlowerWithPotHold);
        return true;
    }

    return false;
}

/**
 * @brief Freezes the plant when it is touched on the touch screen.
 * @param pMsg Received message.
 * @param pPointer Screen pointer.
 * @param pTarget Screen point target of the plant.
 * @return Whether the message was handled.
 */
bool PackunFlowerWithPot::receiveMsgScreenPoint(const al::SensorMsg* pMsg,
                                                al::ScreenPointer* pPointer,
                                                al::ScreenPointTarget* pTarget) {
    if (al::isNerve(this, &NrvPackunFlowerWithPotPressDown)) {
        return false;
    }

    if (al::isNerve(this, &NrvPackunFlowerWithPotBlowDown)) {
        return false;
    }

    if (al::isNerve(this, &NrvPackunFlowerWithPotDieDown)) {
        return false;
    }

    if (al::isNerve(this, &NrvPackunFlowerWithPotRelease)) {
        return false;
    }

    if (al::isNerve(this, &NrvPackunFlowerWithPotLand)) {
        return false;
    }

    if (al::isNerve(this, &NrvPackunFlowerWithPotHold)) {
        return false;
    }

    if (mStateSupportFreeze->receiveMsgScreenPoint(pMsg, pPointer, pTarget)) {
        if (!al::isNerve(this, &NrvPackunFlowerWithPotSupportFreeze)) {
            mMicRumbler->stopAndReset();
            al::setNerve(this, &NrvPackunFlowerWithPotSupportFreeze);
        }

        return true;
    }

    return false;
}

/** @brief Sleeps in the pot until a player comes close. */
void PackunFlowerWithPot::exeSleep() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "SleepPot");
    }

    if (al::isNearPlayer(this, 100.0f)) {
        al::setNerve(this, &NrvPackunFlowerWithPotFind);
    }
}

/** @brief Plays the wake-up reaction. */
void PackunFlowerWithPot::exeFind() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Find");
    }

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvPackunFlowerWithPotWait);
    }
}

/** @brief Waits and turns towards a nearby player (fast when the player is behind). */
void PackunFlowerWithPot::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait");
    }

    if (al::isNearPlayer(this, 1000.0f)) {
        sead::Vector3f dirToPlayer = al::findNearestPlayerPos(this) - al::getTrans(this);
        al::normalizeOrDirZ(&dirToPlayer);
        sead::Vector3f frontDir;
        al::calcFrontDir(&frontDir, this);

        if (frontDir.dot(dirToPlayer) < 0.0f) {
            al::setNerve(this, &NrvPackunFlowerWithPotTurnFast);
        } else {
            al::setNerve(this, &NrvPackunFlowerWithPotTurn);
        }
    }
}

/** @brief Turns towards the player and attacks once the player is in sight. */
void PackunFlowerWithPot::exeTurn() {
    if (al::isFirstStep(this)) {
        mTurnDegree = 4.0f;
    }

    sead::Vector3f dirToPlayer = al::findNearestPlayerPos(this) - al::getTrans(this);
    al::normalizeOrDirZ(&dirToPlayer);
    sead::Vector3f frontDir;
    al::calcFrontDir(&frontDir, this);
    al::turnQuatFrontToDirDegreeH(this, dirToPlayer, mTurnDegree);

    if (al::isInSightFan(this, al::findNearestPlayerPos(this), frontDir, 500.0f, 5.0f, 45.0f)) {
        al::setNerve(this, &NrvPackunFlowerWithPotAttack);
    }
}

/** @brief Quickly turns around towards a player behind the plant. */
void PackunFlowerWithPot::exeTurnFast() {
    sead::Vector3f dirToPlayer = al::findNearestPlayerPos(this) - al::getTrans(this);
    al::normalizeOrDirZ(&dirToPlayer);
    sead::Vector3f frontDir;
    al::calcFrontDir(&frontDir, this);

    if (al::isFirstStep(this)) {
        sead::Vector3f sideDir;
        al::calcSideDir(&sideDir, this);

        if (sideDir.dot(dirToPlayer) > 0.0f) {
            al::startAction(this, "TurnLeft");
        } else {
            al::startAction(this, "TurnRight");
        }

        mTurnDegree = 4.8f;
    }

    al::turnQuatFrontToDirDegreeH(this, dirToPlayer, mTurnDegree);

    if (al::isInSightFan(this, al::findNearestPlayerPos(this), frontDir, 500.0f, 5.0f, 45.0f)) {
        al::setNerve(this, &NrvPackunFlowerWithPotAttack);
        return;
    }

    if (al::isActionEnd(this)) {
        al::startAction(this, "Wait");
        al::setNerve(this, &NrvPackunFlowerWithPotTurn);
    }
}

/** @brief Bites towards the player. */
void PackunFlowerWithPot::exeAttack() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Attack");
    }

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvPackunFlowerWithPotAfterAttack);
    }
}

/** @brief Rests for a moment after biting. */
void PackunFlowerWithPot::exeAfterAttack() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait");
    }

    if (al::isGreaterEqualStep(this, 40)) {
        al::setNerve(this, &NrvPackunFlowerWithPotWait);
    }
}

/** @brief Slides after being pushed until it settles on the ground again. */
void PackunFlowerWithPot::exePush() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait");
        al::onCollide(this);
    }

    if (mIsEnableReset && rc::isInWaterArea(this)) {
        if (mIsSingleMode) {
            setBlowFromWater();
            return;
        }

        al::setNerve(this, &NrvPackunFlowerWithPotDieDown);
        return;
    }

    al::addVelocityToGravity(this, 1.1f);

    if (!al::isOnGround(this, 0, 0.0f) || !isOnGroundCollision()) {
        return;
    }

    if (mIsPushed) {
        al::scaleVelocity(this, 0.6f);
        return;
    }

    al::offCollide(this);
    al::setVelocityZero(this);
    attachToCollision();
    al::setNerve(this, &NrvPackunFlowerWithPotWait);
}

/** @brief Lets the hold state run while the player carries the plant. */
void PackunFlowerWithPot::exeHold() {
    if (mIsSingleMode) {
        rc::disappearGuideGameWindow(this);
        mIsShowGuide = false;
    }

    al::updateNerveState(this);
}

/** @brief Falls after being released by the player until it lands. */
void PackunFlowerWithPot::exeRelease() {
    if (al::isFirstStep(this)) {
        al::onCollide(this);
        sead::Vector3f trans = al::getTrans(this);

        if (mHolderSensor != nullptr) {
            al::resetPosition(this, al::getTrans(al::getSensorHost(mHolderSensor)), false);
        }

        al::setTrans(this, trans);
        al::addTransOffsetLocalDir(this, 33.0f, 1);
        al::startAction(this, "Wait");
    }

    if (mIsEnableReset && rc::isInWaterArea(this)) {
        if (mIsSingleMode) {
            setBlowFromWater();
            return;
        }

        al::setNerve(this, &NrvPackunFlowerWithPotDieDown);
        return;
    }

    al::addVelocityToGravity(this, 1.1f);

    if (!al::isOnGround(this, 0, 0.0f) || !isOnGroundCollision()) {
        return;
    }

    if (mIsSingleMode && mIsPushed) {
        al::scaleVelocity(this, 0.6f);
        return;
    }

    al::offCollide(this);
    al::setVelocityZero(this);
    attachToCollision();
    al::setNerve(this, &NrvPackunFlowerWithPotLand);
}

/** @brief Plays the landing reaction. */
void PackunFlowerWithPot::exeLand() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Land");
    }

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvPackunFlowerWithPotWait);
    }
}

/** @brief Gets squashed, breaking the pot, and dies with an item. */
void PackunFlowerWithPot::exePressDown() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "PressDown");
        al::startAction(mPot, "Break");
        al::setVelocityZero(this);
        al::changeEnvTextureStamp(this);
        mMicRumbler->stopAndReset();
    }

    if (al::isActionEnd(this)) {
        al::resetEnvTexture(this);
        al::appearItem(this);
        al::startHitReactionDeath(this);
        emitFallLeafEffect();
        kill();
    }
}

/** @brief Dies after being knocked down, respawning if resetting is enabled. */
void PackunFlowerWithPot::exeDieDown() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "BlowDown");
        al::startAction(mPot, "Break");
        al::setVelocityZero(this);
        al::changeEnvTextureStamp(this);
        mMicRumbler->stopAndReset();
    }

    if (al::isActionEnd(this)) {
        al::resetEnvTexture(this);
        al::startHitReactionDeath(this);
        emitFallLeafEffect();

        if (mIsEnableReset && !mIsHiddenWhileHeld) {
            startRespawn();
            return;
        }

        kill();
    }
}

/** @brief Gets blown away by the blow-down state, breaking the pot. */
void PackunFlowerWithPot::exeBlowDown() {
    if (al::isFirstStep(this)) {
        al::startAction(mPot, "Break");
        mMicRumbler->stopAndReset();
        emitFallLeafEffect();
    }

    if (al::updateNerveState(this)) {
        al::appearItem(this);
        al::startHitReactionDeath(this);
        kill();
    }
}

/** @brief Stays frozen by the touch screen until released. */
void PackunFlowerWithPot::exeSupportFreeze() {
    if (al::updateNerveState(this)) {
        al::setNerve(this, &NrvPackunFlowerWithPotWait);
    }
}

/** @brief Moves back to the initial pose and reappears after a delay. */
void PackunFlowerWithPot::exeRespawn() {
    if (al::isStep(this, 179)) {
        mMtxConnector->clear();
        mMtxConnector->setBaseQuatTrans(mInitQuat, mInitTrans);
        al::attachMtxConnectorToCollision(mMtxConnector, this, false);
        al::startAction(this, "Wait");
        al::tryStartSklAnimIfExist(this, "SleepPot");
        al::resetPosition(this, mInitTrans, false);
        al::resetPosition(mPot, al::getTrans(this) - sead::Vector3f::ey * 33.0f, false);

        if (!isOnGroundCollision()) {
            al::setNerve(this, &NrvPackunFlowerWithPotRespawn);
        }

        mHolderSensor = nullptr;

        if (mIsSingleMode) {
            al::setQuat(this, mInitQuat);
        }
    }

    if (al::isStep(this, 180)) {
        al::tryEmitEffect(this, "Appear", nullptr);
        mPot->appear();
        al::startSe(this, "HrVanish");
        al::startAction(mPot, "Wait");
        al::showSilhouetteModel(this);
        al::showModelIfHide(this);
        al::onCollide(this);
        al::validateHitSensors(this);
        al::invalidateHitSensor(this, "EatAttack");
        al::validateClipping(this);
        al::setNerve(this, &NrvPackunFlowerWithPotSleep);
    }
}
