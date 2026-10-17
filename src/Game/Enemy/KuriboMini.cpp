#include "Enemy/KuriboMini.hpp"

#include <math/seadMathCalcCommon.h>

#include "Enemy/EnemyStateBlowDown.hpp"
#include "Enemy/EnemyStateUtil.hpp"
#include "Enemy/TargetFinder.hpp"
#include "Enemy/WalkerStateChase.hpp"
#include "Enemy/WalkerStateJump.hpp"
#include "Enemy/WalkerStateParam.hpp"
#include "Enemy/WalkerStateRouteDokanMove.hpp"
#include "Enemy/WalkerStateWander.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Audio/System/AudioVolumeCtrl.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/Item/ItemUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Screen/ScreenPointTarget.hpp"
#include "Library/Thread/Functor.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ScoreUtil.hpp"

namespace {
NERVE_DECL(KuriboMini, Wait)
NERVE_DECL(KuriboMini, Wander)
NERVE_DECL(KuriboMini, Chase)
NERVE_DECL(KuriboMini, Attack)
NERVE_DECL(KuriboMini, RouteDokan)
NERVE_DECL(KuriboMini, BlowDown)
NERVE_DECL(KuriboMini, AppearStart)
NERVE_DECL(KuriboMini, SwitchWait)
NERVE_DECL(KuriboMini, MicReaction)
NERVE_DECL(KuriboMini, PressDown)
NERVE_DECL(KuriboMini, Blow)
NERVE_DECL(KuriboMini, Flick)
NERVE_DECL(KuriboMini, Land)
NERVE_DECL(KuriboMini, Recover)
NERVE_DECL(KuriboMini, RouteDokanDeath)
NERVE_DECL(KuriboMini, AppearLoop)
NERVE_DECL(KuriboMini, AppearEnd)
NERVE_DECL(KuriboMini, RunStart)
NERVE_DECL(KuriboMini, Lost)
// The game keeps Lost as a constant object right after its vtable and the others in .data.
KuriboMiniNrvWait NrvKuriboMiniWait;
KuriboMiniNrvWander NrvKuriboMiniWander;
KuriboMiniNrvChase NrvKuriboMiniChase;
KuriboMiniNrvAttack NrvKuriboMiniAttack;
KuriboMiniNrvRouteDokan NrvKuriboMiniRouteDokan;
KuriboMiniNrvBlowDown NrvKuriboMiniBlowDown;
KuriboMiniNrvAppearStart NrvKuriboMiniAppearStart;
KuriboMiniNrvSwitchWait NrvKuriboMiniSwitchWait;
KuriboMiniNrvMicReaction NrvKuriboMiniMicReaction;
KuriboMiniNrvPressDown NrvKuriboMiniPressDown;
KuriboMiniNrvBlow NrvKuriboMiniBlow;
KuriboMiniNrvFlick NrvKuriboMiniFlick;
KuriboMiniNrvLand NrvKuriboMiniLand;
KuriboMiniNrvRecover NrvKuriboMiniRecover;
KuriboMiniNrvRouteDokanDeath NrvKuriboMiniRouteDokanDeath;
KuriboMiniNrvAppearLoop NrvKuriboMiniAppearLoop;
KuriboMiniNrvAppearEnd NrvKuriboMiniAppearEnd;
KuriboMiniNrvRunStart NrvKuriboMiniRunStart;
const KuriboMiniNrvLost NrvKuriboMiniLost{};

typedef al::FunctorV0M<KuriboMini*, void (KuriboMini::*)()> KuriboMiniFunctor;

TargetFinderParam sTargetFinderParam(1500.0f, 180.0f, 75.0f, 90, 800.0f, -1.0f, -1.0f, -1.0f,
                                     true);
WalkerStateParam sWalkerStateParam(1.5f, 0.98f, 0.9f, 500.0f, 1500.0f, 70.0f, 80.0f, 150.0f);
WalkerStateParam sWalkerStateParamAttack(1.0f, 0.99f, 0.9f, 500.0f, 1500.0f, 70.0f, 80.0f,
                                         150.0f);
WalkerStateJumpParam sJumpParam(14.0f, "Attack", true);
EnemyStateBlowDownParam sBlowDownParam(false);
}  // namespace

