#include "MapObj/DoorKey.hpp"

#include <attributes.h>

#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Collision/Collider.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Movement/FlashingCtrl.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Movement/RumbleCalculator.hpp"
#include "Library/Math/MatrixUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Project/AreaObj/AreaObj.hpp"
#include "Project/AreaObj/AreaObjGroup.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Collision/CollisionPartsFilterBase.hpp"
#include "Project/Collision/CollisionPartsTriangle.hpp"
#include "Project/Collision/CollisionUtil.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Enemy/EnemyStateUtil.hpp"
#include "Layout/GuideGameWindow.hpp"
#include "NPC/ActorStateSupportStroke.hpp"
#include "MapObj/DoorLock.hpp"
#include "MapObj/ItemStatePlayerHold.hpp"
#include "MapObj/ItemStatePlayerHoldParam.hpp"
#include "MapObj/ItemStatePopUpFront.hpp"
#include "Player/Normal/PlayerActor.hpp"
#include "Player/Normal/WaterUtil.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"

namespace {
NERVE_DECL(DoorKey, Wait)
NERVE_DECL(DoorKey, PlayerHold)
NERVE_DECL(DoorKey, Throw)
NERVE_DECL(DoorKey, PopUpAppear)
NERVE_DECL(DoorKey, WaitHide)
NERVE_DECL(DoorKey, OpenThrow)
NERVE_DECL(DoorKey, DisAppear)
NERVE_DECL(DoorKey, Stroked)

// Non-const nerve objects: the game merges them into one block.
DoorKeyNrvWait NrvDoorKeyWait;
DoorKeyNrvPlayerHold NrvDoorKeyPlayerHold;
DoorKeyNrvThrow NrvDoorKeyThrow;
DoorKeyNrvPopUpAppear NrvDoorKeyPopUpAppear;
DoorKeyNrvWaitHide NrvDoorKeyWaitHide;
DoorKeyNrvOpenThrow NrvDoorKeyOpenThrow;
DoorKeyNrvDisAppear NrvDoorKeyDisAppear;
DoorKeyNrvStroked NrvDoorKeyStroked;

/// Pop-up used when the key is pulled out of the ground.
ItemStatePopUpFrontParam sPullOutParam(sead::Vector3f(0.0f, 5.0f, 0.0f), 5.0f, 1.0f, 0, 0.0f, true,
                                       "PullOut", false, nullptr);
/// Flight of the key after the player throws it.
ItemStatePopUpFrontParam sThrowParam(sead::Vector3f(0.0f, 8.0f, 25.0f), 1.3f, 1.05f, 3, 0.99f,
                                     false, "Wait", true, nullptr);
/// Hold offsets of the key for each player size and pose.
ItemStatePlayerHoldParam sPlayerHoldParam(
    sead::Vector3f(-25.0f, 0.0f, 70.0f), sead::Vector3f(-25.0f, 0.0f, 50.0f),
    sead::Vector3f(-30.0f, 0.0f, 30.0f), sead::Vector3f(-25.0f, 0.0f, 50.0f),
    sead::Vector3f(-30.0f, 0.0f, 30.0f), sead::Vector3f(-25.0f, 0.0f, 50.0f),
    sead::Vector3f(-25.0f, 0.0f, 50.0f), sead::Vector3f(-30.0f, 0.0f, 45.0f),
    sead::Vector3f(-25.0f, 0.0f, 50.0f), sead::Vector3f(-30.0f, 0.0f, 45.0f),
    sead::Vector3f(0.0f, 0.0f, -90.0f));
}  // namespace

static bool isHitPolyToPos(const al::LiveActor* pActor, const sead::Vector3f& rPos);

/**
 * @brief Checks whether the key stands on a floor that destroys it (lava or fire).
 * @param pActor The key.
 * @return Whether the key is on a damaging floor.
 */
static inline bool isOnDamageFloor(const al::LiveActor* pActor) {
    if (!al::isOnGround(pActor, 0, 0.0f)) {
        return false;
    }

    const char* materialName = al::getCollidedFloorMaterialCodeName(pActor);
    const char* codeName = al::getCollidedFloorCodeName(pActor);
    return al::isEqualString(materialName, "Lava") ||
           al::isEqualString(materialName, "DamageFire") || al::isEqualString(codeName, "Lava") ||
           al::isEqualString(codeName, "DamageFire");
}

/**
 * @brief Casts an arrow from the center of the key.
 * @param pActor The key.
 * @param rDir Direction of the arrow.
 * @param length Length of the arrow.
 * @return Whether the arrow hit a polygon.
 */
