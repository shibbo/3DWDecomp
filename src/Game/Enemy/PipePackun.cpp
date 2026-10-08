#include "Enemy/PipePackun.hpp"

#include <math/seadQuat.h>
#include <math/seadVector.h>

#include "Enemy/ActorStateSupportFreeze.hpp"
#include "Enemy/PipePackunBody.hpp"
#include "Enemy/PipePackunWaterSurfaceModel.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Item/ItemUtil.hpp"
#include "Library/Joint/JointControllerKeeper.hpp"
#include "Library/Joint/JointSpringControllerHolder.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Thread/Functor.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ScoreUtil.hpp"

namespace {
NERVE_DECL(PipePackun, Move)
NERVE_DECL(PipePackun, SupportFreeze)
NERVE_DECL(PipePackun, Wait)
NERVE_DECL(PipePackun, Sleep)
NERVE_DECL(PipePackun, SleepStart)
NERVE_DECL(PipePackun, AttackSuccess)
NERVE_DECL(PipePackun, Trampled)
NERVE_DECL(PipePackun, Down)
// Non-const nerve objects: the game keeps them together in .data in this order.
PipePackunNrvMove NrvPipePackunMove;
PipePackunNrvSupportFreeze NrvPipePackunSupportFreeze;
PipePackunNrvWait NrvPipePackunWait;
PipePackunNrvSleep NrvPipePackunSleep;
PipePackunNrvSleepStart NrvPipePackunSleepStart;
PipePackunNrvAttackSuccess NrvPipePackunAttackSuccess;
PipePackunNrvTrampled NrvPipePackunTrampled;
PipePackunNrvDown NrvPipePackunDown;

ActorStateSupportFreezeParam sSupportFreezeParam(true, 15, false, true, 120,
                                                 sead::Vector3f(0.0f, 100.0f, 0.0f));

typedef al::FunctorV0M<PipePackun*, void (PipePackun::*)()> PipePackunFunctor;
}  // namespace

/**
 * @brief Constructs the Pipe Piranha Plant.
 * @param pName Actor name.
 */
PipePackun::PipePackun(const char* pName) : al::LiveActor(pName) {}

/** @brief Kills the plant and turns on its "dead" stage switch. */
void PipePackun::kill() {
    al::LiveActor::kill();
    al::onSwitchDeadOn(this);
}

/**
 * @brief Initializes the head, stem, hole and water surface models, nerves and switch listeners.
 * @param rInfo Actor placement and scene information.
 */
