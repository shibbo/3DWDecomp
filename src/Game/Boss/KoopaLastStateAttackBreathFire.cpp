#include "Boss/KoopaLastStateAttackBreathFire.hpp"
#include "Boss/KoopaLastFunction.hpp"
#include "MapObj/KoopaFireBall.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/LiveActor/Common/LiveActorGroup.hpp"
#include <cmath>
namespace {
NERVE_DECL(KoopaLastStateAttackBreathFire, ClimbEnd);
NERVE_DECL(KoopaLastStateAttackBreathFire, Wait);
NERVE_DECL(KoopaLastStateAttackBreathFire, Prepare);
NERVE_DECL(KoopaLastStateAttackBreathFire, Attack);
NERVE_DECL(KoopaLastStateAttackBreathFire, End);
NERVE_DECL(KoopaLastStateAttackBreathFire, RestartWait);
NERVES_MAKE_NOSTRUCT(KoopaLastStateAttackBreathFire, ClimbEnd, Wait, Prepare, Attack, End, RestartWait)
}
/** @brief Creates the breath attack and fireball pool. @param pActor Host actor. @param rInfo Initialization data. */
KoopaLastStateAttackBreathFire::KoopaLastStateAttackBreathFire(al::LiveActor* pActor, const al::ActorInitInfo& rInfo)
    : al::ActorStateBase("炎の息ステート", pActor) {
    initNerve(&NrvKoopaLastStateAttackBreathFirePrepare, 0);
    al::tryGetArg(&mStartWait, rInfo, "TopAttackStartWait");
    mFireBalls = new al::DeriveActorGroup<KoopaFireBall>("クッパファイアーボールリスト", 8);
    auto* group = mFireBalls;
    for (int i = 0; i < group->getMaxActorCount(); i++) {
        auto* fire = new KoopaFireBall("クッパファイアーボール", false, nullptr);
        al::initCreateActorNoPlacementInfo(fire, rInfo);
        group->registerActor(fire);
    }
}
/** @brief Destroys the breath attack state. */
KoopaLastStateAttackBreathFire::~KoopaLastStateAttackBreathFire() = default;
/** @brief Begins the climb-end animation. */
void KoopaLastStateAttackBreathFire::appear() {
    al::NerveStateBase::appear();
    al::setNerve(this, &NrvKoopaLastStateAttackBreathFireClimbEnd);
}
/** @brief Deactivates every pooled fireball. */
void KoopaLastStateAttackBreathFire::killAllFire() { mFireBalls->killAll(); }
/** @brief Finishes climbing and chooses the initial wait. */
void KoopaLastStateAttackBreathFire::exeClimbEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(mHostActor, "LastCornerBrethFireClimbEnd");
        al::invalidateClipping(mHostActor);
    }
    KoopaLastFunction::explosionCollision(mHostActor, "Body");
    if (al::isActionEnd(mHostActor)) {
        if (mStartWait == 0) al::setNerve(this, &NrvKoopaLastStateAttackBreathFirePrepare);
        else al::setNerve(this, &NrvKoopaLastStateAttackBreathFireWait);
    }
}
/** @brief Waits for the configured attack delay. */
void KoopaLastStateAttackBreathFire::exeWait() {
    if (al::isFirstStep(this)) al::startAction(mHostActor, "LastCornerBrethFireWait");
    if (al::isLessStep(this, mStartWait)) return;
    al::setNerve(this, &NrvKoopaLastStateAttackBreathFirePrepare);
}
/** @brief Plays the warning animation before firing. */
void KoopaLastStateAttackBreathFire::exePrepare() {
    if (al::isFirstStep(this)) al::startAction(mHostActor, "LastCornerBrethFireSign");
    if (al::isActionEnd(mHostActor)) al::setNerve(this, &NrvKoopaLastStateAttackBreathFireAttack);
}
/** @brief Launches three fireballs along the mouth direction. */
void KoopaLastStateAttackBreathFire::exeAttack() {
    if (al::isFirstStep(this)) al::startAction(mHostActor, "LastCornerBrethFire");
    if (al::getNerveStep(this) % 14 == 0 && al::isLessStep(this, 41)) {
        auto* fire = mFireBalls->tryFindDeadDeriveActor();
        if (!fire) return;
        int index = al::getNerveStep(this) / 14;
        float angle = 0.0f;
        if (index >= 1) {
            float rise = 0.0f / index;
            float run = rise + 0.0f;
            if (!al::isNearZero(run, 0.001f)) angle = std::atan(rise * 0.0f / run) * 57.2957763671875f;
        }
        const auto* joint = al::getJointMtxPtr(mHostActor, "Mouth1");
        sead::Vector3f direction = sead::Vector3f::ez;
        sead::Vector3f position(joint->m[0][3], joint->m[1][3], joint->m[2][3]);
        joint->getBase(direction, 1);
        al::verticalizeVec(&direction, sead::Vector3f::ey, direction);
        if (al::normalizeOrZero(&direction)) direction = sead::Vector3f::ez;
        sead::Vector3f side = direction.cross(sead::Vector3f::ey);
        al::rotateVectorDegree(&direction, direction, side, angle);
        sead::Vector3f spawnPos = direction * 0.0f + position - sead::Vector3f::ey * 50.0f;

        fire->setLandType(KoopaFireBall::LandType::NoLand);
        fire->setKoopaLastAction(true);
        fire->setLifeFrame(135);
        fire->appearAttack(spawnPos, spawnPos + direction * 1000.0f, index * 0.0f + 10.0f);
        if (al::isFirstStep(this)) fire->setActionName("ShotKoopaLastLineFirst");
        else fire->setActionName("ShotKoopaLastLine");
    }
    if (al::isLessStep(this, 40)) return;
    al::setNerve(this, &NrvKoopaLastStateAttackBreathFireEnd);
}
/** @brief Plays the end animation before cooling down. */
void KoopaLastStateAttackBreathFire::exeEnd() {
    if (al::isFirstStep(this)) al::startAction(mHostActor, "LastCornerBrethFireEnd");
    if (al::isActionEnd(mHostActor)) al::setNerve(this, &NrvKoopaLastStateAttackBreathFireRestartWait);
}
/** @brief Waits between repeated breath attacks. */
void KoopaLastStateAttackBreathFire::exeRestartWait() {
    if (al::isFirstStep(this)) al::startAction(mHostActor, "LastCornerBrethFireWait");
    if (al::isLessStep(this, 190)) return;
    al::setNerve(this, &NrvKoopaLastStateAttackBreathFirePrepare);
}
