#include "Enemy/NokonokoNaked.hpp"

#include "Enemy/ActorStateSupportFreeze.hpp"
#include "Enemy/EnemyStateBlowDown.hpp"
#include "Enemy/EnemyStateUtil.hpp"
#include "Enemy/Nokonoko.hpp"
#include "Enemy/TargetFinder.hpp"
#include "Enemy/WalkerStateChase.hpp"
#include "Enemy/WalkerStateFindPlayer.hpp"
#include "Enemy/WalkerStateFunction.hpp"
#include "Enemy/WalkerStateParam.hpp"
#include "Enemy/WalkerStateRailMove.hpp"
#include "Enemy/WalkerStateWander.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Rail/RailUtil.hpp"
#include "MapObj/Koura.hpp"
#include "Player/Normal/WaterUtil.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ScoreUtil.hpp"

// Nerve that shares the execute function of another nerve.
#define NOKONOKO_NAKED_NERVE_SHARED_DECL(Action, ExeFunc)                                          \
    class NokonokoNakedNrv##Action : public al::Nerve {                                            \
    public:                                                                                        \
        void execute(al::NerveKeeper* pKeeper) const override {                                    \
            (pKeeper->getParent<NokonokoNaked>())->exe##ExeFunc();                                 \
        }                                                                                          \
    };

namespace {
NERVE_DECL(NokonokoNaked, RailMove)
NERVE_DECL(NokonokoNaked, Wait)
NERVE_DECL(NokonokoNaked, RunAway)
NERVE_DECL(NokonokoNaked, Wander)
NERVE_DECL(NokonokoNaked, Chase)
NERVE_DECL(NokonokoNaked, FindPlayer)
NERVE_DECL(NokonokoNaked, BlowDown)
NERVE_DECL(NokonokoNaked, PressDownBlow)
NERVE_DECL(NokonokoNaked, SupportFreeze)
NERVE_DECL(NokonokoNaked, ChaseKoura)
NERVE_DECL(NokonokoNaked, AttachKoura)
NERVE_DECL(NokonokoNaked, Attack)
NERVE_DECL(NokonokoNaked, PressDown)
NERVE_DECL(NokonokoNaked, PressDownPress)
NERVE_DECL(NokonokoNaked, TrampleKoura)
NOKONOKO_NAKED_NERVE_SHARED_DECL(TrampleKouraUpper, TrampleKoura)
NERVE_DECL(NokonokoNaked, AfterEject)
NERVE_DECL(NokonokoNaked, TrampleKouraSlide)
NERVE_DECL(NokonokoNaked, ChaseKouraRetry)
NERVE_DECL(NokonokoNaked, ChaseStart)
NERVE_DECL(NokonokoNaked, ChaseEnd)
NERVE_DECL(NokonokoNaked, FaceToKoura)
NERVE_DECL(NokonokoNaked, ChaseKouraStart)
NERVE_DECL(NokonokoNaked, ChaseKouraBreak)
NERVE_DECL(NokonokoNaked, ChaseKouraEnd)
NERVE_DECL(NokonokoNaked, WaitAttach)

NokonokoNakedNrvRailMove NrvNokonokoNakedRailMove;
NokonokoNakedNrvWait NrvNokonokoNakedWait;
NokonokoNakedNrvRunAway NrvNokonokoNakedRunAway;
NokonokoNakedNrvWander NrvNokonokoNakedWander;
NokonokoNakedNrvChase NrvNokonokoNakedChase;
NokonokoNakedNrvFindPlayer NrvNokonokoNakedFindPlayer;
NokonokoNakedNrvBlowDown NrvNokonokoNakedBlowDown;
NokonokoNakedNrvPressDownBlow NrvNokonokoNakedPressDownBlow;
NokonokoNakedNrvSupportFreeze NrvNokonokoNakedSupportFreeze;
NokonokoNakedNrvChaseKoura NrvNokonokoNakedChaseKoura;
NokonokoNakedNrvAttachKoura NrvNokonokoNakedAttachKoura;
NokonokoNakedNrvAttack NrvNokonokoNakedAttack;
NokonokoNakedNrvPressDown NrvNokonokoNakedPressDown;
NokonokoNakedNrvPressDownPress NrvNokonokoNakedPressDownPress;
NokonokoNakedNrvTrampleKoura NrvNokonokoNakedTrampleKoura;
NokonokoNakedNrvTrampleKouraUpper NrvNokonokoNakedTrampleKouraUpper;
NokonokoNakedNrvAfterEject NrvNokonokoNakedAfterEject;
NokonokoNakedNrvTrampleKouraSlide NrvNokonokoNakedTrampleKouraSlide;
NokonokoNakedNrvChaseKouraRetry NrvNokonokoNakedChaseKouraRetry;
NokonokoNakedNrvChaseStart NrvNokonokoNakedChaseStart;
NokonokoNakedNrvChaseEnd NrvNokonokoNakedChaseEnd;
const NokonokoNakedNrvFaceToKoura NrvNokonokoNakedFaceToKoura{};
const NokonokoNakedNrvChaseKouraStart NrvNokonokoNakedChaseKouraStart{};
NokonokoNakedNrvChaseKouraBreak NrvNokonokoNakedChaseKouraBreak;
const NokonokoNakedNrvChaseKouraEnd NrvNokonokoNakedChaseKouraEnd{};
NokonokoNakedNrvWaitAttach NrvNokonokoNakedWaitAttach;

TargetFinderParam sTargetFinderParam(1000.0f, 180.0f, 75.0f, 90, -1.0f, 500.0f, 500.0f, 1500.0f,
                                     false);
WalkerStateParam sWalkerStateParam(2.25f, 0.98f, 0.89f, 500.0f, 1000.0f, 80.0f, 40.0f, 150.0f);
WalkerStateParam sWalkerStateParamBreak(2.25f, 0.98f, 0.94f, 500.0f, 1000.0f, 80.0f, 40.0f,
                                        150.0f);
WalkerStateParam sWalkerStateParamSlide(2.25f, 0.995f, 0.95f, 500.0f, 1000.0f, 80.0f, 40.0f,
                                        150.0f);
WalkerStateFindPlayerParam sFindPlayerParam(30, 8.0f, false, "Walk");
WalkerStateRailMoveParam sRailMoveParam;
EnemyStateBlowDownParam sBlowDownParam(false);
ActorStateSupportFreezeParam sSupportFreezeParam(true, 15, false, true, 120,
                                                 sead::Vector3f(0.0f, 150.0f, 0.0f));
}  // namespace