void PipePackun::init(const al::ActorInitInfo& rInfo) {
    al::tryGetArg(reinterpret_cast<s32*>(&mMoveType), rInfo, "MoveType");
    al::tryGetArg(&mIsDamageType2D, rInfo, "IsDamageType2D");

    if (rInfo.getActorSceneInfo().isSingleMode) {
        al::initActorWithArchiveName(this, rInfo, "PipePackunHeadFur", nullptr);
    } else {
        al::initActorWithArchiveName(this, rInfo, "PipePackunHead", nullptr);
    }

    al::initNerve(this, &NrvPipePackunMove, 1);
    mStateSupportFreeze = new ActorStateSupportFreeze(this, &sSupportFreezeParam);
    al::initNerveState(this, mStateSupportFreeze, &NrvPipePackunSupportFreeze, "[state]DRC拘束");

    mBody = new PipePackunBody(this, "パイプパックン胴体");
    al::initCreateActorWithPlacementInfo(mBody, rInfo);
    al::initJointControllerKeeper(this, 8);
    al::initJointLocalZRotator(this, &mLeafDegree, "LeafRootL");
    al::initJointLocalZRotator(this, &mLeafDegree, "LeafRootR");
    mJointSpringControllerHolder = new al::JointSpringControllerHolder();
    mJointSpringControllerHolder->init(this, "JointSpringControllerInfo");

    f32 clippingRadius = 0.0f;
    sead::Vector3f headDir;
    mBody->calcPosAndDir(&mClippingCenter, &headDir, mBody->getTotalLength());
    mBody->calcClippingSphere(&mClippingCenter, &clippingRadius, headDir * 80.0f + mClippingCenter,
                              120.0f);
    al::setClippingInfo(this, clippingRadius, &mClippingCenter);

    mMinCoord = 0.0f;
    al::tryGetArg(&mMinCoord, rInfo, "MinCoord");

    if (mMinCoord > mBody->getTotalLength()) {
        mMinCoord = mBody->getTotalLength();
    }

    f32 startPositionRate = 0.0f;
    al::tryGetArg(&startPositionRate, rInfo, "StartPositionRate");
    mCoord = al::lerpValue(startPositionRate, mMinCoord, mBody->getTotalLength());
    mBody->setCoord(mCoord);

    sead::Vector3f front = sead::Vector3f::ez;
    mBody->calcPosAndDir(al::getTransPtr(this), &front, mCoord);

    if (al::isParallelDirection(front, sead::Vector3f::ey, 0.01f)) {
        sead::Vector3f side;
        al::calcSideDir(&side, this);
        al::makeQuatFrontSide(al::getQuatPtr(this), front, side);
    } else {
        al::makeQuatFrontUp(al::getQuatPtr(this), front, mUpDir);
    }

    bool isUpSideDown = false;
    al::tryGetArg(&isUpSideDown, rInfo, "IsUpSideDown");

    if (isUpSideDown) {
        sead::Quatf* quat = al::getQuatPtr(this);
        al::rotateQuatZDirDegree(quat, *quat, 180.0f);
    }

    const char* holeModelName = nullptr;

    if (al::tryGetStringArg(&holeModelName, rInfo, "HoleModelName")) {
        mHoleModel = new al::LiveActor("穴モデル");
        al::initActorWithArchiveName(mHoleModel, rInfo, holeModelName, nullptr);
        f32 holeCoord = 0.0f;
        al::tryGetArg(&holeCoord, rInfo, "HoleModelPosCoord");
        // Virtual slot 0x130 (getName in the current LiveActor declaration); result unused.
        mHoleModel->getName();
        sead::Vector3f holeUp = sead::Vector3f::ey;
        mBody->calcPosAndDir(al::getTransPtr(mHoleModel), &holeUp, holeCoord);
        sead::Quatf holeQuat = sead::Quatf::unit;
        al::makeQuatUpNoSupport(&holeQuat, holeUp);
        al::rotateQuatYDirDegree(&holeQuat, holeQuat, al::getRandomDegree());
        al::updatePoseQuat(mHoleModel, holeQuat);
        mHoleModel->makeActorAppeared();
    }

    if (al::calcLinkChildNum(rInfo, "WaterSurfacePoint") != 0) {
        mWaterSurfaceModel = new PipePackunWaterSurfaceModel("パイプパックン水面モデル");
        al::initLinksActor(mWaterSurfaceModel, rInfo, "WaterSurfacePoint", 0);
        mWaterSurfaceModel->setSurfaceCoord(
            mBody->calcNearCoord(al::getTrans(mWaterSurfaceModel)));
        mWaterSurfaceModel->setCoord(mCoord);
    }

    al::tryGetArg(&mMoveSpeed, rInfo, "MoveSpeed");
    al::startAction(this, "Wait");

    if (al::listenStageSwitchOnStart(this, PipePackunFunctor(this, &PipePackun::start))) {
        al::setNerve(this, &NrvPipePackunWait);
    }

    if (startPositionRate >= 1.0f && mMoveType == MoveType::Extend) {
        al::setNerve(this, &NrvPipePackunSleep);
    }

    al::listenStageSwitchOnKill(this, PipePackunFunctor(this, &PipePackun::killSwitch));
    makeActorAppeared();
}

/** @brief Starts moving when the start switch turns on. */
void PipePackun::start() {
    if (al::isNerve(this, &NrvPipePackunWait)) {
        al::setNerve(this, &NrvPipePackunMove);
    }
}

