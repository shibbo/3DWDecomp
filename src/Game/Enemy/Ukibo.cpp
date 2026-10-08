#include "Enemy/Ukibo.hpp"

#include "Enemy/ActorMicRumbler.hpp"
#include "Enemy/ActorStateSupportFreeze.hpp"
#include "Enemy/EnemyStateBlowDown.hpp"
#include "Enemy/EnemyStateUtil.hpp"
#include "Enemy/TargetFinder.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Item/ItemUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/LiveActor/Util/ActorAnimUtil.hpp"
#include "Library/LiveActor/Util/ActorModelUtil.hpp"
#include "Library/LiveActor/Util/ActorSceneUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Project/AreaObj/AreaObj.hpp"
#include "Project/AreaObj/AreaObjUtil.hpp"
#include "Project/AreaObj/AreaShape.hpp"
#include "Project/Base/StringUtil.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ScoreUtil.hpp"

namespace {
NERVE_DECL(Ukibo, Wait)
NERVE_DECL(Ukibo, BlowDown)
NERVE_DECL(Ukibo, SupportFreeze)
NERVE_DECL(Ukibo, PressDownHipDrop)
NERVE_DECL(Ukibo, ChaseImmediate)
NERVE_DECL(Ukibo, PressDown)
NERVE_DECL(Ukibo, Move)
NERVE_DECL(Ukibo, Find)
NERVE_DECL(Ukibo, Chase)
NERVE_DECL(Ukibo, ChaseEnd)
NERVE_DECL(Ukibo, SleepStart)
// Non-const nerve objects: the game keeps them in .data.
UkiboNrvChaseImmediate NrvUkiboChaseImmediate;
UkiboNrvMove NrvUkiboMove;
UkiboNrvChaseEnd NrvUkiboChaseEnd;
UkiboNrvWait NrvUkiboWait;
UkiboNrvBlowDown NrvUkiboBlowDown;
UkiboNrvSupportFreeze NrvUkiboSupportFreeze;
UkiboNrvPressDownHipDrop NrvUkiboPressDownHipDrop;
UkiboNrvPressDown NrvUkiboPressDown;
UkiboNrvFind NrvUkiboFind;
UkiboNrvChase NrvUkiboChase;
UkiboNrvSleepStart NrvUkiboSleepStart;

const sead::Vector3f sWaterSearchStep(0.0f, -50.0f, 0.0f);
const sead::Vector3f sSurfaceCheckStart(0.0f, 500.0f, 0.0f);
const sead::Vector3f sSurfaceCheckEnd(0.0f, -500.0f, 0.0f);

/** @brief Target finder and knockback parameters, tweaked after default construction. */
struct UkiboParam {
    UkiboParam() : mBlowDownParam(false) {
        mTargetFinderParam._0 = 750.0f;
        mTargetFinderParam._4 = 120.0f;
        mTargetFinderParam._8 = 70.0f;
        mTargetFinderParam._10 = 750.0f;
        mBlowDownParam.mAction = "UkiboBlowDown";
    }

    TargetFinderParam mTargetFinderParam;
    EnemyStateBlowDownParam mBlowDownParam;
};

UkiboParam sParam;
ActorStateSupportFreezeParam sSupportFreezeParam(true, 15, false, true, 120,
                                                 sead::Vector3f(0.0f, 150.0f, 0.0f));
}  // namespace

/** @brief Picks a random destination inside the move area. */
inline void Ukibo::decideDestination() {
    f32 offsetY = mOffsetY;
    f32 offsetX = al::getRandom() * al::getRandom((mMoveAreaMax.x - mMoveAreaMin.x) * -0.5f,
                                                  (mMoveAreaMax.x - mMoveAreaMin.x) * 0.5f);
    f32 offsetZ = al::getRandom() * al::getRandom((mMoveAreaMax.z - mMoveAreaMin.z) * -0.5f,
                                                  (mMoveAreaMax.z - mMoveAreaMin.z) * 0.5f);
    mDestination = getMoveAreaCenter() + sead::Vector3f(offsetX, offsetY, offsetZ);
}