/**
 * @brief Constructs a NokonokoNaked.
 * @param pName Actor name.
 * @param pNokonoko Nokonoko that owns this body and its shell.
 */
NokonokoNaked::NokonokoNaked(const char* pName, Nokonoko* pNokonoko)
    : al::LiveActor(pName), mNokonoko(pNokonoko) {}

/**
 * @brief Gets the shell of the owning Nokonoko.
 * @return Shell actor.
 */
inline Koura* NokonokoNaked::getKoura() const {
    return mNokonoko->getKoura();
}

/** @brief Touches the floor, dies in ink or deep water and updates the shell chase state. */
void NokonokoNaked::control() {
    al::HitSensor* pGroundSensor = al::tryGetCollidedGroundSensor(this);
    if (pGroundSensor != nullptr) {
        al::sendMsgEnemyFloorTouch(pGroundSensor, al::getHitSensor(this, "Body"));
    }

    if (GameDataFunction::isSingleMode(GameDataHolderAccessor(this))) {
        if (rc::isCollidedInkSlow(this) ||
            al::getTrans(this).y <
                WaterUtil::getOceanWaterHeight(this, al::getTrans(this), false) + -100.0f) {
            al::startHitReactionDeath(this);
            kill();
        }
    }

    updateKouraChaseState();
}

/** @brief Checks whether the shell is close enough (and not sliding for too long) to chase. */
void NokonokoNaked::updateKouraChaseState() {
    mIsNearKoura = false;
    if (al::isDead(getKoura())) {
        return;
    }

    mKouraSlideFrame++;
    if (!getKoura()->isSlide()) {
        mKouraSlideFrame = 0;
    } else if (mKouraSlideFrame > 120) {
        return;
    }

    sead::Vector3f kouraTrans = al::getTrans(getKoura());
    if ((kouraTrans - al::getTrans(this)).squaredLength() >= 3000.0f * 3000.0f) {
        return;
    }

    mIsNearKoura = true;
}