/** @brief Disappears together with the stem and water surface when the kill switch turns on. */
void PipePackun::killSwitch() {
    al::startHitReactionDisappear(this);
    mBody->killSwitch();

    if (mWaterSurfaceModel != nullptr) {
        mWaterSurfaceModel->kill();
    }

    kill();
}

/**
 * @brief Bites players and pushes shells, keys and map objects away.
 * @param pSelf Sensor of the plant.
 * @param pOther Sensor of the other actor.
 */
void PipePackun::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isSensorEnemyAttack(pSelf) && isEnableAttack()) {
        sead::Vector3f dir;
        al::calcDirBetweenSensors(&dir, pSelf, pOther);
        rc::sendMsgItemReflect(pOther, pSelf, dir);

        if (al::isSensorRide(pOther)) {
            al::sendMsgPush(pOther, pSelf);
        }

        if (al::sendMsgEnemyAttack(pOther, pSelf)) {
            if (al::isNerve(this, &NrvPipePackunSleep) ||
                al::isNerve(this, &NrvPipePackunSleepStart)) {
                mNextNerve = &NrvPipePackunSleepStart;
            } else if (al::isNerve(this, &NrvPipePackunWait)) {
                mNextNerve = &NrvPipePackunWait;
            } else {
                mNextNerve = &NrvPipePackunMove;
            }

            al::setNerve(this, &NrvPipePackunAttackSuccess);
            return;
        }
    }

    if (al::isSingleMode(this)) {
        if (al::isSensorKickKoura(pOther) || al::isSensorDoorKey(pOther)) {
            al::sendMsgPush(pOther, pSelf);
        } else if (al::isSensorMapObj(pOther)) {
            al::sendMsgPushStrong(pOther, pSelf);
        }
    }
}

/**
 * @brief Checks whether the plant can currently bite.
 * @return Whether the plant is in a normal state or recovering from a trample.
 */
bool PipePackun::isEnableAttack() const {
    if (al::isNerve(this, &NrvPipePackunWait) || al::isNerve(this, &NrvPipePackunSleepStart) ||
        al::isNerve(this, &NrvPipePackunSleep) || al::isNerve(this, &NrvPipePackunSupportFreeze) ||
        al::isNerve(this, &NrvPipePackunMove)) {
        return true;
    }

    if (al::isNerve(this, &NrvPipePackunTrampled)) {
        return al::isGreaterEqualStep(this, 15);
    }

    return false;
}

/**
 * @brief Handles tramples, attacks and invincible attacks.
 * @param pMsg Received message.
 * @param pOther Sensor of the sender.
 * @param pSelf Sensor of the plant.
 * @return Whether the message was handled.
 */
