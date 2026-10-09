#include "Enemy/Swimmer.hpp"

#include <math/seadMathCalcCommon.h>

#include "Enemy/ActorJointLookController.hpp"
#include "Enemy/ActorStateSupportFreeze.hpp"
#include "Enemy/EnemyStateBlowDown.hpp"
#include "Enemy/EnemyStateUtil.hpp"
#include "Enemy/TargetFinder.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Item/ItemUtil.hpp"
#include "Library/Joint/JointControllerKeeper.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/LiveActor/Util/ActorModelUtil.hpp"
#include "Library/LiveActor/Util/ActorSceneInfo.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Project/AreaObj/AreaObj.hpp"
#include "Project/AreaObj/AreaObjUtil.hpp"
#include "Project/AreaObj/AreaShape.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ScoreUtil.hpp"

// Nerve that shares the execute function of another nerve.
#define SWIMMER_NERVE_SHARED_DECL(Action, ExeFunc)                                                 \
    class SwimmerNrv##Action : public al::Nerve {                                                  \
    public:                                                                                        \
        void execute(al::NerveKeeper* pKeeper) const override {                                    \
            (pKeeper->getParent<Swimmer>())->exe##ExeFunc();                                       \
        }                                                                                          \
    };

namespace {
NERVE_DECL(Swimmer, Move)
NERVE_DECL(Swimmer, BlowDown)
SWIMMER_NERVE_SHARED_DECL(BlowDownBobsled, BlowDown)
NERVE_DECL(Swimmer, SupportFreeze)
NERVE_DECL(Swimmer, PressDown)
NERVE_DECL(Swimmer, LockOn)
NERVE_DECL(Swimmer, Turn)
NERVE_DECL(Swimmer, Find)
NERVE_DECL(Swimmer, Chase)
NERVE_DECL(Swimmer, ChaseEnd)
// Non-const nerve objects: the game keeps them in .data in this order.
SwimmerNrvChase NrvSwimmerChase;
SwimmerNrvChaseEnd NrvSwimmerChaseEnd;
SwimmerNrvMove NrvSwimmerMove;
SwimmerNrvBlowDown NrvSwimmerBlowDown;
SwimmerNrvBlowDownBobsled NrvSwimmerBlowDownBobsled;
SwimmerNrvSupportFreeze NrvSwimmerSupportFreeze;
SwimmerNrvPressDown NrvSwimmerPressDown;
SwimmerNrvLockOn NrvSwimmerLockOn;
SwimmerNrvTurn NrvSwimmerTurn;
SwimmerNrvFind NrvSwimmerFind;

/** @brief Static tuning parameters shared by every Swimmer. */
struct SwimmerParam {
    /** @brief Builds the parameters, overriding the defaults of the state parameters. */
    SwimmerParam() {
        targetFinder._0 = 750.0f;
        targetFinder._4 = 120.0f;
        targetFinder._8 = 70.0f;
        targetFinder._10 = 750.0f;
        blowDownBobsled.mSpeed = 20.0f;
        blowDownBobsled.mJumpSpeed = 30.0f;
        supportFreeze.mIsStroke = true;
        supportFreeze.mStrokeEffectInterval = 15;
        supportFreeze.mIsAppearItem = true;
        supportFreeze.mStrokeFrame = 120;
        supportFreeze.mItemOffset.set(0.0f, 100.0f, 0.0f);
        jointLook.mRange.set(-20.0f, 20.0f);
    }

    sead::Vector3f areaSearchStep = {0.0f, -50.0f, 0.0f};
    sead::Vector3f areaSearchUp = {0.0f, 500.0f, 0.0f};
    sead::Vector3f areaCheckDown = {0.0f, -500.0f, 0.0f};
    s32 _24;  // never initialized nor used
    TargetFinderParam targetFinder;
    EnemyStateBlowDownParam blowDown{false};
    EnemyStateBlowDownParam blowDownBobsled{false};
    ActorStateSupportFreezeParam supportFreeze;
    ActorJointLookControllerParam jointLook;
};

SwimmerParam sParam;

/**
 * @brief Gets the absolute value of a float the way the game computes it.
 * @param value Value to take the absolute value of.
 * @return Absolute value.
 */
inline f32 absF(f32 value) {
    return value > 0.0f ? value : 0.0f - value;
}
}  // namespace

