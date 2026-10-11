#include "Enemy/Teresa.hpp"

#include "Enemy/EnemyStateUtil.hpp"
#include "Enemy/FlyerStateChase.hpp"
#include "Enemy/FlyerStateChaseParam.hpp"
#include "Enemy/FlyerStateFindPlayer.hpp"
#include "Enemy/FlyerStateFindPlayerParam.hpp"
#include "Enemy/FlyerStateFunction.hpp"
#include "Enemy/FlyerStateParam.hpp"
#include "Enemy/FlyerStateReturnArea.hpp"
#include "Enemy/FlyerStateReturnAreaParam.hpp"
#include "Enemy/TargetFinder.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Item/ItemUtil.hpp"
#include "Library/Light/PrePassLightFunction.hpp"
#include "Library/LiveActor/ActorAreaFunction.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Thread/Functor.hpp"
#include "Project/AreaObj/AreaObjUtil.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ScoreUtil.hpp"
#include <attributes.h>
#include <gfx/seadColor.h>

namespace {
NERVE_DECL(Teresa, Appear)
NERVE_DECL(Teresa, Chase)
NERVE_DECL(Teresa, FindPlayer)
NERVE_DECL(Teresa, ReturnArea)
NERVE_DECL(Teresa, Down)
NERVE_DECL(Teresa, DisappearOutArea)
NERVE_DECL(Teresa, AttackHit)
NERVE_DECL(Teresa, ReactionWait)
NERVE_DECL(Teresa, ReactionAppear)
NERVE_DECL(Teresa, Reaction)
NERVE_DECL(Teresa, DamageLight)
NERVE_DECL(Teresa, Wait)
NERVE_DECL(Teresa, ShyStart)
NERVE_DECL(Teresa, ShyWait)
NERVE_DECL(Teresa, ShyEndSign)
NERVE_DECL(Teresa, ShyEnd)
// Non-const nerve objects: the game keeps them in .data in this order.
TeresaNrvAppear NrvTeresaAppear;
TeresaNrvChase NrvTeresaChase;
TeresaNrvFindPlayer NrvTeresaFindPlayer;
TeresaNrvReturnArea NrvTeresaReturnArea;
TeresaNrvDown NrvTeresaDown;
TeresaNrvDisappearOutArea NrvTeresaDisappearOutArea;
TeresaNrvAttackHit NrvTeresaAttackHit;
TeresaNrvReactionWait NrvTeresaReactionWait;
TeresaNrvReactionAppear NrvTeresaReactionAppear;
TeresaNrvReaction NrvTeresaReaction;
TeresaNrvDamageLight NrvTeresaDamageLight;
TeresaNrvWait NrvTeresaWait;
TeresaNrvShyStart NrvTeresaShyStart;
TeresaNrvShyWait NrvTeresaShyWait;
TeresaNrvShyEndSign NrvTeresaShyEndSign;
TeresaNrvShyEnd NrvTeresaShyEnd;

typedef al::FunctorV0M<Teresa*, void (Teresa::*)()> TeresaFunctor;

sead::Color4f sBodyLightColor(3.0f, 1.0f, 1.0f, 1.0f);
sead::Color4f sBodyBackLightColor(0.8f, 0.0f, 0.0f, 1.0f);
sead::Color4f sBlendColor(0.0f, 0.0f, 0.0f, 1.0f);
TargetFinderParam sTargetFinderParam(1500.0f, 180.0f, 90.0f, 0x7fffffff, -1.0f, -1.0f, -1.0f,
                                     2500.0f, false);
FlyerStateParam sFlyerStateParam(0.9f, 0.9f);
FlyerStateFindPlayerParam sFindPlayerParam(2.0f, const_cast<char*>("Find"));
FlyerStateChaseParam sChaseParam(0.6f, 600, 2.0f, 0.0f, 10, 2, "Chase", "Wait");
FlyerStateReturnAreaParam sReturnAreaParam(0.97f, 90, 3.0f, 0.35f, 30.0f, 300);

/**
 * @brief Starts an action on the actor and on its first sub actor.
 * @param pActor Actor to start the action on.
 * @param pActionName Action name.
 */
inline void startActionWithSubActor(al::LiveActor* pActor, const char* pActionName) {
    al::startAction(pActor, pActionName);
    al::startAction(al::getSubActor(pActor, 0), pActionName);
}

/**
 * @brief Deletes an effect if it is being emitted.
 * @param pKeeper Effect keeper owning the effect.
 * @param pEffectName Effect name.
 */
ALWAYS_INLINE inline void deleteEffectIfEmitting(al::IUseEffectKeeper* pKeeper,
                                                 const char* pEffectName) {
    if (al::isEffectEmitting(pKeeper, pEffectName)) {
        al::deleteEffect(pKeeper, pEffectName);
    }
}
}  // namespace