/**
 * @brief Constructs a KuriboMini.
 * @param pName Actor name.
 */
KuriboMini::KuriboMini(const char* pName) : al::LiveActor(pName) {}

/**
 * @brief Initializes the model, walker states, nerves and stage switch listeners.
 * @param rInfo Placement info of the actor.
 */
void KuriboMini::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "KuriboMini", nullptr);
    mTargetFinder = new TargetFinder(this, &sTargetFinderParam);
    mInitTrans.set(al::getTrans(this));

    bool isEnableCliffCheck = true;
    al::tryGetArg(&isEnableCliffCheck, rInfo, "IsEnableCliffCheck");
    mWanderParam = new WalkerStateWanderParam(60, 180, 0.08f, 3.0f, 20.0f, 500.0f,
                                              isEnableCliffCheck, "Walk", "Wait");
    mChaseParam = new WalkerStateChaseParam(0.6f, 130.0f, 300.0f, 2.5f, 5.0f, true,
                                            isEnableCliffCheck, "Run", "Wait", -1.0f);
    mStateWander = new WalkerStateWander(this, al::getFrontPtr(this), &sWalkerStateParam,
                                         mWanderParam, nullptr);
    mStateChase = new WalkerStateChase(this, al::getFrontPtr(this), mTargetFinder,
                                       &sWalkerStateParam, mChaseParam, true, nullptr);
    mStateAttack = new WalkerStateJump(this, &sWalkerStateParamAttack, &sJumpParam);
    mStateRouteDokanMove = new WalkerStateRouteDokanMove(this, rInfo, &sWalkerStateParam, nullptr);
    mStateBlowDown = new EnemyStateBlowDown(this, &sBlowDownParam);

    al::initNerve(this, &NrvKuriboMiniWait, 5);
    al::initNerveState(this, mStateWander, &NrvKuriboMiniWander, "[state]徘徊");
    al::initNerveState(this, mStateChase, &NrvKuriboMiniChase, "[state]追いかけ");
    al::initNerveState(this, mStateAttack, &NrvKuriboMiniAttack, "[state]攻撃");
    al::initNerveState(this, mStateRouteDokanMove, &NrvKuriboMiniRouteDokan,
                       "[state]ルート土管移動");
    al::initNerveState(this, mStateBlowDown, &NrvKuriboMiniBlowDown, "[state]吹き飛ばし");
    al::createAndSetColliderSpecialPurpose(this, "MoveLimit");
    if (al::isValidSwitchAppear(this)) {
        al::setNerve(this, &NrvKuriboMiniAppearStart);
    }

    if (al::listenStageSwitchOnStart(this, KuriboMiniFunctor(this, &KuriboMini::startBySwitch))) {
        al::setNerve(this, &NrvKuriboMiniSwitchWait);
    }

    al::listenStageSwitchOn(this, "SwitchResetPosition",
                            KuriboMiniFunctor(this, &KuriboMini::resetInitTrans));
    al::listenStageSwitchOnKill(this, KuriboMiniFunctor(this, &KuriboMini::kill));
    al::trySyncStageSwitchAppear(this);
}

/** @brief Starts walking when the start switch turns on while waiting for it. */
void KuriboMini::startBySwitch() {
    if (al::isNerve(this, &NrvKuriboMiniSwitchWait)) {
        al::setNerve(this, &NrvKuriboMiniWait);
    }
}

/** @brief Moves the KuriboMini back to its initial position if it is walking around. */
void KuriboMini::resetInitTrans() {
    if (!isEnableResetPosition()) {
        return;
    }

    al::resetPosition(this, mInitTrans, false);
    al::setVelocityZero(this);
    al::setNerve(this, &NrvKuriboMiniWait);
}