static inline bool isHitPolyOnArrow(const al::LiveActor* pActor, const sead::Vector3f& rDir,
                                    f32 length) {
    al::Triangle triangle;
    sead::Vector3f hitPos;
    al::CollisionPartsFilterActor filter(pActor);
    return alCollisionUtil::getFirstPolyOnArrow(pActor, &hitPos, &triangle,
                                                al::getTrans(pActor) + sead::Vector3f::ey * 80.0f,
                                                rDir * length, &filter, nullptr);
}

/**
 * @brief Checks whether the key is pinched by polygons on both sides of an axis.
 * @param pActor The key.
 * @param rAxis The axis to check.
 * @return Whether both sides of the axis hit a polygon.
 */
static ALWAYS_INLINE bool isSandwichedOnAxis(const al::LiveActor* pActor,
                                             const sead::Vector3f& rAxis) {
    bool isHitFront = isHitPolyOnArrow(pActor, rAxis, 85.0f);
    bool isHitBack = isHitPolyOnArrow(pActor, -rAxis, 85.0f);
    return isHitFront && isHitBack;
}

/**
 * @brief Constructs the key.
 * @param pName Name of the actor.
 */
DoorKey::DoorKey(const char* pName) : al::LiveActor(pName) {}

/**
 * @brief Initializes the key, its states and its respawn position.
 * @param rInfo Actor init info.
 */
void DoorKey::init(const al::ActorInitInfo& rInfo) {
    mIsSingleMode = rInfo.getActorSceneInfo().isSingleMode;
    al::initActorWithArchiveName(this, rInfo, "DoorKey", nullptr);
    al::trySetShadowLength(this, rInfo, nullptr);
    bool isComplete = SingleModeDataFunction::isIslandScenarioIDComplete(this, rInfo);
    al::initNerve(this, &NrvDoorKeyWait, 3);

    mStatePlayerHold = new ItemStatePlayerHold(this, &sPlayerHoldParam, false, false);
    mStatePlayerHold->initColliderControl();
    mStateThrow = new ItemStatePopUpFront(this);
    mStatePullOut = new ItemStatePopUpFront(this);
    mStatePullOut->setParam(sPullOutParam, nullptr);
    al::initNerveState(this, mStatePlayerHold, &NrvDoorKeyPlayerHold, "プレイヤーに持たれる");
    al::initNerveState(this, mStateThrow, &NrvDoorKeyThrow, "投げられる");
    al::initNerveState(this, mStatePullOut, &NrvDoorKeyPopUpAppear, "引っこ抜きから出現");

    mColliderRadius = getCollider()->getRadius();
    mColliderOffsetY = getCollider()->getOffsetY();
    mFlashingCtrl = new al::FlashingCtrl(this, true, false);
    mStateStroke = new ActorStateSupportStroke(this);
    if (isComplete) {
        makeActorDead();
    } else {
        makeActorAppeared();
    }

    mInitTrans = al::getTrans(this);
    al::calcFrontDir(&mInitFront, this);
    mIsFirstWait = true;
    al::hideSilhouetteModel(this);
    al::validateCeilWallFloorMaterialCode(this);
    al::tryGetZoneID(&mZoneId, *rInfo.mPlacementInfo);
    if (mIsSingleMode) {
        mRumble = new al::RumbleCalculatorCosMultLinear(2.5f, 1.5707963705062866f, 0.2f, 30);
    }
}

/**
 * @brief Hides the key unless it is waiting at its first position or being held.
 * @return Whether the key was hidden.
 */
bool DoorKey::hideActor() {
    if ((!mIsFirstWait && al::isNerve(this, &NrvDoorKeyWait)) ||
        al::isNerve(this, &NrvDoorKeyPlayerHold)) {
        return false;
    }

    return al::LiveActor::hideActor();
}

/** @brief Puts the key back at its placement position if it was moved or lost. */
void DoorKey::respawn() {
    if (mIsUsed) {
        return;
    }

    if (al::isNerve(this, &NrvDoorKeyPlayerHold) || al::isNerve(this, &NrvDoorKeyThrow) ||
        al::isNerve(this, &NrvDoorKeyWaitHide)) {
        return;
    }

    if (!mIsFirstWait && al::isNerve(this, &NrvDoorKeyWait)) {
        return;
    }

    al::setNerve(this, &NrvDoorKeyWait);
    al::startHitReaction(this, "消失");
    al::setTrans(this, mInitTrans);
    al::makeQuatFrontUp(al::getQuatPtr(this), mInitFront, sead::Vector3f::ey);
    al::onCollide(this);
    if (al::isDead(this)) {
        makeActorAppeared();
    }

    al::showModelIfHide(this);
}