/**
 * @brief Initializes the model, states and nerves.
 * @param rInfo Placement info of the actor.
 */
void NokonokoNaked::init(const al::ActorInitInfo& rInfo) {
    if (rInfo.getActorSceneInfo().isSingleMode) {
        al::initActorWithArchiveName(this, rInfo, "NokonokoNakedFur", nullptr);
    } else {
        al::initActorWithArchiveName(this, rInfo, "NokonokoNaked", nullptr);
    }

    if (al::isExistRail(this)) {
        al::initNerve(this, &NrvNokonokoNakedRailMove, 8);
        mStateRailMove = new WalkerStateRailMove(this, &sWalkerStateParam, &sRailMoveParam);
        al::initNerveState(this, mStateRailMove, &NrvNokonokoNakedRailMove, "[state]レール移動");
        mStateRailMove->setPositionToStart();
    } else {
        al::initNerve(this, &NrvNokonokoNakedWait, 7);
    }

    bool isEnableCliffCheck = true;
    al::tryGetArg(&isEnableCliffCheck, rInfo, "IsEnableCliffCheck");
    mWanderParam = new WalkerStateWanderParam(30, 150, 0.3f, 4.0f, 20.0f, 500.0f,
                                              isEnableCliffCheck, "Walk", "Wait");
    mRunAwayParam = new WalkerStateWanderParam(30, 150, 0.6f, 8.0f, 20.0f, 500.0f,
                                               isEnableCliffCheck, "Run", "Wait");
    mChaseParam = new WalkerStateChaseParam(1.0f, 130.0f, 500.0f, 3.5f, -1.0f, false,
                                            isEnableCliffCheck, "Run", "Wait", -1.0f);
    mTargetFinder = new TargetFinder(this, &sTargetFinderParam);
    mStateRunAway = new WalkerStateWander(this, al::getFrontPtr(this), &sWalkerStateParam,
                                          mRunAwayParam, nullptr);
    mStateWander = new WalkerStateWander(this, al::getFrontPtr(this), &sWalkerStateParam,
                                         mWanderParam, mTargetFinder);
    mStateChase = new WalkerStateChase(this, al::getFrontPtr(this), mTargetFinder,
                                       &sWalkerStateParam, mChaseParam, true, nullptr);
    mStateFindPlayer = new WalkerStateFindPlayer(this, al::getFrontPtr(this), mTargetFinder,
                                                 &sWalkerStateParam, &sFindPlayerParam, nullptr);
    mStateBlowDown = new EnemyStateBlowDown(this, &sBlowDownParam);
    mStatePressDownBlow = new EnemyStateBlowDown(this, &sBlowDownParam);
    mStateSupportFreeze = new ActorStateSupportFreeze(this, &sSupportFreezeParam);
    al::initNerveState(this, mStateRunAway, &NrvNokonokoNakedRunAway, "[state]逃げ回り");
    al::initNerveState(this, mStateWander, &NrvNokonokoNakedWander, "[state]徘徊");
    al::initNerveState(this, mStateChase, &NrvNokonokoNakedChase, "[state]追いかけ");
    al::initNerveState(this, mStateFindPlayer, &NrvNokonokoNakedFindPlayer,
                       "[state]プレーヤー発見");
    al::initNerveState(this, mStateBlowDown, &NrvNokonokoNakedBlowDown, "[state]吹き飛ばし");
    al::initNerveState(this, mStatePressDownBlow, &NrvNokonokoNakedPressDownBlow,
                       "[state]つぶれ吹き飛ばし");
    al::initNerveState(this, mStateSupportFreeze, &NrvNokonokoNakedSupportFreeze,
                       "[state]フリーズ");
    al::createAndSetColliderSpecialPurpose(this, "MoveLimit");
    al::startAction(this, "Wait");
    makeActorAppeared();
}

/** @brief Appears and shows the model. */
void NokonokoNaked::appear() {
    al::LiveActor::appear();
    al::showModelIfHide(this);
}

