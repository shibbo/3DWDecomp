#include "Boss/TentackHead.hpp"

#include "Boss/TentackBase.hpp"
#include "Boss/TentackHill.hpp"
#include "Boss/TentackRockFaller.hpp"
#include "Enemy/ActorJointLookController.hpp"
#include "Enemy/EnemyStateUtil.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Joint/JointControllerKeeper.hpp"
#include "Library/Joint/JointSpringController.hpp"
#include "Library/Light/PrePassLightFunction.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Movement/RumbleCalculator.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Obj/PartsFunction.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ScoreUtil.hpp"

// Nerve that runs the execute function of another nerve.
#define TENTACK_HEAD_NERVE_SHARED_DECL(Action, ExeFunc)                                            \
    class TentackHeadNrv##Action : public al::Nerve {                                              \
    public:                                                                                        \
        void execute(al::NerveKeeper* pKeeper) const override {                                    \
            (pKeeper->getParent<TentackHead>())->exe##ExeFunc();                                   \
        }                                                                                          \
    };

// Non-const nerve object: these nerves are merged into one data block.
#define TENTACK_HEAD_NERVE_MAKE(Class, Action) Class##Nrv##Action Nrv##Class##Action;

namespace {
NERVE_DECL(TentackHead, DemoStart)
NERVES_MAKE_NOSTRUCT(TentackHead, DemoStart)
NERVE_DECL(TentackHead, DemoEnd)
NERVE_DECL(TentackHead, Push)
NERVE_DECL(TentackHead, Disappear)
NERVE_DECL(TentackHead, Damage)
TENTACK_HEAD_NERVE_SHARED_DECL(WaitFixed, Wait)
NERVE_DECL(TentackHead, Wait)
NERVE_DECL(TentackHead, Back)
NERVE_DECL(TentackHead, BackWait)
NERVE_DECL(TentackHead, PushSign)
TENTACK_HEAD_NERVE_SHARED_DECL(EatAndDisappear, Eat)
NERVE_DECL(TentackHead, AttackRockStart)
NERVE_DECL(TentackHead, AttackTentacleStart)
NERVE_DECL(TentackHead, Eat)
NERVE_DECL(TentackHead, Shot)
NERVE_DECL(TentackHead, Cry)
FOR_EACH(TENTACK_HEAD_NERVE_MAKE, TentackHead, DemoEnd, Push, Disappear, Damage, WaitFixed, Wait,
         Back, BackWait, PushSign, EatAndDisappear, AttackRockStart, AttackTentacleStart, Eat,
         Shot, Cry)

/** @brief Name of the appearance spotlight. */
const char* const cLightName = "登場スポットライト";
}  // namespace

/**
 * @brief Creates the head and its floor model, rock faller, trample rumble and look controller.
 * @param pName Actor name.
 * @param pHost Boss the head reports to.
 * @param isSubHead Whether the head is one of the two heads of the third battle.
 */
TentackHead::TentackHead(const char* pName, TentackBase* pHost, bool isSubHead)
    : al::LiveActor(pName), mHost(pHost), mHill(new TentackHill("テンタック本体の足元モデル")),
      mRockFaller(new TentackRockFaller("テンタック岩降らし[本体用]", this)), mDamage(0),
      mFrontDir(sead::Vector3f::ez), mLightMtx(sead::Matrix34f::ident), mTurnSpeed(0.0f),
      mLookPlayer(nullptr), mLookChangeStep(0),
      mRumble(new al::RumbleCalculatorCosMultLinear(2.5f, 2.0f, 0.05f, 20)), mRumbleStep(-1),
      mPushSensors(new al::HitSensor*[3]), mIsReappearRockFaller(false),
      mJointLook(new ActorJointLookController(this, 1)), mIsSubHead(isSubHead) {}

/**
 * @brief Loads the head model, its floor model and rock faller, sensors and joint controllers.
 * @param rInfo Actor placement and scene initialization information.
 */
