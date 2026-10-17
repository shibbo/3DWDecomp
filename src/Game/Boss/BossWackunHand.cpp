#include "Boss/BossWackunHand.hpp"
#include "Boss/BossWackun.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Math/MathUtil.hpp"

#include "Library/Nerve/NerveSetup.hpp"
namespace {
NERVE_DECL(BossWackunHand, Controled);
NERVE_DECL(BossWackunHand, Wait);
NERVE_DECL(BossWackunHand, MoveLeft);
NERVE_DECL(BossWackunHand, MoveRight);
NERVES_MAKE_NOSTRUCT(BossWackunHand, Controled, Wait, MoveLeft, MoveRight)
}
/** @brief Creates the hand attached to the boss. @param pBoss Host boss. */
BossWackunHand::BossWackunHand(BossWackun* pBoss) : al::LiveActor("ボスワックン手"), mBoss(pBoss) {}
/** @brief Destroys the hand. */
BossWackunHand::~BossWackunHand() = default;
/** @brief Loads the hand and starts its wait state. @param rInfo Initialization data. */
void BossWackunHand::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, sead::SafeString("BossWackunHand"), nullptr);
    al::initNerve(this, &NrvBossWackunHandWait, 0);
    makeActorAppeared();
}
/** @brief Builds the rotation for the current side of the boss. */
void BossWackunHand::updateLocalRotation() {
    al::makeQuatZDegree(&mLocalQuat, al::wrapValue(mCurrentIndex * 90.0f, 360.0f));
}
/** @brief Combines the boss pose with the local hand rotation. */
void BossWackunHand::updateHostPose() {
    al::multVecPose(al::getTransPtr(this), mBoss, sead::Vector3f::zero);
    auto* quat = al::getQuatPtr(this);
    quat->setMul(al::getQuat(mBoss), mLocalQuat);
}
/** @brief Follows the boss while externally controlled. */
void BossWackunHand::exeControled() {
    if (al::isFirstStep(this)) updateLocalRotation();
    updateHostPose();
}
/** @brief Waits on the current side of the boss. */
void BossWackunHand::exeWait() {
    if (al::isFirstStep(this)) { al::startAction(this, "Wait"); updateLocalRotation(); }
    updateHostPose();
}
/** @brief Moves one side left and repeats until reaching the target. */
void BossWackunHand::exeMoveLeft() {
    if (al::isFirstStep(this)) { al::startAction(this, "MoveLeft"); updateLocalRotation(); }
    updateHostPose();
    if (al::isActionEnd(this)) {
        mCurrentIndex = al::modi(mCurrentIndex + 5, 4);
        if (mCurrentIndex == mTargetIndex) al::setNerve(this, &NrvBossWackunHandWait);
        else al::setNerve(this, &NrvBossWackunHandMoveLeft);
    }
}
/** @brief Moves one side right and repeats until reaching the target. */
void BossWackunHand::exeMoveRight() {
    if (al::isFirstStep(this)) { al::startAction(this, "MoveRight"); updateLocalRotation(); }
    updateHostPose();
    if (al::isActionEnd(this)) {
        mCurrentIndex = al::modi(mCurrentIndex + 3, 4);
        if (mCurrentIndex == mTargetIndex) al::setNerve(this, &NrvBossWackunHandWait);
        else al::setNerve(this, &NrvBossWackunHandMoveRight);
    }
}
/** @brief Enters the externally controlled state. */
void BossWackunHand::startControled() { al::setNerve(this, &NrvBossWackunHandControled); }
/** @brief Starts waiting. */
void BossWackunHand::startWait() { al::setNerve(this, &NrvBossWackunHandWait); }
/** @brief Chooses the shorter direction around the four sides. @param targetIndex Target side index. */
void BossWackunHand::startMove(s32 targetIndex) {
    mTargetIndex = al::modi(targetIndex + 4, 4);
    if (mTargetIndex == mCurrentIndex) { al::setNerve(this, &NrvBossWackunHandWait); return; }
    int diff = mTargetIndex - mCurrentIndex;
    if (sead::Mathi::abs(diff) > 2) diff = -diff;
    if (diff > 0) al::setNerve(this, &NrvBossWackunHandMoveLeft);
    else al::setNerve(this, &NrvBossWackunHandMoveRight);
}
/** @brief Checks whether movement has finished. @return Whether waiting. */
bool BossWackunHand::isEndMove() const { return al::isNerve(this, &NrvBossWackunHandWait); }