/** @brief Updates the stroke state, the pickup guide and the hit rumble. */
void DoorKey::control() {
    mStateStroke->update();
    al::LiveActor::control();

    bool isGuideUser = rc::isCurrentGuideGameWindowUser(this);
    if (!isGuideUser) {
        mIsShowGuide = false;
    }

    bool isEnableGuide =
        al::isNerve(this, &NrvDoorKeyWait) ||
        (al::isNerve(this, &NrvDoorKeyPopUpAppear) && al::isGreaterEqualStep(this, 10));
    if (isEnableGuide && mIsPlayerCanCarry) {
        auto* player = static_cast<PlayerActor*>(al::tryFindNearestPlayerActor(this));
        if (player != nullptr && player->getHoldingSensor() == nullptr && !mIsShowGuide) {
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
    } else if (isEnableGuide ? mIsShowGuide :
                               isGuideUser && rc::isGuideGameWindowActive(this)) {
        rc::disappearGuideGameWindow(this);
        mIsShowGuide = false;
    }

    mIsPlayerCanCarry = false;
    if (mRumble != nullptr && !mRumble->isEnd()) {
        mRumble->calc();
        al::setScaleY(this, mRumble->getValueY() + 1.0f);
    } else {
        al::setScaleY(this, 1.0f);
    }
}

/** @brief Updates the collider, following the holder while the key is carried. */
void DoorKey::updateCollider() {
    if (mStatePlayerHold->isDead()) {
        al::LiveActor::updateCollider();
        return;
    }

    mStatePlayerHold->updateCollider(al::getHitSensor(this, "Body"));
}

/**
 * @brief Kills the key, as when its door is opened by another key.
 * @param isPlaySe Whether to play the splash sound.
 */
void DoorKey::triggerKillForce(bool isPlaySe) {
    if (isPlaySe) {
        al::startSe(this, "Splash");
    }

    killForce();
}

/** @brief Kills the key if it is used, otherwise hides it until it respawns. */
void DoorKey::killForce() {
    if (al::isNerve(this, &NrvDoorKeyWaitHide)) {
        return;
    }

    if (mIsEnableDisappearReaction && mIsSameZone) {
        al::startHitReaction(this, "消失");
    }

    if (mIsUsed) {
        kill();
        return;
    }

    al::hideModelIfShow(this);
    al::setNerve(this, &NrvDoorKeyWaitHide);
    al::invalidateClipping(this);
}

/**
 * @brief Pushes and hits the actors touched by the key, and opens doors.
 * @param pSelf Sensor of the key.
 * @param pOther Sensor that was touched.
 */
void DoorKey::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isNerve(this, &NrvDoorKeyDisAppear) || al::isNerve(this, &NrvDoorKeyOpenThrow)) {
        return;
    }

    if (mIsSingleMode && al::isNerve(this, &NrvDoorKeyThrow) && al::isSensorName(pSelf, "Body") &&
        (al::isSensorEnemyBody(pOther) || al::isSensorEnemy(pOther) || al::isSensorNpc(pOther) ||
         al::isSensorMapObj(pOther) || al::isSensorKoopaJr(pOther) || al::isSensorRide(pOther) ||
         al::isSensorBindableGoal(pOther) || al::isSensorBindableGoalItem(pOther) ||
         al::isSensorKickKoura(pOther))) {
        al::sendMsgKeyThrow(pOther, pSelf);
        return;
    }

    if (al::isNerve(this, &NrvDoorKeyPlayerHold)) {
        if (mHolderSensor != nullptr && al::isSensorBlockTransparent(pOther) &&
            al::sendMsgPlayerUpperPunch(pOther, mHolderSensor)) {
            return;
        }

        if (mIsSingleMode && al::isSensorName(pOther, "DoorKeyHit") && mHolderSensor != nullptr &&
            al::isCollidedCeiling(this)) {
            al::sendMsgPlayerUpperPunch(pOther, mHolderSensor);
            return;
        }

        if (al::isSensorName(pSelf, "Hold") && al::isSensorName(pOther, "HoldKey")) {
            if (al::isLessStep(this, 10)) {
                return;
            }

            al::LiveActor* player = rc::findNearestActivePlayerActor(this);
            if (player == nullptr) {
                return;
            }

            sead::Vector3f playerFront;
            al::calcFrontDir(&playerFront, player);
            playerFront.y = 0.0f;
            sead::Vector3f lockFront;
            al::calcFrontDir(&lockFront, al::getSensorHost(pOther));
            lockFront.y = 0.0f;
            if (al::calcAngleDegree(lockFront, -playerFront) > 50.0f) {
                return;
            }

            auto* lock = static_cast<DoorLock*>(al::getSensorHost(pOther));
            mOpenStartTrans = al::getTrans(this);
            al::calcJointPos(&mOpenGoalTrans, lock, "Goal");
            const sead::Vector3f& lockTrans = al::getTrans(lock);
            const sead::Vector3f& playerTrans = al::getTrans(player);
            sead::Vector3f toLock(lockTrans.x - playerTrans.x, 0.0f, lockTrans.z - playerTrans.z);
            if (al::calcAngleDegree(playerFront, toLock) > 40.0f) {
                return;
            }

            mIsSameZone = lock->getZoneId() == mZoneId;
            if (!al::sendMsgKeyOpen(pOther, pSelf)) {
                return;
            }

            rc::requestPlayerRelease(mHolderSensor);
            releaseKey();
            mStateThrow->setParam(sThrowParam, nullptr);
            al::calcFrontDir(&mOpenStartFront, this);
            al::calcFrontDir(&mOpenGoalFront, lock);
            mIsOpened = true;
            mOpenGoalFront = -mOpenGoalFront;
            if (mIsSameZone) {
                al::setNerve(this, &NrvDoorKeyOpenThrow);
                return;
            }

            if (mIsUsed) {
                kill();
                return;
            }

            al::hideModelIfShow(this);
            al::invalidateClipping(this);
            al::setNerve(this, &NrvDoorKeyWaitHide);
            return;
        }
    }

    if (al::isNerve(this, &NrvDoorKeyThrow)) {
        if (al::isSensorName(pSelf, "Hold")) {
            if (al::isSensorMapObj(pOther) && al::isSensorName(pOther, "ThrowKey")) {
                if (!al::sendMsgKeyOpen(pOther, pSelf)) {
                    return;
                }

                al::setNerve(this, &NrvDoorKeyDisAppear);
                mIsOpened = true;
                mStateThrow->kill();
                return;
            }

            if (al::isSensorMapObj(pOther) && al::isSensorHostName(pOther, "DoorLock")) {
                if (al::sendMsgBallItemGet(pOther, pSelf)) {
                    return;
                }

                if (al::sendMsgBallAttack(pOther, pSelf, nullptr)) {
                    return;
                }
            }
        }

        if (!al::isSensorName(pSelf, "Body")) {
            return;
        }

        if (al::isSensorPlayer(pOther)) {
            al::sendMsgPush(pSelf, pOther);
            return;
        }

        if ((al::isSensorEnemyBody(pOther) || al::isSensorNpc(pOther)) &&
            al::sendMsgBallAttack(pOther, pSelf, nullptr)) {
            return;
        }

        if (mIsSingleMode && al::isSensorDoorKey(pOther)) {
            al::sendMsgPush(pOther, pSelf);
        }

        return;
    }

    if (al::isNerve(this, &NrvDoorKeyWait) && al::isSensorName(pSelf, "Body")) {
        if (al::isSensorPlayer(pOther)) {
            al::sendMsgPush(pOther, pSelf);
        }

        if (!mIsSingleMode) {
            return;
        }

        if (al::isSensorNpc(pOther) || al::isSensorEnemy(pOther)) {
            if (al::isSensorPackunWithPot(pOther) && al::isSensorName(pOther, "Push")) {
                al::sendMsgPush(pOther, pSelf);
                return;
            }

            if (al::isSensorHostName(pOther, "雲ボーナス大砲")) {
                al::sendMsgKeyThrow(pOther, pSelf);
                al::sendMsgPush(pSelf, pOther);
                return;
            }

            al::sendMsgPush(pOther, pSelf);
        }

        if (al::isSensorKickKoura(pOther) || al::isSensorMapObj(pOther)) {
            al::sendMsgPush(pOther, pSelf);
        }

        return;
    }

    if (!mIsSingleMode) {
        return;
    }

    if (al::isSensorName(pSelf, "Hold") && al::isSensorMapObj(pOther) &&
        al::isSensorName(pOther, "ThrowKey") && al::sendMsgKeyOpen(pOther, pSelf)) {
        al::setNerve(this, &NrvDoorKeyDisAppear);
        mIsOpened = true;
        if (mHolderSensor != nullptr) {
            rc::requestPlayerRelease(mHolderSensor);
            mHolderSensor = nullptr;
        }
    }
}