/**
 * @brief Picks up the shell, pushes enemies and NPCs, hits map objects and attacks players.
 * @param pSelf Sensor of the NokonokoNaked.
 * @param pOther Sensor that was hit.
 */
void NokonokoNaked::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isHideModel(this)) {
        return;
    }

    if (al::getSensorHost(pOther) == getKoura() &&
        al::isNerve(this, &NrvNokonokoNakedChaseKoura) && getKoura()->isAttachableWithNokonoko()) {
        al::setNerve(this, &NrvNokonokoNakedAttachKoura);
        return;
    }

    if (al::isSensorEnemyBody(pOther) || al::isSensorNpc(pOther)) {
        al::sendMsgPush(pOther, pSelf);
    }

    if (GameDataFunction::isSingleMode(GameDataHolderAccessor(this))) {
        if (al::isSensorMapObj(pOther)) {
            al::sendMsgNpcTouch(pOther, pSelf);
        } else if (al::isSensorHostName(pOther, "CatGull")) {
            al::sendMsgExplosion(pOther, pSelf, nullptr);
        }
    }

    if (al::isSensorEnemyAttack(pSelf) &&
        (al::isSensorPlayer(pOther) || al::isSensorKoopaJr(pOther)) && isEnableAttack()) {
        al::sendMsgPush(pOther, pSelf);
        if (al::sendMsgEnemyAttackForCrossoverSensor(pOther, pSelf) &&
            !al::isNerve(this, &NrvNokonokoNakedAttack)) {
            al::faceToTarget(this, al::getSensorPos(pOther));
            al::setNerve(this, &NrvNokonokoNakedAttack);
        }
    }
}

/**
 * @brief Checks whether the NokonokoNaked can attack players.
 * @return Whether it is neither coming out of its shell nor knocked down.
 */
bool NokonokoNaked::isEnableAttack() {
    if (al::isNerve(this, &NrvNokonokoNakedTrampleKoura)) {
        return false;
    }

    if (al::isNerve(this, &NrvNokonokoNakedTrampleKouraUpper)) {
        return false;
    }

    if (al::isNerve(this, &NrvNokonokoNakedAfterEject)) {
        return false;
    }

    if (al::isNerve(this, &NrvNokonokoNakedPressDown)) {
        return false;
    }

    if (al::isNerve(this, &NrvNokonokoNakedPressDownPress)) {
        return false;
    }

    if (al::isNerve(this, &NrvNokonokoNakedPressDownBlow)) {
        return false;
    }

    if (al::isNerve(this, &NrvNokonokoNakedBlowDown)) {
        return false;
    }

    return !al::isNerve(this, &NrvNokonokoNakedTrampleKouraSlide);
}

/**
 * @brief Handles pushes, stomps, blow downs, goal kills and Piranha Plant bites.
 * @param pMsg Received message.
 * @param pOther Sensor that sent the message.
 * @param pSelf Sensor of the NokonokoNaked.
 * @return Whether the message was handled.
 */