/**
 * @brief Constructs a Swimmer.
 * @param pName Actor name.
 */
Swimmer::Swimmer(const char* pName)
    : al::LiveActor(pName), mJointLookController(new ActorJointLookController(this, 2)) {}

/**
 * @brief Initializes the model, states, collider and eye look controller.
 * @param rInfo Placement info of the actor.
 */
void Swimmer::init(const al::ActorInitInfo& rInfo) {
    al::initActor(this, rInfo);
    al::initNerve(this, &NrvSwimmerMove, 3);
    al::startActionAtRandomFrame(this, "Move");
    mTargetFinder = new TargetFinder(this, &sParam.targetFinder);
    mStateBlowDown = new EnemyStateBlowDown(this, &sParam.blowDown);
    mStateBlowDownBobsled = new EnemyStateBlowDown(this, &sParam.blowDownBobsled);
    mStateSupportFreeze = new ActorStateSupportFreeze(this, &sParam.supportFreeze);
    al::initNerveState(this, mStateBlowDown, &NrvSwimmerBlowDown, "吹き飛び死亡");
    al::initNerveState(this, mStateBlowDownBobsled, &NrvSwimmerBlowDownBobsled,
                       "ボブスレー吹き飛び死亡");
    al::initNerveState(this, mStateSupportFreeze, &NrvSwimmerSupportFreeze, "DRCなでなで");
    al::createAndSetColliderSpecialPurpose(this, "MoveLimit");

    bool isOffCollide = false;
    if (al::tryGetArg(&isOffCollide, rInfo, "IsOffCollide") && isOffCollide) {
        al::offCollide(this);
    }

    al::tryGetArg(&mIsValidSlowdownArea, rInfo, "IsValidSlowdownArea");

    sead::Vector3f front = sead::Vector3f::ez;
    al::rotateVectorDegreeY(&front, al::getRandomDegree());
    al::setFront(this, front);

    al::initJointControllerKeeper(this, 2);
    mJointLookController->appendJoint("EyeL", sead::Vector3f::ey, &sParam.jointLook);
    mJointLookController->appendJoint("EyeR", sead::Vector3f::ey, &sParam.jointLook);
    makeActorAppeared();
}

/**
 * @brief Picks a new random point to swim to inside the territory, at the swimming depth.
 */
inline void Swimmer::resetTargetPos() {
    f32 offsetY = mSurfaceOffsetY;
    f32 randomX = al::getRandom();
    f32 sizeX = mTerritoryMax.x - mTerritoryMin.x;
    randomX *= al::getRandom(-sizeX * 0.5f, sizeX * 0.5f);
    f32 randomZ = al::getRandom();
    f32 sizeZ = mTerritoryMax.z - mTerritoryMin.z;
    randomZ *= al::getRandom(-sizeZ * 0.5f, sizeZ * 0.5f);
    sead::Vector3f center = getTerritoryCenter();
    mTargetPos.set(randomX + center.x, offsetY + center.y, randomZ + center.z);
}

/**
 * @brief Finds the water area below the actor and sets up the territory box from it. The actor is
 * killed when there is no water area.
 */
void Swimmer::initAfterPlacement() {
    for (s32 i = 1; i <= 10; i++) {
        mWaterArea = rc::tryFindAreaObj(this, rc::AreaObjType::WaterArea,
                                        al::getTrans(this) + sParam.areaSearchStep * i);
        if (mWaterArea != nullptr) {
            break;
        }
    }

    if (mWaterArea == nullptr ||
        !al::checkAreaObjCollisionByArrow(al::getTransPtr(this), nullptr, mWaterArea,
                                          al::getTrans(this),
                                          al::getTrans(this) + sParam.areaCheckDown)) {
        kill();
        return;
    }

    sead::Vector3f areaTrans = {0.0f, 0.0f, 0.0f};
    mWaterArea->getAreaShape()->calcTrans(&areaTrans);
    mSurfaceOffsetY = al::getTrans(this).y - areaTrans.y;

    const sead::Vector3f& rScale = mWaterArea->getAreaShape()->mScale;
    sead::Vector3f halfSize(absF(rScale.x * 1000.0f), 0.0f, absF(rScale.z * 1000.0f));
    halfSize *= 0.5f;
    mTerritoryMin = areaTrans - halfSize;
    mTerritoryMax = halfSize + areaTrans;
    resetTargetPos();
}

