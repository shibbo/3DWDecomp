#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Boss/BunbunStateShellAttack.hpp"
#include "Boss/Bunbun.hpp"
#include "Boss/BunbunChameleon.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/ActorParamHolderUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Util/ProjectMsgUtil.hpp"
namespace {
NERVE_DECL(BunbunStateShellAttack, ChangeShell);
NERVE_DECL(BunbunStateShellAttack, AttackSign);
NERVE_DECL(BunbunStateShellAttack, AttackStart);
NERVE_DECL(BunbunStateShellAttack, Attack);
NERVE_DECL(BunbunStateShellAttack, AttackEnd);
NERVE_DECL(BunbunStateShellAttack, RecoverShellStart);
NERVE_DECL(BunbunStateShellAttack, RecoverShell);
NERVES_MAKE_NOSTRUCT(BunbunStateShellAttack, AttackSign, AttackStart, Attack, AttackEnd)
NERVES_MAKE_STRUCT(BunbunStateShellAttack, ChangeShell, RecoverShellStart, RecoverShell)
} // namespace
struct BunbunShellMoveParam {
    explicit BunbunShellMoveParam(al::LiveActor *pHost) {
        start = al::findActorParamMove(pHost, "ShellAttackStart");
        attack = al::findActorParamMove(pHost, "ShellAttack");
        fast = al::findActorParamMove(pHost, "ShellAttackFast");
        end = al::findActorParamMove(pHost, "ShellAttackEnd");
    }
    const al::ActorParamMove *start;
    const al::ActorParamMove *attack;
    const al::ActorParamMove *fast;
    const al::ActorParamMove *end;
};
/** @brief Creates the shell models and movement parameters. @param pHost Boss actor. @param rInfo
 * Initialization data. */
