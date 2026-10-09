#include "Enemy/Crawler.hpp"

#include <math/seadQuat.h>

#include "Enemy/CrawlerWalkState.hpp"
#include "Enemy/EnemyAttachItem.hpp"
#include "Enemy/EnemyStateBlowDown.hpp"
#include "Enemy/EnemyStateUtil.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/KeyPose/KeyPoseKeeperUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Movement/RumbleCalculator.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Thread/Functor.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ScoreUtil.hpp"

namespace {
NERVE_DECL(Crawler, LaunchStandby)
NERVE_DECL(Crawler, WalkStandby)
NERVE_DECL(Crawler, Stop)
NERVE_DECL(Crawler, Walk)
NERVE_DECL(Crawler, BlowDown)
NERVE_DECL(Crawler, PressDown)
NERVE_DECL(Crawler, Launch)
// Non-const nerve objects: the game keeps them in .data in this order.
CrawlerNrvLaunchStandby NrvCrawlerLaunchStandby;
CrawlerNrvWalkStandby NrvCrawlerWalkStandby;
CrawlerNrvStop NrvCrawlerStop;
CrawlerNrvWalk NrvCrawlerWalk;
CrawlerNrvBlowDown NrvCrawlerBlowDown;
CrawlerNrvPressDown NrvCrawlerPressDown;
CrawlerNrvLaunch NrvCrawlerLaunch;

typedef al::FunctorV0M<Crawler*, void (Crawler::*)()> CrawlerFunctor;

sead::Vector3f sItemOffset(-25.0f, 30.0f, 0.0f);
sead::Vector3f sCoinItemOffset(-20.0f, 80.0f, 0.0f);
sead::Vector3f sBigItemOffset(-30.0f, 110.0f, 0.0f);
EnemyStateBlowDownParam sBlowDownParam(false);
CrawlerWalkStateParam sWalkStateParam;
CrawlerWalkStateParam sBigWalkStateParam(5.0f, 4.5f, 7.0f, 2.2f, -20.0f, 170.0f, 20.0f, 160.0f,
                                         160.0f, 160.0f, 5);

constexpr f32 cScoreComboFactor = 100.0f;
constexpr s32 cDamageTime = 20;
constexpr s32 cTouchCountMax = 120;
}  // namespace

/**
 * @brief Constructs a Crawler.
 * @param pName Actor name.
 */
Crawler::Crawler(const char* pName) : al::LiveActor(pName) {}

/**
 * @brief Initializes the model of the variant, the carried item, the nerves, the blow down and
 * walk states, the key pose move and the kill switch.
 * @param rInfo Placement info of the actor.
 */
