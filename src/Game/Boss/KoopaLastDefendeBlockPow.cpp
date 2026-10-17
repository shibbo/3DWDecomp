#include "Boss/KoopaLastDefendeBlockPow.hpp"
#include "Boss/KoopaLastFunction.hpp"
#include "Boss/KoopaLastDemoEndActorHolder.hpp"
#include "MapObj/KoopaFireBall.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/LiveActor/Common/LiveActorGroup.hpp"
#include "Library/Joint/JointSpringController.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"
namespace {
NERVE_DECL(KoopaLastDefendeBlockPow, Wait);
NERVE_DECL(KoopaLastDefendeBlockPow, PowDamage);
NERVE_DECL(KoopaLastDefendeBlockPow, LastPowDamage);
NERVE_DECL(KoopaLastDefendeBlockPow, Demo);
NERVE_DECL(KoopaLastDefendeBlockPow, Shout);
NERVE_DECL(KoopaLastDefendeBlockPow, AttackCurtainStart);
NERVE_DECL(KoopaLastDefendeBlockPow, AttackCurtain);
NERVE_DECL(KoopaLastDefendeBlockPow, AttackCurtainEnd);
NERVES_MAKE_NOSTRUCT(KoopaLastDefendeBlockPow, Wait, PowDamage, LastPowDamage, Demo, Shout,
                     AttackCurtainStart, AttackCurtain, AttackCurtainEnd)
} // namespace
/** @brief Creates the boss defending the final POW block. @param pName Actor name. */
KoopaLastDefendeBlockPow::KoopaLastDefendeBlockPow(const char *pName) : LiveActor(pName) {}
/** @brief Destroys the defender. */
KoopaLastDefendeBlockPow::~KoopaLastDefendeBlockPow() = default;
/** @brief Creates the fireball pool and ending actors. @param rInfo Initialization data. */
void KoopaLastDefendeBlockPow::init(const al::ActorInitInfo &rInfo) {
    mSprings.allocBuffer(16, nullptr);
    KoopaLastFunction::initActorKoopaLastCommon(this, rInfo, "DefendeBlockPow", &mSprings);
    al::initNerve(this, &NrvKoopaLastDefendeBlockPowWait, 0);
    mFireballs = new al::DeriveActorGroup<KoopaFireBall>("クッパファイアーボールリスト", 48);
    auto *group = mFireballs;
    for (int i = 0; i < group->getMaxActorCount(); i++) {
        auto *fire = new KoopaFireBall("クッパファイアーボール", false, nullptr);
        al::initCreateActorNoPlacementInfo(fire, rInfo);
        group->registerActor(fire);
    }
    mWristMtx = al::getJointMtxPtr(this, "WristR");
    mDemoHolder = new KoopaLastDemoEndActorHolder(this);
    mDemoHolder->init(rInfo, mWristMtx);
    mHomeQuat = al::getQuat(this);
    mLeftQuat = mHomeQuat;
    mRightQuat = mHomeQuat;
    al::rotateQuatYDirDegree(&mRightQuat, mRightQuat, 80.0f);
    al::rotateQuatYDirDegree(&mLeftQuat, mLeftQuat, -80.0f);
    if (!al::tryListenStageSwitchAppear(this))
        appear();
}
/** @brief Shows the defender and resets its wait state. */
void KoopaLastDefendeBlockPow::appear() {
    al::LiveActor::appear();
    mDemoHolder->appear();
    al::setNerve(this, &NrvKoopaLastDefendeBlockPowWait);
}
/** @brief Requires no additional per-frame control. */
void KoopaLastDefendeBlockPow::control() {}
/** @brief Damages or pushes a player touching the attack sensor. */
void KoopaLastDefendeBlockPow::attackSensor(al::HitSensor *pSelf, al::HitSensor *pOther) {
    if (al::isSensorEnemyAttack(pSelf) && al::isSensorPlayer(pOther) &&
        !al::sendMsgEnemyAttack(pOther, pSelf))
        al::sendMsgPush(pOther, pSelf);
}
/** @brief Handles shared messages and POW damage. @return Whether handled. */
bool KoopaLastDefendeBlockPow::receiveMsg(const al::SensorMsg *pMsg, al::HitSensor *pOther,
                                          al::HitSensor *pSelf) {
    if (KoopaLastFunction::receiveMsgCommon(pMsg, pOther, pSelf))
        return true;
    if (KoopaLastFunction::isReceivePowBlockMsg(pMsg, pOther, pSelf)) {
        al::setNerve(this, &NrvKoopaLastDefendeBlockPowPowDamage);
        return true;
    }
    return false;
}
/** @brief Handles shared pointer interactions. @return Whether handled. */
bool KoopaLastDefendeBlockPow::receiveMsgScreenPoint(const al::SensorMsg *pMsg,
                                                     al::ScreenPointer *pPointer,
                                                     al::ScreenPointTarget *pTarget) {
    return KoopaLastFunction::receiveMsgScreenPointCommon(pMsg, pPointer, pTarget);
}
/** @brief Disables interactions and starts the final POW reaction. */
void KoopaLastDefendeBlockPow::startLastPowDamage() {
    al::invalidateHitSensors(this);
    al::invalidateClipping(this);
    al::setNerve(this, &NrvKoopaLastDefendeBlockPowLastPowDamage);
}
/** @brief Disables joint springs and starts the ending cutscene. */
void KoopaLastDefendeBlockPow::startEndDemo() {
    al::invalidateHitSensors(this);
    al::invalidateClipping(this);
    for (int i = 0, count = mSprings.size(); i < count; i++)
        mSprings.at(i)->setControlRate(0);
    al::setNerve(this, &NrvKoopaLastDefendeBlockPowDemo);
}
/** @brief Waits before the opening shout. */
void KoopaLastDefendeBlockPow::exeWait() {
    if (al::isFirstStep(this))
        al::startAction(this, "LastTopWait");
    if (!al::isLessStep(this, 390))
        al::setNerve(this, &NrvKoopaLastDefendeBlockPowShout);
}
/** @brief Plays the shout before attacking. */
void KoopaLastDefendeBlockPow::exeShout() {
    if (al::isFirstStep(this))
        al::startAction(this, "LastTopShout");
    if (al::isActionEnd(this))
        al::setNerve(this, &NrvKoopaLastDefendeBlockPowAttackCurtainStart);
}
/** @brief Prepares the fireball sweep. */
void KoopaLastDefendeBlockPow::exeAttackCurtainStart() {
    if (al::isFirstStep(this))
        al::startAction(this, "LastTopBreathFireSign");
    if (al::isActionEnd(this))
        al::setNerve(this, &NrvKoopaLastDefendeBlockPowAttackCurtain);
}
/** @brief Sweeps short-lived fireballs across the arena. */
void KoopaLastDefendeBlockPow::exeAttackCurtain() {
    if (al::isFirstStep(this))
        al::startAction(this, "LastTopBreathFire");
    int step = al::getNerveStep(this);
    if (step % 7 == 0) {
        auto *fire = mFireballs->tryFindDeadDeriveActor();
        if (!fire)
            return;
        int phase = step % 120;
        if (phase > 60)
            phase = 120 - phase;
        float rate = sead::Mathf::clamp(float(phase) / 60.0f, 0.0f, 1.0f);
        al::slerpQuat(&mAimQuat, mLeftQuat, mRightQuat, rate);
        sead::Matrix34f matrix;
        matrix.fromQuat(mAimQuat);
        matrix.setBase(3, al::getTrans(this));
        sead::Vector3f up = matrix.getBase(1);
        sead::Vector3f front = matrix.getBase(2);
        sead::Vector3f pos(0, 250, 400);
        al::rotateVectorDegree(&up, up, front, 0);
        pos *= matrix;
        fire->setLandType(KoopaFireBall::LandType::NoLand);
        fire->setKoopaLastAction(true);
        fire->setLifeFrame(45);
        fire->appearAttack(pos, pos - up * 1000.0f, 20.0f);
        fire->setActionName("ShotKoopaLastCircle");
    }
    if (!al::isLessStep(this, 120))
        al::setNerve(this, &NrvKoopaLastDefendeBlockPowAttackCurtainEnd);
}
/** @brief Ends the sweep before starting another. */
void KoopaLastDefendeBlockPow::exeAttackCurtainEnd() {
    if (al::isFirstStep(this))
        al::startAction(this, "LastTopBreathFireEnd");
    if (al::isActionEnd(this))
        al::setNerve(this, &NrvKoopaLastDefendeBlockPowAttackCurtainStart);
}
/** @brief Clears fireballs and reacts to a POW hit. */
void KoopaLastDefendeBlockPow::exePowDamage() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "DownLastTopSmall");
        mFireballs->killAll();
    }
    if (al::isActionEnd(this))
        al::setNerve(this, &NrvKoopaLastDefendeBlockPowAttackCurtainStart);
}
/** @brief Plays the final POW reaction and stops the battle music. */
void KoopaLastDefendeBlockPow::exeLastPowDamage() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "DownLastTop");
        mFireballs->killAll();
        al::stopBgm(this, "Koopa2", 5, -1);
    }
}
/** @brief Plays the ending and changes the music. */
void KoopaLastDefendeBlockPow::exeDemo() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "DemoBattleLastKoopaEnd");
        mDemoHolder->startEndDemo();
    }
    if (al::isGreaterEqualStep(this, 650))
        al::startBgm(this, "AfterBattle", -1, 0, -1, -1);
    if (al::isActionEnd(this)) {
        al::isLessStep(this, 650);
        kill();
    }
}
