#include "Enemy/Kuribo.hpp"

#include "Enemy/ActorMicRumbler.hpp"
#include "Enemy/ActorStateSupportFreeze.hpp"
#include "Enemy/EnemyStateBlowDown.hpp"
#include "Enemy/EnemyStateUtil.hpp"
#include "Enemy/TargetFinder.hpp"
#include "Enemy/WalkerStateChase.hpp"
#include "Enemy/WalkerStateFall.hpp"
#include "Enemy/WalkerStateFindPlayer.hpp"
#include "Enemy/WalkerStateFunction.hpp"
#include "Enemy/WalkerStateJump.hpp"
#include "Enemy/WalkerStateParam.hpp"
#include "Enemy/WalkerStateRouteDokanMove.hpp"
#include "Enemy/WalkerStateWander.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/StageSwitch/StageSwitchFunc.hpp"
#include "Library/Thread/Functor.hpp"
#include "MapObj/BoxKuribo.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/InkUtil.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ScoreUtil.hpp"

// Nerve that shares the execute function of another nerve.
#define KURIBO_NERVE_SHARED_DECL(Action, ExeFunc)                                                  \
    class KuriboNrv##Action : public al::Nerve {                                                   \
    public:                                                                                        \
        void execute(al::NerveKeeper* pKeeper) const override {                                    \
            (pKeeper->getParent<Kuribo>())->exe##ExeFunc();                                        \
        }                                                                                          \
    };

namespace {
NERVE_DECL(Kuribo, SupportFreeze)
NERVE_DECL(Kuribo, PressDown)
NERVE_DECL(Kuribo, BlowDown)
NERVE_DECL(Kuribo, Chase)
KURIBO_NERVE_SHARED_DECL(JumpTrampolineChase, JumpTrampoline)
NERVE_DECL(Kuribo, Wait)
NERVE_DECL(Kuribo, Wander)
NERVE_DECL(Kuribo, FindPlayer)
NERVE_DECL(Kuribo, JumpTrampoline)
NERVE_DECL(Kuribo, Attack)
NERVE_DECL(Kuribo, JumpJumpPanel)
KURIBO_NERVE_SHARED_DECL(JumpJumpPanelChase, JumpJumpPanel)
NERVE_DECL(Kuribo, Fall)
NERVE_DECL(Kuribo, RouteDokan)
NERVE_DECL(Kuribo, RouteDokanDeath)
// Non-const nerve objects: the game keeps them in .data in this order.
KuriboNrvSupportFreeze NrvKuriboSupportFreeze;
KuriboNrvPressDown NrvKuriboPressDown;
KuriboNrvBlowDown NrvKuriboBlowDown;
KuriboNrvChase NrvKuriboChase;
KuriboNrvJumpTrampolineChase NrvKuriboJumpTrampolineChase;
KuriboNrvWait NrvKuriboWait;
KuriboNrvWander NrvKuriboWander;
KuriboNrvFindPlayer NrvKuriboFindPlayer;
KuriboNrvJumpTrampoline NrvKuriboJumpTrampoline;
KuriboNrvAttack NrvKuriboAttack;
KuriboNrvJumpJumpPanel NrvKuriboJumpJumpPanel;
KuriboNrvJumpJumpPanelChase NrvKuriboJumpJumpPanelChase;
KuriboNrvFall NrvKuriboFall;
KuriboNrvRouteDokan NrvKuriboRouteDokan;
KuriboNrvRouteDokanDeath NrvKuriboRouteDokanDeath;

typedef al::FunctorV0M<Kuribo*, void (Kuribo::*)()> KuriboFunctor;

TargetFinderParam sTargetFinderParam(1500.0f, 180.0f, 75.0f, 90, -1.0f, 500.0f, 500.0f, 2000.0f,
                                     true);
WalkerStateParam sWalkerStateParam(1.5f, 0.98f, 0.9f, 500.0f, 1500.0f, 70.0f, 80.0f, 150.0f);
WalkerStateParam sRouteDokanWalkerStateParam(2.0f, 0.98f, 0.8f, 500.0f, 1500.0f, 70.0f, 80.0f,
                                             150.0f);
WalkerStateFindPlayerParam sFindPlayerParam(30, 7.5f, true, "Walk");
WalkerStateParam sFindWalkerStateParam(1.0f, 0.99f, 0.9f, 500.0f, 1500.0f, 70.0f, 80.0f, 150.0f);
WalkerStateJumpParam sFindJumpParam(14.0f, "Find", true);
WalkerStateJumpParam sAttackJumpParam(14.0f, "Attack", true);
WalkerStateJumpParam sTrampolineJumpParam(30.0f, "JumpTrampoline", false);
WalkerStateJumpParam sJumpPanelJumpParam(45.0f, "JumpTrampoline", false);
ActorStateSupportFreezeParam sSupportFreezeParam(true, 15, false, true, 120,
                                                 sead::Vector3f(0.0f, 150.0f, 0.0f));
EnemyStateBlowDownParam sBlowDownParam(false);

/**
 * @brief Calculates the horizontal direction from one sensor to another.
 * @param pDir Output direction (Z axis when the sensors are at the same place).
 * @param pTo Sensor the direction points to.
 * @param pFrom Sensor the direction starts at.
 */
inline void calcSensorDirH(sead::Vector3f* pDir, const al::HitSensor* pTo,
                           const al::HitSensor* pFrom) {
    const sead::Vector3f& to = al::getSensorPos(pTo);
    const sead::Vector3f& from = al::getSensorPos(pFrom);
    pDir->set(to.x - from.x, 0.0f, to.z - from.z);
    if (al::normalizeOrZero(pDir)) {
        pDir->e = sead::Vector3f::ez.e;
    }
}
}  // namespace