bool PipePackun::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                            al::HitSensor* pSelf) {
    if (al::isMsgPlayerTrample(pMsg) || al::isMsgPlayerObjHipDropReflectAll(pMsg) ||
        al::isMsgPlayerCooperationHipDrop(pMsg) || al::isMsgBallAttack(pMsg) ||
        al::isMsgPlayerTailAttack(pMsg) || al::isMsgPlayerFireBallAttack(pMsg) ||
        al::isMsgPlayerClimbAttack(pMsg) || al::isMsgPlayerSpinAttack(pMsg) ||
        al::isMsgPlayerBoomerangReflect(pMsg) || al::isMsgPlayerBodyLanding(pMsg) ||
        al::isMsgPlayerBodyAttackReflect(pMsg) || al::isMsgExplosion(pMsg) ||
        (al::isSingleMode(this) &&
         (al::isMsgKeyThrow(pMsg) || al::isMsgKickKouraAttack(pMsg) ||
          al::isMsgLaserAttack(pMsg) || al::isMsgEnemyAttackFire(pMsg) ||
          rc::isMsgBobsledBodyAttack(pMsg)))) {
        if (al::isMsgPlayerTrample(pMsg) && al::getActorVelocity(pOther).y > 0.0f) {
            return false;
        }

        if (al::isSensorEnemyBody(pSelf)) {
            if (isEnableTrample(pMsg)) {
                rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);

                if (al::isMsgPlayerTailAttack(pMsg) || al::isMsgPlayerClimbAttack(pMsg) ||
                    al::isMsgPlayerSpinAttack(pMsg)) {
                    mInvincibleTimer = 25;
                } else {
                    mInvincibleTimer =
                        al::isMsgExplosion(pMsg) || al::isMsgPlayerCooperationHipDrop(pMsg) ? 25 :
                                                                                              5;
                }

                if (al::isMsgPlayerCooperationHipDrop(pMsg) ||
                    al::isMsgPlayerObjHipDropReflectAll(pMsg) ||
                    al::isMsgPlayerBodyAttackReflect(pMsg) || al::isMsgExplosion(pMsg)) {
                    mTrampleDepth = 1000.0f;
                    mTrampleFrame = 60;
                    al::startSe(this, "PgTrampledStrong");
                } else {
                    mTrampleDepth = 400.0f;
                    mTrampleFrame = 35;
                    al::startSe(this, "PgTrampled");
                }

                mIsTrampledBySensor = true;
                mTrampleSensor = pOther;
                al::setNerve(this, &NrvPipePackunTrampled);
                return true;
            }
        } else if (al::isSingleMode(this) && rc::isMsgBobsledBodyAttack(pMsg) &&
                   isEnableTrample(pMsg)) {
            mInvincibleTimer = 40;
            mTrampleDepth = 1000.0f;
            mTrampleFrame = 60;
            al::startSe(this, "PgTrampledStrong");
            mIsTrampledBySensor = true;
            mTrampleSensor = pOther;
            al::setNerve(this, &NrvPipePackunTrampled);
            return true;
        }
    }

    if (al::isMsgPlayerInvincibleAttack(pMsg)) {
        rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        hitInvincibleAttack(pMsg, pOther);
        return true;
    }

    return false;
}

/**
 * @brief Checks whether the plant can be trampled.
 * @param pMsg Trample message (unused).
 * @return Whether the plant is in a normal state or no longer invincible after a trample.
 */
bool PipePackun::isEnableTrample(const al::SensorMsg* pMsg) const {
    if (al::isNerve(this, &NrvPipePackunWait) || al::isNerve(this, &NrvPipePackunSleepStart) ||
        al::isNerve(this, &NrvPipePackunSleep) || al::isNerve(this, &NrvPipePackunMove) ||
        al::isNerve(this, &NrvPipePackunSupportFreeze)) {
        return true;
    }

    if (al::isNerve(this, &NrvPipePackunTrampled)) {
        return mInvincibleTimer <= 0;
    }

    return false;
}

/**
 * @brief Disappears with an item when hit by an invincible player.
 * @param pMsg Received message.
 * @param pOther Sensor of the attacker.
 */
void PipePackun::hitInvincibleAttack(const al::SensorMsg* pMsg, al::HitSensor* pOther) {
    rc::addScoreCombo(this, pOther, pMsg, 0.0f);
    al::startHitReactionDisappear(this);
    sead::Vector3f pos;
    sead::Vector3f dir;
    mBody->calcPosAndDir(&pos, &dir, mCoord);
    al::appearItemTiming(this, "死亡", pos, dir * 2.0f, pOther, false);
    mBody->killSwitch();

    if (mWaterSurfaceModel != nullptr) {
        mWaterSurfaceModel->kill();
    }

    kill();
}

/**
 * @brief Handles being touched (trampled) or frozen from the touch screen.
 * @param pMsg Received message.
 * @param pPointer Screen pointer that sent the message.
 * @param pTarget Screen point target of the plant.
 * @return Whether the message was handled.
 */