/** @brief Handles kill areas, microphone input and blowing away by a nearby hip drop. */
void KuriboMini::control() {
    EnemyStateUtil::tryKillByAreaOrMaterialCode(this);
    if (isEnableDown() && !al::isNerve(this, &NrvKuriboMiniMicReaction) &&
        al::isMicInputOn(this) && !al::isNerve(this, &NrvKuriboMiniSwitchWait) &&
        !al::isNerve(this, &NrvKuriboMiniRouteDokan)) {
        al::setNerve(this, &NrvKuriboMiniMicReaction);
        return;
    }

    if (mHipDropBlowDelay < 1) {
        return;
    }

    if (rc::isPlayerOnGround(mHipDropSensor) && !al::isNerve(this, &NrvKuriboMiniPressDown)) {
        sead::Vector3f dir =
            al::getTrans(this) - al::getTrans(al::getSensorHost(mHipDropSensor));
        al::setVelocitySeparateHV(this, dir, 10.0f, 20.0f);
        dir.y = 0.0f;
        al::normalize(&dir);
        al::setFront(this, -dir);
        al::setNerve(this, &NrvKuriboMiniBlow);
        mHipDropSensor = nullptr;
        mHipDropBlowDelay = 0;
        return;
    }

    mHipDropBlowDelay--;
}

/**
 * @brief Checks whether the KuriboMini can be defeated.
 * @return Whether it is not already defeated or appearing.
 */
bool KuriboMini::isEnableDown() const {
    if (al::isNerve(this, &NrvKuriboMiniPressDown)) {
        return false;
    }

    if (al::isNerve(this, &NrvKuriboMiniBlowDown)) {
        return false;
    }

    if (al::isNerve(this, &NrvKuriboMiniAppearStart)) {
        return false;
    }

    if (al::isNerve(this, &NrvKuriboMiniAppearLoop)) {
        return false;
    }

    if (al::isNerve(this, &NrvKuriboMiniAppearEnd)) {
        return false;
    }

    return !al::isNerve(this, &NrvKuriboMiniRouteDokanDeath);
}

/** @brief Starts the death hit reaction and kills the actor. */
void KuriboMini::kill() {
    al::startHitReactionDeath(this);
    al::LiveActor::kill();
}

/**
 * @brief Pushes other enemies and objects, enters route pipes and attacks players.
 * @param pSelf Sensor of the KuriboMini.
 * @param pOther Sensor that was hit.
 */
void KuriboMini::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (!isEnableDown() || al::isNerve(this, &NrvKuriboMiniFlick) ||
        al::isNerve(this, &NrvKuriboMiniMicReaction) || al::isNerve(this, &NrvKuriboMiniLand)) {
        return;
    }

    if (al::isSensorName(pSelf, "Body")) {
        if (al::isSensorEnemyBody(pOther)) {
            if (al::isNerve(this, &NrvKuriboMiniRouteDokan)) {
                al::sendMsgPushStrong(pOther, pSelf);
            } else {
                al::sendMsgPushAndKillVelocityToTarget(this, pSelf, pOther);
            }
        }

        if (al::isSensorKickKoura(pOther) || al::isSensorNpc(pOther)) {
            al::sendMsgPush(pOther, pSelf);
            return;
        }

        if (mStateRouteDokanMove->tryStart(pSelf, pOther)) {
            al::setNerve(this, &NrvKuriboMiniRouteDokan);
            return;
        }
    }

    if (al::isSensorEnemyAttack(pSelf) && al::isSensorPlayer(pOther)) {
        al::sendMsgPush(pOther, pSelf);
        if (al::sendMsgEnemyAttackForCrossoverSensor(pOther, pSelf)) {
            al::setNerve(this, &NrvKuriboMiniAttack);
        }
    }
}

/**
 * @brief Handles pushes, stomps, blow downs, kicks, Piranha Plant bites, route pipe attacks
 * and goal kills.
 * @param pMsg Received message.
 * @param pOther Sensor that sent the message.
 * @param pSelf Sensor of the KuriboMini.
 * @return Whether the message was handled.
 */