/**
 * @brief Constructs a Kuribo.
 * @param pName Actor name.
 */
Kuribo::Kuribo(const char* pName) : al::LiveActor(pName) {}

/** @brief Updates the timers, the mic rumbler, trampolines and the kill areas. */
void Kuribo::control() {
    if (mAttackInvalidTime > 0) {
        mAttackInvalidTime--;
    }

    if (mClippingInvalidTime > 0) {
        mClippingInvalidTime--;
        if (mClippingInvalidTime == 0) {
            mClippingInvalidTime = -1;
            al::validateClipping(this);
        }
    }

    if (!al::isNerve(this, &NrvKuriboSupportFreeze) && !al::isNerve(this, &NrvKuriboPressDown) &&
        !al::isNerve(this, &NrvKuriboBlowDown)) {
        mMicRumbler->update();
    }

    if (al::isCollidedGround(this)) {
        al::HitSensor* groundSensor = al::tryGetCollidedGroundSensor(this);
        if (groundSensor != nullptr) {
            al::sendMsgEnemyFloorTouch(groundSensor, al::getHitSensor(this, "Body"));
            if (rc::sendMsgEnemyFloorTouchTrampoline(groundSensor, al::getHitSensor(this, "Body"))) {
                if (al::isNerve(this, &NrvKuriboChase)) {
                    al::setNerve(this, &NrvKuriboJumpTrampolineChase);
                } else if (al::isNerve(this, &NrvKuriboWait) ||
                           al::isNerve(this, &NrvKuriboWander) ||
                           al::isNerve(this, &NrvKuriboFindPlayer)) {
                    al::setNerve(this, &NrvKuriboJumpTrampoline);
                }
            }
        }
    }

    if (!EnemyStateUtil::tryKillByAreaOrMaterialCodeWithHitReaction(this) && GameDataFunction::isSingleMode(this) &&
        (InkUtil::isInInkLimitSphere(this) || rc::isInWaterArea(this))) {
        al::startHitReactionDeath(this);
        kill();
    }

    if (mIsRequestOnCollide) {
        al::onCollide(this);
        mIsRequestOnCollide = false;
    }
}

/**
 * @brief Initializes the model, states, Goomba box, stage switches and nerves.
 * @param rInfo Placement info of the actor.
 */