/**
 * @brief Constructs a Teresa.
 * @param pName Actor name.
 */
Teresa::Teresa(const char* pName) : al::LiveActor(pName) {}

/**
 * @brief Initializes the model (normal or big), nerves, stage switches, area group and states.
 * @param rInfo Placement info of the actor.
 */
void Teresa::init(const al::ActorInitInfo& rInfo) {
    const char* objectName = "Teresa";
    al::getObjectName(&objectName, rInfo);
    if (al::isEqualString(objectName, "TeresaBig")) {
        al::initActorWithArchiveName(this, rInfo, "TeresaBig", nullptr);
        mIsBig = true;
    } else {
        al::initActorWithArchiveName(this, rInfo, "Teresa", nullptr);
        mIsBig = false;
    }

    al::setMaterialProgrammable(this);
    al::initNerve(this, &NrvTeresaAppear, 3);
    startActionWithSubActor(this, "Appear");
    al::createRenderState(this);
    mInitTrans.set(al::getTrans(this));
    al::listenStageSwitchOnKill(this, TeresaFunctor(this, &Teresa::killBySwitch));
    if (al::listenStageSwitchOnOff(this, "SwitchAppear",
                                   TeresaFunctor(this, &Teresa::appearBySwitch),
                                   TeresaFunctor(this, &Teresa::disappearBySwitch))) {
        makeActorDead();
    } else {
        makeActorAppeared();
    }

    al::syncSensorScaleY(this);
    mLinkAreaGroup = al::createLinkAreaGroup(this, rInfo, "ChaseArea",
                                             "追いかけ有効エリアグループ", "子供エリア");
    mTargetFinder = new TargetFinder(this, &sTargetFinderParam);
    mStateChase = new FlyerStateChase(this, al::getFrontPtr(this), mTargetFinder,
                                      &sFlyerStateParam, &sChaseParam);
    mStateFindPlayer = new FlyerStateFindPlayer(this, al::getFrontPtr(this), mTargetFinder,
                                                &sFlyerStateParam, &sFindPlayerParam);
    mStateReturnArea = new FlyerStateReturnArea(this, al::getFrontPtr(this), al::getTrans(this),
                                                &sFlyerStateParam, &sReturnAreaParam);
    al::initNerveState(this, mStateChase, &NrvTeresaChase, "[state]飛行敵追いかけ");
    al::initNerveState(this, mStateFindPlayer, &NrvTeresaFindPlayer,
                       "[state]飛行敵プレイヤー発見");
    al::initNerveState(this, mStateReturnArea, &NrvTeresaReturnArea, "[state]エリアに戻る");
    mStateChase->_21 = true;
    mStateFindPlayer->_21 = true;
    mShadowIntensity = al::getShadowIntensity(this, "体影");
    al::setEffectFollowMtxPtr(this, "Shy", al::getJointMtxPtr(this, "Center"));
    al::setCustomRenderEnable(this, true);
}

/** @brief Dies (with item) when the kill stage switch turns on. */
void Teresa::killBySwitch() {
    rc::killBySwitchAndAppearItem(this, nullptr);
}