bool NokonokoNaked::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                               al::HitSensor* pSelf) {
    if (al::isHideModel(this)) {
        return false;
    }

    if (al::isMsgPush(pMsg) &&
        !(al::getSensorHost(pOther) == getKoura() &&
          al::isNerve(this, &NrvNokonokoNakedAttachKoura)) &&
        al::tryReceiveMsgPushAndAddVelocity(this, pMsg, pOther, pSelf, 5.0f)) {
        return true;
    }

    if (isReceivableAttack() && al::isSensorEnemyBody(pSelf)) {
        if (al::isMsgPlayerObjHipDropAll(pMsg)) {
            rc::setAppearItemFactorByMsg(al::getSensorHost(pSelf), pMsg, pOther);
            rc::addScoreCombo(this, pOther, pMsg, 100.0f);
            rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
            al::setNerve(this, &NrvNokonokoNakedPressDown);
            return true;
        }

        if (EnemyStateUtil::tryRequestPressDown(pMsg, pOther, pSelf, true)) {
            rc::addScoreCombo(this, pOther, pMsg, 100.0f);
            mStatePressDownBlow->setBlowDir(al::getSensorHost(pOther));
            rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
            al::setNerve(this, &NrvNokonokoNakedPressDownPress);
            return true;
        }

        if (EnemyStateUtil::tryRequestBlowDownAndNextNerve(pMsg, pOther, pSelf, mStateBlowDown,
                                                           &NrvNokonokoNakedBlowDown, true)) {
            rc::addScoreCombo(this, pOther, pMsg, 100.0f);
            return true;
        }

        if (al::isMsgEnemyAttack(pMsg)) {
            rc::setAppearItemFactorByMsg(this, pMsg, pOther);
            rc::addScoreCombo(this, pOther, pMsg, 100.0f);
            mStatePressDownBlow->setBlowDir(al::getSensorHost(pOther));
            al::setNerve(this, &NrvNokonokoNakedPressDownPress);
            return true;
        }
    }

    if (isReceivableAttack() && al::isMsgGoalKill(pMsg)) {
        al::startHitReactionDeath(this);
        al::setAppearItemFactor(this, "直接攻撃", pOther);
        al::appearItem(this);
        rc::addScoreCombo(this, pOther, pMsg, 100.0f);
        kill();
        return true;
    }

    if (GameDataFunction::isSingleMode(GameDataHolderAccessor(this)) &&
        rc::isMsgPackunEatStart(pMsg)) {
        return true;
    }

    return false;
}

/**
 * @brief Checks whether the NokonokoNaked can be stomped or blown away.
 * @return Whether it is neither coming out of its shell nor already knocked down.
 */
bool NokonokoNaked::isReceivableAttack() {
    if (al::isNerve(this, &NrvNokonokoNakedTrampleKoura)) {
        return false;
    }

    if (al::isNerve(this, &NrvNokonokoNakedTrampleKouraUpper)) {
        return false;
    }

    if (al::isNerve(this, &NrvNokonokoNakedPressDown)) {
        return false;
    }

    if (al::isNerve(this, &NrvNokonokoNakedPressDownPress)) {
        return false;
    }

    if (al::isNerve(this, &NrvNokonokoNakedPressDownBlow)) {
        return false;
    }

    if (al::isNerve(this, &NrvNokonokoNakedBlowDown)) {
        return false;
    }

    return !al::isNerve(this, &NrvNokonokoNakedTrampleKouraSlide);
}

/**
 * @brief Freezes the NokonokoNaked when it is touched on the screen.
 * @param pMsg Received message.
 * @param pPointer Screen pointer that touched.
 * @param pTarget Touched screen point target.
 * @return Whether the message was handled.
 */
bool NokonokoNaked::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                          al::ScreenPointTarget* pTarget) {
    if (!mStateSupportFreeze->receiveMsgScreenPoint(pMsg, pPointer, pTarget)) {
        return false;
    }

    if (!al::isNerve(this, &NrvNokonokoNakedSupportFreeze) &&
        !al::isNerve(this, &NrvNokonokoNakedBlowDown) &&
        !al::isNerve(this, &NrvNokonokoNakedPressDown) &&
        !al::isNerve(this, &NrvNokonokoNakedPressDownPress) &&
        !al::isNerve(this, &NrvNokonokoNakedPressDownBlow) &&
        !al::isNerve(this, &NrvNokonokoNakedTrampleKoura) &&
        !al::isNerve(this, &NrvNokonokoNakedTrampleKouraUpper)) {
        al::setNerve(this, &NrvNokonokoNakedSupportFreeze);
    }

    return true;
}

/** @brief Lets the owning Nokonoko put its shell back on after clipping ends. */
void NokonokoNaked::endClipped() {
    al::LiveActor::endClipped();
    mNokonoko->tryStartWear();
}

/** @brief Comes out of the shell after the shell was stomped. */
void NokonokoNaked::startEject() {
    appear();
    al::hideModelIfShow(this);
    al::setNerve(this, &NrvNokonokoNakedTrampleKoura);
}

/** @brief Comes out of the shell after the shell was hit from below. */
void NokonokoNaked::startEjectUpper() {
    appear();
    al::hideModelIfShow(this);
    al::setNerve(this, &NrvNokonokoNakedTrampleKouraUpper);
}