/**
 * @brief Checks whether the actor is horizontally inside the move area, with a margin.
 * @return Whether the actor is inside the move area.
 */
inline bool Ukibo::isInMoveArea() const {
    const sead::Vector3f min = mMoveAreaMin;
    const sead::Vector3f max = mMoveAreaMax;
    f32 centerX = (min.x + max.x) * 0.5f;
    f32 rangeX = (max.x - min.x) * 0.5f - 200.0f;
    if (!al::isInRange(al::getTrans(this).x, centerX - rangeX, centerX + rangeX)) {
        return false;
    }

    f32 centerZ = (min.z + max.z) * 0.5f;
    f32 rangeZ = (max.z - min.z) * 0.5f - 200.0f;
    return al::isInRange(al::getTrans(this).z, centerZ - rangeZ, centerZ + rangeZ);
}

/** @brief Turns towards the target and accelerates forwards. */
inline void Ukibo::addChaseVelocity() {
    al::turnDirectionToTargetDegree(this, al::getFrontPtr(this), al::getTrans(mTarget), 1.0f);
    sead::Vector3f* velocity = al::getVelocityPtr(this);
    velocity->setScaleAdd(0.4f, al::getFront(this), *velocity);
    al::scaleVelocity(this, 0.9f);
}

/**
 * @brief Constructs an Ukibo and its swim ring actor.
 * @param pName Actor name.
 */
Ukibo::Ukibo(const char* pName)
    : al::LiveActor(pName), mFloatActor(new al::LiveActor("ウキボーの浮き輪")) {}

/**
 * @brief Initializes the Goomba and swim ring models, the states and the nerves.
 * @param rInfo Placement info of the actor.
 */
void Ukibo::init(const al::ActorInitInfo& rInfo) {
    if (rInfo.getActorSceneInfo().isSingleMode) {
        al::initActorWithArchiveName(this, rInfo, "KuriboFur", "Ukibo");
    } else {
        al::initActorWithArchiveName(this, rInfo, "Kuribo", "Ukibo");
    }

    al::initNerve(this, &NrvUkiboWait, 2);
    al::initActorWithArchiveName(mFloatActor, rInfo, "UkiboFloat", nullptr);
    if (al::isSingleMode(rInfo)) {
        setGlobalAlphaPtr(&mFloatActor->mGlobalAlphaLastFrame);
    }

    mTargetFinder = new TargetFinder(this, &sParam.mTargetFinderParam);
    mStateBlowDown = new EnemyStateBlowDown(this, &sParam.mBlowDownParam);
    mStateSupportFreeze = new ActorStateSupportFreeze(this, &sSupportFreezeParam);
    al::initNerveState(this, mStateBlowDown, &NrvUkiboBlowDown, "吹き飛び死亡");
    al::initNerveState(this, mStateSupportFreeze, &NrvUkiboSupportFreeze, "DRC拘束");
    al::createAndSetColliderSpecialPurpose(this, "MoveLimit");

    bool isOffCollide = false;
    if (al::tryGetArg(&isOffCollide, rInfo, "IsOffCollide") && isOffCollide) {
        al::offCollide(this);
    }

    mMicRumbler = new ActorMicRumbler(this, nullptr);
    startActionWithFloat("MoveSurface");
    mFloatActor->makeActorAppeared();
    makeActorAppeared();

    const sead::Vector3f& front = al::getFront(this);
    mInitFront.x = front.x;
    mInitFront.y = front.y;
    mInitFront.z = front.z;
}

/**
 * @brief Starts an action on the Goomba and the matching action on the swim ring.
 * @param pActionName Action name without the "Ukibo" / "UkiboFloat" prefix.
 */
void Ukibo::startActionWithFloat(const char* pActionName) {
    al::StringTmp<32> actionName("Ukibo%s", pActionName);
    al::StringTmp<32> floatActionName("UkiboFloat%s", pActionName);
    if (al::isActionPlaying(this, actionName.cstr())) {
        return;
    }

    al::startAction(this, actionName.cstr());
    al::startAction(mFloatActor, floatActionName.cstr());
}