/**
 * @brief Releases the key from its holder and prepares the throw.
 * @param pSensor Sensor of the thrower.
 */
void DoorKey::throwKey(al::HitSensor* pSensor) {
    releaseKey();
    mStateThrow->setParam(sThrowParam, pSensor);
}

/**
 * @brief Handles pickup, throw, pushes and attacks.
 * @param pMsg The message.
 * @param pOther Sensor of the sender.
 * @param pSelf Sensor of the key.
 * @return Whether the message was accepted.
 */
bool DoorKey::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) {
    if (al::isNerve(this, &NrvDoorKeyDisAppear) || al::isNerve(this, &NrvDoorKeyOpenThrow)) {
        return false;
    }

    if (al::isMsgPlayerDisregard(pMsg)) {
        if (al::isNerve(this, &NrvDoorKeyPlayerHold)) {
            return true;
        }

        return al::isNerve(this, &NrvDoorKeyThrow);
    }

    if (al::isMsgPlayerCanCarry(pMsg)) {
        mIsPlayerCanCarry = true;
        return true;
    }

    if (rc::isMsgAskControlUserId(pMsg, mHolderSensor)) {
        return true;
    }

    if (al::isSensorName(pSelf, "Hold")) {
        if (al::isMsgPlayerHideItem(pMsg)) {
            al::hideModelIfShow(this);
            al::tryKillEmitterAndParticleAll(this);
        } else if (al::isMsgPlayerShowItem(pMsg)) {
            al::showModelIfHide(this);
        }

        if (al::isNerve(this, &NrvDoorKeyWait) ||
            (al::isNerve(this, &NrvDoorKeyPopUpAppear) && al::isGreaterEqualStep(this, 10))) {
            if (al::isMsgPlayerCarryFront(pMsg) &&
                mStatePlayerHold->tryStartCarryFront(pMsg, pOther, false)) {
                mHolderSensor = pOther;
                al::showModelIfHide(this);
                al::setNerve(this, &NrvDoorKeyPlayerHold);
                return true;
            }

            if (al::isMsgPlayerCarryUpTest(pMsg)) {
                return true;
            }
        } else if (al::isNerve(this, &NrvDoorKeyPlayerHold)) {
            if (al::isMsgPlayerReleaseDamage(pMsg)) {
                return false;
            }

            if (!mStatePlayerHold->receiveMsg(pMsg, pOther, pSelf)) {
                return false;
            }

            throwKey(pOther);
            al::setNerve(this, &NrvDoorKeyThrow);
            return true;
        }
    }

    if (al::isSensorName(pSelf, "Attack")) {
        // The original looks up the attacker here without using it.
        al::getSensorHost(pOther);
        if (rc::isMsgJumpPanelAction(pMsg)) {
            mJumpPanelTimer = 60;
            if (al::getVelocityPtr(this)->y <= 0.0f && al::isNerve(this, &NrvDoorKeyWait)) {
                al::setVelocityY(this, 100.0f);
            } else {
                al::setVelocityY(this, 50.0f);
            }

            return true;
        }

        if (mIsSingleMode && al::isNerve(this, &NrvDoorKeyPlayerHold) &&
            al::isSensorHostName(pOther, "サーチキラー")) {
            return false;
        }

        if (!mIsSingleMode || mRumble == nullptr) {
            return false;
        }

        if (al::isMsgPlayerFireBallAttack(pMsg) || al::isMsgPlayerBoomerangAttack(pMsg) ||
            al::isMsgPlayerTailAttack(pMsg) || al::isMsgEnemyAttackBoomerang(pMsg) ||
            al::isMsgBlockUpperPunch(pMsg) || al::isMsgPlayerSpinAttack(pMsg) ||
            al::isMsgKickKouraReflect(pMsg) || al::isMsgExplosion(pMsg) ||
            al::isMsgEnemyAttackFire(pMsg)) {
            if (!mRumble->isEnd()) {
                return true;
            }

            al::startHitReactionHit(this);
            if (!al::isMsgEnemyAttackBoomerang(pMsg) ||
                !al::isSensorHostName(pOther, "カメック魔法球")) {
                rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
            }

            mRumble->start(0);
            return true;
        }

        if (!al::isSensorKoopaJr(pOther)) {
            return false;
        }

        if (!al::isMsgPlayerSpinAttack(pMsg) && !al::isMsgPlayerHipDropAll(pMsg)) {
            return false;
        }

        if (!mRumble->isEnd()) {
            return true;
        }

        al::startHitReactionHit(this);
        rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        mRumble->start(0);
        return true;
    }

    if (al::isNerve(this, &NrvDoorKeyWait) && al::isMsgPush(pMsg)) {
        sead::Vector3f dir = al::getSensorPos(pSelf) - al::getSensorPos(pOther);
        dir.y = 0.0f;
        if (al::normalizeOrZero(&dir)) {
            dir = sead::Vector3f::ez;
        }

        sead::Vector3f* velocity = al::getVelocityPtr(this);
        *velocity = dir * 5.0f + *velocity;
        return true;
    }

    if (rc::isMsgNeedleRollerAttack(pMsg) &&
        (al::isNerve(this, &NrvDoorKeyWait) || al::isNerve(this, &NrvDoorKeyThrow))) {
        killForce();
        return true;
    }

    if (al::isMsgEnemyAttackFire(pMsg)) {
        return true;
    }

    if (al::isNerve(this, &NrvDoorKeyThrow) &&
        (al::isSensorNpc(pOther) || al::isSensorKoopaJr(pOther) ||
         al::isSensorBindableGoal(pOther) || al::isSensorBindableGoalItem(pOther)) &&
        al::isMsgPush(pMsg) && al::isSensorName(pSelf, "Body")) {
        sead::Vector3f dir = al::getSensorPos(pSelf) - al::getSensorPos(pOther);
        dir.y = 0.0f;
        al::normalizeOrDirZ(&dir);
        f32 dot = al::getVelocity(this).dot(dir);
        if (dot >= 0.0f) {
            return true;
        }

        f32 scale = 5.0f - dot;
        sead::Vector3f* velocity = al::getVelocityPtr(this);
        *velocity = dir * scale + *velocity;
        return true;
    }

    if (mIsSingleMode && al::isMsgPlayerDisregard(pMsg) &&
        al::isNerve(this, &NrvDoorKeyPlayerHold)) {
        return true;
    }

    if (!al::isSensorName(pSelf, "Body")) {
        return false;
    }

    if ((al::isSensorKoopaJr(pOther) || al::isSensorRide(pOther) ||
         al::isSensorHostName(pOther, "GigaBell")) &&
        al::tryReceiveMsgPushAndAddVelocityH(this, pMsg, pOther, pSelf, 5.0f)) {
        return true;
    }

    if (al::isSensorEnemyBody(pOther) ||
        al::isSensorHostName(pOther, "KinopioBrigadeNpcSingleMode")) {
        if (rc::isMsgBlockRailRide(pMsg) || al::isMsgPlayerFireBallAttack(pMsg)) {
            return false;
        }

        if (al::tryReceiveMsgPushAndAddVelocity(this, pMsg, pOther, pSelf, 5.0f)) {
            return true;
        }
    }

    sead::Vector3f reflectDir = sead::Vector3f::zero;
    if (rc::tryGetItemReflectHitDir(&reflectDir, pMsg)) {
        reflectDir.y = 0.0f;
        if (!al::normalizeOrZero(&reflectDir) && al::getVelocity(this).dot(reflectDir) < 0.0f) {
            al::setVelocity(this, reflectDir * al::calcSpeed(this));
            return true;
        }
    }

    if (al::isMsgBowserPush(pMsg) && !al::isNerve(this, &NrvDoorKeyWaitHide) &&
        !al::isNerve(this, &NrvDoorKeyPlayerHold)) {
        killForce();
        return true;
    }

    return false;
}