/** @brief Reappears at the initial position when the appear stage switch turns on. */
void Teresa::appearBySwitch() {
    if (al::isNerve(this, &NrvTeresaDown)) {
        return;
    }

    al::setRenderStateBlendColor(this, sBlendColor);
    mAlpha = 1.0f;
    al::setTrans(this, mInitTrans);
    startActionWithSubActor(this, "Appear");
    al::LiveActor::appear();
    al::setNerve(this, &NrvTeresaAppear);
}

/** @brief Fades out when the appear stage switch turns off. */
void Teresa::disappearBySwitch() {
    if (al::isNerve(this, &NrvTeresaDown) || al::isNerve(this, &NrvTeresaDisappearOutArea)) {
        return;
    }

    al::setNerve(this, &NrvTeresaDisappearOutArea);
}

/** @brief Kills the actor with the death hit reaction. */
void Teresa::kill() {
    al::startHitReactionDeath(this);
    al::LiveActor::kill();
}

/** @brief Updates the shadow intensities from the alpha and counts down the reaction timer. */
void Teresa::control() {
    mIsTouchAssist = false;
    al::setShadowIntensityUser(this, mShadowIntensity * 255.0f * mAlpha, "体影");
    if (mIsBig) {
        al::setShadowIntensityUser(this, mShadowIntensity * 255.0f * mAlpha, "右腕");
        al::setShadowIntensityUser(this, mShadowIntensity * 255.0f * mAlpha, "左腕");
        al::setShadowIntensityUser(this, mShadowIntensity * 255.0f * mAlpha, "しっぽ");
        al::setShadowIntensityUser(this, mShadowIntensity * 255.0f * mAlpha, "tongue");
    }

    if (mDisappearReactionTime > 0) {
        mDisappearReactionTime--;
    }
}

/**
 * @brief Pushes other enemies and attacks the player.
 * @param pSelf Own sensor.
 * @param pOther Other sensor.
 */
void Teresa::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isNerve(this, &NrvTeresaAppear) || al::isNerve(this, &NrvTeresaDown) ||
        al::isNerve(this, &NrvTeresaReaction) || al::isNerve(this, &NrvTeresaReactionWait) ||
        al::isNerve(this, &NrvTeresaReactionAppear) ||
        al::isNerve(this, &NrvTeresaDisappearOutArea)) {
        return;
    }

    if (al::isSensorEnemyBody(pSelf) && al::isSensorEnemyBody(pOther)) {
        al::sendMsgPushAndKillVelocityToTarget(this, pSelf, pOther);
    }

    if (al::isSensorEnemyAttack(pSelf) && al::isSensorPlayer(pOther) &&
        al::sendMsgEnemyAttack(pOther, pSelf)) {
        al::setNerve(this, &NrvTeresaAttackHit);
    }
}

/**
 * @brief Handles restore, player attacks, ghost blow downs, light flashes and pushes.
 * @param pMsg Received message.
 * @param pOther Sensor of the sender.
 * @param pSelf Own sensor.
 * @return Whether the message was handled.
 */
