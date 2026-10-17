#include "Enemy/Chorobon.hpp"
#include "Enemy/EnemyStateUtil.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/Util/ActorSceneInfo.hpp"
#include "Library/Movement/RumbleCalculator.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Rail/RailUtil.hpp"
#include "Library/Thread/Functor.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ScoreUtil.hpp"

namespace {
NERVE_DECL(Chorobon, Move)
NERVE_DECL(Chorobon, Vanish)
NERVE_DECL(Chorobon, Reaction)
NERVES_MAKE_NOSTRUCT(Chorobon, Move, Vanish, Reaction)
}

/** @brief Creates a rail Fuzzy. @param pName Actor name. @param pSpeed Shared movement speed. */
Chorobon::Chorobon(const char* pName, const f32* pSpeed) : ChorobonNode(pName, pSpeed) {
    mRumble = new al::RumbleCalculatorCosMultLinear(2.5f, 2.0f, 0.7f, 30);
}

/** @brief Initializes the model, rail, and kill switch. @param rInfo Placement information. */
void Chorobon::init(const al::ActorInitInfo& rInfo) {
    const char* pModel = "Chorobon";
    alPlacementFunction::tryGetModelName(&pModel, rInfo);
    al::initActorWithArchiveName(this, rInfo, pModel, nullptr);
    al::initNerve(this, &NrvChorobonMove, 0);
    al::startAction(this, "Wait");
    al::trySetShadowLength(this, rInfo, nullptr);
    al::setRailPosToStart(this);
    al::setRailClippingInfo(&mClippingCenter, this, 100.0f, 100.0f);
    al::listenStageSwitchOnKill(this, al::Functor(this, &Chorobon::killBySwitch));
    makeActorAppeared();
}

/** @brief Removes a living Fuzzy when its switch activates. */
void Chorobon::killBySwitch() {
    if (al::isDead(this))
        return;
    al::startHitReactionDeath(this);
    kill();
}

/** @brief Advances along the rail at the holder's speed. */
void Chorobon::control() { al::moveSyncRail(this, *mParam); }

/** @brief Attacks players and pushes nearby actors. @param pSelf Own sensor. @param pOther Contact sensor. */
void Chorobon::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (!al::isSensorEnemyAttack(pSelf) || al::isNerve(this, &NrvChorobonVanish))
        return;
    if ((al::isSensorPlayer(pOther) || al::isSensorKoopaJr(pOther)) &&
        al::sendMsgEnemyAttackForCrossoverSensor(pOther, pSelf)) {
        tryReaction(nullptr);
    } else if (GameDataFunction::isSingleMode(GameDataHolderAccessor(this)) &&
        (al::isSensorNpc(pOther) || al::isSensorEnemyBody(pOther) || al::isSensorKoopaJr(pOther) ||
         al::isSensorKickKoura(pOther) || al::isSensorMapObj(pOther))) {
        if (al::isSensorPackunWithPot(pOther))
            rc::sendMsgPushConnected(pOther, pSelf);
        else
            al::sendMsgPush(pOther, pSelf);
    }
}

/** @brief Starts a recoil reaction while moving. @param pMsg Incoming attack, or null for touch. @return Whether recoil started. */
bool Chorobon::tryReaction(const al::SensorMsg* pMsg) {
    if (!al::isNerve(this, &NrvChorobonMove))
        return false;
    if (pMsg && (al::isMsgEnemyAttackFire(pMsg) || al::isMsgEnemyRouteDokanFire(pMsg) ||
        al::isMsgPlayerFireBallAttack(pMsg) || al::isMsgPlayerRouteDokanFireBallAttack(pMsg)))
        al::startHitReaction(this, "反応[ファイアボール]");
    else
        al::startHitReaction(this, "反応");
    al::setNerve(this, &NrvChorobonReaction);
    return true;
}

/** @brief Handles defeat, recoil, and Piranha Plant messages. @param pMsg Message. @param pOther Sender. @param pSelf Own sensor. @return Whether handled. */
bool Chorobon::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) {
    if (al::isNerve(this, &NrvChorobonVanish))
        return false;
    if (al::isMsgPlayerInvincibleAttack(pMsg) || al::isMsgPlayerGiantAttack(pMsg) || al::isMsgLaserAttack(pMsg)) {
        rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        rc::setAppearItemFactorByMsg(this, pMsg, pOther);
        rc::addScoreCombo(this, pOther, pMsg, 100.0f);
        al::setNerve(this, &NrvChorobonVanish);
        return true;
    }
    if (getSceneInfo()->isSingleMode) {
        if (rc::isMsgBobsledTrample(pMsg) || rc::isMsgBobsledBodyAttack(pMsg)) {
            rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
            rc::setAppearItemFactorByMsg(this, pMsg, pOther);
            rc::addScoreCombo(this, pOther, pMsg, 100.0f);
            al::setNerve(this, &NrvChorobonVanish);
            return true;
        }
        if (al::isMsgKeyThrow(pMsg) && tryReaction(pMsg))
            return false;
    }
    if ((EnemyStateUtil::isMsgBlowDown(pMsg) || EnemyStateUtil::isMsgRouteDokanAttack(pMsg)) && tryReaction(pMsg)) {
        if (al::isMsgPlayerFireBallAttack(pMsg) || al::isMsgPlayerRouteDokanFireBallAttack(pMsg)) {
            rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
            return true;
        }
        return false;
    }
    if (rc::isMsgPackunEatStart(pMsg))
        return true;
    if (rc::isMsgPackunEat(pMsg)) {
        rc::addScoreCombo(this, pOther, pMsg, 100.0f);
        kill();
        return true;
    }
    return false;
}

/** @brief Handles touch recoil. @param pMsg Message. @param pPointer Touch pointer. @param pTarget Touch target. @return Whether handled. */
bool Chorobon::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer, al::ScreenPointTarget* pTarget) {
    return al::isMsgTouchAssistAll(pMsg) && tryReaction(nullptr);
}

/** @brief Keeps the current movement animation. */
void Chorobon::exeMove() {}

/** @brief Applies a brief vertical recoil before resuming movement. */
void Chorobon::exeReaction() {
    if (al::isLessEqualStep(this, 3))
        return;
    if (al::isStep(this, 4))
        mRumble->start(0);
    if (al::isGreaterEqualStep(this, 30)) {
        al::setScaleY(this, 1.0f);
        al::setNerve(this, &NrvChorobonMove);
    } else {
        mRumble->calc();
        al::setScaleY(this, mRumble->getValueY() + 1.0f);
    }
}

/** @brief Plays the defeat animation and releases an item. */
void Chorobon::exeVanish() {
    if (al::isFirstStep(this))
        al::startAction(this, "Vanish");
    if (al::isActionEnd(this)) {
        al::startHitReactionDeath(this);
        al::appearItem(this);
        kill();
    }
}