void TentackHead::init(const al::ActorInitInfo& rInfo) {
    const char* suffix = nullptr;
    if (al::isObjectName(rInfo, "Tentack")) {
        suffix = mIsSubHead ? "Lv3" : nullptr;
    }

    const char* archiveName =
        al::isObjectName(rInfo, "TentackLv2") ? "TentackHeadLv2" : "TentackHead";
    al::initActorWithArchiveName(this, rInfo, archiveName, suffix);
    al::initNerve(this, &NrvTentackHeadDemoStart, 0);
    al::calcFrontDir(&mFrontDir, this);
    al::hideModel(this);
    mHill->initActorWithModelName(rInfo, "TentackHeadHill", nullptr);
    mRockFaller->init(rInfo);
    al::initPrePassLightMtxConnector(this, cLightName, &mLightMtx);
    al::killPrePassLight(this, cLightName, -1);
    mPushSensors[0] = al::getHitSensor(this, "BodySpine1");
    mPushSensors[1] = al::getHitSensor(this, "BodySpine2");
    mPushSensors[2] = al::getHitSensor(this, "BodySpine3");
    al::initJointControllerKeeper(this, 3);

    auto* lookParam = new ActorJointLookControllerParam(
        2.0f, sead::Vector2f(-20.0f, 20.0f), true, al::getJointMtxPtr(this, "Spine4"), nullptr);
    mJointLook->appendJoint("Spine4", sead::Vector3f::ez, lookParam);

    al::JointSpringController* spring = al::initJointSpringController(this, "Cloth1");
    spring->setFriction(0.85f);
    spring->setLimitDegree(10.0f);
    spring = al::initJointSpringController(this, "Cloth2");
    spring->setFriction(0.85f);
    spring->setLimitDegree(10.0f);
    makeActorDead();
}

/** @brief Appears and starts the battle entrance demo. */
void TentackHead::appear() {
    al::LiveActor::appear();
    al::setNerve(this, &NrvTentackHeadDemoStart);
}

/** @brief Kills the head together with its floor model and rock faller. */
void TentackHead::kill() {
    al::LiveActor::kill();
    if (al::isAlive(mHill)) {
        mHill->kill();
    }

    if (al::isAlive(mRockFaller)) {
        mRockFaller->kill();
    }
}

/** @brief Updates the look controller and the squash animation played when trampled. */
void TentackHead::control() {
    mJointLook->update();
    if (mRumbleStep < 0) {
        return;
    }

    if (mRumbleStep == 0) {
        mRumble->start(0);
        al::startSe(this, "PgTrampled");
    }

    if (mRumbleStep >= 20) {
        al::setScaleY(this, 1.0f);
        mRumbleStep = -1;
        return;
    }

    mRumble->calc();
    al::setScaleY(this, mRumble->getValueY() + 1.0f);
    mRumbleStep++;
}

/**
 * @brief Breaks magma balls touching the body and pushes or attacks players.
 * @param pSelf Head sensor.
 * @param pOther Contacted sensor.
 */
void TentackHead::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isSensorEnemyBody(pSelf) || isPushSensor(pSelf)) {
        if (rc::sendMsgTentackMagmaBallBreak(pOther, pSelf)) {
            return;
        }
    }

    if (!al::isSensorEnemyAttack(pSelf)) {
        return;
    }

    if (isPushSensor(pSelf)) {
        al::sendMsgPushStrong(pOther, pSelf);
        return;
    }

    if (al::isNerve(this, &NrvTentackHeadDemoEnd) || !al::isSensorPlayer(pOther)) {
        return;
    }

    if (al::isSensorName(pSelf, "Attack")) {
        if (!al::isNerve(this, &NrvTentackHeadPush)) {
            return;
        }

        al::sendMsgEnemyAttackForCrossoverSensor(pOther, pSelf);
        al::sendMsgPush(pOther, pSelf);
    }

    if (isEnableAttack()) {
        al::sendMsgEnemyAttackForCrossoverSensor(pOther, pSelf);
        al::sendMsgPush(pOther, pSelf);
    }
}

/**
 * @brief Checks whether a sensor is one of the body push sensors.
 * @param pSensor Sensor to check.
 * @return True if the sensor is BodySpine1, BodySpine2 or BodySpine3.
 */
bool TentackHead::isPushSensor(const al::HitSensor* pSensor) const {
    for (s32 i = 0; i < 3; i++) {
        if (mPushSensors[i] == pSensor) {
            return true;
        }
    }

    return false;
}

/**
 * @brief Checks whether the head is in a state where its body hurts players.
 * @return True late in the push action or while waiting or attacking.
 */