/**
 * @brief Finds the water area below the actor and places it on the water surface. Dies when there
 * is no water.
 */
void Ukibo::initAfterPlacement() {
    for (s32 i = 1; i <= 10; i++) {
        mWaterArea = rc::tryFindAreaObj(this, rc::AreaObjType::WaterArea,
                                        al::getTrans(this) + sWaterSearchStep * i);
        if (mWaterArea != nullptr) {
            break;
        }
    }

    if (mWaterArea == nullptr ||
        !al::checkAreaObjCollisionByArrow(al::getTransPtr(this), nullptr, mWaterArea,
                                          al::getTrans(this) + sSurfaceCheckStart,
                                          al::getTrans(this) + sSurfaceCheckEnd)) {
        kill();
        return;
    }

    sead::Vector3f areaTrans = {0.0f, 0.0f, 0.0f};
    mWaterArea->getAreaShape()->calcTrans(&areaTrans);
    mOffsetY = al::getTrans(this).y - areaTrans.y;

    const sead::Vector3f& scale = mWaterArea->getAreaShape()->mScale;
    f32 sizeX = scale.x * 1000.0f;
    f32 sizeZ = scale.z * 1000.0f;
    // Absolute value written as 0 - x: the game subtracts from +0.0 instead of negating.
    sead::Vector3f halfSize(sizeX > 0.0f ? sizeX : 0.0f - sizeX, 0.0f,
                            sizeZ > 0.0f ? sizeZ : 0.0f - sizeZ);
    halfSize *= 0.5f;
    mMoveAreaMin = areaTrans - halfSize;
    mMoveAreaMax = halfSize + areaTrans;
    decideDestination();
}

/** @brief Keeps the swim ring at the Goomba's position and dies when touching slow ink. */
void Ukibo::control() {
    if (al::isNerve(this, &NrvUkiboPressDownHipDrop)) {
        return;
    }

    al::copyPose(mFloatActor, this);
    al::setTrans(mFloatActor, al::getTrans(this));
    if (al::isNerve(this, &NrvUkiboPressDown) || al::isNerve(this, &NrvUkiboBlowDown) ||
        al::isNerve(this, &NrvUkiboPressDownHipDrop)) {
        return;
    }

    mMicRumbler->update();
    al::setScale(mFloatActor, al::getScale(this));
    if (GameDataFunction::isSingleMode(this) && rc::isCollidedInkSlow(this)) {
        kill();
    }
}

/** @brief Appears together with the swim ring. */
void Ukibo::appear() {
    al::LiveActor::makeActorAppeared();
    mFloatActor->makeActorAppeared();
}

/** @brief Reappears facing the initial direction and immediately chases the nearest player. */
void Ukibo::reappear() {
    al::setFront(this, mInitFront);
    al::setNerve(this, &NrvUkiboChaseImmediate);
    startActionWithFloat("MoveSurfaceStart");
    updateMoveSurface();
    al::copyPose(mFloatActor, this);
    al::setTrans(mFloatActor, al::getTrans(this));
    mFloatActor->makeActorAppeared();
    makeActorAppeared();
}

/** @brief Follows the movement of the water area and keeps the actor on its surface. */
void Ukibo::updateMoveSurface() {
    sead::Vector3f areaTrans = {0.0f, 0.0f, 0.0f};
    mWaterArea->getAreaShape()->calcTrans(&areaTrans);

    sead::Vector3f areaMove = areaTrans - getMoveAreaCenter();
    *al::getTransPtr(this) += areaMove;
    mDestination += areaMove;

    sead::Vector3f halfSize = (mMoveAreaMax - mMoveAreaMin) * 0.5f;
    mMoveAreaMin = areaTrans - halfSize;
    mMoveAreaMax = halfSize + areaTrans;
    al::setTransY(this, getMoveAreaCenter().y + mOffsetY);
}

/** @brief Dies together with the swim ring. */
void Ukibo::makeActorDead() {
    al::LiveActor::makeActorDead();
    mFloatActor->makeActorDead();
}