bool Teresa::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) {
    if (al::isMsgRestore(pMsg)) {
        makeActorAppeared();
        al::setTrans(this, mInitTrans);
        al::setNerve(this, &NrvTeresaAppear);
        startActionWithSubActor(this, "Appear");
        return true;
    }

    if (!al::isNerve(this, &NrvTeresaAppear) && !al::isNerve(this, &NrvTeresaDown)) {
        if (al::isMsgPlayerInvincibleAttack(pMsg) || al::isMsgPlayerGiantAttack(pMsg) ||
            al::isMsgLaserAttack(pMsg)) {
            rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
            rc::addScoreCombo(this, pOther, pMsg, 100.0f);
            al::setAppearItemFactor(this, "直接攻撃", pOther);
            al::setNerve(this, &NrvTeresaDown);
            return true;
        }

        if (EnemyStateUtil::isMsgBlowDownForGhost(pMsg)) {
            if (al::isNerve(this, &NrvTeresaReactionWait) ||
                al::isNerve(this, &NrvTeresaReactionAppear)) {
                al::setNerve(this, &NrvTeresaReactionWait);
                if (mDisappearReactionTime == 0) {
                    al::startHitReactionDisappear(this);
                    mDisappearReactionTime = 40;
                    return false;
                }
            } else {
                al::setNerve(this, &NrvTeresaReaction);
            }
        } else if (al::isMsgGoalKill(pMsg)) {
            rc::addScoreCombo(this, pOther, pMsg, 100.0f);
            al::setAppearItemFactor(this, "直接攻撃", pOther);
            al::setNerve(this, &NrvTeresaDown);
            return true;
        } else if (!al::isNerve(this, &NrvTeresaReaction) &&
                   !al::isNerve(this, &NrvTeresaReactionWait) &&
                   !al::isNerve(this, &NrvTeresaReactionAppear) &&
                   !al::isNerve(this, &NrvTeresaDisappearOutArea)) {
            if (al::isMsgLightFlash(pMsg) || al::isMsgHeadlightFlash(pMsg)) {
                mIsHeadlightFlash = al::isMsgHeadlightFlash(pMsg);
                if (!al::isNerve(this, &NrvTeresaDamageLight)) {
                    al::setNerve(this, &NrvTeresaDamageLight);
                    mLightSensor = pOther;
                }

                mIsReceiveLight = true;
                return true;
            }

            if (!al::isNerve(this, &NrvTeresaShyStart) && !al::isNerve(this, &NrvTeresaShyWait) &&
                !al::isNerve(this, &NrvTeresaShyEndSign) && !al::isNerve(this, &NrvTeresaShyEnd) &&
                al::tryReceiveMsgPushAndAddVelocity(this, pMsg, pOther, pSelf, 1.0f)) {
                return true;
            }
        }
    }

    return false;
}

/**
 * @brief Reacts to the touch assist (touch screen) by hiding.
 * @param pMsg Received message.
 * @param pPointer Screen pointer.
 * @param pTarget Own screen point target.
 * @return Whether the message was handled.
 */
bool Teresa::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                   al::ScreenPointTarget* pTarget) {
    if (al::isNerve(this, &NrvTeresaDown)) {
        return false;
    }

    if (al::isMsgTouchAssist(pMsg)) {
        mIsTouchAssist = true;
        if (!al::isNerve(this, &NrvTeresaReaction) &&
            !al::isNerve(this, &NrvTeresaReactionWait) &&
            !al::isNerve(this, &NrvTeresaReactionAppear) &&
            !al::isNerve(this, &NrvTeresaDisappearOutArea)) {
            al::setNerve(this, &NrvTeresaReaction);
        }

        return true;
    }

    return false;
}

/** @brief Waits for the appear action to end. */
void Teresa::exeAppear() {
    if (al::isActionEnd(this)) {
        changeNerveByTarget(mTargetFinder, mLinkAreaGroup);
    }
}

/**
 * @brief Picks the next nerve (chase, return to the area or wait) from the target and the area.
 * @param pTargetFinder Target finder.
 * @param pArea Area the actor is limited to, or nullptr.
 */
void Teresa::changeNerveByTarget(TargetFinder* pTargetFinder, al::AreaObjGroup* pArea) {
    pTargetFinder->refindTarget();
    if (pArea != nullptr) {
        if (!al::tryIsInAreaObj(pArea, al::getTrans(this))) {
            al::setNerve(this, &NrvTeresaReturnArea);
        } else if (!pTargetFinder->isFoundTargetNow()) {
            al::setNerve(this, &NrvTeresaWait);
        } else if (al::tryIsInAreaObj(pArea, al::getTrans(pTargetFinder->getTarget()))) {
            al::setNerve(this, &NrvTeresaChase);
        } else {
            al::setNerve(this, &NrvTeresaReturnArea);
        }
    } else if (pTargetFinder->isFoundTargetNow()) {
        al::setNerve(this, &NrvTeresaChase);
    } else {
        al::setNerve(this, &NrvTeresaWait);
    }
}