bool TentackHead::isEnableAttack() const {
    if (al::isActionPlaying(this, "Push")) {
        return al::getActionFrame(this) > 70.0f;
    }

    return al::isNerve(this, &NrvTentackHeadWait) || al::isNerve(this, &NrvTentackHeadWaitFixed) ||
           al::isNerve(this, &NrvTentackHeadEat) ||
           al::isNerve(this, &NrvTentackHeadEatAndDisappear) ||
           al::isNerve(this, &NrvTentackHeadShot) ||
           al::isNerve(this, &NrvTentackHeadAttackRockStart) ||
           al::isNerve(this, &NrvTentackHeadAttackTentacleStart) ||
           al::isNerve(this, &NrvTentackHeadCry);
}

/**
 * @brief Reflects player projectiles and takes damage from stomps.
 * @param pMsg Incoming sensor message.
 * @param pOther Sending sensor.
 * @param pSelf Head sensor.
 * @return True if the message was handled.
 */
bool TentackHead::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                             al::HitSensor* pSelf) {
    if (al::isSensorEnemyBody(pSelf) || isPushSensor(pSelf)) {
        if (al::isMsgPlayerFireBallAttack(pMsg) || al::isMsgPlayerBoomerangReflect(pMsg)) {
            rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
            return true;
        }
    }

    if (!al::isSensorEnemyBody(pSelf)) {
        return false;
    }

    if (!EnemyStateUtil::isMsgPressDownForCrossoverSensor(pMsg, pOther, pSelf) &&
        !al::isMsgPlayerObjHipDropReflectAll(pMsg) && !al::isMsgPlayerBodyAttackReflect(pMsg)) {
        return false;
    }

    if (al::isNerve(this, &NrvTentackHeadDamage) || al::isNerve(this, &NrvTentackHeadDemoEnd) ||
        al::isNerve(this, &NrvTentackHeadDisappear) || al::isNerve(this, &NrvTentackHeadBack)) {
        mRumbleStep = 0;
        return true;
    }

    if (!isEnableAttack() &&
        !(al::isNerve(this, &NrvTentackHeadDisappear) && al::isLessEqualStep(this, 10))) {
        return false;
    }

    if (al::isMsgPlayerBodyAttackReflect(pMsg) &&
        al::getSensorPos(pOther).y < al::getSensorPos(pSelf).y + 100.0f) {
        return false;
    }

    rc::addScore(this, pOther, 100.0f, mDamage);
    mRockFaller->kill();
    if (mDamage >= 2) {
        mHost->receiveDamage(this, true);
        al::setNerve(this, &NrvTentackHeadDemoEnd);
        return true;
    }

    mDamage++;
    mHost->receiveDamage(this, false);
    al::setNerve(this, &NrvTentackHeadDamage);
    return true;
}

/**
 * @brief Accepts touch assist from the screen pointer.
 * @param pMsg Incoming screen point message.
 * @param pPointer Pointer that sent the message.
 * @param pTarget Target that was pointed at.
 * @return True for touch assist messages.
 */
bool TentackHead::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                        al::ScreenPointTarget* pTarget) {
    return al::isMsgTouchAssistAll(pMsg);
}

/** @brief Rises out of the floor under a spotlight at the start of the battle. */
void TentackHead::exeDemoStart() {
    if (al::isStep(this, 90)) {
        al::startAction(this, "DemoBattleStart");
        mHill->appear();

        const sead::Vector3f& trans = al::getTrans(this);
        mLightMtx.makeT(trans.x, trans.y + 2300.0f, trans.z);
        al::appearPrePassLight(this, cLightName, 0);
    }

    if (al::isStep(this, 150)) {
        al::appearBreakModelRandomRotateY(al::getSubActor(this, "攻撃岩出現モデル"));
    }

    if (al::isStep(this, 91)) {
        al::showModelIfHide(this);
    }

    if (al::isGreaterStep(this, 90) && al::isActionEnd(this)) {
        al::setNerve(this, &NrvTentackHeadWaitFixed);
    }
}

/** @brief Waits, turning to the nearest player unless the facing direction is fixed. */
void TentackHead::exeWait() {
    if (al::isFirstStep(this)) {
        al::tryStartActionIfNotPlaying(this, "Wait");
        mHill->trySetWaitIfNotPlaying();
    }

    if (al::isNerve(this, &NrvTentackHeadWait)) {
        updateLookPlayer();
        if (mLookPlayer != nullptr) {
            turnToTargetGently(al::getTrans(mLookPlayer), 0.75f);
        }
    }
}

