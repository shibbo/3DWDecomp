#include "Boss/TentackStateAttackShot.hpp"
#include "Boss/TentackHead.hpp"
#include "Boss/TentackHeadAttackBall.hpp"
#include "Boss/TentackTentacle.hpp"
#include "Boss/TentackTentacleGroup.hpp"
#include "Boss/ITentackSwingTentacleHolder.hpp"
#include "Enemy/ActorJointLookController.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/LiveActor/Common/LiveActorGroup.hpp"
namespace {
NERVE_DECL(TentackStateAttackShot, Turn);
class TentackStateAttackShotNrvTurnNext : public al::Nerve {
public: void execute(al::NerveKeeper* pKeeper) const override { pKeeper->getParent<TentackStateAttackShot>()->exeTurn(); }
};
NERVE_DECL(TentackStateAttackShot, Shot);
NERVE_DECL(TentackStateAttackShot, ShotSign);
NERVES_MAKE_NOSTRUCT(TentackStateAttackShot, Turn, TurnNext, Shot, ShotSign)
/** @brief Compares tentacle swing order and position. @param head Host head. @param holder Swing-order provider. @param a New target. @param b Existing target. @return Whether a precedes b. */
bool isBefore(const TentackHead* head, const ITentackSwingTentacleHolder* holder, const TentackTentacle* a, const TentackTentacle* b) {
    int aId = holder->calcSwingTentacleId(a);
    int bId = holder->calcSwingTentacleId(b);
    if (aId != bId) return aId < bId;
    sead::Vector3f aPos(0.0f, 0.0f, 0.0f), bPos(0.0f, 0.0f, 0.0f);
    al::multVecPose(&aPos, head, al::getTrans(a));
    al::multVecPose(&bPos, head, al::getTrans(b));
    if (al::isNearZero(aPos.x - bPos.x, 0.001f)) return aPos.z < bPos.z;
    return bPos.x < aPos.x;
}
}
/** @brief Creates the ordered target list and attack-ball pool. @param pHead Host head. @param rInfo Initialization data. @param pSwingHolder Swing-order provider. */
TentackStateAttackShot::TentackStateAttackShot(TentackHead* pHead, const al::ActorInitInfo& rInfo,
    const ITentackSwingTentacleHolder* pSwingHolder)
    : al::HostStateBase<TentackHead>("テンタックの攻撃弾発射ステート", pHead), mSwingHolder(pSwingHolder), mBalls(nullptr), _40(3.0f) {
    initNerve(&NrvTentackStateAttackShotTurn, 0);
    mTargets.allocBuffer(8, nullptr);
    mBalls = new al::DeriveActorGroup<TentackHeadAttackBall>("テンタック攻撃弾グループ", 7);
    auto* group = mBalls;
    for (int i = 0; i < group->getMaxActorCount(); i++) {
        auto* ball = new TentackHeadAttackBall("テンタック攻撃弾");
        al::initCreateActorNoPlacementInfo(ball, rInfo);
        group->registerActor(ball);
    }
}
/** @brief Destroys the attack state. */
TentackStateAttackShot::~TentackStateAttackShot() = default;
/** @brief Starts turning toward the first target. */
void TentackStateAttackShot::appear() { al::NerveStateBase::appear(); al::setNerve(this, &NrvTentackStateAttackShotTurn); }
/** @brief Ends the state and clears its targets. */
void TentackStateAttackShot::kill() { al::NerveStateBase::kill(); mTargets.clear(); }
/** @brief Inserts a tentacle in firing order. @param pTentacle Tentacle to register. */
void TentackStateAttackShot::registerTentacle(TentackTentacle* pTentacle) {
    if (mTargets.isEmpty()) { mTargets.pushBack(pTentacle); return; }
    for (int i = 0; i < mTargets.size(); i++) {
        const auto* target = mTargets[i];
        if (isBefore(getHost(), mSwingHolder, pTentacle, target)) {
            mTargets.insert(i, pTentacle);
            return;
        }
    }
    mTargets.pushBack(pTentacle);
}
/** @brief Registers living non-attack tentacles. @param pGroup Tentacle group. */
void TentackStateAttackShot::registerTentacleGroup(TentackTentacleGroup* pGroup) {
    for (int i = 0; i < pGroup->size(); i++) {
        if (!pGroup->unsafeAt(i)->mCurInfo.mIsAttack && !al::isDead(pGroup->at(i))) registerTentacle(pGroup->at(i));
    }
}
/** @brief Gets the first target position. @return First target position. */
const sead::Vector3f& TentackStateAttackShot::getFirstTarget() const { return al::getTrans(mTargets.front()); }
/** @brief Kills all active attack balls. */
void TentackStateAttackShot::receiveDamage() { mBalls->killAll(); }
/** @brief Turns the head and aims its joints at the next target. */
void TentackStateAttackShot::exeTurn() {
    bool turned = getHost()->turnToTargetGently(getFirstTarget(), _40);
    sead::Vector3f target(0.0f, 0.0f, 0.0f);
    mTargets.front()->calcStepTrans(&target);
    getHost()->getJointLookController()->setLookTarget(target);
    if (turned) {
        if (al::isNerve(this, &NrvTentackStateAttackShotTurnNext)) al::setNerve(this, &NrvTentackStateAttackShotShot);
        else al::setNerve(this, &NrvTentackStateAttackShotShotSign);
    }
}
/** @brief Plays the warning animation before the first shot. */
void TentackStateAttackShot::exeShotSign() {
    if (al::isFirstStep(this)) al::startAction(getHost(), "ShotSign");
    if (al::isActionEnd(getHost())) al::setNerve(this, &NrvTentackStateAttackShotShot);
}
/** @brief Fires and advances through the ordered targets. */
void TentackStateAttackShot::exeShot() {
    if (al::isFirstStep(this)) {
        getHost()->tryStartActionShot();
        mTargets.front()->eatAttachItemIfAttached(true);
        mTargets.front()->endSwingForce();
    }
    if (al::isStep(this, 0)) {
        sead::Vector3f target(0.0f, 0.0f, 0.0f);
        mTargets.front()->calcStepTrans(&target);
        mBalls->tryFindDeadDeriveActor()->shot(getHost(), target);
    }
    if (getHost()->isWaitAll()) {
        mTargets.popFront();
        if (!mTargets.isEmpty()) al::setNerve(this, &NrvTentackStateAttackShotTurnNext);
        else { getHost()->getJointLookController()->stopLook(); kill(); }
    }
}