/** @brief Waits until a player comes near or the shell can be chased. */
void NokonokoNaked::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait");
        al::setVelocityToGravity(this, sWalkerStateParam.mGravity);
    }

    mTargetFinder->update();
    if (isActive(2500.0f)) {
        al::setNerve(this, &NrvNokonokoNakedWander);
        return;
    }

    if (isKouraChasable()) {
        al::setNerve(this, &NrvNokonokoNakedChaseKouraRetry);
    }
}

/**
 * @brief Checks whether the NokonokoNaked should be active.
 * @param distance Distance to the nearest player within which it is active.
 * @return Whether it is in the air or near a player.
 */
bool NokonokoNaked::isActive(f32 distance) const {
    if (!al::isOnGround(this, 0, 0.0f)) {
        return true;
    }

    return al::isNearPlayer(this, distance);
}

/**
 * @brief Checks whether the shell is near and the NokonokoNaked has not given up on it.
 * @return Whether the shell can be chased.
 */
bool NokonokoNaked::isKouraChasable() const {
    return mIsNearKoura && !mIsGiveUpChaseKoura;
}

/** @brief Wanders around until a player is found or the shell can be chased. */
void NokonokoNaked::exeWander() {
    if (al::updateNerveState(this)) {
        if (al::isGreaterEqualStep(this, 60)) {
            al::setNerve(this, &NrvNokonokoNakedFindPlayer);
        } else {
            al::setNerve(this, &NrvNokonokoNakedChaseStart);
        }

        return;
    }

    if (!isActive(3000.0f)) {
        al::setNerve(this, &NrvNokonokoNakedWait);
        return;
    }

    if (isKouraChasable()) {
        al::setNerve(this, &NrvNokonokoNakedChaseKouraRetry);
    }
}

/** @brief Turns toward a found player and then chases it. */
void NokonokoNaked::exeFindPlayer() {
    al::updateNerveStateAndNextNerve(this, &NrvNokonokoNakedChase);
}

/** @brief Plays the attack animation. */
void NokonokoNaked::exeAttack() {
    if (al::isFirstStep(this)) {
        al::setVelocityZero(this);
        al::startAction(this, "Attack");
    }

    WalkerStateFunction::calcPassiveMovement(this, &sWalkerStateParam);
    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvNokonokoNakedWait);
    }
}

/** @brief Turns toward the target while starting to run. */
void NokonokoNaked::exeChaseStart() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "RunStart");
    }

    WalkerStateFunction::calcPassiveMovement(this, &sWalkerStateParam);
    mTargetFinder->update();
    const al::LiveActor* pTarget = mTargetFinder->getTarget();
    if (pTarget != nullptr) {
        al::turnDirectionToTargetDegree(this, al::getFrontPtr(this), al::getTrans(pTarget),
                                        mChaseParam->getRunAnimRate());
    }

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvNokonokoNakedChase);
    }
}

/** @brief Chases the target and wanders around the place where it was lost. */
void NokonokoNaked::exeChase() {
    if (al::updateNerveState(this)) {
        mStateWander->setWanderCenter(al::getTrans(this));
        if (al::isGreaterEqualStep(this, 180)) {
            al::setNerve(this, &NrvNokonokoNakedChaseEnd);
        } else {
            al::setNerve(this, &NrvNokonokoNakedWander);
        }

        return;
    }

    if (isKouraChasable()) {
        al::setNerve(this, &NrvNokonokoNakedChaseKouraRetry);
    }
}

/** @brief Stops running after a long chase. */
void NokonokoNaked::exeChaseEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "RunEnd");
    }

    WalkerStateFunction::calcPassiveMovement(this, &sWalkerStateParam);
    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvNokonokoNakedWander);
    }
}

/** @brief Walks along the rail. */
void NokonokoNaked::exeRailMove() {
    al::updateNerveState(this);
}