/** @brief Switches the look target to the nearest active player, restarting the blend. */
void TentackHead::updateLookPlayer() {
    al::LiveActor* player =
        rc::calcActivePlayerNum(this) >= 1 ? rc::findNearestActivePlayerActor(this) : nullptr;
    if (player != mLookPlayer) {
        mLookPlayer = player;
        mLookChangeStep = 15;
    }
}

/**
 * @brief Turns a step toward a target position on the horizontal plane.
 * @param rTarget Position to face.
 * @param speed Maximum turn per frame in degrees.
 * @return True once the head faces the target.
 */
bool TentackHead::turnToTargetGently(const sead::Vector3f& rTarget, f32 speed) {
    const sead::Vector3f& trans = al::getTrans(this);
    f32 dx = rTarget.x - trans.x;
    f32 dz = rTarget.z - trans.z;
    if (al::isNearZero(sead::Mathf::sqrt(dx * dx + dz * dz), 0.001f)) {
        return true;
    }

    sead::Vector3f dir = rTarget - al::getTrans(this);
    al::normalize(&dir);
    return turnToDirectionGently(dir, speed);
}

/** @brief Raises the rock attack, then waits. */
void TentackHead::exeAttackRockStart() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "AttackRockStart");
    }

    al::setNerveAtActionEnd(this, &NrvTentackHeadWait);
}

/** @brief Signals the tentacle attack, then waits facing a fixed direction. */
void TentackHead::exeAttackTentacleStart() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "AttackTentacleStart");
    }

    al::setNerveAtActionEnd(this, &NrvTentackHeadWaitFixed);
}

/** @brief Flinches from a stomp, then sinks back into the floor. */
void TentackHead::exeDamage() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Damage");
        al::startHitReactionPressDown(this);
        mJointLook->stopLook();
        mIsReappearRockFaller = false;
    }

    al::setNerveAtActionEnd(this, &NrvTentackHeadBack);
}

/** @brief Sinks into the floor and switches off the spotlight. */
void TentackHead::exeBack() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Back");
        mHill->setReverse();
    }

    if (al::isActionEnd(this)) {
        mHill->setDisappear();
        al::killPrePassLight(this, cLightName, 5);
        al::setNerve(this, &NrvTentackHeadBackWait);
    }
}

/** @brief Stays under the floor for a while. */
void TentackHead::exeBackWait() {
    if (al::isGreaterEqualStep(this, 60)) {
        al::setNerve(this, &NrvTentackHeadPushSign);
    }
}

/** @brief Cracks the floor where the head is about to burst out. */
void TentackHead::exePushSign() {
    al::LiveActor* crackModel = al::getSubActor(this, "ひび割れモデル");
    if (al::isFirstStep(this)) {
        crackModel->appear();
        al::appearPrePassLight(this, cLightName, 5);
        al::rotateQuatYDirDegree(crackModel, al::getRandomDegree());
        al::startAction(crackModel, "Appear");
        al::hideModelIfShow(crackModel);
        al::startSe(this, "PgPushSign");
        al::faceToDirection(this, mFrontDir);
    }

    if (al::isStep(this, 1)) {
        al::showModelIfHide(crackModel);
    }

    if (al::isGreaterEqualStep(this, 60)) {
        al::setNerve(this, &NrvTentackHeadPush);
    }
}

/** @brief Bursts out of the floor again, breaking the cracked floor. */
void TentackHead::exePush() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Push");
        mHill->appear();
        al::getSubActor(this, "ひび割れモデル")->kill();
        al::appearBreakModelRandomRotateY(al::getSubActor(this, "ひび割れモデル[壊れ]"));
        al::appearBreakModelRandomRotateY(al::getSubActor(this, "攻撃岩出現モデル"));
        mRumble->reset();
        al::setScaleY(this, 1.0f);
        if (mIsReappearRockFaller) {
            mRockFaller->appear();
            mIsReappearRockFaller = false;
        }

        al::startSeWithParam(this, "PgAppearAgain", 1.0f);
        al::setSeSeqLocalVariableDefault(this, 0, mDamage);
    }

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvTentackHeadWaitFixed);
    }
}