/**
 * @brief Dies together with the swim ring.
 * @param isNoReaction Whether to skip the death hit reaction.
 */
void Ukibo::killComplete(bool isNoReaction) {
    if (!al::isDead(this) && !isNoReaction) {
        al::startHitReactionDeath(this);
    }

    al::LiveActor::kill();
    mFloatActor->kill();
}

/** @brief Dies when the kill stage switch turns on. */
void Ukibo::killBySwitch() {
    kill();
}

/** @brief Dies with a hit reaction together with the swim ring. */
void Ukibo::kill() {
    al::startHitReactionDeath(this);
    al::LiveActor::kill();
    mFloatActor->kill();
}

/**
 * @brief Attacks players and pushes other enemies and NPCs.
 * @param pSelf Sensor of the Ukibo.
 * @param pOther Sensor that was hit.
 */
void Ukibo::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isNerve(this, &NrvUkiboPressDown) || al::isNerve(this, &NrvUkiboBlowDown) ||
        al::isNerve(this, &NrvUkiboPressDownHipDrop)) {
        return;
    }

    if (al::isSensorEnemyAttack(pSelf) &&
        (al::isSensorPlayer(pOther) || al::isSensorKoopaJr(pOther))) {
        al::sendMsgPush(pOther, pSelf);
        al::sendMsgEnemyAttackForCrossoverSensor(pOther, pSelf);
    }

    if (al::isSensorEnemyBody(pSelf) &&
        (al::isSensorEnemyBody(pOther) || al::isSensorNpc(pOther))) {
        al::sendMsgPushAndKillVelocityToTarget(this, pSelf, pOther);
    }
}

/**
 * @brief Handles pushes, stomps, knockbacks and Piranha Plant bites.
 * @param pMsg Received message.
 * @param pOther Sensor that sent the message.
 * @param pSelf Sensor of the Ukibo that received the message.
 * @return Whether the message was handled.
 */
bool Ukibo::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) {
    if (!al::isSensorEnemyBody(pSelf)) {
        return false;
    }

    if (al::isNerve(this, &NrvUkiboPressDown) || al::isNerve(this, &NrvUkiboBlowDown) ||
        al::isNerve(this, &NrvUkiboPressDownHipDrop)) {
        return false;
    }

    if (al::tryReceiveMsgPushAndAddVelocity(this, pMsg, pOther, pSelf, 0.8f)) {
        return true;
    }

    if (EnemyStateUtil::tryRequestPressDown(pMsg, pOther, pSelf, true)) {
        rc::addScoreCombo(this, pOther, pMsg, 100.0f);
        mMicRumbler->stopAndReset();
        if (al::isMsgPlayerObjHipDropAll(pMsg)) {
            mTarget = al::getSensorHost(pOther);
            al::setNerve(this, &NrvUkiboPressDownHipDrop);
        } else {
            al::setNerve(this, &NrvUkiboPressDown);
        }

        return true;
    }

    if (EnemyStateUtil::tryRequestBlowDownAndNextNerve(pMsg, pOther, pSelf, mStateBlowDown,
                                                       &NrvUkiboBlowDown, true)) {
        rc::addScoreCombo(this, pOther, pMsg, 100.0f);
        mMicRumbler->stopAndReset();
        return true;
    }

    if (!GameDataFunction::isSingleMode(this)) {
        return false;
    }

    if (rc::isMsgPackunEatStart(pMsg)) {
        return true;
    }

    if (rc::isMsgPackunEat(pMsg)) {
        kill();
        return true;
    }

    if (al::isMsgKeyThrow(pMsg)) {
        rc::setAppearItemFactorByMsg(this, pMsg, pOther);
        al::setNerve(this, &NrvUkiboBlowDown);
        return true;
    }

    if (al::isSensorHostName(pSelf, "パックンフラワー（鉢植えあり")) {
        rc::sendMsgPushConnected(pSelf, pOther);
    }

    return false;
}

/**
 * @brief Gets frozen by the touch screen.
 * @param pMsg Received message.
 * @param pPointer Screen pointer that sent the message.
 * @param pTarget Screen point target of the Ukibo.
 * @return Whether the message was handled.
 */