bool PipePackun::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                       al::ScreenPointTarget* pTarget) {
    if (al::isMsgTouchAssistTrig(pMsg) && isEnableTrample(pMsg)) {
        mTrampleDepth = 400.0f;
        mTrampleFrame = 35;
        mTrampleSensor = nullptr;
        mIsTrampledBySensor = false;
        al::startSe(this, "PgTrampled");
        al::setNerve(this, &NrvPipePackunTrampled);
        return true;
    }

    if (!isEnableSupportFreeze()) {
        return false;
    }

    if (!mStateSupportFreeze->receiveMsgScreenPoint(pMsg, pPointer, pTarget)) {
        return false;
    }

    if (al::isNerve(this, &NrvPipePackunSupportFreeze)) {
        return true;
    }

    if (al::isNerve(this, &NrvPipePackunSleep) || al::isNerve(this, &NrvPipePackunSleepStart)) {
        mNextNerve = &NrvPipePackunSleepStart;
    } else if (al::isNerve(this, &NrvPipePackunWait)) {
        mNextNerve = &NrvPipePackunWait;
    } else {
        mNextNerve = &NrvPipePackunMove;
    }

    al::setNerve(this, &NrvPipePackunSupportFreeze);
    return true;
}

/**
 * @brief Checks whether the plant can be frozen from the touch screen.
 * @return Whether the plant is neither trampled nor down.
 */
bool PipePackun::isEnableSupportFreeze() const {
    if (al::isNerve(this, &NrvPipePackunTrampled) || al::isNerve(this, &NrvPipePackunDown)) {
        return false;
    }

    return true;
}

/** @brief Updates the stem, leaves, water state and head orientation. */
void PipePackun::control() {
    mBody->setCoord(mCoord);
    f32 springRate = al::lerpValue(mCoord - mMinCoord, 150.0f, 50.0f, 1.0f, 0.0f);
    mJointSpringControllerHolder->setControlRateAll(springRate);
    mLeafDegree = al::lerpValue(springRate, -30.0f, 0.0f);

    if (mInvincibleTimer > 0) {
        mInvincibleTimer--;
    }

    if (mWaterSurfaceModel != nullptr) {
        mWaterSurfaceModel->setCoord(mCoord);
    }

    sead::Vector3f front = sead::Vector3f::ez;
    mBody->calcPosAndDir(al::getTransPtr(this), &front, mCoord);
    sead::Vector3f mouthPos;
    al::multVecPose(&mouthPos, this, sead::Vector3f(0.0f, 0.0f, 80.0f));

    if (rc::isInWaterArea(this, mouthPos)) {
        if (!mIsInWater) {
            al::startHitReaction(this, "水に入った");
        }

        mIsInWater = true;
        al::setMaterialCode(this, "InWater");
    } else {
        if (mIsInWater) {
            al::startHitReaction(this, "水から出た");
        }

        mIsInWater = false;
        al::setMaterialCode(this, "");
    }

    al::updateSeMaterialWater(this, mIsInWater);

    if (!al::isParallelDirection(front, sead::Vector3f::ey, 0.01f)) {
        sead::Vector3f up;
        al::calcUpDir(&up, this);
        sead::Quatf* quat = al::getQuatPtr(this);
        sead::Vector3f targetUp = mUpDir.dot(up) >= 0.0f ? mUpDir : -mUpDir;
        al::turnQuatYDirRadian(quat, *quat, targetUp, sead::Mathf::deg2rad(10.0f));
    }

    sead::Quatf* quat = al::getQuatPtr(this);
    al::turnQuatZDirRate(quat, *quat, front, 1.0f);
}

/** @brief Waits for the start switch. */
void PipePackun::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait");
    }
}

/** @brief Plays the fall-asleep animation, then sleeps. */
void PipePackun::exeSleepStart() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "SleepStart");
    }

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvPipePackunSleep);
    }
}

/** @brief Sleeps at the end of the stem. */
void PipePackun::exeSleep() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Sleep");
    }
}