/**
 * @brief Lets the key be stroked with the touch screen while it waits.
 * @param pMsg The message.
 * @param pPointer The screen pointer.
 * @param pTarget The screen point target of the key.
 * @return Whether the key was stroked.
 */
bool DoorKey::receiveMsgScreenPointSM(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                      al::ScreenPointTarget* pTarget) {
    if (!al::isNerve(this, &NrvDoorKeyWait)) {
        return false;
    }

    if (!mStateStroke->receiveMsgScreenPoint(pMsg, pPointer, pTarget)) {
        return false;
    }

    if (!al::isNerve(this, &NrvDoorKeyStroked)) {
        al::setNerve(this, &NrvDoorKeyStroked);
    }

    return true;
}

/**
 * @brief Moves the key from the player's hands into the lock.
 * @param step Number of steps the motion takes.
 */
void DoorKey::updateOpenThrowPose(s32 step) {
    if (!al::isNerve(this, &NrvDoorKeyOpenThrow)) {
        return;
    }

    f32 rate = al::calcNerveRate(this, step);
    sead::Vector3f trans;
    al::lerpVec(&trans, mOpenStartTrans, mOpenGoalTrans, rate);
    sead::Vector3f front;
    al::lerpVec(&front, mOpenStartFront, mOpenGoalFront, rate);
    sead::Vector3f up;
    al::calcUpDir(&up, this);
    sead::Matrix34f mtx;
    al::makeMtxFrontUpPos(&mtx, front, up, trans);
    al::updatePoseMtx(this, &mtx);
    if (al::isGreaterEqualStep(this, step)) {
        al::setNerve(this, &NrvDoorKeyDisAppear);
    }
}