bool KuriboMini::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                            al::HitSensor* pSelf) {
    if (!isEnableDown()) {
        return false;
    }

    if (!al::isSensorName(pSelf, "Body")) {
        if (al::isSensorName(pSelf, "Blow") && al::isMsgPlayerObjHipDropAll(pMsg)) {
            mHipDropBlowDelay = 2;
            mHipDropSensor = pOther;
        }

        return false;
    }

    if (!al::isNerve(this, &NrvKuriboMiniRouteDokan) &&
        (al::isMsgPush(pMsg) || al::isMsgPushStrong(pMsg) || al::isMsgPushVeryStrong(pMsg))) {
        const sead::Vector3f& selfPos = al::getSensorPos(pSelf);
        sead::Vector3f dir = selfPos - al::getSensorPos(pOther);
        sead::Vector3f dirH(dir.x, 0.0f, dir.z);
        if (al::isNearZero(dirH, 0.001f)) {
            dir = sead::Vector3f::ez;
        }

        al::normalizeOrDirZ(&dir);
        f32 speed = al::isMsgPushStrong(pMsg) ? 6.0f : 0.5f;
        f32 addSpeed = speed - al::getVelocity(this).dot(dir);
        if (addSpeed > 0.0f) {
            sead::Vector3f* velocity = al::getVelocityPtr(this);
            *velocity += dir * addSpeed;
        }

        return true;
    }

    if (EnemyStateUtil::tryRequestPressDownAndNextNerve(pMsg, pOther, pSelf, this,
                                                        &NrvKuriboMiniPressDown, true)) {
        rc::setAppearItemFactorByMsg(this, pMsg, pOther);
        rc::addScoreCombo(this, pOther, pMsg, 100.0f);
        return true;
    }

    if (EnemyStateUtil::isMsgBlowDown(pMsg) || rc::isMsgBullAttack(pMsg)) {
        EnemyStateUtil::requestBlowDown(pMsg, pOther, pSelf, mStateBlowDown, true);
        rc::setAppearItemFactorByMsg(this, pMsg, pOther);
        rc::addScoreCombo(this, pOther, pMsg, 100.0f);
        al::setNerve(this, &NrvKuriboMiniBlowDown);
        return true;
    }

    if ((al::isMsgPlayerKick(pMsg) || al::isMsgPlayerObjRollingAttack(pMsg)) &&
        (al::isNerve(this, &NrvKuriboMiniLand) || al::isNerve(this, &NrvKuriboMiniRecover))) {
        EnemyStateUtil::requestBlowDown(pMsg, pOther, pSelf, mStateBlowDown, true);
        rc::addScoreCombo(this, pOther, pMsg, 100.0f);
        al::setNerve(this, &NrvKuriboMiniBlowDown);
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

    if (al::isNerve(this, &NrvKuriboMiniRouteDokan) && mStateRouteDokanMove->isMove()) {
        if (rc::isMsgRouteDokanPlayerAttack(pMsg)) {
            rc::addScoreCombo(this, pOther, pMsg, 100.0f);
            al::appearItemTiming(this, "ルート土管死亡");
            rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
            al::setNerve(this, &NrvKuriboMiniRouteDokanDeath);
            return true;
        }

        if (EnemyStateUtil::isMsgRouteDokanAttack(pMsg)) {
            rc::addScoreCombo(this, pOther, pMsg, 100.0f);
            if (rc::tryFindRelativeControlUserId(pOther) != -1) {
                al::appearItemTiming(this, "ルート土管死亡");
            }

            al::setNerve(this, &NrvKuriboMiniRouteDokanDeath);
            return true;
        }

        return false;
    }

    if (al::isMsgGoalKill(pMsg)) {
        rc::addScoreCombo(this, pOther, pMsg, 100.0f);
        al::setAppearItemFactor(this, "直接攻撃", pOther);
        al::appearItem(this);
        kill();
        return true;
    }

    return false;
}

/**
 * @brief Handles touch-screen assist: a tap stomps the KuriboMini, a touch flicks it away.
 * @param pMsg Received message.
 * @param pPointer Screen pointer that touched the actor.
 * @param pTarget Screen point target of the actor.
 * @return Whether the message was handled.
 */
bool KuriboMini::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                       al::ScreenPointTarget* pTarget) {
    if (!isEnableDown() || al::isNerve(this, &NrvKuriboMiniRouteDokan)) {
        return false;
    }

    if (al::isMsgTouchAssistTrig(pMsg)) {
        rc::setAppearItemFactorByMsg(this, pMsg, pPointer);
        rc::addScoreCombo(this, pPointer, pMsg, 100.0f);
        al::setNerve(this, &NrvKuriboMiniPressDown);
        return true;
    }

    if (al::isMsgTouchAssist(pMsg)) {
        mFlickPos = al::getHitScreenPointTargetPos(pPointer);
        al::setNerve(this, &NrvKuriboMiniFlick);
        return true;
    }

    return false;
}

