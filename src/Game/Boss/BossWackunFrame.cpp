#include "Boss/BossWackunFrame.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"

namespace {
NERVE_DECL(BossWackunFrame, Wait);
NERVE_DECL(BossWackunFrame, Damage);
NERVES_MAKE_NOSTRUCT(BossWackunFrame, Wait, Damage)
}

/**
 * @brief Creates the frame attached to BossWackun.
 * @param pBoss Boss whose pose the frame follows.
 */
BossWackunFrame::BossWackunFrame(BossWackun* pBoss)
    : al::LiveActor("ボスワックン枠"),
      mBoss(reinterpret_cast<al::LiveActor*>(pBoss)) {
    // BossWackun's LiveActor base is at offset zero; its definition is not reconstructed yet.
}

/**
 * @brief Initializes the frame model and its waiting nerve, then appears.
 * @param rInfo Actor placement and scene initialization information.
 */
void BossWackunFrame::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "BossWackunFrame", nullptr);
    al::initNerve(this, &NrvBossWackunFrameWait, 0);
    makeActorAppeared();
}

/** @brief Plays the death reaction and removes the frame. */
void BossWackunFrame::kill() {
    al::startHitReactionDeath(this);
    al::LiveActor::kill();
}

/** @brief Follows the boss pose while waiting. */
void BossWackunFrame::exeWait() {
    al::copyPose(this, mBoss);
}

/** @brief Continues following the boss pose while damaged. */
void BossWackunFrame::exeDamage() {
    al::copyPose(this, mBoss);
}

/** @brief Disables frame collision and enters the damage nerve. */
void BossWackunFrame::setDamage() {
    al::invalidateCollisionParts(this);
    al::setNerve(this, &NrvBossWackunFrameDamage);
}

/** @brief Restores collision and plays the recovery action. */
void BossWackunFrame::revival() {
    al::validateCollisionParts(this);
    al::setNerve(this, &NrvBossWackunFrameWait);
    al::startAction(this, "Recover");
}

/** @brief Destroys the frame actor's base resources. */
BossWackunFrame::~BossWackunFrame() = default;