/** @brief Hovers in place until a target in the area is found. */
void Teresa::exeWait() {
    FlyerStateFunction::recoverHeight(this, mInitTrans.y, &sFlyerStateParam);
    if (al::isFirstStep(this) && !al::isActionPlaying(this, "Wait")) {
        startActionWithSubActor(this, "Wait");
    }

    al::setVelocityZero(this);
    mTargetFinder->update();
    if (mTargetFinder->isFoundTargetNow()) {
        al::AreaObjGroup* area = mLinkAreaGroup;
        if (area == nullptr || al::tryIsInAreaObj(area, mTargetFinder->getTargetPos())) {
            al::setNerve(this, &NrvTeresaChase);
        }
    }
}

/** @brief Updates the find player state, then chases or waits. */
void Teresa::exeFindPlayer() {
    FlyerStateFunction::recoverHeight(this, mInitTrans.y, &sFlyerStateParam);
    if (al::updateNerveState(this)) {
        if (mTargetFinder->isFoundTargetNow()) {
            al::setNerve(this, &NrvTeresaChase);
        } else {
            al::setNerve(this, &NrvTeresaWait);
        }
    }
}

/**
 * @brief Checks whether the actor and a living player are in each other's sight fan.
 * @param pActor Teresa actor.
 * @return Whether the actor and a player see each other.
 */
ALWAYS_INLINE inline bool isSeeEachOtherPlayer(al::LiveActor* pActor) {
    s32 playerNum = al::getPlayerNumMax(pActor);
    for (s32 i = 0; i < playerNum; i++) {
        al::LiveActor* player = al::getPlayerActor(pActor, i);
        if (rc::isPlayerDeadOrBubble(player)) {
            continue;
        }

        sead::Vector3f playerFront;
        al::calcFrontDir(&playerFront, player);
        if (al::isInSightFan(pActor, al::getTrans(player), al::getFront(pActor),
                             sTargetFinderParam._1C, 60.0f, 80.0f) &&
            al::isInSightFan(player, al::getTrans(pActor), playerFront, sTargetFinderParam._1C,
                             60.0f, 80.0f)) {
            return true;
        }
    }

    return false;
}

/** @brief Chases the target, returns to the area when leaving it and hides when looked at. */
void Teresa::exeChase() {
    if (al::isFirstStep(this)) {
        mStateChase->_20 = true;
    }

    FlyerStateFunction::recoverHeight(this, mInitTrans.y, &sFlyerStateParam);
    if (al::updateNerveState(this)) {
        changeNerveByTarget(mTargetFinder, mLinkAreaGroup);
        return;
    }

    al::AreaObjGroup* area = mLinkAreaGroup;
    if (area != nullptr && !al::tryIsInAreaObj(area, al::getTrans(this))) {
        if (!mTargetFinder->isFoundTarget() ||
            !al::tryIsInAreaObj(mLinkAreaGroup, al::getTrans(mTargetFinder->getTarget()))) {
            al::setNerve(this, &NrvTeresaReturnArea);
            return;
        }
    }

    if (mTargetFinder->isFoundTargetNow() && isSeeEachOtherPlayer(this)) {
        al::setNerve(this, &NrvTeresaShyStart);
    }
}

/** @brief Flies back into the area, or looks for the player again when they enter it. */
void Teresa::exeReturnArea() {
    if (al::isFirstStep(this) && !al::isActionPlaying(this, "Wait")) {
        startActionWithSubActor(this, "Wait");
    }

    mTargetFinder->update();
    if (mTargetFinder->isFoundTarget() &&
        al::tryIsInAreaObj(mLinkAreaGroup, al::getTrans(mTargetFinder->getTarget()))) {
        al::setNerve(this, &NrvTeresaFindPlayer);
        return;
    }

    if (al::updateNerveState(this)) {
        changeNerveByTarget(mTargetFinder, mLinkAreaGroup);
    }
}