/** @brief Eats, then either disappears or waits. */
void TentackHead::exeEat() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Eat");
    }

    if (al::isActionEnd(this)) {
        if (al::isNerve(this, &NrvTentackHeadEatAndDisappear)) {
            al::setNerve(this, &NrvTentackHeadDisappear);
        } else {
            al::setNerve(this, &NrvTentackHeadWaitFixed);
        }
    }
}

/** @brief Sinks into the floor without taking damage. */
void TentackHead::exeDisappear() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Disappear");
    }

    al::setNerveAtActionEnd(this, &NrvTentackHeadBackWait);
}

/** @brief Plays the shot action, then waits. */
void TentackHead::exeShot() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Shot");
    }

    al::setNerveAtActionEnd(this, &NrvTentackHeadWaitFixed);
}

/** @brief Plays the cry action, then waits. */
void TentackHead::exeCry() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Cry");
    }

    al::setNerveAtActionEnd(this, &NrvTentackHeadWaitFixed);
}

/** @brief Plays the defeat action, turning back to the initial direction, then dies. */
void TentackHead::exeDemoEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "DamageDead");
        al::startHitReactionPressDown(this);
        mJointLook->stopLook();
    }

    if (al::isGreaterEqualStep(this, 105)) {
        turnToDirectionGently(mFrontDir, 0.5f);
    }

    if (al::isActionEnd(this)) {
        kill();
    }
}

/**
 * @brief Turns a step toward a direction around the Y axis, slowing down near it.
 * @param rDir Direction to face.
 * @param speed Maximum turn per frame in degrees.
 * @return True once the head faces the direction.
 */
bool TentackHead::turnToDirectionGently(const sead::Vector3f& rDir, f32 speed) {
    f32 targetAngle =
        al::calcAngleOnPlaneDegree(sead::Vector3f::ez, rDir, sead::Vector3f::ey);
    sead::Vector3f front(0.0f, 0.0f, 0.0f);
    al::calcFrontDir(&front, this);
    f32 frontAngle = al::calcAngleOnPlaneDegree(sead::Vector3f::ez, front, sead::Vector3f::ey);
    f32 diff = al::calcAngleOnPlaneDegree(front, rDir, sead::Vector3f::ey);

    f32 step = diff > 0.0f ? speed : -speed;
    f32 absDiff = diff > 0.0f ? diff : -diff;
    if (absDiff < 30.0f) {
        f32 rate = al::normalizeAbs(diff, 0.0f, 30.0f);
        f32 slowStep = rate > 0.0f ? al::lerpValue(rate, 0.05f, speed) :
                                     -al::lerpValue(-rate, 0.05f, speed);
        f32 absStep = slowStep > 0.0f ? slowStep : -slowStep;
        if (diff > 0.0f) {
            step = diff < absStep ? diff : absStep;
        } else {
            step = diff > -absStep ? diff : -absStep;
        }
    }

    if (mLookChangeStep > 0) {
        f32 rate = al::normalize(static_cast<f32>(15 - mLookChangeStep), 0.0f, 15.0f);
        step = al::lerpValue(rate, mTurnSpeed, step);
        mLookChangeStep--;
    }

    if (mLookChangeStep == 0) {
        mTurnSpeed = step;
    }

    if (!al::isSameSign(targetAngle, frontAngle) && frontAngle > 0.0f && step > 0.0f) {
        step = -step;
    }

    al::rotateQuatYDirDegree(this, step);
    return al::isNearZero(diff - step, 0.1f);
}

/** @brief Restarts the blend toward the current look target. */
void TentackHead::changeLookTarget() {
    mLookChangeStep = 15;
}

/**
 * @brief Starts the rock attack if the head is waiting.
 * @return True if the attack was started.
 */
bool TentackHead::tryStartActionAttackRockIfWait() {
    if (isWaitAll()) {
        al::setNerve(this, &NrvTentackHeadAttackRockStart);
        return true;
    }

    return false;
}

/**
 * @brief Checks whether the head is waiting, turning or not.
 * @return True while in either wait state.
 */
bool TentackHead::isWaitAll() const {
    return al::isNerve(this, &NrvTentackHeadWait) || al::isNerve(this, &NrvTentackHeadWaitFixed);
}