/** @brief Pops out of the stomped shell and gets pushed forward. */
void NokonokoNaked::exeTrampleKoura() {
    if (al::isFirstStep(this)) {
        al::invalidateClipping(this);
        al::showModelIfHide(this);
        bool isUpper = al::isNerve(this, &NrvNokonokoNakedTrampleKouraUpper);
        getKoura()->appearFromNokonoko(this, isUpper);
        al::startAction(this, "Trampled");
        al::setVelocity(this, al::getFront(this) * 25.0f);
    }

    WalkerStateFunction::calcPassiveMovement(this, &sWalkerStateParamSlide);
    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvNokonokoNakedTrampleKouraSlide);
    }
}

/** @brief Slides on the ground until it slows down. */
void NokonokoNaked::exeTrampleKouraSlide() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "SlideLoop");
    }

    WalkerStateFunction::calcPassiveMovement(this, &sWalkerStateParamSlide);
    if (al::isCollidedGround(this)) {
        const sead::Vector3f& velocity = al::getVelocity(this);
        if (velocity.x * velocity.x + velocity.z * velocity.z < 8.0f * 8.0f) {
            al::setNerve(this, &NrvNokonokoNakedAfterEject);
        }
    }
}

/** @brief Gets back up after sliding out of the shell. */
void NokonokoNaked::exeAfterEject() {
    if (al::isFirstStep(this)) {
        al::scaleVelocityHV(this, 0.0f, 1.0f);
        al::startAction(this, "Recover");
    }

    WalkerStateFunction::calcPassiveMovement(this, &sWalkerStateParam);
    if (al::isActionEnd(this)) {
        al::validateClipping(this);
        al::setNerve(this, &NrvNokonokoNakedFaceToKoura);
    }
}

/** @brief Waits for a while before turning toward the shell again. */
void NokonokoNaked::exeChaseKouraRetry() {
    if (al::isFirstStep(this)) {
        al::setVelocityZero(this);
        al::startAction(this, "Wait");
    }

    WalkerStateFunction::calcPassiveMovement(this, &sWalkerStateParam);
    if (al::isGreaterStep(this, 120)) {
        al::setNerve(this, &NrvNokonokoNakedFaceToKoura);
    }
}

/** @brief Turns toward the shell. */
void NokonokoNaked::exeFaceToKoura() {
    if (al::isFirstStep(this)) {
        al::setVelocityZero(this);
        al::startAction(this, "Turn");
    }

    WalkerStateFunction::calcPassiveMovement(this, &sWalkerStateParam);
    if (al::turnDirectionToTargetDegree(this, al::getFrontPtr(this), al::getTrans(getKoura()),
                                        18.0f)) {
        al::setNerve(this, &NrvNokonokoNakedChaseKouraStart);
    }
}

/** @brief Notices the shell. */
void NokonokoNaked::exeChaseKouraStart() {
    if (al::isFirstStep(this)) {
        al::setVelocityZero(this);
        al::startAction(this, "FindShell");
    }

    WalkerStateFunction::calcPassiveMovement(this, &sWalkerStateParam);
    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvNokonokoNakedChaseKoura);
    }
}

/** @brief Runs after the shell and gives up after a while. */
void NokonokoNaked::exeChaseKoura() {
    if (al::isFirstStep(this)) {
        if (!al::isSklAnimPlaying(this, "RunChase", 0)) {
            al::startAction(this, "RunChase");
        }

        mKouraSlideFrame = 0;
    }

    if (!isKouraChasable()) {
        if (al::isGreaterEqualStep(this, 180)) {
            al::setNerve(this, &NrvNokonokoNakedChaseKouraBreak);
        } else {
            al::setNerve(this, &NrvNokonokoNakedWait);
        }

        return;
    }

    al::walkAndTurnToTarget(this, al::getTrans(getKoura()), 0.8f, sWalkerStateParam.mGravity,
                            sWalkerStateParam.mGroundFriction, 8.0f, false);
    if (al::isGreaterEqualStep(this, 600)) {
        mIsGiveUpChaseKoura = true;
        al::setNerve(this, &NrvNokonokoNakedChaseKouraBreak);
    }
}

/** @brief Stops running after the shell. */
void NokonokoNaked::exeChaseKouraBreak() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "RunEnd");
    }

    WalkerStateFunction::calcPassiveMovement(this, &sWalkerStateParamBreak);
    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvNokonokoNakedChaseKouraEnd);
    }
}

