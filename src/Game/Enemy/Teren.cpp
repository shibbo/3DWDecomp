#include "Enemy/Teren.hpp"
#include <cmath>
#include <gfx/seadColor.h>
#include "Enemy/ActorRailBrakeMover.hpp"
#include "Enemy/EnemyStateUtil.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Rail/RailUtil.hpp"
#include "Library/StageSwitch/StageSwitchFunc.hpp"
#include "Library/Thread/Functor.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/ScoreUtil.hpp"
namespace {
NERVE_DECL(Teren, Move)
NERVE_DECL(Teren, Appear)
NERVE_DECL(Teren, Die)
NERVE_DECL(Teren, DamageLight)
NERVE_DECL(Teren, Reaction)
NERVE_DECL(Teren, ReactionWait)
NERVE_DECL(Teren, ReactionAppear)
NERVES_MAKE_NOSTRUCT(Teren, Move, Appear, Die, DamageLight, Reaction, ReactionWait, ReactionAppear)
const sead::Color4f sTerenBlendColor(0.0f, 0.0f, 0.0f, 1.0f);
}
/** @brief Creates the floating ghost. @param pName Actor name. */
Teren::Teren(const char* pName) : al::LiveActor(pName) {}
/** @brief Initializes the model, rail movement, shadows, and switches. @param rInfo Placement information. */
void Teren::init(const al::ActorInitInfo& rInfo) {
    bool invalidSilhouette = false;
    al::tryGetArg(&invalidSilhouette, rInfo, "IsInvalidSilhouette");
    al::initActorWithArchiveName(this, rInfo, "Teren", invalidSilhouette ? "NoSilhouette" : nullptr);
    al::initNerve(this, &NrvTerenMove, 0);
    al::createRenderState(this);
    al::startAction(this, "Wait");
    if (getSubActorKeeper()) al::startAction(al::getSubActor(this, 0), "Wait");
    al::setMaterialProgrammable(this);
    if (al::isExistRail(this)) {
        al::setSyncRailToNearestPos(this);
        mRailMover = new ActorRailBrakeMover(this, 5.0f, nullptr);
    }
    mInitialPos.set(al::getTrans(this));
    s32 shadowLength = -1;
    if (al::tryGetArg(&shadowLength, rInfo, "ShadowLength") && shadowLength > 0) al::setShadowDropLength(this, static_cast<f32>(shadowLength), "シャドウマスク");
    al::trySyncStageSwitchAppear(this);
    al::listenStageSwitchOnKill(this, al::FunctorV0M<Teren*, void (Teren::*)()>(this, &Teren::killBySwitch));
    al::setCustomRenderEnable(this, true);
}
/** @brief Kills the actor when its switch activates. */
void Teren::killBySwitch() { if (!al::isDead(this)) kill(); }
/** @brief Moves along the rail and adds a vertical floating cycle. */
void Teren::control() {
    if (!al::isNerve(this, &NrvTerenAppear) && !al::isNerve(this, &NrvTerenDie) && al::isExistRail(this) && mRailMover->moveSyncRailBrake()) al::reverseRail(this);
    sead::Vector3f pos = al::isExistRail(this) ? al::getRailPos(this) : mInitialPos;
    pos.y += sinf(al::normalize(static_cast<f32>(mFloatStep), 0.0f, 60.0f) * 360.0f * 0.017453292f) * 15.0f;
    al::setTrans(this, pos);
    if (al::isExistRail(this)) {
        f32 y = al::getRailDir(this).y;
        if ((y > 0.0f ? y : -y) > 0.4f) return;
    }
    mFloatStep = al::modi(mFloatStep + 61, 60);
}
/** @brief Starts the appearance animation on the actor and its subactor. */
void Teren::appear() {
    al::LiveActor::appear();
    al::setNerve(this, &NrvTerenAppear);
    al::startAction(this, "Appear");
    if (getSubActorKeeper()) al::startAction(al::getSubActor(this, 0), "Appear");
    al::hideModelIfShow(this);
}
/** @brief Plays the death reaction and removes the ghost. */
void Teren::kill() { al::startHitReactionDeath(this); al::LiveActor::kill(); }
/** @brief Damages players while the ghost is visible. @param pSelf Own sensor. @param pOther Contact sensor. */
void Teren::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isSensorEnemyAttack(pSelf) && (al::isNerve(this, &NrvTerenMove) || al::isNerve(this, &NrvTerenDamageLight)) && al::isSensorPlayer(pOther)) al::sendMsgEnemyAttackForCrossoverSensor(pOther, pSelf);
}
/** @brief Handles lethal attacks, light exposure, and ghost reactions. @param pMsg Message. @param pOther Sender. @param pSelf Own sensor. @return Whether handled. */
bool Teren::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) {
    if (!al::isSensorEnemyBody(pSelf) || al::isNerve(this, &NrvTerenDie)) return false;
    if (al::isMsgGoalKill(pMsg)) {
        al::startHitReactionDeath(this);
        rc::addScoreCombo(this, pOther, pMsg, 100.0f);
        rc::setAppearItemFactorByMsg(this, pMsg, pOther);
        al::appearItem(this);
        kill();
        return true;
    }
    if (al::isMsgPlayerInvincibleAttack(pMsg) || al::isMsgPlayerGiantAttack(pMsg) || al::isMsgLaserAttack(pMsg)) {
        rc::addScoreCombo(this, pOther, pMsg, 100.0f);
        rc::setAppearItemFactorByMsg(this, pMsg, pOther);
        al::setNerve(this, &NrvTerenDie);
        return true;
    }
    if (al::isMsgLightFlash(pMsg) || al::isMsgHeadlightFlash(pMsg)) {
        mLightTimer = 3;
        if (al::isNerve(this, &NrvTerenDamageLight)) {
            if (al::isGreaterEqualStep(this, al::isMsgHeadlightFlash(pMsg) ? 120 : 60)) {
                rc::setAppearItemFactorByMsg(this, pMsg, pOther);
                rc::addScore(this, pOther, 100.0f, 0);
                al::appearItem(this);
                kill();
            }
        } else {
            if (al::isNerve(this, &NrvTerenReaction) || al::isNerve(this, &NrvTerenReactionWait) || al::isNerve(this, &NrvTerenReactionAppear)) {
                al::startHitReactionAppear(this);
                al::emitEffect(this, "Glow", nullptr);
                al::setShadowIntensityUser(this, 255, "シャドウマスク");
                al::setRenderStateBlendColor(this, sTerenBlendColor);
                al::resetRenderState(this);
            }
            al::setNerve(this, &NrvTerenDamageLight);
        }
        return true;
    }
    if ((al::isNerve(this, &NrvTerenAppear) || al::isNerve(this, &NrvTerenMove) || al::isNerve(this, &NrvTerenReactionAppear)) && EnemyStateUtil::isMsgBlowDownForGhost(pMsg)) al::setNerve(this, &NrvTerenReaction);
    return false;
}
/** @brief Handles touch reactions and extends the hidden state. @param pMsg Message. @param pPointer Pointer. @param pTarget Touch target. @return Whether handled. */
bool Teren::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer, al::ScreenPointTarget* pTarget) {
    if (al::isMsgTouchAssist(pMsg) && al::isNerve(this, &NrvTerenReactionWait) && al::isLessEqualStep(this, 5)) return true;
    if (al::isNerve(this, &NrvTerenAppear) || al::isNerve(this, &NrvTerenMove) || al::isNerve(this, &NrvTerenReactionWait) || al::isNerve(this, &NrvTerenReactionAppear)) {
        if (!al::isMsgTouchAssist(pMsg)) return false;
        if (al::isNerve(this, &NrvTerenReactionWait)) al::setNerve(this, &NrvTerenReactionWait);
        else al::setNerve(this, &NrvTerenReaction);
        return true;
    }
    return false;
}
/** @brief Reveals the model and waits for the appearance animation. */
void Teren::exeAppear() {
    if (al::isFirstStep(this)) al::showModelIfHide(this);
    al::setNerveAtActionEnd(this, &NrvTerenMove);
}
/** @brief Ensures the floating animation is playing. */
void Teren::exeMove() { if (al::isFirstStep(this)) al::tryStartActionIfNotPlaying(this, "Wait"); }
/** @brief Fades the ghost out after a hit. */
void Teren::exeReaction() {
    if (al::isFirstStep(this)) {
        al::startHitReactionDisappear(this);
        al::tryDeleteEffect(this, "Glow");
        al::setShadowIntensityUser(this, 0, "シャドウマスク");
    }
    f32 alpha = al::lerpValue(al::calcNerveRate(this, 0), 1.0f, 0.0f);
    al::setRenderStateBlendColor(this, sead::Color4f(0.0f, 0.0f, 0.0f, alpha));
    if (al::isGreaterEqualStep(this, 0)) al::setNerve(this, &NrvTerenReactionWait);
}
/** @brief Holds the ghost invisible briefly. */
void Teren::exeReactionWait() { if (al::isGreaterEqualStep(this, 10)) al::setNerve(this, &NrvTerenReactionAppear); }
/** @brief Restores opacity, glow, and shadow over sixty frames. */
void Teren::exeReactionAppear() {
    f32 alpha = al::lerpValue(al::calcNerveRate(this, 60), 0.0f, 1.0f);
    al::setRenderStateBlendColor(this, sead::Color4f(0.0f, 0.0f, 0.0f, alpha));
    al::setShadowIntensityUser(this, static_cast<s32>(al::calcNerveValue(this, 60, 0.0f, 255.0f)), "シャドウマスク");
    if (al::isGreaterEqualStep(this, 60)) {
        al::startHitReactionAppear(this);
        al::emitEffect(this, "Glow", nullptr);
        al::setShadowIntensityUser(this, 255, "シャドウマスク");
        al::setRenderStateBlendColor(this, sTerenBlendColor);
        al::resetRenderState(this);
        al::setNerve(this, &NrvTerenMove);
    }
}
/** @brief Reacts to sustained light and recovers when exposure ends. */
void Teren::exeDamageLight() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "DamageLight");
        if (getSubActorKeeper()) al::startAction(al::getSubActor(this, 0), "DamageLight");
    }
    if (mLightTimer-- <= 0) {
        al::startAction(this, "Wait");
        if (getSubActorKeeper()) al::startAction(al::getSubActor(this, 0), "Wait");
        al::setNerve(this, &NrvTerenMove);
    }
}
/** @brief Plays the death animation and releases the item. */
void Teren::exeDie() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Die");
        if (getSubActorKeeper()) al::startAction(al::getSubActor(this, 0), "Die");
    }
    if (al::isActionEnd(this)) { al::appearItem(this); kill(); }
}