/** @brief Starts the tentacle attack signal. */
void TentackHead::startActionAttackTentacle() {
    al::setNerve(this, &NrvTentackHeadAttackTentacleStart);
}

/**
 * @brief Starts eating if the head is waiting.
 * @return True if the action was started.
 */
bool TentackHead::tryStartActionEat() {
    if (isWaitAll()) {
        al::setNerve(this, &NrvTentackHeadEat);
        return true;
    }

    return false;
}

/**
 * @brief Starts eating and then disappearing if the head is waiting.
 * @return True if the action was started.
 */
bool TentackHead::tryStartActionEatAndDisappear() {
    if (isWaitAll()) {
        mIsReappearRockFaller = true;
        al::setNerve(this, &NrvTentackHeadEatAndDisappear);
        return true;
    }

    return false;
}

/**
 * @brief Starts the shot action if the head is waiting.
 * @return True if the action was started.
 */
bool TentackHead::tryStartActionShot() {
    if (isWaitAll()) {
        al::setNerve(this, &NrvTentackHeadShot);
        return true;
    }

    return false;
}

/**
 * @brief Starts the cry action if the head is waiting.
 * @return True if the action was started.
 */
bool TentackHead::tryStartActionCry() {
    if (isWaitAll()) {
        al::setNerve(this, &NrvTentackHeadCry);
        return true;
    }

    return false;
}

/** @brief Waits while turning to the nearest player. */
void TentackHead::setWait() {
    al::setNerve(this, &NrvTentackHeadWait);
}

/** @brief Waits without turning. */
void TentackHead::setWaitFixed() {
    al::setNerve(this, &NrvTentackHeadWaitFixed);
}

/** @brief Sinks into the floor, bringing the rock faller back on the next push. */
void TentackHead::setDisappear() {
    mIsReappearRockFaller = true;
    al::setNerve(this, &NrvTentackHeadDisappear);
}

/**
 * @brief Checks whether the head waits without turning.
 * @return True in the fixed wait state.
 */
bool TentackHead::isWaitFixed() const {
    return al::isNerve(this, &NrvTentackHeadWaitFixed);
}

/**
 * @brief Checks whether the head is sinking, under the floor or pushing back out.
 * @return True early in the push action or while sinking or hidden.
 */
bool TentackHead::isBackOrPush() const {
    if (al::isActionPlaying(this, "Push")) {
        return al::getActionFrame(this) <= 70.0f;
    }

    return al::isNerve(this, &NrvTentackHeadBack) || al::isNerve(this, &NrvTentackHeadBackWait) ||
           al::isNerve(this, &NrvTentackHeadPushSign) || al::isNerve(this, &NrvTentackHeadPush) ||
           al::isNerve(this, &NrvTentackHeadDisappear);
}

/**
 * @brief Checks whether the head is reacting to damage.
 * @return True while flinching, sinking after a hit or defeated.
 */
bool TentackHead::isDamageAction() const {
    return al::isNerve(this, &NrvTentackHeadDamage) || al::isNerve(this, &NrvTentackHeadBack) ||
           al::isNerve(this, &NrvTentackHeadDemoEnd);
}

/**
 * @brief Checks whether the head is defeated.
 * @return True in the defeat state.
 */
bool TentackHead::isDemoEnd() const {
    return al::isNerve(this, &NrvTentackHeadDemoEnd);
}

/** @brief Skips the entrance demo, showing the head and its floor model at once. */
void TentackHead::cancelDemoAppear() {
    al::startAction(this, "Wait");
    al::clearSklAnimInterpole(this);
    al::showModelIfHide(this);
    if (al::isDead(mHill)) {
        mHill->appear();
    }

    if (al::isAlive(al::getSubActor(this, "攻撃岩出現モデル"))) {
        al::getSubActor(this, "攻撃岩出現モデル")->kill();
    }

    al::tryDeleteEffect(this, "DemoStart");
    al::tryDeleteEffect(this, "DemoStartRock");
    al::setNerve(this, &NrvTentackHeadWait);
}

/**
 * @brief Gets the radius of the head.
 * @return Head radius.
 */
f32 TentackHead::getHeadRadius() {
    return 550.0f;
}

/** @brief Destroys the head. */
TentackHead::~TentackHead() = default;