void Crawler::init(const al::ActorInitInfo& rInfo) {
    const char* objectName = nullptr;
    al::tryGetObjectName(&objectName, rInfo);
    if (al::isEqualSubString(objectName, "CrawlerNeedle")) {
        al::initActorWithArchiveName(this, rInfo, "CrawlerNeedle", nullptr);
        mActorName = "CrawlerNeedle";
        mIsNeedle = true;
    } else if (al::isEqualSubString(objectName, "CrawlerBig")) {
        mActorName = "CrawlerBig";
        al::initActorWithArchiveName(this, rInfo, "CrawlerBig", nullptr);
        mIsBig = true;
    } else {
        al::initActorWithArchiveName(this, rInfo, "Crawler", nullptr);
    }

    al::tryAddDisplayOffset(this, rInfo);
    al::tryGetDisplayOffset(&mDisplayOffset, rInfo);

    const char* itemName = "NoItem";
    al::tryGetStringArg(&itemName, rInfo, "ItemType");
    if (mIsCoinItem) {
        itemName = "Coin";
    }

    if (!al::isEqualString(itemName, "NoItem")) {
        sead::Vector3f offset = {0.0f, 0.0f, 0.0f};
        if (mIsCoinItem) {
            offset = sCoinItemOffset;
        } else if (mIsBig) {
            offset = sBigItemOffset;
        } else {
            offset = sItemOffset;
        }

        mAttachItem = new EnemyAttachItem(this, "クローラーアイテム", itemName,
                                          mIsCoinItem ? 0 : rc::getItemType(rInfo), nullptr,
                                          al::getJointMtxPtr(this, "JointRoot"), offset, false);
        al::initCreateActorWithPlacementInfo(mAttachItem, rInfo);
    }

    if (al::isEqualSubString(objectName, "Launcher") ||
        al::isEqualSubString(objectName, "Generator")) {
        mIsLauncher = true;
        al::initNerve(this, &NrvCrawlerLaunchStandby, 2);
        makeActorDead();
        if (mAttachItem != nullptr) {
            mAttachItem->makeActorDead();
            mAttachItem->getItemActor()->makeActorDead();
        }
    } else {
        al::tryGetArg(&mWalkDelay, rInfo, "WalkDelay");
        sead::Vector3f trans = al::getTrans(this);
        sead::Vector3f up = sead::Vector3f::zero;
        al::calcUpDir(&up, this);
        al::setTrans(this, trans + up * al::getColliderRadius(this));
        al::initNerve(this, &NrvCrawlerWalkStandby, 2);
        if (al::isValidStageSwitch(this, "SwitchStart")) {
            al::setNerve(this, &NrvCrawlerStop);
        } else if (mWalkDelay == 0) {
            al::setNerve(this, &NrvCrawlerWalk);
        }

        makeActorAppeared();
    }

    al::createAndSetColliderSpecialPurpose(this, "MoveLimit");
    s32 keyMoveOffset = 0;
    mKeyPoseKeeper = al::createKeyPoseKeeper(rInfo);
    al::tryGetArg(&keyMoveOffset, rInfo, "KeyMoveOffset");
    mRumble = new al::RumbleCalculatorCosMultLinear(mIsBig ? 2.5f : 3.0f, 1.5707963705062866f,
                                                    mIsBig ? 0.2f : 0.35f, 30);
    mStateBlowDown = new EnemyStateBlowDown(this, &sBlowDownParam);
    mStateWalk = new CrawlerWalkState(this, mKeyPoseKeeper, mDisplayOffset,
                                      mIsBig ? &sBigWalkStateParam : &sWalkStateParam);
    al::initNerveState(this, mStateBlowDown, &NrvCrawlerBlowDown, "[state]吹き飛ばし");
    al::initNerveState(this, mStateWalk, &NrvCrawlerWalk, "[state]クローラー歩き");

    s32 keyPoseCount = al::getKeyPoseCount(mKeyPoseKeeper);
    if (keyPoseCount >= 2 && keyMoveOffset >= 1) {
        mWalkDelay = 0;
        al::setNerve(this, &NrvCrawlerWalk);
        for (s32 i = 0; i < keyPoseCount; i++) {
            s32 moveTime = al::calcKeyMoveMoveTime(mKeyPoseKeeper);
            if (keyMoveOffset <= moveTime) {
                mStateWalk->setKeyMoveStartStep(keyMoveOffset);
                break;
            }

            al::nextKeyPose(mKeyPoseKeeper);
            keyMoveOffset -= moveTime;
        }
    }

    al::listenStageSwitchOnKill(this, CrawlerFunctor(this, &Crawler::killBySwitch));
    f32 walkSpeed = 1.0f;
    if (al::tryGetArg(&walkSpeed, rInfo, "WalkSpeed")) {
        mStateWalk->setWalkSpeed(walkSpeed);
    }
}

/** @brief Kills the crawler when the kill switch turns on. */
void Crawler::killBySwitch() {
    if (al::isDead(this)) {
        return;
    }

    kill();
}

/** @brief Appears, waiting to be launched when it belongs to a launcher. */
void Crawler::appear() {
    if (mIsLauncher) {
        al::offCollide(this);
        al::setNerve(this, &NrvCrawlerLaunchStandby);
    }

    al::LiveActor::appear();
}