void Kuribo::init(const al::ActorInitInfo& rInfo) {
    if (rInfo.getActorSceneInfo().isSingleMode) {
        al::initActorWithArchiveName(this, rInfo, "KuriboFur", nullptr);
    } else {
        al::initActor(this, rInfo);
    }

    mTargetFinder = new TargetFinder(this, &sTargetFinderParam);
    mMicRumbler = new ActorMicRumbler(this, nullptr);
    bool isEnableCliffCheck = true;
    al::tryGetArg(&isEnableCliffCheck, rInfo, "IsEnableCliffCheck");
    mWanderParam = new WalkerStateWanderParam(10, 120, 0.2f, 3.0f, 20.0f, 500.0f,
                                              isEnableCliffCheck, "Walk", "Wait");
    mChaseParam = new WalkerStateChaseParam(0.8f, 130.0f, 500.0f, 2.8f, 5.0f, false,
                                            isEnableCliffCheck, "Run", "Wait", -1.0f);
    mStateWander = new WalkerStateWander(this, al::getFrontPtr(this), &sWalkerStateParam,
                                         mWanderParam, mTargetFinder);
    mStateChase = new WalkerStateChase(this, al::getFrontPtr(this), mTargetFinder,
                                       &sWalkerStateParam, mChaseParam, true, nullptr);
    mStateFindPlayer =
        new WalkerStateFindPlayer(this, al::getFrontPtr(this), mTargetFinder,
                                  &sFindWalkerStateParam, &sFindPlayerParam, &sFindJumpParam);
    mStateAttack = new WalkerStateJump(this, &sFindWalkerStateParam, &sAttackJumpParam);
    mStateJumpTrampoline = new WalkerStateJump(this, &sWalkerStateParam, &sTrampolineJumpParam);
    mStateJumpJumpPanel = new WalkerStateJump(this, &sWalkerStateParam, &sJumpPanelJumpParam);
    mStateFall = new WalkerStateFall(this, &sWalkerStateParam);
    mStateBlowDown = new EnemyStateBlowDown(this, &sBlowDownParam);
    mStateSupportFreeze = new ActorStateSupportFreeze(this, &sSupportFreezeParam);
    mStateRouteDokanMove =
        new WalkerStateRouteDokanMove(this, rInfo, &sRouteDokanWalkerStateParam, nullptr);

    al::initNerve(this, &NrvKuriboWait, 12);
    al::initNerveState(this, mStateWander, &NrvKuriboWander, "[state]徘徊");
    al::initNerveState(this, mStateChase, &NrvKuriboChase, "[state]追いかけ");
    al::initNerveState(this, mStateFindPlayer, &NrvKuriboFindPlayer, "[state]プレーヤー発見");
    al::initNerveState(this, mStateAttack, &NrvKuriboAttack, "[state]攻撃");
    al::initNerveState(this, mStateJumpTrampoline, &NrvKuriboJumpTrampoline,
                       "[state]トランポリンジャンプ");
    al::initNerveState(this, mStateJumpTrampoline, &NrvKuriboJumpTrampolineChase,
                       "[state]トランポリンジャンプ追跡中");
    al::initNerveState(this, mStateJumpJumpPanel, &NrvKuriboJumpJumpPanel,
                       "[state]ジャンプパネルジャンプ");
    al::initNerveState(this, mStateJumpJumpPanel, &NrvKuriboJumpJumpPanelChase,
                       "[state]ジャンプパネルジャンプ追跡中");
    al::initNerveState(this, mStateFall, &NrvKuriboFall, "[state]落下");
    al::initNerveState(this, mStateBlowDown, &NrvKuriboBlowDown, "[state]吹き飛ばし");
    al::initNerveState(this, mStateSupportFreeze, &NrvKuriboSupportFreeze, "[state]フリーズ");
    al::initNerveState(this, mStateRouteDokanMove, &NrvKuriboRouteDokan,
                       "[state]ルート土管移動");
    al::createAndSetColliderSpecialPurpose(this, "MoveLimit");

    bool isChangeBoxKuribo = false;
    if (al::tryGetArg(&isChangeBoxKuribo, rInfo, "IsChangeBoxKuribo") && isChangeBoxKuribo) {
        mBoxKuribo = new BoxKuribo("クリボーボックス");
        al::initCreateActorNoPlacementInfo(mBoxKuribo, rInfo);
    }

    mIsStayBySwitch = al::listenStageSwitchOn(this, "SwitchWait",
                                              KuriboFunctor(this, &Kuribo::cancelStayBySwitch));
    al::listenStageSwitchOnKill(this, KuriboFunctor(this, &Kuribo::killBySwitch));
    if (al::listenStageSwitchOnAppear(this, KuriboFunctor(this, &Kuribo::appearBySwitch))) {
        makeActorDead();
    } else {
        makeActorAppeared();
    }

    const sead::Vector3f& trans = al::getTrans(this);
    mInitTrans.x = trans.x;
    mInitTrans.y = trans.y;
    mInitTrans.z = trans.z;
    const sead::Vector3f& front = al::getFront(this);
    mInitFront.x = front.x;
    mInitFront.y = front.y;
    mInitFront.z = front.z;
}