/**
 * @brief Kills the actor with the death hit reaction.
 */
void Swimmer::kill() {
    al::startHitReactionDeath(this);
    al::LiveActor::kill();
}

/**
 * @brief Updates the eye look controller.
 */
void Swimmer::control() {
    mJointLookController->update();
}

/**
 * @brief Attacks the player and pushes other enemies.
 * @param pSelf Own sensor.
 * @param pOther Other sensor.
 */
void Swimmer::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isNerve(this, &NrvSwimmerPressDown) || al::isNerve(this, &NrvSwimmerBlowDown) ||
        al::isNerve(this, &NrvSwimmerBlowDownBobsled)) {
        return;
    }

    if (al::isSensorEnemyAttack(pSelf) && al::isSensorPlayer(pOther)) {
        al::sendMsgPush(pOther, pSelf);
        al::sendMsgEnemyAttackForCrossoverSensor(pOther, pSelf);
    }

    if (al::isSensorEnemyBody(pSelf) && al::isSensorEnemyBody(pOther)) {
        al::sendMsgPushAndKillVelocityToTarget(this, pSelf, pOther);
    }
}

/**
 * @brief Handles pushes, tramples and blow-down attacks.
 * @param pMsg Received message.
 * @param pOther Sender sensor.
 * @param pSelf Own sensor.
 * @return Whether the message was handled.
 */
bool Swimmer::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) {
    if (al::isNerve(this, &NrvSwimmerPressDown) || al::isNerve(this, &NrvSwimmerBlowDown) ||
        al::isNerve(this, &NrvSwimmerBlowDownBobsled)) {
        return false;
    }

    if (!al::isSensorEnemyBody(pSelf)) {
        return false;
    }

    if (al::tryReceiveMsgPushAndAddVelocity(this, pMsg, pOther, pSelf, 0.8f)) {
        return true;
    }

    if (al::isMsgPlayerTrampleForCrossoverSensor(pMsg, pOther, pSelf) ||
        al::isMsgPlayerObjHipDropReflectAll(pMsg) || al::isMsgBallTrample(pMsg) ||
        rc::isMsgBobsledTrample(pMsg) || rc::isMsgTuccondorAttack(pMsg)) {
        if (!rc::isMsgBobsledTrample(pMsg)) {
            rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        }

        rc::addScoreCombo(this, pOther, pMsg, 100.0f);
        al::offCollide(this);
        rc::setAppearItemFactorByMsg(this, pMsg, pOther);
        al::setNerve(this, &NrvSwimmerPressDown);
        return !rc::isMsgBobsledTrample(pMsg);
    }

    if (rc::isMsgBobsledBodyAttack(pMsg)) {
        if (mActorSceneInfo->isSingleMode) {
            rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        }

        rc::addScoreCombo(this, pOther, pMsg, 100.0f);
        al::startHitReaction(this, "吹き飛びヒット[ボブスレー]");
        mStateBlowDownBobsled->setBlowDir(pOther, pSelf);
        al::setAppearItemFactor(this, "直接攻撃", pOther);
        al::setNerve(this, &NrvSwimmerBlowDownBobsled);
        return true;
    }

    if (EnemyStateUtil::tryRequestBlowDown(pMsg, pOther, pSelf, mStateBlowDown, true)) {
        rc::addScoreCombo(this, pOther, pMsg, 100.0f);
        al::setNerve(this, &NrvSwimmerBlowDown);
        return true;
    }

    return false;
}

/**
 * @brief Freezes the actor when it is touched on the touch screen.
 * @param pMsg Received message.
 * @param pPointer Screen pointer.
 * @param pTarget Own screen point target.
 * @return Whether the message was handled.
 */
bool Swimmer::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                    al::ScreenPointTarget* pTarget) {
    if (al::isNerve(this, &NrvSwimmerPressDown)) {
        return false;
    }

    if (al::isNerve(this, &NrvSwimmerBlowDown)) {
        return false;
    }

    if (al::isNerve(this, &NrvSwimmerBlowDownBobsled)) {
        return false;
    }

    if (!mStateSupportFreeze->receiveMsgScreenPoint(pMsg, pPointer, pTarget)) {
        return false;
    }

    if (!al::isNerve(this, &NrvSwimmerSupportFreeze)) {
        mResumeNerve = getNerveKeeper()->getCurrentNerve();
        mResumeStep = getNerveKeeper()->mNerveStep;
        al::setNerve(this, &NrvSwimmerSupportFreeze);
    }

    return true;
}