/** @brief Gives up on the shell. */
void NokonokoNaked::exeChaseKouraEnd() {
    if (al::isFirstStep(this)) {
        al::setVelocityZero(this);
        al::startAction(this, "GiveUp");
    }

    WalkerStateFunction::calcPassiveMovement(this, &sWalkerStateParam);
    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvNokonokoNakedWait);
    }
}

/** @brief Jumps into the shell and hands control back to the Nokonoko. */
void NokonokoNaked::exeAttachKoura() {
    if (al::isFirstStep(this)) {
        al::setVelocityZero(this);
        al::startAction(this, "RecoverShellStart");
        sead::Vector3f dir = al::getTrans(getKoura()) - al::getTrans(this);
        dir.y = 0.0f;
        f32 frameMax = al::getActionFrameMax(this, "RecoverShellStart");
        if (frameMax > 0.0f) {
            al::setVelocity(this, dir * (1.0f / frameMax));
        }
    }

    if (al::isOnGround(this, 0, 0.0f)) {
        if (al::isCollidedGroundEdgeOrCorner(this)) {
            al::addVelocityToGravity(this, 1.0f);
        }

        al::scaleVelocity(this, sWalkerStateParam.mAirFriction);
    } else {
        al::addVelocityToGravity(this, sWalkerStateParam.mGravity);
        al::scaleVelocity(this, sWalkerStateParam.mAirFriction);
    }

    if (!getKoura()->isAttachableWithNokonoko()) {
        al::setNerve(this, &NrvNokonokoNakedWait);
        return;
    }

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvNokonokoNakedWaitAttach);
        mNokonoko->attachKoura();
    }
}

/** @brief Waits inside the shell until the Nokonoko takes over. */
void NokonokoNaked::exeWaitAttach() {
    WalkerStateFunction::calcPassiveMovement(this, &sWalkerStateParam);
}

/** @brief Runs away and starts chasing the shell when it comes near. */
void NokonokoNaked::exeRunAway() {
    al::updateNerveState(this);
    sead::Vector3f kouraTrans = al::getTrans(getKoura());
    sead::Vector3f trans = al::getTrans(this);
    if (al::isDead(getKoura()) || (kouraTrans - trans).squaredLength() >= 3000.0f * 3000.0f) {
        return;
    }

    al::setNerve(this, &NrvNokonokoNakedChaseKoura);
}

/** @brief Gets stomped flat and dies. */
void NokonokoNaked::exePressDown() {
    if (al::isFirstStep(this)) {
        al::invalidateClipping(this);
        al::changeEnvTextureStamp(this);
        al::setVelocityZero(this);
        al::startAction(this, "PressDown");
    }

    rc::tryAppearItemPressDown(this, nullptr);
    if (al::isActionEnd(this)) {
        al::validateClipping(this);
        al::resetEnvTexture(this);
        al::startHitReactionDeath(this);
        kill();
    }
}

/** @brief Gets squashed before being blown away. */
void NokonokoNaked::exePressDownPress() {
    if (al::isFirstStep(this)) {
        al::changeEnvTextureStamp(this);
        al::invalidateClipping(this);
        al::setVelocityZero(this);
        al::startAction(this, "PressDownPress");
    }

    if (al::isActionEnd(this)) {
        al::resetEnvTexture(this);
        al::setNerve(this, &NrvNokonokoNakedPressDownBlow);
    }
}

/** @brief Gets blown away after being squashed and dies. */
void NokonokoNaked::exePressDownBlow() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "PressDownBlow");
    }

    if (al::updateNerveState(this)) {
        al::startHitReactionDeath(this);
        kill();
        al::validateClipping(this);
        al::appearItem(this);
    }
}

/** @brief Gets blown away and dies. */
void NokonokoNaked::exeBlowDown() {
    if (al::updateNerveState(this)) {
        al::appearItem(this);
        al::startHitReactionDeath(this);
        kill();
    }
}

/** @brief Stays frozen and waits afterwards. */
void NokonokoNaked::exeSupportFreeze() {
    if (al::updateNerveState(this)) {
        al::setNerve(this, &NrvNokonokoNakedWait);
    }
}