/** @brief Turns to the nearest player while playing the appear animation. */
void KuriboMini::exeAppearStart() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "AppearStart");
        sead::Vector3f dir = al::getTrans(this) - al::findNearestPlayerPos(this);
        al::turnToDirection(this, -dir, 180.0f);
    }

    al::addVelocityToGravity(this, sWalkerStateParam.mGravity);
    al::scaleVelocity(this, sWalkerStateParam.mAirFriction);
    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvKuriboMiniAppearLoop);
    }
}

/** @brief Falls after appearing until it lands. */
void KuriboMini::exeAppearLoop() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "AppearLoop");
    }

    al::addVelocityToGravity(this, sWalkerStateParam.mGravity);
    al::scaleVelocity(this, sWalkerStateParam.mAirFriction);
    if (al::isOnGround(this, 0, 0.0f)) {
        al::setNerve(this, &NrvKuriboMiniAppearEnd);
    }
}

/** @brief Plays the landing animation of the appearance. */
void KuriboMini::exeAppearEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "AppearEnd");
        al::setVelocityZero(this);
    }

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvKuriboMiniWait);
    }
}

/** @brief Waits for the start switch. */
void KuriboMini::exeSwitchWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait");
        al::setVelocityToGravity(this, sWalkerStateParam.mGravity);
    }
}

/** @brief Waits until a player comes near. */
void KuriboMini::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait");
        al::setVelocityToGravity(this, sWalkerStateParam.mGravity);
    }

    if (al::isNearPlayer(this, 3000.0f)) {
        al::setNerve(this, &NrvKuriboMiniWander);
    }
}

/** @brief Wanders around and starts running once a target is found. */
void KuriboMini::exeWander() {
    al::updateNerveState(this);
    if (al::isNearPlayer(this, 3000.0f)) {
        mTargetFinder->update();
        if (mTargetFinder->isFoundTarget()) {
            al::setNerve(this, &NrvKuriboMiniRunStart);
        }
    } else {
        al::setNerve(this, &NrvKuriboMiniWait);
    }
}

/** @brief Plays the run start animation before chasing. */
void KuriboMini::exeRunStart() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "RunStart");
    }

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvKuriboMiniChase);
    }
}

/** @brief Chases the target and wanders around the lost point afterwards. */
void KuriboMini::exeChase() {
    if (al::updateNerveStateAndNextNerve(this, &NrvKuriboMiniLost)) {
        mStateWander->setWanderCenter(al::getTrans(this));
    }
}

/** @brief Looks around after losing the target. */
void KuriboMini::exeLost() {
    if (al::isFirstStep(this)) {
        al::setVelocityZero(this);
        al::startAction(this, "Lost");
    }

    mTargetFinder->update();
    if (mTargetFinder->isFoundTarget()) {
        al::setNerve(this, &NrvKuriboMiniRunStart);
        return;
    }

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvKuriboMiniWander);
    }
}

/** @brief Jumps at a player. */
void KuriboMini::exeAttack() {
    al::updateNerveStateAndNextNerve(this, &NrvKuriboMiniWait);
}

/** @brief Moves through a route pipe. */
void KuriboMini::exeRouteDokan() {
    if (al::updateNerveState(this)) {
        al::setNerve(this, &NrvKuriboMiniWait);
    }
}