/** @brief Makes the key appear popping up above its spawner. */
void DoorKey::appearPopUpAbove() {
    appear();
    al::hideModelIfShow(this);
    al::setNerve(this, &NrvDoorKeyPopUpAppear);
}

/** @brief Makes the key appear popping up in front of its spawner. */
void DoorKey::appearPopUpFront() {
    appear();
    al::hideModelIfShow(this);
    al::setNerve(this, &NrvDoorKeyPopUpAppear);
}

/** @return Whether the key can appear again (it is not being used on a door). */
bool DoorKey::isAppearNext() const {
    return !al::isNerve(this, &NrvDoorKeyOpenThrow) && !al::isNerve(this, &NrvDoorKeyDisAppear);
}

/** @brief Waits on the ground, flashing before it disappears and respawns. */
void DoorKey::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait");
        al::setVelocityZero(this);
        al::showModelIfHide(this);
        mFlashingCtrl->start(1500);
        mIsBlinkHigh = false;
        mIsFirstWait = false;
        al::validateClipping(this);
        al::validateHitSensors(this);
        mSandwichTime = 0;
        if (mIsResetTrans) {
            al::setTrans(this, mInitTrans);
        }

        mIsResetTrans = false;
    }

    sead::Vector3f offset = al::getTrans(this) - mInitTrans;
    offset.y = 0.0f;
    if (!al::isNearZero(offset, 0.001f)) {
        mFlashingCtrl->movement();
    }

    if (mFlashingCtrl->isNowJustFlashed()) {
        if (mIsBlinkHigh) {
            al::startSe(this, "BlinkH");
            mIsBlinkHigh = false;
        } else {
            al::startSe(this, "BlinkL");
            mIsBlinkHigh = true;
        }
    }

    if (mFlashingCtrl->isEnded()) {
        killForce();
        return;
    }

    if (mJumpPanelTimer > 0 || !al::isOnGround(this, 0, 0.0f)) {
        al::addVelocityToGravity(this, 2.0f);
        al::scaleVelocity(this, 0.97f);
        if (mJumpPanelTimer > 0) {
            mJumpPanelTimer--;
        }
    } else {
        al::scaleVelocity(this, 0.5f);
    }

    if ((al::isGreaterEqualStep(this, 30) &&
         rc::isInAreaObj(this, rc::AreaObjType::DisappearDoorKeyArea)) ||
        rc::isInWaterArea(this) ||
        (mIsSingleMode && al::isGreaterEqualStep(this, 30) && rc::isCollidedNeedle(this))) {
        killForce();
        return;
    }

    if (!al::isOnGround(this, 0, 0.0f)) {
        if (EnemyStateUtil::tryKillByAreaOrMaterialCodeWithHitReaction(this)) {
            return;
        }
    } else if (isOnDamageFloor(this)) {
        killForce();
        return;
    }

    if (tryKillBySandwichWall()) {
        return;
    }

    al::holdSe(this, "PgWait");
}