/** @brief Stops waiting for the stage switch. */
void Kuribo::cancelStayBySwitch() {
    if (mIsStayBySwitch) {
        mIsStayBySwitch = false;
    }
}

/** @brief Dies when the kill stage switch turns on. */
void Kuribo::killBySwitch() {
    if (isEnableDown()) {
        al::startHitReactionDeath(this);
        kill();
    }
}

/** @brief Appears and falls when the appear stage switch turns on. */
void Kuribo::appearBySwitch() {
    if (isEnableDown()) {
        appear();
        mClippingInvalidTime = 60;
        al::invalidateClipping(this);
        al::setNerve(this, &NrvKuriboFall);
    }
}

/** @brief Reappears at the initial position and falls. */
void Kuribo::reappear() {
    if (GameDataFunction::isSingleMode(this)) {
        al::startAction(this, "Wait");
    }

    al::offCollide(this);
    al::resetPosition(this, mInitTrans, false);
    al::setFront(this, mInitFront);
    al::setNerve(this, &NrvKuriboFall);
    makeActorAppeared();
    mIsRequestOnCollide = true;
}

/**
 * @brief Pushes other enemies and map objects, attacks players and enters route pipes.
 * @param pSelf Sensor of the Kuribo.
 * @param pOther Sensor that was hit.
 */
void Kuribo::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (isDown()) {
        return;
    }

    if (al::isSensorEnemyBody(pOther)) {
        if (al::isNerve(this, &NrvKuriboRouteDokan) && mStateRouteDokanMove->isEject()) {
            if (!al::sendMsgPushStrong(pOther, pSelf)) {
                al::sendMsgPush(pOther, pSelf);
            }
        } else {
            al::sendMsgPushAndKillVelocityToTarget(this, pSelf, pOther);
        }
    }

    if (al::isSensorEnemyAttack(pSelf) &&
        (al::isSensorPlayer(pOther) || al::isSensorKoopaJr(pOther))) {
        if (!isEnableDown()) {
            return;
        }

        al::sendMsgPush(pOther, pSelf);
        if (!isEnableAttack()) {
            return;
        }

        if (al::sendMsgEnemyAttackForCrossoverSensor(pOther, pSelf) &&
            !al::isNerve(this, &NrvKuriboAttack) && !al::isNerve(this, &NrvKuriboRouteDokan)) {
            al::faceToTarget(this, al::getSensorPos(pOther));
            al::setNerve(this, &NrvKuriboAttack);
        }

        return;
    }

    if (GameDataFunction::isSingleMode(this) && al::isSensorGoalItem(pOther)) {
        al::sendMsgEnemyAttackFire(pOther, pSelf);
    }

    if (al::isSensorMapObj(pOther) || al::isSensorNpc(pOther)) {
        if (isEnableDown()) {
            al::sendMsgPush(pOther, pSelf);
        }

        return;
    }

    if (isEnableRouteDokan() && mStateRouteDokanMove->tryStart(pSelf, pOther)) {
        al::setNerve(this, &NrvKuriboRouteDokan);
    }
}

/**
 * @brief Checks whether the Kuribo can be knocked down.
 * @return Whether it can be knocked down.
 */
