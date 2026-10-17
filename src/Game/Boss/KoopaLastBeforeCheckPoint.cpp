#include "Boss/KoopaLastBeforeCheckPoint.hpp"
#include "Boss/KoopaLastFunction.hpp"
#include "MapObj/KoopaFireBall.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/LiveActor/Common/LiveActorGroup.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"
#include "Project/Bgm/BgmPlayingRequest.hpp"
namespace {
NERVE_DECL(KoopaLastBeforeCheckPoint, Wait);
NERVE_DECL(KoopaLastBeforeCheckPoint, Appear);
NERVE_DECL(KoopaLastBeforeCheckPoint, Damage);
NERVE_DECL(KoopaLastBeforeCheckPoint, Land);
NERVE_DECL(KoopaLastBeforeCheckPoint, AttackStart);
NERVE_DECL(KoopaLastBeforeCheckPoint, AttackCircle);
NERVE_DECL(KoopaLastBeforeCheckPoint, AttackEnd);
NERVES_MAKE_NOSTRUCT(KoopaLastBeforeCheckPoint, Wait, Appear, Damage, Land, AttackStart, AttackCircle, AttackEnd)
}
/** @brief Creates the checkpoint encounter actor. @param pName Actor name. */
KoopaLastBeforeCheckPoint::KoopaLastBeforeCheckPoint(const char* pName) : LiveActor(pName) {}
/** @brief Destroys the encounter actor. */
KoopaLastBeforeCheckPoint::~KoopaLastBeforeCheckPoint() = default;
/** @brief Creates the fireball pool and sweep orientations. @param rInfo Initialization data. */
void KoopaLastBeforeCheckPoint::init(const al::ActorInitInfo& rInfo) {
    KoopaLastFunction::initActorKoopaLastCommon(this, rInfo, "BeforeCheckPoint", nullptr);
    al::initNerve(this, &NrvKoopaLastBeforeCheckPointWait, 0);
    mFireballs = new al::DeriveActorGroup<KoopaFireBall>("クッパファイアーボールリスト", 32);
    auto* group = mFireballs;
    for (int i = 0; i < group->getMaxActorCount(); i++) {
        auto* fire = new KoopaFireBall("クッパファイアーボール", false, nullptr);
        al::initCreateActorNoPlacementInfo(fire, rInfo);
        group->registerActor(fire);
    }
    mHomePos = al::getTrans(this);
    mAimQuat = al::getQuat(this);
    mLeftQuat = mAimQuat;
    mRightQuat = mAimQuat;
    al::rotateQuatYDirDegree(&mRightQuat, mRightQuat, 70.0f);
    al::rotateQuatYDirDegree(&mLeftQuat, mLeftQuat, -70.0f);
    if (!al::tryListenStageSwitchAppear(this)) appear();
}
/** @brief Resets the encounter and begins descending. */
void KoopaLastBeforeCheckPoint::appear() {
    al::LiveActor::appear();
    mAttackSide = 0;
    al::resetPosition(this, mHomePos, false);
    al::setNerve(this, &NrvKoopaLastBeforeCheckPointAppear);
}
/** @brief Attacks players or pushes them while down. @param pSelf Own sensor. @param pOther Target sensor. */
void KoopaLastBeforeCheckPoint::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isSensorEnemyAttack(pSelf) && al::isSensorPlayer(pOther))
        if (al::isNerve(this, &NrvKoopaLastBeforeCheckPointDamage) || !al::sendMsgEnemyAttack(pOther, pSelf)) al::sendMsgPush(pOther, pSelf);
}
/** @brief Handles shared messages and POW damage. @return Whether handled. */
bool KoopaLastBeforeCheckPoint::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) {
    if (KoopaLastFunction::receiveMsgCommon(pMsg, pOther, pSelf)) return true;
    if (al::isNerve(this, &NrvKoopaLastBeforeCheckPointDamage)) return false;
    if (KoopaLastFunction::isReceivePowBlockMsg(pMsg, pOther, pSelf)) {
        al::setNerve(this, &NrvKoopaLastBeforeCheckPointDamage);
        return true;
    }
    return false;
}
/** @brief Handles the shared pointer interaction. @return Whether handled. */
bool KoopaLastBeforeCheckPoint::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer, al::ScreenPointTarget* pTarget) {
    return KoopaLastFunction::receiveMsgScreenPointCommon(pMsg, pPointer, pTarget);
}
/** @brief Descends to the encounter position. */
void KoopaLastBeforeCheckPoint::exeAppear() {
    if (al::isFirstStep(this)) {
        al::invalidateClipping(this);
        al::startAction(this, "BreathFireFallLoop");
    }
    float height = al::calcNerveEaseInValue(this, 10, 1000.0f, 0.0f);
    al::setTrans(this, height * sead::Vector3f::ey + mHomePos);
    if (!al::isLessStep(this, 10)) {
        al::validateClipping(this);
        al::setNerve(this, &NrvKoopaLastBeforeCheckPointLand);
    }
}
/** @brief Plays the landing action before the first attack. */
void KoopaLastBeforeCheckPoint::exeLand() {
    if (al::isFirstStep(this)) al::startAction(this, "BreathFireFallEnd");
    if (al::isActionEnd(this)) al::setNerve(this, &NrvKoopaLastBeforeCheckPointAttackStart);
}
/** @brief Waits briefly between attack sweeps. */
void KoopaLastBeforeCheckPoint::exeWait() {
    if (al::isFirstStep(this)) al::startAction(this, "BreathFireWait");
    if (al::isLessStep(this, 15)) return;
    al::setNerve(this, &NrvKoopaLastBeforeCheckPointAttackStart);
}
/** @brief Prepares the next sweep direction. */
void KoopaLastBeforeCheckPoint::exeAttackStart() {
    if (al::isFirstStep(this)) al::startAction(this, mAttackSide == 0 ? "BreathFireRightStart" : "BreathFireLeftStart");
    if (al::isActionEnd(this)) al::setNerve(this, &NrvKoopaLastBeforeCheckPointAttackCircle);
}
/** @brief Launches fireballs along the rotating sweep. */
void KoopaLastBeforeCheckPoint::exeAttackCircle() {
    if (al::isFirstStep(this)) al::startAction(this, mAttackSide == 0 ? "BreathFireRight" : "BreathFireLeft");
    int interval = mAttackSide == 0 ? 2 : 3;
    float angle = mAttackSide == 0 ? 28.0f : 53.0f;
    const auto& from = mAttackSide == 0 ? mLeftQuat : mRightQuat;
    const auto& to = mAttackSide == 0 ? mRightQuat : mLeftQuat;
    int step = al::getNerveStep(this);
    if (step % interval == 0 && al::isLessStep(this, 31)) {
        auto* fire = mFireballs->tryFindDeadDeriveActor();
        if (!fire) return;
        float rate = sead::Mathf::clamp(float(step) / 30.0f, 0.0f, 1.0f);
        al::slerpQuat(&mAimQuat, from, to, rate);
        sead::Matrix34f matrix;
        matrix.fromQuat(mAimQuat);
        matrix.setBase(3, al::getTrans(this));
        sead::Vector3f front = matrix.getBase(2);
        sead::Vector3f side = matrix.getBase(0);
        sead::Vector3f pos(0.0f, 350.0f, 350.0f);
        al::rotateVectorDegree(&front, front, side, angle);
        pos *= matrix;
        fire->setLandType(KoopaFireBall::LandType::Short);
        fire->setExtinguishOnSlope(true);
        fire->setKoopaLastAction(true);
        fire->appearAttack(pos, pos + front * 1000.0f, 30.0f);
        fire->setActionName("ShotKoopaLast");
    }
    if (al::isActionEnd(this)) al::setNerve(this, &NrvKoopaLastBeforeCheckPointAttackEnd);
}
/** @brief Alternates the sweep direction after the attack. */
void KoopaLastBeforeCheckPoint::exeAttackEnd() {
    if (al::isFirstStep(this)) al::startAction(this, mAttackSide == 0 ? "BreathFireRightEnd" : "BreathFireLeftEnd");
    if (al::isActionEnd(this)) {
        mAttackSide = (mAttackSide + 1) % 2;
        al::setNerve(this, &NrvKoopaLastBeforeCheckPointWait);
    }
}
/** @brief Plays the POW defeat sequence and changes the music. */
void KoopaLastBeforeCheckPoint::exeDamage() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "DownBlockPow");
        al::startHitReaction(this, "ＰＯＷブロックダメージ");
        al::stopBgmSyncedCurBgmBeat(this, al::BgmPlayingRequest("Koopa", 10, 0, 0), 2);
        al::startBgmSyncedCurBgmBeat(this, al::BgmPlayingRequest("KoopaDown", 0, 0, 0), 2);
        mFireballs->killAll();
    }
    if (al::isActionEnd(this)) { al::tryOnStageSwitch(this, "SwitchDeadOn"); kill(); }
}
NERVE_IMPL(KoopaLastBeforeCheckPoint, Wait)
NERVE_IMPL(KoopaLastBeforeCheckPoint, Appear)
NERVE_IMPL(KoopaLastBeforeCheckPoint, Damage)
NERVE_IMPL(KoopaLastBeforeCheckPoint, Land)
NERVE_IMPL(KoopaLastBeforeCheckPoint, AttackStart)
NERVE_IMPL(KoopaLastBeforeCheckPoint, AttackCircle)
NERVE_IMPL(KoopaLastBeforeCheckPoint, AttackEnd)
