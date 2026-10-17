#include "Boss/TentackRockFaller.hpp"
#include "Boss/TentackStateFallRock.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"

namespace {
NERVE_DECL(TentackRockFaller, FallRock);
NERVES_MAKE_NOSTRUCT(TentackRockFaller, FallRock)
TentackStateFallRockParam sFallRockParam;
}

/**
 * @brief Creates a falling-rock controller for a Tentack head.
 * @param pName Actor name.
 * @param pHead Head controlled by the falling-rock state.
 */
TentackRockFaller::TentackRockFaller(const char* pName, TentackHead* pHead)
    : al::LiveActor(pName) {
    mFallRockState = new TentackStateFallRock(pHead, &sFallRockParam);
}

/**
 * @brief Registers the controller and its state, leaving the actor inactive.
 * @param rInfo Scene and executor initialization information.
 */
void TentackRockFaller::init(const al::ActorInitInfo& rInfo) {
    al::initActorSceneInfo(this, rInfo);
    al::initExecutorWatchObj(this, rInfo);
    al::initNerve(this, &NrvTentackRockFallerFallRock, 1);
    al::initNerveState(this, mFallRockState, &NrvTentackRockFallerFallRock, "岩降らし");
    makeActorDead();
}

/** @brief Appears and restarts the falling-rock nerve. */
void TentackRockFaller::appear() {
    al::LiveActor::appear();
    al::setNerve(this, &NrvTentackRockFallerFallRock);
}

/** @brief Updates the falling-rock state and dies when it finishes. */
void TentackRockFaller::exeFallRock() {
    if (al::updateNerveState(this)) {
        kill();
    }
}

/** @brief Enables forced following in the falling-rock state. */
void TentackRockFaller::setValidFollowForce() {
    mFallRockState->mIsValidFollowForce = true;
}

/** @brief Destroys the controller's base actor resources. */
TentackRockFaller::~TentackRockFaller() = default;