bool Kuribo::isEnableDown() const {
    if (al::isNerve(this, &NrvKuriboPressDown)) {
        return false;
    }

    if (al::isNerve(this, &NrvKuriboBlowDown)) {
        return false;
    }

    if (al::isNerve(this, &NrvKuriboRouteDokanDeath)) {
        return false;
    }

    return isEnableAttack();
}

/**
 * @brief Checks whether the Kuribo can attack players.
 * @return Whether the attack is not disabled after a jump panel jump.
 */
bool Kuribo::isEnableAttack() const {
    return mAttackInvalidTime < 1;
}

/**
 * @brief Checks whether the Kuribo can enter a route pipe.
 * @return Whether a route pipe move can start.
 */
bool Kuribo::isEnableRouteDokan() const {
    if (al::isNerve(this, &NrvKuriboPressDown)) {
        return false;
    }

    if (al::isNerve(this, &NrvKuriboBlowDown)) {
        return false;
    }

    if (al::isNerve(this, &NrvKuriboRouteDokanDeath)) {
        return false;
    }

    return isEnableAttack();
}

/**
 * @brief Handles restores, pushes, route pipe attacks, stomps, blow downs and jump panels.
 * @param pMsg Received message.
 * @param pOther Sensor that sent the message.
 * @param pSelf Sensor of the Kuribo.
 * @return Whether the message was handled.
 */