/** @brief Updates the shadow, the timers, the floor rotation, the walk rumble and death areas. */
void Crawler::control() {
    if (al::isNerve(this, &NrvCrawlerLaunchStandby)) {
        return;
    }

    al::setShadowDropDirActorDown(this);
    sead::Vector3f up;
    al::calcUpDir(&up, this);
    al::setShadowDropLengthEvenPlaneNormal(this, up);
    if (mTouchCount > 0) {
        mTouchCount--;
    }

    if (al::getKeyPoseCount(mKeyPoseKeeper) == 1 && al::isOnGround(this, 0, 0.0f)) {
        sead::Quatf rotate;
        al::calcColliderFloorRotatePower(this, &rotate);
        al::getFrontPtr(this)->rotate(rotate);
    }

    if (mDamageTime > 0) {
        mDamageTime--;
    }

    if (al::isNerve(this, &NrvCrawlerWalk)) {
        mRumble->calc();
        al::setScaleY(this, mRumble->getValueY() + 1.0f);
    }

    al::tryKillByDeathArea(this);
}

/** @brief Kills the crawler together with its carried item. */
void Crawler::kill() {
    if (mAttachItem != nullptr && al::isAlive(mAttachItem)) {
        mAttachItem->kill();
        if (al::isAlive(mAttachItem->getItemActor())) {
            mAttachItem->getItemActor()->kill();
        }
    }

    al::startHitReactionDeath(this);
    al::LiveActor::kill();
}

/**
 * @brief Pushes enemies away and attacks players.
 * @param pSelf Own sensor.
 * @param pOther Touched sensor.
 */
void Crawler::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isNerve(this, &NrvCrawlerPressDown) || al::isNerve(this, &NrvCrawlerLaunchStandby) ||
        al::isNerve(this, &NrvCrawlerBlowDown)) {
        return;
    }

    if (al::isNerve(this, &NrvCrawlerLaunch) && !mIsEnableHitInLaunch) {
        return;
    }

    if (al::isSensorEnemyBody(pOther)) {
        al::sendMsgPushAndKillVelocityToTarget(this, pSelf, pOther);
    }

    if (al::isSensorEnemyAttack(pSelf) && al::isSensorPlayer(pOther)) {
        if (!al::sendMsgEnemyAttack(pOther, pSelf)) {
            al::sendMsgPush(pOther, pSelf);
        }
    }
}

/**
 * @brief Handles goal kills, tramples, hip drops and blow down attacks.
 * @param pMsg Received message.
 * @param pOther Sensor of the sender.
 * @param pSelf Own sensor.
 * @return Whether the message was handled.
 */
bool Crawler::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) {
    if (al::isNerve(this, &NrvCrawlerLaunchStandby) || al::isNerve(this, &NrvCrawlerPressDown) ||
        al::isNerve(this, &NrvCrawlerBlowDown)) {
        return false;
    }

    if (al::isMsgGoalKill(pMsg)) {
        al::setAppearItemFactor(this, "直接攻撃", pOther);
        al::appearItem(this);
        rc::addScoreCombo(this, pOther, pMsg, cScoreComboFactor);
        kill();
        return true;
    }

    if (mIsNeedle) {
        sead::Vector3f up = -al::getGravity(this);
        if ((al::isMsgPlayerTrample(pMsg) || al::isMsgPlayerObjHipDropAll(pMsg)) &&
            al::isNearAngleDegree(up, sead::Vector3f::ey, 30.0f)) {
            return false;
        }
    }

    if (al::isNerve(this, &NrvCrawlerLaunch) && !mIsEnableHitInLaunch) {
        return false;
    }

    if (mIsBig) {
        if (al::isMsgPlayerTrample(pMsg) || al::isMsgPlayerObjHipDropAll(pMsg) ||
            al::isMsgPlayerObjHipDropReflectAll(pMsg) || al::isMsgBallTrample(pMsg) ||
            al::isMsgKickKouraReflect(pMsg) || al::isMsgPlayerBoomerangReflect(pMsg) ||
            al::isMsgPlayerBodyAttackReflect(pMsg) || al::isMsgPlayerObjHipDropHighJump(pMsg) ||
            EnemyStateUtil::isMsgBlowDown(pMsg)) {
            if (al::isMsgPlayerInvincibleAttack(pMsg)) {
                return false;
            }

            if (mDamageTime == 0) {
                rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
                startTrampleReaction();
            }

            return true;
        }
    } else {
        if (al::isMsgPlayerTrample(pMsg)) {
            if (mDamageTime == 0) {
                rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
                startTrampleReaction();
            }

            return true;
        }

        if (al::isMsgPlayerObjHipDropAll(pMsg) || al::isMsgBallTrample(pMsg)) {
            al::setNerve(this, &NrvCrawlerPressDown);
            rc::addScoreCombo(this, pOther, pMsg, cScoreComboFactor);
            rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
            rc::setAppearItemFactorByMsg(this, pMsg, pOther);
            return true;
        }

        if (EnemyStateUtil::isMsgBlowDown(pMsg) || al::isMsgBlockLowerPunch(pMsg)) {
            al::setGravity(this, -sead::Vector3f::ey);
            EnemyStateUtil::requestBlowDown(pMsg, pOther, pSelf, mStateBlowDown, true);
            al::setNerve(this, &NrvCrawlerBlowDown);
            rc::addScoreCombo(this, pOther, pMsg, cScoreComboFactor);
            return true;
        }

        if (al::isMsgEnemyAttack(pMsg)) {
            rc::startHitReactionBlowHitMessage(pMsg, this, pOther, pSelf);
            rc::setAppearItemFactorByMsg(this, pMsg, pOther);
            mStateBlowDown->setBlowDir(pOther, pSelf);
            al::setNerve(this, &NrvCrawlerBlowDown);
            al::invalidateClipping(this);
            return true;
        }
    }

    if (al::isMsgLaserAttack(pMsg)) {
        rc::startHitReactionBlowHitMessage(pMsg, this, pOther, pSelf);
        rc::setAppearItemFactorByMsg(this, pMsg, pOther);
        mStateBlowDown->setBlowDir(pOther, pSelf);
        al::setNerve(this, &NrvCrawlerBlowDown);
        al::invalidateClipping(this);
        return true;
    }

    return false;
}