/**
 * @brief Kills the key when it stays stuck between polygons for too long.
 * @return Whether the key was killed.
 */
bool DoorKey::tryKillBySandwichWall() {
    if ((al::isCollidedWall(this) && isHitPolyToPos(this, al::getCollidedWallPos(this))) ||
        (al::isCollidedGround(this) && isHitPolyToPos(this, al::getCollidedGroundPos(this))) ||
        (al::isCollidedCeiling(this) && isHitPolyToPos(this, al::getCollidedCeilingPos(this))) ||
        isSandwichedOnAxis(this, sead::Vector3f::ex) ||
        isSandwichedOnAxis(this, sead::Vector3f::ey) ||
        isSandwichedOnAxis(this, sead::Vector3f::ez)) {
        if (mSandwichTime++ >= 31) {
            killForce();
            return true;
        }

        return false;
    }

    mSandwichTime = 0;
    return false;
}

/** @brief Waits hidden, then reappears at the placement position. */
void DoorKey::exeWaitHide() {
    if (mIsUsed) {
        kill();
        return;
    }

    if (al::isFirstStep(this) && !mFlashingCtrl->isEnded()) {
        al::tryDeleteEffectAndParticle(this, "Wait");
        al::tryDeleteEffectAndParticle(this, "Hold");
    }

    if (!al::isGreaterEqualStep(this, 60) && mIsSameZone) {
        return;
    }

    al::showModelIfHide(this);
    al::setTrans(this, mInitTrans);
    al::makeQuatFrontUp(al::getQuatPtr(this), mInitFront, sead::Vector3f::ey);
    al::Collider* collider = getCollider();
    collider->setRadius(mColliderRadius);
    collider->setOffsetY(mColliderOffsetY);
    al::onCollide(this);
    al::validateHitSensors(this);
    al::setNerve(this, &NrvDoorKeyWait);
    al::startSe(this, "PgAppear");
    mIsSameZone = true;
    mIsResetTrans = true;
}

