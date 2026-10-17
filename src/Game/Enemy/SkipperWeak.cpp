#include "Enemy/SkipperWeak.hpp"
#include "Enemy/ActorStateSupportFreeze.hpp"
#include "Enemy/EnemyStateUtil.hpp"
#include "Enemy/TargetFinder.hpp"
#include "Enemy/WalkerStateChase.hpp"
#include "Enemy/WalkerStateFunction.hpp"
#include "Enemy/WalkerStateParam.hpp"
#include "Enemy/WalkerStateWander.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/ActorAreaFunction.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Obj/BreakModel.hpp"
#include "Library/StageSwitch/StageSwitchFunc.hpp"
#include "Library/Thread/Functor.hpp"
#include "Project/AreaObj/AreaObjUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ScoreUtil.hpp"
#include "Util/ItemUtil.hpp"
#include "Library/Obj/PartsFunction.hpp"
namespace {
NERVE_DECL(SkipperWeak, Wander)
NERVE_DECL(SkipperWeak, Chase)
NERVE_DECL(SkipperWeak, SupportFreeze)
NERVE_DECL(SkipperWeak, AttackSuccess)
NERVE_DECL(SkipperWeak, TurnToPlayer)
NERVE_DECL(SkipperWeak, Find)
NERVE_DECL(SkipperWeak, Land)
NERVES_MAKE_STRUCT(SkipperWeak, Wander, Chase, SupportFreeze)
NERVES_MAKE_NOSTRUCT(SkipperWeak, AttackSuccess, TurnToPlayer, Find, Land)
TargetFinderParam sTargetParam(1500.0f, 120.0f, 80.0f, 60, 5000.0f, -1.0f, -1.0f, -1.0f, false);
WalkerStateParam sWalkerParam(2.25f, 0.98f, 0.85f, 250.0f, 700.0f, 180.0f, 70.0f, 150.0f);
WalkerStateChaseParam sChaseParam(0.7f, 120.0f, 300.0f, 5.0f, 30.0f, false, true, "Run", "Wait", -1.0f);
WalkerStateWanderParam sWanderParam(60, 180, 0.35f, 2.0f, 10.0f, 750.0f, true, "Walk", "Wait");
ActorStateSupportFreezeParam sFreezeParam(true, 15, false, true, 120, sead::Vector3f(0.0f, 150.0f, 0.0f));
}
/** @brief Creates a fragile Skipsqueak. @param pName Actor name. */
SkipperWeak::SkipperWeak(const char* pName) : al::LiveActor(pName) {}
/** @brief Initializes movement, chase areas, and the break model. @param rInfo Placement information. */
void SkipperWeak::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "Skipper", nullptr);
    mTargetFinder = new TargetFinder(this, &sTargetParam);
    mStateChase = new WalkerStateChase(this, al::getFrontPtr(this), mTargetFinder, &sWalkerParam, &sChaseParam, false, nullptr);
    mStateWander = new WalkerStateWander(this, al::getFrontPtr(this), &sWalkerParam, &sWanderParam, nullptr);
    mFreeze = new ActorStateSupportFreeze(this, &sFreezeParam);
    al::initNerve(this, &NrvSkipperWeak.Wander, 4);
    al::initNerveState(this, mStateChase, &NrvSkipperWeak.Chase, "追いかけ");
    al::initNerveState(this, mStateWander, &NrvSkipperWeak.Wander, "うろつき");
    al::initNerveState(this, mFreeze, &NrvSkipperWeak.SupportFreeze, "フリーズ");
    al::listenStageSwitchOnKill(this, al::FunctorV0M<SkipperWeak*, void (SkipperWeak::*)()>(this, &SkipperWeak::killSwitch));
    mChaseArea = al::createLinkAreaGroup(this, rInfo, "ChaseArea", "追いかけ有効エリアグループ", "子供エリア");
    mBreakModel = new al::BreakModel(this, "よわスキッパー壊れモデル", "SkipperBreak", nullptr, nullptr, "Break", true);
    al::initCreateActorNoPlacementInfo(mBreakModel, rInfo);
    makeActorAppeared();
}
/** @brief Removes the enemy when its kill switch activates. */
void SkipperWeak::killSwitch() { al::startHitReactionDisappear(this); kill(); }
/** @brief Pushes enemies and attacks players. @param pSelf Own sensor. @param pOther Contact sensor. */
void SkipperWeak::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isSensorEnemyBody(pSelf) && al::isSensorEnemyBody(pOther)) al::sendMsgPushAndKillVelocityToTarget(this, pSelf, pOther);
    if (al::isSensorEnemyAttack(pSelf) && al::isSensorPlayer(pOther)) {
        al::sendMsgPush(pOther, pSelf);
        if (al::sendMsgEnemyAttack(pOther, pSelf)) {
            al::faceToTarget(this, al::getSensorPos(pOther));
            al::setNerve(this, &NrvSkipperWeakAttackSuccess);
        }
    }
}
/** @brief Handles pushing and attacks that break the enemy. @param pMsg Message. @param pOther Sender. @param pSelf Own sensor. @return Whether handled. */
bool SkipperWeak::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) {
    if (!al::isSensorEnemyBody(pSelf)) return false;
    if (al::isMsgGoalKill(pMsg)) {
        al::startHitReactionDeath(this);
        al::setAppearItemFactor(this, "直接攻撃", pOther);
        al::appearItemTiming(this, "Coin");
        rc::addScoreCombo(this, pOther, pMsg, 100.0f);
        kill();
        return true;
    }
    if (al::tryReceiveMsgPushAndAddVelocityH(this, pMsg, pOther, pSelf, 5.0f)) return true;
    if (al::isSensorEnemyBody(pSelf) && (EnemyStateUtil::isMsgPressDownForCrossoverSensor(pMsg, pOther, pSelf) || EnemyStateUtil::isMsgBlowDown(pMsg))) {
        rc::addScoreCombo(this, pOther, pMsg, 100.0f);
        rc::setAppearItemFactorByMsg(this, pMsg, pOther);
        al::appearItemTiming(this, "Coin");
        al::appearBreakModelRandomRotateY(mBreakModel);
        rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        al::startHitReactionDeath(this);
        kill();
        return true;
    }
    return false;
}
/** @brief Handles support freeze touches. @param pMsg Message. @param pPointer Pointer. @param pTarget Touch target. @return Whether handled. */
bool SkipperWeak::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer, al::ScreenPointTarget* pTarget) {
    if (!mFreeze->receiveMsgScreenPoint(pMsg, pPointer, pTarget)) return false;
    if (!al::isNerve(this, &NrvSkipperWeak.SupportFreeze)) al::setNerve(this, &NrvSkipperWeak.SupportFreeze);
    return true;
}
/** @brief Wanders until a grounded chase can start. */
void SkipperWeak::exeWander() {
    al::updateNerveState(this);
    mTargetFinder->update();
    if (mTargetFinder->isFoundTarget() && (!mChaseArea || al::tryIsInAreaObj(mChaseArea, al::getTrans(mTargetFinder->getTarget()))) && al::isGreaterEqualStep(this, sWanderParam.mWaitTime) && al::isOnGround(this, 0, 0.0f)) al::setNerve(this, &NrvSkipperWeakTurnToPlayer);
}
/** @brief Turns toward the selected player. */
void SkipperWeak::exeTurnToPlayer() {
    if (al::isFirstStep(this)) { al::setVelocityZero(this); al::startAction(this, "Walk"); }
    WalkerStateFunction::calcPassiveMovement(this, &sWalkerParam);
    if (al::turnDirectionToTargetDegree(this, al::getFrontPtr(this), al::getTrans(mTargetFinder->getTarget()), 8.0f)) al::setNerve(this, &NrvSkipperWeakFind);
}
/** @brief Plays the discovery animation before chasing. */
void SkipperWeak::exeFind() {
    if (al::isFirstStep(this)) al::startAction(this, "Find");
    if (al::isActionEnd(this)) al::setNerve(this, &NrvSkipperWeak.Chase);
}
/** @brief Chases while the target remains in the allowed area. */
void SkipperWeak::exeChase() {
    if (!mTargetFinder->isExistTarget() || (mChaseArea && !al::tryIsInAreaObj(mChaseArea, al::getTrans(mTargetFinder->getTarget()))) || al::updateNerveState(this)) al::setNerve(this, &NrvSkipperWeak.Wander);
}
/** @brief Recovers from landing. */
void SkipperWeak::exeLand() {
    if (al::isFirstStep(this)) al::startAction(this, "Land");
    updateVelocity();
    if (al::isActionEnd(this)) al::setNerve(this, &NrvSkipperWeak.Wander);
}
/** @brief Applies friction, gravity, and a terminal speed. */
void SkipperWeak::updateVelocity() {
    if (al::isOnGround(this, 0, 0.0f)) {
        al::scaleVelocity(this, 0.86f);
        al::addVelocity(this, sead::Vector3f::ey * -0.5f);
    } else {
        al::scaleVelocity(this, 0.98f);
        al::addVelocity(this, sead::Vector3f::ey * -2.5f);
    }
    if (al::getVelocity(this).squaredLength() > 10000.0f) al::limitVelocity(this, 100.0f);
}
/** @brief Resets the wandering center after a support freeze. */
void SkipperWeak::exeSupportFreeze() {
    if (al::updateNerveState(this)) {
        mStateWander->setWanderCenter(al::getTrans(this));
        al::setNerve(this, &NrvSkipperWeak.Wander);
    }
}
/** @brief Celebrates a successful attack, then resumes movement. */
void SkipperWeak::exeAttackSuccess() {
    if (al::isFirstStep(this)) { al::startAction(this, "AttackSuccess"); al::setVelocityZeroH(this); }
    WalkerStateFunction::calcPassiveMovement(this, &sWalkerParam);
    if (al::isActionEnd(this)) {
        mTargetFinder->update();
        if (mTargetFinder->isExistTarget()) al::setNerve(this, &NrvSkipperWeak.Chase);
        else al::setNerve(this, &NrvSkipperWeak.Wander);
    }
}