/** @brief Dies after being hit inside a route pipe. */
void KuriboMini::exeRouteDokanDeath() {
    if (al::isFirstStep(this)) {
        al::startHitReaction(this, "ルート土管死亡");
        kill();
    }
}

/** @brief Plays the stomped animation and dies. */
void KuriboMini::exePressDown() {
    if (al::isFirstStep(this)) {
        al::setVelocityZero(this);
        al::startAction(this, "PressDown");
        al::changeEnvTextureStamp(this);
    }

    if (al::isActionEnd(this)) {
        al::resetEnvTexture(this);
        al::appearItem(this);
        kill();
    }
}

/** @brief Is blown away and dies. */
void KuriboMini::exeBlowDown() {
    if (al::updateNerveState(this)) {
        al::appearItem(this);
        kill();
    }
}

/** @brief Flies after being blown away until it lands. */
void KuriboMini::exeBlow() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Blow");
    }

    al::addVelocityToGravity(this, sWalkerStateParam.mGravity);
    al::scaleVelocity(this, sWalkerStateParam.mAirFriction);
    if (al::isOnGround(this, 0, 0.0f)) {
        al::setNerve(this, &NrvKuriboMiniLand);
    }
}

/** @brief Is flicked away from the touched position. */
void KuriboMini::exeFlick() {
    if (al::isFirstStep(this)) {
        sead::Vector3f dir = al::getTrans(this) - mFlickPos;
        al::setVelocitySeparateHV(this, dir, 20.0f, 20.0f);
        dir.y = 0.0f;
        al::normalize(&dir);
        al::setFront(this, -dir);
        al::startAction(this, "Blow");
    }

    al::addVelocityToGravity(this, sWalkerStateParam.mGravity);
    al::scaleVelocity(this, sWalkerStateParam.mAirFriction);
    if (al::isOnGround(this, 0, 0.0f)) {
        al::setNerve(this, &NrvKuriboMiniLand);
    }
}

/** @brief Is blown away from the camera by microphone input, weaker the farther it is. */
void KuriboMini::exeMicReaction() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Blow");
        sead::Vector3f dir = al::getTrans(this) - al::getCameraPos(this);
        sead::Vector3f sideDir;
        al::calcCameraSideDir(&sideDir, this);
        sead::Vector3f sideVec;
        al::parallelizeVec(&sideVec, sideDir, dir);
        dir += sideVec * 5.0f;

        f32 distance = dir.length();
        f32 rate;
        if (distance < 2000.0f) {
            rate = 1.0f;
        } else {
            rate = sead::Mathf::clamp((distance - 2000.0f) * -0.000125f + 1.0f, 0.0f, 1.0f);
        }

        f32 speed = rate * 30.0f;
        al::setVelocitySeparateHV(this, dir, speed, speed);
        dir.y = 0.0f;
        al::normalize(&dir);
        al::setFront(this, -dir);
    }

    al::addVelocityToGravity(this, sWalkerStateParam.mGravity);
    al::scaleVelocity(this, sWalkerStateParam.mAirFriction);
    if (al::isOnGround(this, 0, 0.0f)) {
        al::setNerve(this, &NrvKuriboMiniLand);
    }
}

/** @brief Lands after being blown away. */
void KuriboMini::exeLand() {
    if (al::isFirstStep(this)) {
        al::setVelocityZero(this);
        al::startAction(this, "BlowLand");
    }

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvKuriboMiniRecover);
    }
}

/** @brief Gets back up after landing. */
void KuriboMini::exeRecover() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "BlowRecover");
    }

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvKuriboMiniWait);
    }
}

/** @brief Starts the appear animation. */
void KuriboMini::appearStart() {
    al::setNerve(this, &NrvKuriboMiniAppearStart);
}

/**
 * @brief Checks whether the KuriboMini may be moved back to its initial position.
 * @return Whether it is walking around normally.
 */
bool KuriboMini::isEnableResetPosition() const {
    if (!isEnableDown()) {
        return false;
    }

    if (al::isNerve(this, &NrvKuriboMiniSwitchWait)) {
        return false;
    }

    return !al::isNerve(this, &NrvKuriboMiniRouteDokan);
}