bool Ukibo::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                  al::ScreenPointTarget* pTarget) {
    if (al::isNerve(this, &NrvUkiboPressDown)) {
        return false;
    }

    if (al::isNerve(this, &NrvUkiboBlowDown)) {
        return false;
    }

    if (al::isNerve(this, &NrvUkiboPressDownHipDrop)) {
        return false;
    }

    if (mStateSupportFreeze->receiveMsgScreenPoint(pMsg, pPointer, pTarget)) {
        if (!al::isNerve(this, &NrvUkiboSupportFreeze)) {
            mNerveBeforeFreeze = getNerveKeeper()->getCurrentNerve();
            al::setNerve(this, &NrvUkiboSupportFreeze);
        }

        return true;
    }

    return false;
}

/** @brief Starts floating on the surface, then waits. */
void Ukibo::exeSleepStart() {
    if (al::isFirstStep(this)) {
        startActionWithFloat("MoveSurfaceStart");
    }

    updateMoveSurface();
    al::setNerveAtActionEnd(this, &NrvUkiboWait);
}

/** @brief Floats in place for a while before moving. */
void Ukibo::exeWait() {
    if (al::isFirstStep(this) && !al::isActionPlaying(this, "MoveSurface")) {
        startActionWithFloat("MoveSurface");
    }

    updateMoveSurface();
    if (al::isGreaterEqualStep(this, 60)) {
        al::setNerve(this, &NrvUkiboMove);
    }
}

/** @brief Floats towards the destination while spinning, and looks for players. */
void Ukibo::exeMove() {
    sead::Vector3f dir = mDestination;
    dir -= al::getTrans(this);
    dir.y = 0.0f;
    if (al::isFirstStep(this)) {
        startActionWithFloat("MoveSurface");
        al::setVelocityToDirection(this, dir, 0.1f);
        mMoveAngle = al::calcAngleOnPlaneDegree(sead::Vector3f::ez, al::getFront(this),
                                                sead::Vector3f::ey);
    }

    updateMoveSurface();
    mTargetFinder->update();
    if (mTargetFinder->getTarget() != nullptr && isInMoveArea() &&
        al::isInRange(al::getTrans(this).y - mTargetFinder->getTargetPos().y, -150.0f, 150.0f)) {
        al::setNerve(this, &NrvUkiboFind);
        return;
    }

    sead::Vector3f toDestination = mDestination;
    toDestination -= al::getTrans(this);
    al::verticalizeVec(&toDestination, al::getGravity(this), toDestination);
    if (toDestination.length() < 5.0f || al::isGreaterEqualStep(this, 600)) {
        decideDestination();
        al::setNerve(this, &NrvUkiboWait);
        return;
    }

    sead::Vector3f front = sead::Vector3f::ez;
    mMoveAngle = al::wrapAngle(mMoveAngle + 0.25f);
    al::rotateVectorDegreeY(&front, mMoveAngle);
    al::setFront(this, front);
    al::addVelocityToDirection(this, dir, 0.1f);
    al::scaleVelocity(this, 0.9f);
}

/** @brief Stops and turns to the found player. */
void Ukibo::exeFind() {
    if (al::isFirstStep(this)) {
        mTarget = mTargetFinder->getTarget();
        startActionWithFloat("Find");
        al::setVelocityZero(this);
    }

    al::turnToTarget(this, al::getTrans(mTarget), 7.5f);
    updateMoveSurface();
    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvUkiboChase);
    }
}