/**
 * @brief Swims towards the target point and starts locking on when a target is found nearby.
 */
void Swimmer::exeMove() {
    if (al::isFirstStep(this)) {
        al::tryStartActionIfNotPlaying(this, "Move");
        sead::Vector3f dir = mTargetPos - al::getTrans(this);
        al::verticalizeVec(&dir, al::getGravity(this), dir);
        al::setVelocityToDirection(this, dir, 0.1f);
    }

    updateMoveSurface();
    mTargetFinder->update();
    if (mTargetFinder->isExistTarget() && isInTerritory() &&
        al::isInRange(al::getTrans(this).y - mTargetFinder->getTargetPos().y, -250.0f, 250.0f)) {
        setSwimmerNerve(&NrvSwimmerLockOn);
        return;
    }

    sead::Vector3f dir = mTargetPos;
    dir -= al::getTrans(this);
    al::verticalizeVec(&dir, al::getGravity(this), dir);
    if (dir.length() < 5.0f || isGreaterEqualStep(600)) {
        setSwimmerNerve(&NrvSwimmerTurn);
        return;
    }

    al::turnDirectionToTargetDegree(this, al::getFrontPtr(this), mTargetPos, 3.0f);
    sead::Vector3f* pVelocity = al::getVelocityPtr(this);
    pVelocity->setScaleAdd(0.1f, al::getFront(this), *pVelocity);
    al::scaleVelocity(this, 0.9f);
}

/**
 * @brief Moves the actor and its territory along with the water area, keeping the swimming depth.
 */
void Swimmer::updateMoveSurface() {
    sead::Vector3f areaTrans = {0.0f, 0.0f, 0.0f};
    mWaterArea->getAreaShape()->calcTrans(&areaTrans);

    sead::Vector3f offset = areaTrans - getTerritoryCenter();
    *al::getTransPtr(this) += offset;
    mTargetPos += offset;

    sead::Vector3f halfSize = getTerritoryHalfSize();
    mTerritoryMin = areaTrans - halfSize;
    mTerritoryMax = areaTrans + halfSize;
    al::setTransY(this, getTerritoryCenter().y + mSurfaceOffsetY);
}

/**
 * @brief Checks whether the actor is horizontally inside its territory.
 * @return Whether the actor is inside the territory.
 */
bool Swimmer::isInTerritory() const {
    sead::Vector3f center = getTerritoryCenter();
    sead::Vector3f halfSize = getTerritoryHalfSize();
    if (mIsValidSlowdownArea) {
        halfSize.x -= 200.0f;
        halfSize.z -= 200.0f;
    }

    if (al::isInRange(al::getTrans(this).x, center.x - halfSize.x, center.x + halfSize.x) &&
        al::isInRange(al::getTrans(this).z, center.z - halfSize.z, center.z + halfSize.z)) {
        return true;
    }

    return false;
}

/**
 * @brief Changes the nerve and forgets the nerve to resume after a freeze.
 * @param pNerve Next nerve.
 */
void Swimmer::setSwimmerNerve(const al::Nerve* pNerve) {
    al::setNerve(this, pNerve);
    mResumeNerve = nullptr;
    mResumeStep = 0;
}

/**
 * @brief Checks the nerve step, counting the steps from before a freeze.
 * @param step Step to compare against.
 * @return Whether the step was reached.
 */
bool Swimmer::isGreaterEqualStep(s32 step) {
    return al::getNerveStep(this) + mResumeStep >= step;
}

/**
 * @brief Stops and turns towards the found target.
 */
void Swimmer::exeLockOn() {
    if (al::isFirstStep(this)) {
        al::setVelocityZero(this);
    }

    updateMoveSurface();
    if (!mTargetFinder->isExistTarget()) {
        setSwimmerNerve(&NrvSwimmerMove);
        return;
    }

    if (al::turnDirectionToTargetDegree(this, al::getFrontPtr(this),
                                        mTargetFinder->getTargetPos(), 3.0f)) {
        mChaseTarget = mTargetFinder->getTarget();
        setSwimmerNerve(&NrvSwimmerFind);
    }
}