bool Kuribo::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) {
    if (al::isMsgRestore(pMsg)) {
        makeActorAppeared();
        mIsRequestOnCollide = true;
        al::offCollide(this);
        al::resetPosition(this, mInitTrans, false);
        al::setFront(this, mInitFront);
        al::setNerve(this, &NrvKuriboFall);
        return true;
    }

    if (isDown()) {
        return false;
    }

    if (al::isNerve(this, &NrvKuriboRouteDokan) && mStateRouteDokanMove->isEject()) {
        if (al::isMsgPush(pMsg) || al::isMsgPushStrong(pMsg)) {
            sead::Vector3f dir;
            calcSensorDirH(&dir, pSelf, pOther);
            sead::Vector3f* velocity = al::getVelocityPtr(this);
            *velocity += dir * 3.0f;
            return true;
        }
    } else {
        if (al::isNerve(this, &NrvKuriboAttack) || al::isNerve(this, &NrvKuriboFindPlayer)) {
            if (al::isMsgPush(pMsg) || al::isMsgPushStrong(pMsg)) {
                return true;
            }
        }

        if (al::isMsgPush(pMsg)) {
            if (al::tryReceiveMsgPushAndAddVelocity(this, pMsg, pOther, pSelf, 3.0f)) {
                return true;
            }
        } else if (al::isMsgPushStrong(pMsg)) {
            sead::Vector3f dir;
            calcSensorDirH(&dir, pSelf, pOther);
            f32 speed = 10.0f - al::getVelocity(this).dot(dir);
            if (speed > 0.0f) {
                sead::Vector3f* velocity = al::getVelocityPtr(this);
                *velocity += dir * speed;
            }

            return true;
        }
    }

    if (al::isNerve(this, &NrvKuriboRouteDokan) && mStateRouteDokanMove->isMove() &&
        al::isSensorEnemyBody(pSelf)) {
        if (rc::isMsgRouteDokanPlayerAttack(pMsg)) {
            rc::addScoreCombo(this, pOther, pMsg, 100.0f);
            al::appearItemTiming(this, "ルート土管死亡");
            rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
            al::setNerve(this, &NrvKuriboRouteDokanDeath);
            return true;
        }

        if (EnemyStateUtil::isMsgRouteDokanAttack(pMsg)) {
            rc::addScoreCombo(this, pOther, pMsg, 100.0f);
            if (rc::tryFindRelativeControlUserId(pOther) != -1) {
                al::appearItemTiming(this, "ルート土管死亡");
            }

            al::setNerve(this, &NrvKuriboRouteDokanDeath);
            return true;
        }
    } else if (isEnableDown() && al::isSensorEnemyBody(pSelf)) {
        if (EnemyStateUtil::tryRequestPressDownAndNextNerve(pMsg, pOther, pSelf, this,
                                                            &NrvKuriboPressDown, true)) {
            mClippingInvalidTime = -1;
            al::invalidateClipping(this);
            rc::addScoreCombo(this, pOther, pMsg, 100.0f);
            return true;
        }

        if (EnemyStateUtil::tryRequestBlowDownAndNextNerve(pMsg, pOther, pSelf, mStateBlowDown,
                                                           &NrvKuriboBlowDown, true)) {
            rc::addScoreCombo(this, pOther, pMsg, 100.0f);
            return true;
        }

        if (al::isMsgEnemyAttack(pMsg)) {
            if (GameDataFunction::isSingleMode(this) && al::isSensorHostName(pOther, "TuccondorFur") &&
                al::isSensorHostName(pOther, "ツッコンドルトラップ") &&
                !al::isSensorName(pOther, "AttackHead")) {
                al::setNerve(this, &NrvKuriboBlowDown);
            } else {
                al::setNerve(this, &NrvKuriboPressDown);
            }

            rc::setAppearItemFactorByMsg(this, pMsg, pOther);
            mClippingInvalidTime = -1;
            al::invalidateClipping(this);
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
    }

    if (rc::isMsgJumpPanelAction(pMsg)) {
        if (al::isNerve(this, &NrvKuriboChase)) {
            al::setNerve(this, &NrvKuriboJumpJumpPanelChase);
        } else if (al::isNerve(this, &NrvKuriboWait) || al::isNerve(this, &NrvKuriboWander) ||
                   al::isNerve(this, &NrvKuriboFindPlayer) ||
                   al::isNerve(this, &NrvKuriboSupportFreeze)) {
            al::setNerve(this, &NrvKuriboJumpJumpPanel);
        }
    }

    if (al::isMsgGoalKill(pMsg)) {
        rc::addScoreCombo(this, pOther, pMsg, 100.0f);
        al::startHitReactionDeath(this);
        kill();
        return true;
    }

    if (al::isMsgLaserAttack(pMsg)) {
        al::startHitReactionDeath(this);
        kill();
        return true;
    }

    return false;
}

/**
 * @brief Freezes the Kuribo when it is touched on the screen.
 * @param pMsg Received message.
 * @param pPointer Screen pointer that touched.
 * @param pTarget Touched screen point target.
 * @return Whether the message was handled.
 */
bool Kuribo::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                   al::ScreenPointTarget* pTarget) {
    if (!mStateSupportFreeze->receiveMsgScreenPoint(pMsg, pPointer, pTarget)) {
        return false;
    }

    if (al::isNerve(this, &NrvKuriboSupportFreeze) || al::isNerve(this, &NrvKuriboBlowDown) ||
        al::isNerve(this, &NrvKuriboPressDown) || al::isNerve(this, &NrvKuriboRouteDokan) ||
        al::isNerve(this, &NrvKuriboRouteDokanDeath)) {
        return true;
    }

    mMicRumbler->stopAndReset();
    al::setNerve(this, &NrvKuriboSupportFreeze);
    return true;
}

/**
 * @brief Checks whether the Kuribo has been defeated.
 * @return Whether it is stomped, blown away or killed in a route pipe.
 */
bool Kuribo::isDown() const {
    return al::isNerve(this, &NrvKuriboPressDown) || al::isNerve(this, &NrvKuriboBlowDown) ||
           al::isNerve(this, &NrvKuriboRouteDokanDeath);
}

/** @brief Waits until a player comes near and the stage switch allows moving. */
void Kuribo::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait");
        al::setVelocityToGravity(this, sWalkerStateParam.mGravity);
    }

    WalkerStateFunction::calcPassiveMovement(this, &sWalkerStateParam);
    mTargetFinder->update();
    if (isActive(2000.0f) && !mIsStayBySwitch) {
        al::setNerve(this, &NrvKuriboWander);
    }
}

/**
 * @brief Checks whether the Kuribo should be active.
 * @param distance Distance to the nearest player within which it is active.
 * @return Whether it is in the air or near a player.
 */