/** @brief Rumbles, starts the trample reaction and becomes invincible for a while. */
void Crawler::startTrampleReaction() {
    mRumble->start(0);
    al::startHitReaction(this, "踏み");
    mDamageTime = cDamageTime;
    al::startVisAnim(this, "Trampled");
}

/**
 * @brief Reacts to touch screen pokes and drops an item after being petted long enough.
 * @param pMsg Received message.
 * @param pPointer Screen pointer.
 * @param pTarget Own screen point target.
 * @return Whether the message was handled.
 */
bool Crawler::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                    al::ScreenPointTarget* pTarget) {
    if (al::isMsgTouchAssistTrig(pMsg) && mDamageTime == 0) {
        startTrampleReaction();
        return true;
    }

    if (!al::isMsgTouchAssist(pMsg)) {
        return false;
    }

    mTouchCount += 2;
    if (mTouchCount > cTouchCountMax) {
        if (mIsTouchItemAppeared) {
            return true;
        }

        mIsTouchItemAppeared = true;
        sead::Vector3f pos = al::getTrans(this);
        if (mIsBig) {
            sead::Vector3f up = sead::Vector3f::ey;
            al::calcUpDir(&up, this);
            pos += up * 300.0f;
        }

        al::appearItemTiming(this, "撫でる", pos, al::getFront(this));
    }

    return true;
}

/** @brief Clips the crawler together with its carried item. */
void Crawler::startClipped() {
    al::LiveActor::startClipped();
    if (mAttachItem != nullptr && al::isAlive(mAttachItem) && !al::isClipped(mAttachItem)) {
        mAttachItem->startClipped();
    }
}

/** @brief Unclips the crawler together with its carried item. */
void Crawler::endClipped() {
    al::LiveActor::endClipped();
    if (mAttachItem != nullptr && al::isAlive(mAttachItem) && al::isClipped(mAttachItem)) {
        mAttachItem->endClipped();
    }
}

/**
 * @brief Places the crawler again (used by generators).
 * @param rFront New front direction.
 * @param rUp New up direction.
 * @param rTrans New position (the collider radius is added along the Y axis).
 */
void Crawler::resetParam(const sead::Vector3f& rFront, const sead::Vector3f& rUp,
                         const sead::Vector3f& rTrans) {
    al::setGravity(this, -rUp);
    al::setFront(this, rFront);
    al::resetPosition(this, rTrans, false);
    al::startAction(this, "Wait");
    sead::Vector3f* trans = al::getTransPtr(this);
    *trans += sead::Vector3f(0.0f, al::getColliderRadius(this), 0.0f);
}