BunbunStateShellAttack::BunbunStateShellAttack(al::LiveActor *pHost, const al::ActorInitInfo &rInfo)
    : ActorStateBase("コウラアタックステート", pHost) {
    mIsSingleMode = rInfo.getActorSceneInfo().isSingleMode;
    mBunbun = static_cast<Bunbun *>(mHostActor);
    mTransparent = mBunbun->isEnableTransparent();
    mShell = new al::LiveActor("コウラ");
    al::initActorWithArchiveNameNoPlacementInfo(mShell, rInfo, "BunbunShell", nullptr);
    mShell->makeActorDead();
    if (mTransparent) {
        mChameleon = new BunbunChameleon("透明ブンブン[コウラ]");
        al::initActorWithArchiveNameNoPlacementInfo(mChameleon, rInfo, "BunbunShellChameleon",
                                                    nullptr);
        mChameleon->makeActorDead();
    }
    mParams = new BunbunShellMoveParam(pHost);
    mAttackParam = mTransparent ? mParams->attack : mParams->fast;
    initNerve(&NrvBunbunStateShellAttack.ChangeShell, 0);
}
/** @brief Destroys the shell attack state. */
BunbunStateShellAttack::~BunbunStateShellAttack() = default;
/** @brief Resets the shell attack and hit cooldown. */
void BunbunStateShellAttack::appear() {
    mIsDead = false;
    al::invalidateClipping(mShell);
    mHitCooldown = 0;
    al::setNerve(this, &NrvBunbunStateShellAttack.ChangeShell);
}
/** @brief Marks the shell attack complete. */
void BunbunStateShellAttack::kill() { mIsDead = true; }
/** @brief Counts down the player-hit cooldown. */
void BunbunStateShellAttack::control() { mHitCooldown = mHitCooldown > 0 ? mHitCooldown - 1 : 0; }
/** @brief Handles player hits, shell reflections, and projectiles. @return Whether handled. */
bool BunbunStateShellAttack::receiveMsg(const al::SensorMsg *pMsg, al::HitSensor *pOther,
                                        al::HitSensor *pSelf) {
    bool koura =
        mIsSingleMode && (al::isMsgPlayerKouraAttack(pMsg) || al::isMsgKickKouraAttack(pMsg));
    if ((al::isMsgPlayerTrample(pMsg) || al::isMsgPlayerObjHipDropReflectAll(pMsg) ||
         al::isMsgPlayerBodyAttackReflect(pMsg) || al::isMsgPlayerTailAttack(pMsg) ||
         al::isMsgPlayerClimbAttack(pMsg) || al::isMsgPlayerSpinAttack(pMsg) || koura) &&
        mHitCooldown == 0) {
        *al::getVelocityPtr(mHostActor) *= 0.9f;
        auto *velocity = al::getVelocityPtr(mHostActor);
        *velocity += sead::Vector3f::ey * 40.0f;
        mHitCooldown = 10;
        if (koura)
            al::sendMsgKickKouraReflect(pOther, pSelf);
        auto *host = mHostActor;
        if (!al::isNerve(host, &NrvBunbunStateShellAttack.ChangeShell) &&
            !al::isNerve(host, &NrvBunbunStateShellAttack.RecoverShellStart) &&
            !al::isNerve(host, &NrvBunbunStateShellAttack.RecoverShell))
            al::startSe(mHostActor, "PgTrampledShell", nullptr);
        return true;
    }
    if (al::isMsgPlayerFireBallAttack(pMsg)) {
        rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        return true;
    }
    return al::isMsgPlayerBoomerangReflect(pMsg) || al::isMsgExplosion(pMsg);
}
inline void BunbunStateShellAttack::walkToPlayer(const al::ActorParamMove *p) {
    al::walkAndTurnToPlayer(mBunbun, p->moveAccel, p->gravity, p->moveFriction, p->turnSpeedDegree,
                            false);
}
inline void BunbunStateShellAttack::walkForward() {
    auto *host = mBunbun;
    const auto &front = al::getFront(host);
    const auto *p = mParams->end;
    al::walkAndTurnToDirection(host, front, p->moveAccel, p->gravity, p->moveFriction,
                               p->turnSpeedDegree, false);
}
/** @brief Transforms the boss into its shell. */
void BunbunStateShellAttack::exeChangeShell() {
    if (al::isFirstStep(this)) {
        al::startAction(mBunbun, "ChangeShell");
        al::setTrans(mShell, al::getTrans(mBunbun));
    }
    walkToPlayer(mParams->start);
    al::setTrans(mShell, al::getTrans(mBunbun));
    if (al::isActionEnd(mBunbun)) {
        mShell->appear();
        al::setColliderRadius(mBunbun, 80);
        al::setColliderOffsetY(mBunbun, 80);
        al::setNerve(this, &NrvBunbunStateShellAttackAttackSign);
    }
}
/** @brief Checks whether the low-health transparent form is enabled. @return Whether transparent.
 */