/** @brief Starts hiding its face from the player. */
void Teresa::exeShyStart() {
    if (al::isFirstStep(this)) {
        startActionWithSubActor(this, "ShyStart");
        al::LiveActor* target = mTargetFinder->getTarget();
        if (target != nullptr) {
            al::faceToTarget(this, al::getTrans(target));
        }

        al::emitEffect(this, "Shy", nullptr);
        al::setVelocityZero(this);
    }

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvTeresaShyWait);
    }
}

/** @brief Hides its face while the player looks at it. */
void Teresa::exeShyWait() {
    FlyerStateFunction::recoverHeight(this, mInitTrans.y, &sFlyerStateParam);
    if (al::isFirstStep(this)) {
        startActionWithSubActor(this, "ShyWait");
    }

    al::requestPrePassLightColor(this, "テレサ体", sBodyLightColor);
    al::requestPrePassLightColor(this, "テレサ体裏面", sBodyBackLightColor);
    mTargetFinder->update();
    if (!isPlayerSight() && al::isGreaterEqualStep(this, 10)) {
        al::setNerve(this, &NrvTeresaShyEndSign);
    }
}

/**
 * @brief Checks whether the target is in the area and a player looks at the actor.
 * @return Whether a player sees the actor.
 */
bool Teresa::isPlayerSight() {
    if (!mTargetFinder->isFoundTargetNow()) {
        return false;
    }

    al::AreaObjGroup* area = mLinkAreaGroup;
    if (area != nullptr && !al::tryIsInAreaObj(area, al::getTrans(mTargetFinder->getTarget()))) {
        return false;
    }

    return isSeeEachOtherPlayer(this);
}

/** @brief Peeks out, hiding again when the player looks. */
void Teresa::exeShyEndSign() {
    if (al::isFirstStep(this)) {
        startActionWithSubActor(this, "ShyEndSign");
    }

    mTargetFinder->update();
    if (isPlayerSight()) {
        al::setNerve(this, &NrvTeresaShyWait);
        return;
    }

    if (al::isGreaterEqualStep(this, 25)) {
        al::setNerve(this, &NrvTeresaShyEnd);
    }
}

/** @brief Stops hiding its face. */
void Teresa::exeShyEnd() {
    if (al::isFirstStep(this)) {
        startActionWithSubActor(this, "ShyEnd");
        deleteShyEffect();
    }

    if (al::isActionEnd(this)) {
        changeNerveByTarget(mTargetFinder, mLinkAreaGroup);
    }
}

/** @brief Deletes the shy effect if it is being emitted. */
void Teresa::deleteShyEffect() {
    deleteEffectIfEmitting(this, "Shy");
}

/** @brief Stunned by a light flash; dies after being lit long enough. */
void Teresa::exeDamageLight() {
    FlyerStateFunction::recoverHeight(this, mInitTrans.y, &sFlyerStateParam);
    if (al::isFirstStep(this)) {
        startActionWithSubActor(this, "DamageLight");
        deleteShyEffect();
    }

    if (!mIsReceiveLight) {
        changeNerveByTarget(mTargetFinder, mLinkAreaGroup);
        return;
    }

    s32 damageStep = mIsHeadlightFlash ? 120 : 60;
    al::setVelocityZero(this);
    mIsReceiveLight = false;
    if (al::isGreaterEqualStep(this, damageStep)) {
        al::setAppearItemFactor(this, "間接攻撃", nullptr);
        al::setNerve(this, &NrvTeresaDown);
        rc::addScore(this, mLightSensor, 0.0f, 0);
    }
}