/** @brief Chases the target until it is lost, leaves the move area or stands on the Ukibo. */
void Ukibo::exeChase() {
    if (al::isFirstStep(this)) {
        startActionWithFloat("ChaseSurface");
        al::setVelocityToDirection(this, al::getFront(this), 0.4f);
        mLostTargetTime = 0;
    }

    updateMoveSurface();
    mTargetFinder->update();
    if (mTargetFinder->getTarget() != nullptr &&
        al::isInRange(al::getTrans(this).y - mTargetFinder->getTargetPos().y, -150.0f, 150.0f)) {
        mLostTargetTime = 0;
    } else {
        mLostTargetTime++;
    }

    s32 targetAboveTime;
    const sead::Vector3f& targetTrans = al::getTrans(mTarget);
    if (targetTrans.y < al::getTrans(this).y + 50.0f) {
        targetAboveTime = 0;
    } else {
        sead::Vector3f toTarget = targetTrans - al::getTrans(this);
        al::verticalizeVec(&toTarget, al::getGravity(this), toTarget);
        targetAboveTime = toTarget.length() < 100.0f ? mTargetAboveTime + 1 : 0;
    }

    mTargetAboveTime = targetAboveTime;
    if (al::isGreaterEqualStep(this, 600) || !isInMoveArea() || mTargetAboveTime > 6 ||
        mLostTargetTime > 180) {
        al::setNerve(this, &NrvUkiboChaseEnd);
        return;
    }

    addChaseVelocity();
}

/** @brief Chases the nearest player right after reappearing. */
void Ukibo::exeChaseImmediate() {
    mTarget = al::findNearestPlayerActor(this);
    if (al::isFirstStep(this)) {
        if (mTarget == nullptr) {
            al::setNerve(this, &NrvUkiboSleepStart);
            return;
        }

        startActionWithFloat("ChaseSurface");
        al::setVelocityToDirection(this, al::getFront(this), 0.4f);
        mLostTargetTime = 0;
    }

    updateMoveSurface();
    mTargetFinder->update();
    al::LiveActor* target = mTargetFinder->getTarget();
    if (target != nullptr) {
        mTarget = target;
        al::setNerve(this, &NrvUkiboChase);
        return;
    }

    addChaseVelocity();
}

/** @brief Slows down, then picks a new destination. */
void Ukibo::exeChaseEnd() {
    al::scaleVelocity(this, 0.9f);
    updateMoveSurface();
    if (al::isGreaterEqualStep(this, 60)) {
        decideDestination();
        al::setNerve(this, &NrvUkiboSleepStart);
    }
}

/** @brief Gets squashed and dies. */
void Ukibo::exePressDown() {
    if (al::isFirstStep(this)) {
        startActionWithFloat("PressDown");
        al::setVelocityZero(this);
        al::changeEnvTextureStamp(this);
        mMicRumbler->stopAndReset();
    }

    rc::tryAppearItemPressDown(this, nullptr);
    if (al::isActionEnd(this)) {
        kill();
    }
}

/** @brief Gets knocked away and dies. */
void Ukibo::exeBlowDown() {
    if (al::isFirstStep(this)) {
        al::startAction(mFloatActor, "UkiboFloatBlowDown");
        mMicRumbler->stopAndReset();
    }

    if (al::updateNerveState(this)) {
        al::appearItem(this);
        kill();
    }
}

/** @brief Gets squashed by a ground pound, sinking down to the attacker, and dies. */
void Ukibo::exePressDownHipDrop() {
    if (al::isFirstStep(this)) {
        startActionWithFloat("HipDropDown");
        al::setVelocityZero(this);
        al::changeEnvTextureStamp(this);
    }

    if (mTarget != nullptr && al::getTrans(mTarget).y < al::getTrans(this).y) {
        al::setTransY(this, al::getTrans(mTarget).y);
    }

    rc::tryAppearItemPressDown(this, nullptr);
    if (al::isActionEnd(this)) {
        al::resetEnvTexture(this);
        kill();
    }
}

/** @brief Stays frozen by the touch screen, then returns to the previous nerve. */
void Ukibo::exeSupportFreeze() {
    if (al::isFirstStep(this)) {
        mFloatSklAnimFrameRate = al::getSklAnimFrameRate(mFloatActor, 0);
        al::setSklAnimFrameRate(mFloatActor, 0.0f, 0);
    }

    if (al::updateNerveState(this)) {
        al::setSklAnimFrameRate(mFloatActor, mFloatSklAnimFrameRate, 0);
        al::setNerve(this, mNerveBeforeFreeze);
    }
}