/** @brief Follows the player while being carried. */
void DoorKey::exePlayerHold() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Hold");
        al::startSe(this, "PgHold");
        al::invalidateClipping(this);
        al::Collider* collider = getCollider();
        collider->setRadius(15.0f);
        collider->setOffsetY(40.0f);
        al::offCollide(this);
        mIsOpened = false;
        _1cc = 0.0f;
        _1d0 = 0.0f;
        rc::disappearGuideGameWindow(this);
        mIsShowGuide = false;
    }

    if (rc::isPlayerDamageInvalid(al::getSensorHost(mHolderSensor)) && al::isCollidedWall(this)) {
        isHitPolyToPos(this, al::getCollidedWallPos(this));
    }

    al::updateNerveState(this);
}

/**
 * @brief Casts an arrow from the center of the key towards a collided position.
 * @param pActor The key.
 * @param rPos The collided position.
 * @return Whether the arrow away from the position hit a polygon.
 */
static bool isHitPolyToPos(const al::LiveActor* pActor, const sead::Vector3f& rPos) {
    sead::Vector3f dir = rPos - (al::getTrans(pActor) + sead::Vector3f::ey * 80.0f);
    f32 length = dir.length();
    dir *= 1.0f / length;
    f32 arrowLength = length + 10.0f;
    if (arrowLength < 85.0f) {
        arrowLength = 85.0f;
    }

    return isHitPolyOnArrow(pActor, -dir, arrowLength);
}

/** @brief Flies after being thrown until the key lands. */
void DoorKey::exeThrow() {
    if (al::isFirstStep(this)) {
        _1d5 = false;
        mSandwichTime = 0;
    }

    al::HitSensor* groundSensor = al::tryGetCollidedGroundSensor(this);
    if (groundSensor != nullptr) {
        al::sendMsgKeyThrow(groundSensor, al::getHitSensor(this, "Body"));
    }

    if (al::updateNerveState(this)) {
        al::startHitReaction(this, "着地");
        al::setNerve(this, &NrvDoorKeyWait);
        return;
    }

    if (rc::isInWaterArea(this) || WaterUtil::isInInkArea(this, al::getTrans(this))) {
        al::startSe(this, "Splash");
        killForce();
        return;
    }

    if (al::isCollidedWall(this)) {
        al::startSe(this, "HitWall");
    }

    if (tryKillBySandwichWall()) {
        al::validateHitSensors(this);
        al::onCollide(this);
    }
}

/** @brief Stops while the door pulls the key in. */
void DoorKey::exeOpenThrow() {
    if (al::isFirstStep(this)) {
        al::setVelocityZero(this);
    }
}

/** @brief Pops up out of the ground, then waits. */
void DoorKey::exePopUpAppear() {
    if (al::isFirstStep(this)) {
        al::showModelIfHide(this);
        al::startSe(this, "PgAppear");
        mInitTrans = al::getTrans(this);
        al::calcFrontDir(&mInitFront, this);
    }

    if (al::updateNerveState(this) && al::isGreaterEqualStep(this, 10)) {
        al::setNerve(this, &NrvDoorKeyWait);
    }
}

/** @brief Disappears after being used on a door. */
void DoorKey::exeDisAppear() {
    if ((mIsOpened && al::isGreaterEqualStep(this, 2)) || (!mIsOpened && al::isFirstStep(this))) {
        if (!mIsOpened) {
            al::startHitReaction(this, "消失");
        }

        kill();
    }
}

/** @brief Waits while the key is being stroked. */
void DoorKey::exeStroked() {
    if (!mStateStroke->isTouch()) {
        al::setNerve(this, &NrvDoorKeyWait);
    }
}

/** @brief Puts the key down at the holder's feet and restores its collider. */
void DoorKey::releaseKey() {
    f32 y = al::getTrans(this).y;
    sead::Vector3f trans = al::getActorTrans(mHolderSensor);
    trans.y = y;
    al::resetPosition(this, trans, false);
    al::onCollide(this);
    mStatePlayerHold->kill();
    mHolderSensor = nullptr;
    al::Collider* collider = getCollider();
    collider->setRadius(mColliderRadius);
    collider->setOffsetY(mColliderOffsetY);
    updateCollider();
    al::setVelocityZero(this);
}

/** @return Whether the key is in the island area of its placement zone. */
bool DoorKey::isInSameIsland() const {
    al::AreaObjGroup* group = rc::tryFindAreaObjGroup(this, rc::AreaObjType::IslandArea);
    if (group == nullptr) {
        return false;
    }

    al::AreaObj* area = group->getInVolumeAreaObj(al::getTrans(this));
    if (area == nullptr) {
        return false;
    }

    return mZoneId == area->mZoneID;
}