/** @brief Fades out after being blown by a ghost blow down or touched. */
void Teresa::exeReaction() {
    FlyerStateFunction::recoverHeight(this, mInitTrans.y, &sFlyerStateParam);
    if (al::isFirstStep(this)) {
        al::startHitReactionDisappear(this);
        al::setVelocityZero(this);
        deleteEffectIfEmitting(this, "Glow");
        if (!mIsBig) {
            deleteEffectIfEmitting(this, "Track");
        }

        deleteShyEffect();
        if (mIsBig) {
            al::setEnableDepthTest(this, false);
            al::setEnableDepthWrite(this, false);
        }
    }

    mAlpha = al::lerpValue(al::calcNerveRate(this, 0), sBlendColor.a, 0.0f);
    al::setRenderStateBlendColor(
        this, sead::Color4f(sBlendColor.r, sBlendColor.g, sBlendColor.b, mAlpha));
    if (al::isGreaterEqualStep(this, 0)) {
        al::setNerve(this, &NrvTeresaReactionWait);
    }
}

/** @brief Stays invisible until it is no longer touched. */
void Teresa::exeReactionWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait");
        al::setRenderStateBlendColor(
            this, sead::Color4f(sBlendColor.r, sBlendColor.g, sBlendColor.b, 0.0f));
        deleteEffectIfEmitting(this, "Glow");
        if (al::isSklAnimExist(this) && al::isSklAnimPlaying(this, 0)) {
            al::setSklAnimFrameRate(this, 0.0f, 0);
        }
    }

    if (!mIsTouchAssist && al::isGreaterEqualStep(this, 10)) {
        if (al::isSklAnimExist(this) && al::isSklAnimPlaying(this, 0)) {
            al::setSklAnimFrameRate(this, 1.0f, 0);
        }

        al::setNerve(this, &NrvTeresaReactionAppear);
    }
}

/** @brief Fades back in. */
void Teresa::exeReactionAppear() {
    if (al::isFirstStep(this)) {
        al::setVelocityZero(this);
        if (mIsBig) {
            al::setEnableDepthTest(this, true);
            al::setEnableDepthWrite(this, true);
        }
    }

    mAlpha = al::lerpValue(al::calcNerveRate(this, 20), 0.0f, sBlendColor.a);
    al::setRenderStateBlendColor(
        this, sead::Color4f(sBlendColor.r, sBlendColor.g, sBlendColor.b, mAlpha));
    if (al::isGreaterEqualStep(this, 20)) {
        al::startHitReactionAppear(this);
        al::resetRenderState(this);
        changeNerveByTarget(mTargetFinder, mLinkAreaGroup);
    }
}

/** @brief Fades out and dies (stage switch off). */
void Teresa::exeDisappearOutArea() {
    if (al::isFirstStep(this)) {
        al::startHitReactionDisappear(this);
        deleteShyEffect();
    }

    f32 alpha = al::lerpValue(al::calcNerveRate(this, 60), sBlendColor.a, 0.0f);
    al::setRenderStateBlendColor(
        this, sead::Color4f(sBlendColor.r, sBlendColor.g, sBlendColor.b, alpha));
    al::setShadowIntensityUser(this, alpha * mShadowIntensity, "体影");
    if (al::isGreaterEqualStep(this, 60)) {
        al::LiveActor::kill();
    }
}

/** @brief Plays the attack hit action facing the target. */
void Teresa::exeAttackHit() {
    if (al::isFirstStep(this)) {
        startActionWithSubActor(this, "AttackHit");
        deleteShyEffect();
        mTargetFinder->refindTarget();
        al::LiveActor* target = mTargetFinder->getTarget();
        if (target != nullptr) {
            al::faceToTarget(this, al::getTrans(target));
        }
    }

    al::setVelocityZero(this);
    if (al::isActionEnd(this)) {
        changeNerveByTarget(mTargetFinder, mLinkAreaGroup);
    }
}

/** @brief Plays the down action, then drops its item and dies. */
void Teresa::exeDown() {
    if (al::isFirstStep(this)) {
        startActionWithSubActor(this, "Down");
        deleteShyEffect();
    }

    al::setVelocityZero(this);
    if (al::isActionEnd(this)) {
        al::appearItem(this);
        kill();
    }
}
