#include "Boss/KoopaLastStateAttackFire.hpp"
#include "Enemy/TargetFinder.hpp"
#include "MapObj/KoopaFireBall.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Math/MatrixUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/LiveActor/Common/LiveActorGroup.hpp"
namespace {
NERVE_DECL(KoopaLastStateAttackFire, Search);
NERVE_DECL(KoopaLastStateAttackFire, Attack);
NERVE_DECL(KoopaLastStateAttackFire, Wait);
NERVES_MAKE_NOSTRUCT(KoopaLastStateAttackFire, Search, Attack, Wait)
TargetFinderParam sTargetFinderParam(3000.0f, 180.0f, 180.0f, 0x7fffffff, -1.0f, -1.0f, -1.0f, 2000.0f, false);
}
/** @brief Creates the target finder and fireball pool. @param pActor Host actor. @param rInfo Initialization data. @param pParam Attack settings. */
KoopaLastStateAttackFire::KoopaLastStateAttackFire(al::LiveActor* pActor, const al::ActorInitInfo& rInfo,
    const KoopaLastStateAttackFireParam* pParam)
    : al::ActorStateBase("炎攻撃ステート", pActor), mParam(pParam), mTargetFinder(nullptr), mFireBalls(nullptr),
      mFrontDir(-sead::Vector3f::ey), mUpDir(sead::Vector3f::ez), mAttackCount(0) {
    initNerve(&NrvKoopaLastStateAttackFireSearch, 0);
    mTargetFinder = new TargetFinder(pActor, &sTargetFinderParam);
    mTargetFinder->setFrontDir(&mFrontDir);
    mTargetFinder->setSupportUpDir(&mUpDir);
    mFireBalls = new al::DeriveActorGroup<KoopaFireBall>("クッパファイアーボールリスト", 6);
    auto* group = mFireBalls;
    for (int i = 0; i < group->getMaxActorCount(); i++) {
        auto* fire = new KoopaFireBall("クッパファイアーボール", false, nullptr);
        al::initCreateActorNoPlacementInfo(fire, rInfo);
        group->registerActor(fire);
    }
}
/** @brief Destroys the fire attack state. */
KoopaLastStateAttackFire::~KoopaLastStateAttackFire() = default;
/** @brief Resets targeting and the attack count. */
void KoopaLastStateAttackFire::appear() {
    sead::Vector3f side;
    al::calcQuatLocalAxisAll(al::getQuat(mHostActor), &side, &mFrontDir, &mUpDir);
    mFrontDir = -mFrontDir;
    mTargetFinder->setFrontDir(&mFrontDir);
    mTargetFinder->setSupportUpDir(&mUpDir);
    mTargetFinder->refindTarget();
    mAttackCount = 0;
    al::setNerve(this, &NrvKoopaLastStateAttackFireSearch);
    al::NerveStateBase::appear();
}
/** @brief Searches for a target until the configured timeout. */
void KoopaLastStateAttackFire::exeSearch() {
    mTargetFinder->update();
    if (mTargetFinder->isFoundTargetNow()) {
        al::setNerve(this, &NrvKoopaLastStateAttackFireAttack);
        return;
    }
    if (mParam->mValue04 >= 0 && al::isGreaterEqualStep(this, mParam->mValue04)) kill();
}
/** @brief Launches an available fireball from the face joint. */
void KoopaLastStateAttackFire::exeAttack() {
    if (al::isFirstStep(this)) {
        auto* fire = mFireBalls->tryFindDeadDeriveActor();
        if (!fire) { al::setNerve(this, &NrvKoopaLastStateAttackFireWait); return; }
        sead::Vector3f pos(0.0f, 0.0f, 0.0f);
        const auto* mtx = al::getJointMtxPtr(mHostActor, "Face");
        al::calcTransLocalOffsetByMtx(&pos, *mtx, sead::Vector3f::ex * 200.0f);
        fire->appearAttack(pos, mTargetFinder->getTargetPos(), 10.0f);
        ++mAttackCount;
    }
    al::setNerve(this, &NrvKoopaLastStateAttackFireWait);
}
/** @brief Waits between shots and checks the attack limit. */
void KoopaLastStateAttackFire::exeWait() {
    if (al::isFirstStep(this)) mTargetFinder->refindTarget();
    mTargetFinder->update();
    if (al::isLessStep(this, mParam->mAttackInterval)) return;
    if (mParam->mValue08 > 0 && mParam->mValue08 < mAttackCount) kill();
    else al::setNerve(this, &NrvKoopaLastStateAttackFireSearch);
}