/** @brief Moves the head along the stem, turning around or falling asleep at its ends. */
void PipePackun::exeMove() {
    // The game queries the first step here but does nothing with the result.
    al::isFirstStep(this);
    sead::Vector3f front;
    al::calcFrontDir(&front, this);
    if (front.y > 0.707f) {
        al::tryStartActionIfNotPlaying(this, "MoveUpper");
    } else {
        al::tryStartActionIfNotPlaying(this, "Move");
    }

    f32 totalLength = mBody->getTotalLength();
    f32 speed = al::calcNerveEaseInOutValue(this, 40, 0.0f, mMoveSpeed);

    if (speed >= 1.0f) {
        al::holdSeWithParam(this, "PgMove", speed, nullptr);
    }

    f32 delta;

    if (mIsMoveBack) {
        delta = -(speed * al::lerpValue(mCoord, mMinCoord, 200.0f, 0.1f, 1.0f));
    } else {
        delta = speed * al::lerpValue(mCoord, totalLength - 200.0f, totalLength, 1.0f, 0.1f);
    }

    mCoord += delta;

    bool isTurn = false;

    if (mCoord < mMinCoord) {
        mCoord = mMinCoord;

        if (mMoveType == MoveType::Reciprocate) {
            if (mIsMoveBack) {
                mIsMoveBack = false;
            }

            isTurn = true;
        }
    }

    if (mCoord > totalLength) {
        mCoord = totalLength;

        if (mMoveType == MoveType::Reciprocate) {
            if (!mIsMoveBack) {
                mIsMoveBack = true;
            }

            isTurn = true;
        } else {
            al::setNerve(this, &NrvPipePackunSleepStart);
        }
    }

    if (isTurn) {
        al::setNerve(this, &NrvPipePackunMove);
    }
}

/** @brief Gets pushed down into the pipe, then slowly comes back out (or goes down). */
void PipePackun::exeTrampled() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Trampled");
        mTrampleStartCoord = mCoord;
    }

    if (!al::isActionPlaying(this, "TrampledLoop") && al::isActionEnd(this)) {
        al::startAction(this, "TrampledLoop");
    }

    f32 coord = al::calcNerveEaseOutValue(this, mTrampleFrame, mTrampleStartCoord,
                                          mTrampleStartCoord - mTrampleDepth);
    mCoord = coord;

    if (coord < mMinCoord) {
        mCoord = mMinCoord;

        if (mIsTrampledBySensor && mTrampleStartCoord - mTrampleDepth <= mMinCoord + -50.0f) {
            al::setNerve(this, &NrvPipePackunDown);
            return;
        }
    } else if (coord > mTrampleStartCoord - mTrampleDepth + 30.0f) {
        al::holdSeWithParam(this, "PgBack", mTrampleDepth, nullptr);
    }

    if (al::isGreaterStep(this, mTrampleFrame + 75)) {
        mTrampleSensor = nullptr;
        al::setNerve(this, &NrvPipePackunMove);
    }
}

/** @brief Plays the bite animation, then returns to the stored nerve. */
void PipePackun::exeAttackSuccess() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "AttackSuccess");
        mTrampleStartCoord = mCoord;
    }

    if (al::isActionEnd(this)) {
        al::setNerve(this, mNextNerve);
        mNextNerve = nullptr;
    }
}

/** @brief Stays frozen by the touch screen, then returns to the stored nerve. */
void PipePackun::exeSupportFreeze() {
    if (al::isFirstStep(this)) {
        mTrampleStartCoord = mCoord;
    }

    if (al::updateNerveState(this)) {
        al::setNerve(this, mNextNerve);
        mNextNerve = nullptr;
    }
}

/** @brief Plays the defeat animation, then dies with an item. */
void PipePackun::exeDown() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Down");
        al::changeEnvTextureStamp(this);

        if (mTrampleSensor != nullptr) {
            rc::addScore(this, mTrampleSensor, 0.0f, 0);
        }
    }

    if (al::isActionEnd(this)) {
        sead::Vector3f front;
        al::calcFrontDir(&front, this);
        al::startHitReactionDeath(this);
        al::appearItemTiming(this, "死亡", al::getTrans(this) + front * 75.0f, front * 2.0f,
                             mTrampleSensor, false);

        if (mWaterSurfaceModel != nullptr) {
            mWaterSurfaceModel->kill();
        }

        mBody->kill();
        al::resetEnvTexture(this);
        kill();
    }
}