/**
 * @brief Plays the find reaction before chasing.
 */
void Swimmer::exeFind() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Find");
        al::setVelocityZero(this);
    }

    updateMoveSurface();
    if (al::isActionEnd(this)) {
        setSwimmerNerve(&NrvSwimmerChase);
    }
}

/**
 * @brief Chases the target until it is lost, out of the territory or out of the water.
 */
void Swimmer::exeChase() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Chase");
        al::setVelocityToDirection(this, al::getFront(this), 0.8f);
        mLostTargetStep = 0;
        mWallHitStep = 0;
    }

    updateMoveSurface();
    mTargetFinder->update();
    mJointLookController->setLookTarget(al::getTrans(mChaseTarget));

    if (mTargetFinder->isExistTarget() &&
        al::isInRange(al::getTrans(this).y - mTargetFinder->getTargetPos().y, -250.0f, 250.0f)) {
        mLostTargetStep = 0;
    } else {
        mLostTargetStep++;
    }

    const sead::Vector3f& rTargetTrans = al::getTrans(mChaseTarget);
    if (rTargetTrans.y < al::getTrans(this).y + 50.0f) {
        mCloseBelowStep = 0;
    } else {
        sead::Vector3f dir = rTargetTrans - al::getTrans(this);
        al::verticalizeVec(&dir, al::getGravity(this), dir);
        if (dir.length() < 100.0f) {
            mCloseBelowStep++;
        } else {
            mCloseBelowStep = 0;
        }
    }

    if (al::isCollidedWall(this)) {
        mWallHitStep++;
    } else {
        mWallHitStep = 0;
    }

    if (isGreaterEqualStep(600) || mWallHitStep > 60 || !isInTerritory() ||
        mCloseBelowStep > 6 || mLostTargetStep > 300) {
        setSwimmerNerve(&NrvSwimmerChaseEnd);
        return;
    }

    al::turnDirectionToTargetDegree(this, al::getFrontPtr(this), al::getTrans(mChaseTarget), 1.0f);
    sead::Vector3f* pVelocity = al::getVelocityPtr(this);
    pVelocity->setScaleAdd(0.8f, al::getFront(this), *pVelocity);
    al::scaleVelocity(this, 0.9f);
}

/**
 * @brief Slows down after a chase and then turns back to the territory.
 */
void Swimmer::exeChaseEnd() {
    al::scaleVelocity(this, 0.9f);
    updateMoveSurface();
    if (isGreaterEqualStep(60)) {
        setSwimmerNerve(&NrvSwimmerTurn);
        al::startSe(this, "PgChaseEnd");
        mJointLookController->stopLook();
    }
}

/**
 * @brief Picks a new target point and turns towards it.
 */
void Swimmer::exeTurn() {
    if (al::isFirstStep(this)) {
        al::tryStartActionIfNotPlaying(this, "Turn");
        al::setVelocityZero(this);
        resetTargetPos();
    }

    updateMoveSurface();
    if (al::turnDirectionToTargetDegree(this, al::getFrontPtr(this), mTargetPos, 3.0f)) {
        setSwimmerNerve(&NrvSwimmerMove);
    }
}

/**
 * @brief Stays frozen and then resumes the nerve from before the freeze.
 */
void Swimmer::exeSupportFreeze() {
    al::updateNerveStateAndNextNerve(this, mResumeNerve);
}

/**
 * @brief Plays the squashed animation and dies.
 */
void Swimmer::exePressDown() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "PressDown");
        al::setVelocityZero(this);
        al::changeEnvTextureStamp(this);
        mJointLookController->stopLook();
    }

    rc::tryAppearItemPressDown(this, nullptr);
    if (al::isActionEnd(this)) {
        al::resetEnvTexture(this);
        kill();
    }
}

/**
 * @brief Gets blown away and dies, dropping an item.
 */
void Swimmer::exeBlowDown() {
    if (al::isFirstStep(this)) {
        mJointLookController->stopLook();
    }

    if (al::updateNerveState(this)) {
        al::appearItem(this);
        kill();
    }
}