bool BunbunStateShellAttack::isEnableTransparent() const {
    return mTransparent && mBunbun->getHp() < 2;
}
/** @brief Hides the boss and plays the shell warning. */
void BunbunStateShellAttack::exeAttackSign() {
    if (al::isFirstStep(this)) {
        al::startAction(mShell, "SpinSign");
        al::hideModel(mBunbun);
    }
    walkToPlayer(mParams->start);
    al::setTrans(mShell, al::getTrans(mBunbun));
    if (al::isActionEnd(mShell)) {
        if (isEnableTransparent())
            al::setTrans(mChameleon, al::getTrans(mBunbun));
        al::setNerve(this, &NrvBunbunStateShellAttackAttackStart);
    }
}
/** @brief Starts spinning, using the transparent shell when enabled. */
void BunbunStateShellAttack::exeAttackStart() {
    if (al::isFirstStep(this)) {
        if (isEnableTransparent()) {
            al::hideModelIfShow(mShell);
            mChameleon->appear();
            al::invalidateClipping(mChameleon);
            al::startAction(mChameleon, "SpinStart");
            al::startSe(mBunbun, "PgInvisible", nullptr);
        } else
            al::startAction(mShell, "SpinStart");
    }
    walkToPlayer(mParams->start);
    al::setTrans(mShell, al::getTrans(mBunbun));
    if (isEnableTransparent())
        al::setTrans(mChameleon, al::getTrans(mBunbun));
    if (al::isActionEnd(mShell))
        al::setNerve(this, &NrvBunbunStateShellAttackAttack);
}
/** @brief Chases the player, reflects from walls, and ends the attack. */
void BunbunStateShellAttack::exeAttack() {
    if (al::isFirstStep(this)) {
        if (isEnableTransparent())
            al::startAction(mChameleon, "Spin");
        al::startAction(mShell, "Spin");
    }
    if (tryReflectWall())
        al::startAction(mShell, "SpinReflect");
    if (al::isActionPlaying(mShell, "SpinReflect") && al::isActionEnd(mShell))
        al::startAction(mShell, "Spin");
    walkToPlayer(mAttackParam);
    sead::Vector3f pos = al::getTrans(mBunbun);
    al::setTrans(mShell, pos);
    bool end;
    if (isEnableTransparent()) {
        al::setTrans(mChameleon, pos);
        float distance = 0;
        al::tryFindNearestPlayerDisatanceFromTarget(&distance, mBunbun, pos);
        if (distance < 300)
            al::tryEmitEffect(mShell, "Spin", nullptr);
        else
            al::tryDeleteEffect(mShell, "Spin");
        end = al::isGreaterEqualStep(this, 450);
    } else
        end = al::isGreaterEqualStep(this, 300);
    if (end) {
        al::setNerve(this, &NrvBunbunStateShellAttackAttackEnd);
        if (mBunbun->isEnableKouraThrow()) {
            al::setVelocityZero(mBunbun);
            al::showModel(mBunbun);
            mShell->kill();
            kill();
        }
    }
}
/** @brief Reflects the shell velocity when it hits a wall. @return Whether reflected. */
bool BunbunStateShellAttack::tryReflectWall() {
    if (!al::isCollidedWallVelocity(mBunbun))
        return false;
    sead::Vector3f normal = al::getCollidedWallNormal(mBunbun);
    if (al::isReverseDirection(normal, al::getFront(mBunbun), 0.2f))
        al::turnDirectionDegree(mBunbun, &normal, al::getFront(mBunbun), 30);
    else if (!al::isReverseDirection(normal, al::getFront(mBunbun), 0.8f))
        al::turnDirectionDegree(mBunbun, &normal, -al::getFront(mBunbun), 20);
    al::calcReflectionVector(al::getVelocityPtr(mBunbun), normal, 1, 0);
    al::normalizeOrDirZ(al::getFrontPtr(mBunbun), al::getVelocity(mBunbun));
    *al::getVelocityPtr(mBunbun) *= 0.5f;
    al::startHitReaction(mBunbun, "反射");
    return true;
}
/** @brief Slows the shell before recovery. */
void BunbunStateShellAttack::exeAttackEnd() {
    if (al::isFirstStep(this) && isEnableTransparent()) {
        al::showModelIfHide(mShell);
        mChameleon->kill();
    }
    tryReflectWall();
    walkForward();
    al::setTrans(mShell, al::getTrans(mBunbun));
    if (al::isGreaterEqualStep(this, 10)) {
        al::showModel(mBunbun);
        al::setNerve(this, &NrvBunbunStateShellAttack.RecoverShellStart);
    }
}
/** @brief Restores the boss collision and begins recovery. */
void BunbunStateShellAttack::exeRecoverShellStart() {
    if (al::isFirstStep(this)) {
        mShell->kill();
        al::faceToTarget(mBunbun, al::findNearestPlayerPos(mBunbun));
        al::startAction(mBunbun, "RecoverShellStart");
        al::setColliderRadius(mBunbun, 180);
        al::setColliderOffsetY(mBunbun, 180);
    }
    walkForward();
    if (al::isActionEnd(mBunbun))
        al::setNerve(this, &NrvBunbunStateShellAttack.RecoverShell);
}
/** @brief Finishes recovery and completes the state. */
void BunbunStateShellAttack::exeRecoverShell() {
    if (al::isFirstStep(this))
        al::startAction(mBunbun, "RecoverShell");
    walkForward();
    if (al::isActionEnd(mBunbun))
        kill();
}
/** @brief Checks whether the shell transformation is complete. @return Whether attacks are enabled.
 */
bool BunbunStateShellAttack::isEnableAttack() const {
    return !al::isNerve(this, &NrvBunbunStateShellAttack.ChangeShell);
}