/**
 * @brief Sets the walk speed of the walk state.
 * @param speed Walk speed.
 */
void Crawler::setWalkSpeed(f32 speed) {
    mStateWalk->setWalkSpeed(speed);
}

/** @brief Starts the launch when waiting in a launcher. */
void Crawler::startLaunch() {
    if (al::isNerve(this, &NrvCrawlerLaunchStandby)) {
        al::setNerve(this, &NrvCrawlerLaunch);
    }
}

/** @brief Starts walking after a launch and makes the carried item appear. */
void Crawler::startWalk() {
    if (!al::isNerve(this, &NrvCrawlerLaunch)) {
        return;
    }

    al::setVelocityZero(this);
    al::setNerve(this, &NrvCrawlerWalk);
    if (mAttachItem != nullptr && al::isDead(mAttachItem)) {
        mAttachItem->appear();
    }
}

/**
 * @brief Checks whether the crawler was defeated.
 * @return Whether the crawler is being blown down or pressed down.
 */
bool Crawler::isDamaged() const {
    return al::isNerve(this, &NrvCrawlerBlowDown) || al::isNerve(this, &NrvCrawlerPressDown);
}

/** @brief Waits until the start switch turns on. */
void Crawler::exeStop() {
    if (al::isFirstStep(this)) {
        al::onCollide(this);
    }

    if (al::isValidStageSwitch(this, "SwitchStart") && al::isOnStageSwitch(this, "SwitchStart")) {
        al::setNerve(this, &NrvCrawlerWalkStandby);
    }
}

/** @brief Waits for the walk delay before walking. */
void Crawler::exeWalkStandby() {
    if (al::isGreaterEqualStep(this, mWalkDelay)) {
        al::setNerve(this, &NrvCrawlerWalk);
    }
}

/** @brief Waits in the launcher. */
void Crawler::exeLaunchStandby() {}

/** @brief Gets launched (moved by the launcher). */
void Crawler::exeLaunch() {}

/** @brief Walks, dying after a while when launched by a launcher. */
void Crawler::exeWalk() {
    al::updateNerveState(this);
    if (al::isGreaterEqualStep(this, mKillStep) && mIsLauncher) {
        mStateWalk->kill();
        kill();
    }
}

/** @brief Gets squashed flat and dies when the press down action ends. */
void Crawler::exePressDown() {
    if (al::isFirstStep(this)) {
        al::onCollide(this);
        al::setVelocityZero(this);
        al::startAction(this, "PressDown");
        al::changeEnvTextureStamp(this);
        if (mAttachItem != nullptr) {
            mAttachItem->endAttach(false);
        }

        sead::Vector3f side = {0.0f, 0.0f, 0.0f};
        sead::Vector3f front = {0.0f, 0.0f, 0.0f};
        al::calcSideDir(&side, this);
        if (al::isParallelDirection(side, sead::Vector3f::ey, 0.01f)) {
            front.setCross(side, sead::Vector3f::ez);
        } else {
            front.setCross(side, sead::Vector3f::ey);
        }

        al::normalize(&front);
        al::setFront(this, front);
        al::setGravity(this, -sead::Vector3f::ey);
        al::setScaleY(this, 1.0f);
    }

    rc::tryAppearItemPressDown(this, nullptr);
    al::addVelocityToGravity(this, 5.0f);
    al::scaleVelocity(this, 0.95f);
    if (al::isOnGround(this, 0, 0.0f)) {
        al::setVelocityZero(this);
        al::resetEnvTexture(this);
        if (al::isActionEnd(this)) {
            kill();
        }
    }
}

/** @brief Gets blown away and dies, dropping its item. */
void Crawler::exeBlowDown() {
    if (al::isFirstStep(this)) {
        mShadowLength = al::getShadowDropLength(this, "体影");
        al::setShadowDropLength(this, 500.0f, "体影");
        if (mAttachItem != nullptr) {
            mAttachItem->endAttach(false);
        }

        al::setColliderFilterCollisionParts(this, nullptr);
    }

    if (al::updateNerveState(this)) {
        al::appearItem(this);
        al::setShadowDropLength(this, mShadowLength, "体影");
        kill();
    }
}