bool Kuribo::isActive(f32 distance) const {
    if (!al::isOnGround(this, 0, 0.0f)) {
        return true;
    }

    return al::isNearPlayer(this, distance);
}

/** @brief Falls and starts wandering after landing. */
void Kuribo::exeFall() {
    al::updateNerveStateAndNextNerve(this, &NrvKuriboWander);
}

/** @brief Wanders around and goes back to waiting when no player is near. */
void Kuribo::exeWander() {
    al::updateNerveStateAndNextNerve(this, &NrvKuriboFindPlayer);
    if (!isActive(2500.0f)) {
        al::setNerve(this, &NrvKuriboWait);
    }
}

/** @brief Jumps on finding a player and then chases it. */
void Kuribo::exeFindPlayer() {
    al::updateNerveStateAndNextNerve(this, &NrvKuriboChase);
}

/** @brief Chases the target and wanders around the place where it was lost. */
void Kuribo::exeChase() {
    if (al::updateNerveStateAndNextNerve(this, &NrvKuriboWander)) {
        mStateWander->setWanderCenter(al::getTrans(this));
    }
}

/** @brief Jumps at a player. */
void Kuribo::exeAttack() {
    al::updateNerveStateAndNextNerve(this, &NrvKuriboWait);
}

/** @brief Gets stomped flat and dies, dropping the Goomba box if it has one. */
void Kuribo::exePressDown() {
    if (al::isFirstStep(this)) {
        al::setVelocityZero(this);
        al::startAction(this, "PressDown");
        al::startHitReactionPressDown(this);
        al::changeEnvTextureStamp(this);
        mMicRumbler->stopAndReset();
    }

    rc::tryAppearItemPressDown(this, nullptr);
    if (al::isActionEnd(this)) {
        mClippingInvalidTime = -1;
        al::validateClipping(this);
        al::startHitReactionDeath(this);
        kill();
        al::resetEnvTexture(this);
        if (mBoxKuribo != nullptr) {
            mBoxKuribo->appearPopUp(al::getTrans(this));
        }
    }
}

/** @brief Gets blown away and dies, dropping the Goomba box if it has one. */
void Kuribo::exeBlowDown() {
    if (al::isFirstStep(this)) {
        mMicRumbler->stopAndReset();
    }

    if (al::updateNerveState(this)) {
        if (mBoxKuribo != nullptr) {
            mBoxKuribo->appearPopUp(al::getTrans(this));
        }

        al::appearItem(this);
        al::startHitReactionDeath(this);
        kill();
    }
}

/** @brief Stays frozen and waits afterwards. */
void Kuribo::exeSupportFreeze() {
    if (al::updateNerveState(this)) {
        al::setNerve(this, &NrvKuriboWait);
    }
}

/** @brief Moves through a route pipe. */
void Kuribo::exeRouteDokan() {
    if (al::updateNerveState(this)) {
        al::setNerve(this, &NrvKuriboWait);
    }
}

/** @brief Dies after being hit inside a route pipe. */
void Kuribo::exeRouteDokanDeath() {
    if (al::isFirstStep(this)) {
        al::startHitReaction(this, "ルート土管死亡");
        kill();
    }
}

/** @brief Jumps on a trampoline and goes back to chasing or waiting. */
void Kuribo::exeJumpTrampoline() {
    if (al::updateNerveState(this)) {
        if (al::isNerve(this, &NrvKuriboJumpTrampolineChase)) {
            al::setNerve(this, &NrvKuriboChase);
        } else {
            al::setNerve(this, &NrvKuriboWait);
        }
    }
}

/** @brief Jumps on a jump panel and goes back to chasing or waiting. */
void Kuribo::exeJumpJumpPanel() {
    if (al::isFirstStep(this)) {
        mAttackInvalidTime = 20;
    }

    if (al::updateNerveState(this)) {
        if (al::isNerve(this, &NrvKuriboJumpJumpPanelChase)) {
            al::setNerve(this, &NrvKuriboChase);
        } else {
            al::setNerve(this, &NrvKuriboWait);
        }
    }
}
